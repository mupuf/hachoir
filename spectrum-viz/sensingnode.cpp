#include "sensingnode.h"

#include <QCoreApplication>
#include <QVector>

#include <stdint.h>
#include <sys/time.h>

SensingNode::SensingNode(QTcpSocket *socket, int clientID, QObject *parent) :
	QObject(parent), clientSocket(socket), clientID(clientID),
	_ringbuffer(100000), pwr_min(125), pwr_max(-125)
{
	connect(clientSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
	connect(clientSocket, SIGNAL(disconnected()), clientSocket, SLOT(deleteLater()));
	connect(clientSocket, SIGNAL(readyRead()), this, SLOT(dataReady()));
}

void SensingNode::pauseUpdates(bool pause)
{
	updatesPaused.store(pause);
}

bool SensingNode::areUpdatesPaused()
{
	return updatesPaused.load();
}

bool SensingNode::fetchEntries(qreal start, qreal end)
{
	QMutexLocker locker(&_ringbufferMutex);

	freeEntries();

	if (start >= 0)
	{
		_entries_start = start;
		_entries_end = end;
	} else {
		_entries_start = _ringbuffer.head() + start;
		_entries_end = _ringbuffer.head() + end;
	}

	_entries = _ringbuffer.getRange(_entries_start, _entries_end);

	return _entries.size() > 0;
}

QMap<QString, QVariant> SensingNode::getEntriesRange()
{
	QMap<QString, QVariant> map;
	map["start"] = (qreal)_entries_start;
	map["end"] = (qreal)_entries_end;
	return map;
}

QObject * SensingNode::selectEntry(qreal pos)
{
	if (pos >= _entries_start && pos < _entries_end)
		return _entries[pos - _entries_start].data();
	else
		return NULL;
}

void SensingNode::freeEntries()
{
	_entries_start = 0;
	_entries_end = 0;
	_entries.clear();
}

void SensingNode::clientDisconnected()
{
	emit requestDestroy(clientID);
}

QByteArray SensingNode::readExactlyNBytes(QTcpSocket *socket, qint64 n)
{
	while (socket->bytesAvailable() < n)
	{
		QCoreApplication::processEvents();
	}

	return socket->read(n);
}

#include "../common/message_utils.h"
void SensingNode::readPowerSpectrumMessage()
{
	uint64_t start, end, time_ns;
	uint16_t steps;

	QByteArray header = readExactlyNBytes(clientSocket, 26);

	size_t pos = 0;
	read_and_update_offset(pos, header.data(), steps);
	read_and_update_offset(pos, header.data(), start);
	read_and_update_offset(pos, header.data(), end);
	read_and_update_offset(pos, header.data(), time_ns);

	QByteArray data = readExactlyNBytes(clientSocket, steps);

	if (updatesPaused.load())
		return;

	PowerSpectrum * ps = new PowerSpectrum(start, end, time_ns, steps, data.data());

	if (ps->powerMax() > pwr_max)
		pwr_max = ps->powerMax();
	if (ps->powerMin() < pwr_min)
		pwr_min = ps->powerMin();

	QMutexLocker locker(&_ringbufferMutex);
	_ringbuffer.pushBack(ps);

	emit powerRangeChanged((pwr_max + 20) / 10 * 10, (pwr_min - 20) / 10 * 10);
	emit frequencyRangeChanged(start,  end);
}

void SensingNode::dataReady()
{
	while (clientSocket->bytesAvailable() > 2)
	{
		QByteArray header = readExactlyNBytes(clientSocket, 2);
		uint8_t msg_type, padding;

		size_t pos = 0;
		read_and_update_offset(pos, header.data(), msg_type);
		read_and_update_offset(pos, header.data(), padding);

		switch (msg_type) {
		case 0x1:
			readPowerSpectrumMessage();
			break;
		default:
			qDebug() << "SensingNode: Invalid message type";
			break;
		}
	}
}

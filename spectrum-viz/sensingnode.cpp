#include "sensingnode.h"

#include <QCoreApplication>
#include <QVector>

#include <stdint.h>
#include <sys/time.h>

#include "../common/message_utils.h"

SensingNode::SensingNode(QTcpSocket *socket, int clientID, QObject *parent) :
	QObject(parent), clientSocket(socket), clientID(clientID),
	_ringbuffer(100000), _ret(10000), pwr_min(125), pwr_max(-125)
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

bool SensingNode::fetchFftEntries(qreal start, qreal end)
{
	QMutexLocker locker(&_renderingMutex);

	freeFftEntries();

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

QMap<QString, QVariant> SensingNode::getFftEntriesRange()
{
	QMap<QString, QVariant> map;
	map["start"] = (qreal)_entries_start;
	map["end"] = (qreal)_entries_end;
	return map;
}

QObject * SensingNode::selectFftEntry(qreal pos)
{
	if (pos >= _entries_start && pos < _entries_end)
		return _entries[pos - _entries_start].get();
	else
		return NULL;
}

void SensingNode::freeFftEntries()
{
	_entries_start = 0;
	_entries_end = 0;
	_entries.clear();
}

int SensingNode::fetchCommunications(qreal timeStart, qreal timeEnd)
{
	QMutexLocker locker(&_renderingMutex);
	freeCommunications();

	_coms = _ret.fetchEntries(timeStart, timeEnd);

	return _coms.size();
}

QMap<QString, QVariant> SensingNode::selectCommunication(qreal pos)
{
	QMap<QString, QVariant> map;

	if (pos >= 0 && pos < _coms.size()) {
		RetEntry *entry = _coms[pos].get();
		map["id"] = (float) entry->id();
		map["timeStart"] = (double) entry->timeStart();
		map["timeEnd"] = (double) entry->timeEnd();
		map["frequencyStart"] = (double) entry->frequencyStart() * 1000;
		map["frequencyEnd"] = (double) entry->frequencyEnd() * 1000;
		map["pwr"] = (int) entry->pwr();
		map["psu"] = entry->psuString();
		map["address"] = (float) entry->address();
	}

	return map;
}

void SensingNode::freeCommunications()
{
	_coms.clear();
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

void SensingNode::readPowerSpectrumMessage(const QByteArray &psMsg)
{
	uint64_t start, end, time_ns;
	uint16_t steps;

	size_t pos = 0;
	read_and_update_offset(pos, psMsg.data(), steps);
	read_and_update_offset(pos, psMsg.data(), start);
	read_and_update_offset(pos, psMsg.data(), end);
	read_and_update_offset(pos, psMsg.data(), time_ns);

	PowerSpectrum * ps = new PowerSpectrum(start, end, time_ns, steps, psMsg.data() + pos);

	if (ps->powerMax() > pwr_max)
		pwr_max = ps->powerMax();
	if (ps->powerMin() < pwr_min)
		pwr_min = ps->powerMin();

	QMutexLocker locker(&_renderingMutex);
	_ringbuffer.push_back(ps);

	emit powerRangeChanged((pwr_max + 20) / 10 * 10, (pwr_min - 20) / 10 * 10);
	emit frequencyRangeChanged(start,  end);
}

void SensingNode::readRETMessage(const QByteArray &retMsg)
{
	QMutexLocker locker(&_renderingMutex);
	_ret.updateFromString(retMsg.data(), retMsg.length());
}

void SensingNode::dataReady()
{
	while (clientSocket->bytesAvailable() > 5)
	{
		QByteArray header = readExactlyNBytes(clientSocket, 5);
		uint8_t msg_type;
		uint32_t msg_length;

		size_t pos = 0;
		read_and_update_offset(pos, header.data(), msg_type);
		read_and_update_offset(pos, header.data(), msg_length);

		/* read the message */
		QByteArray msg = readExactlyNBytes(clientSocket, msg_length);

		if (updatesPaused.load())
			return;

		switch (msg_type) {
		case 0x1:
			readPowerSpectrumMessage(msg);
			break;
		case 0x2:
			readRETMessage(msg);
			break;
		default:
			qDebug() << "SensingNode: Invalid message type";
			break;
		}
	}
}

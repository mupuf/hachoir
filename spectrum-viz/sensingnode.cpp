#include "sensingnode.h"

#include <QVector>

SensingNode::SensingNode(QTcpSocket *socket, int clientID, QObject *parent) :
	QObject(parent), clientSocket(socket), clientID(clientID),
	frontBuffer(new QVector<SpectrumSample>), pwr_min(125), pwr_max(-125)
{
	frontBuffer->append(SpectrumSample(868000000, -90));
	frontBuffer->append(SpectrumSample(869000000, -83));
	frontBuffer->append(SpectrumSample(870000000, -84));
	frontBuffer->append(SpectrumSample(871000000, -86));
	frontBuffer->append(SpectrumSample(872000000, -70));
	frontBuffer->append(SpectrumSample(873000000, -40));

	connect(clientSocket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));
	connect(clientSocket, SIGNAL(disconnected()), clientSocket, SLOT(deleteLater()));
	connect(clientSocket, SIGNAL(readyRead()), this, SLOT(dataReady()));
}

int SensingNode::startReading()
{
	swapMutex.lockForRead();

	return frontBuffer->size();
}

QMap<QString, QVariant> SensingNode::getSample(int i)
{
	QMap<QString, QVariant> map;

	map["freq"] = frontBuffer->at(i).freq();
	map["dbm"] = frontBuffer->at(i).dBM();

	return map;
}

void SensingNode::stopReading()
{
	swapMutex.unlock();
}

void SensingNode::clientDisconnected()
{
	emit requestDestroy(clientID);
}

#include <stdint.h>
#include <sys/time.h>
#include <QCoreApplication>
void SensingNode::dataReady()
{
	QByteArray header = clientSocket->read(28);
	union {
		uint8_t  *u08;
		uint16_t *u16;
		uint32_t *u32;
		uint64_t *u64;
		float *f;
	} ptr;
	ptr.u08 = (uint8_t *)header.data();

	uint8_t msg_type = *ptr.u08++;
	uint8_t padding = *ptr.u08++;
	uint16_t steps = *ptr.u16++;
	uint64_t start = *ptr.u64++;
	uint64_t end = *ptr.u64++;
	uint64_t time_ns = *ptr.u64++;

	if (msg_type | 0x1)
		frontBuffer->clear();

	while (clientSocket->bytesAvailable() < (steps))
	{
		QCoreApplication::processEvents();
	}

	QByteArray data = clientSocket->read(steps);

	for (int i = 0; i < steps; i++)
	{
		float freq = start + i * (end-start) / steps;
		char pwr = data.at(i);
		if (pwr < pwr_min)
			pwr_min = pwr;
		if (pwr > pwr_max)
			pwr_max = pwr;
		frontBuffer->append(SpectrumSample(freq, pwr));
	}

	emit frequencyRangeChanged(start,  end);
	emit powerRangeChanged((pwr_max + 20) / 10 * 10, (pwr_min - 20) / 10 * 10);
	emit dataChanged();
}

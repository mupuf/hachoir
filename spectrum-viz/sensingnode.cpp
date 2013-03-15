#include "sensingnode.h"

#include <QVector>

SensingNode::SensingNode(QTcpSocket *socket, int clientID, QObject *parent) :
	QObject(parent), clientSocket(socket), clientID(clientID),
	frontBuffer(new QVector<SpectrumSample>)
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
#include <QCoreApplication>
void SensingNode::dataReady()
{
	QByteArray header = clientSocket->read(19);
	union {
		uint8_t  *u08;
		uint16_t *u16;
		uint32_t *u32;
		uint64_t *u64;
		float *f;
	} ptr;
	ptr.u08 = (uint8_t *)header.data();

	uint8_t msg_type = *ptr.u08++;
	uint64_t centralFreq = *ptr.u64++;
	uint64_t sampleRate = *ptr.u64++;
	uint16_t fftSize = *ptr.u16++;

	frontBuffer->clear();

	while (clientSocket->bytesAvailable() < (fftSize / 2))
	{
		QCoreApplication::processEvents();
	}

	QByteArray data = clientSocket->read(fftSize / 2);
	for (int i = 0; i < fftSize / 2; i++)
	{
		float freq = centralFreq + i * sampleRate / fftSize;
		frontBuffer->append(SpectrumSample(freq, (char)data.at(i)));
	}

	emit frequencyRangeChanged(centralFreq + sampleRate/2, centralFreq);
	emit dataChanged();
}

#include "sensingnode.h"

#include <QVector>

SensingNode::SensingNode(QObject *parent) :
	QObject(parent), frontBuffer(new QVector<SpectrumSample>)
{
	frontBuffer->append(SpectrumSample(868000000, -90));
	frontBuffer->append(SpectrumSample(869000000, -83));
	frontBuffer->append(SpectrumSample(870000000, -84));
	frontBuffer->append(SpectrumSample(871000000, -86));
	frontBuffer->append(SpectrumSample(872000000, -70));
	frontBuffer->append(SpectrumSample(873000000, -40));
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

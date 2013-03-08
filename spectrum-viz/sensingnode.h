#ifndef SENSINGNODE_H
#define SENSINGNODE_H

#include <QObject>
#include <QMutex>
#include <QVariant>
#include <QMap>
#include <QReadWriteLock>
#include <QSharedPointer>

#include "spectrumsample.h"

class SensingNode : public QObject
{
	Q_OBJECT

private:
	QReadWriteLock swapMutex;
	QMutex backBufferMutex;

	QSharedPointer< QVector<SpectrumSample> > frontBuffer, backBuffer;
public:
	explicit SensingNode(QObject *parent = 0);

	/* returns the number of samples */
	Q_INVOKABLE int startReading();
	Q_INVOKABLE QMap<QString, QVariant> getSample(int i);
	Q_INVOKABLE void stopReading();
	
signals:
	void temperatureChanged(int temp);

public slots:
	
};

#endif // SENSINGNODE_H

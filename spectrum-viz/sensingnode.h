#ifndef SENSINGNODE_H
#define SENSINGNODE_H

#include <QObject>
#include <QMutex>
#include <QVariant>
#include <QMap>
#include <QTcpSocket>
#include <QReadWriteLock>
#include <QSharedPointer>

#include "spectrumsample.h"

class SensingNode : public QObject
{
	Q_OBJECT

private:
	QTcpSocket *clientSocket;
	int clientID;

	QReadWriteLock swapMutex;
	QMutex backBufferMutex;

	QSharedPointer< QVector<SpectrumSample> > frontBuffer, backBuffer;
public:
	explicit SensingNode(QTcpSocket *socket, int clientID, QObject *parent = 0);

	/* returns the number of samples */
	Q_INVOKABLE int startReading();
	Q_INVOKABLE QMap<QString, QVariant> getSample(int i);
	Q_INVOKABLE void stopReading();

signals:
	void powerRangeChanged(qreal high, qreal low);
	void frequencyRangeChanged(qreal high, qreal low);
	void requestDestroy(int clientID);

private slots:
	void clientDisconnected();

};

#endif // SENSINGNODE_H

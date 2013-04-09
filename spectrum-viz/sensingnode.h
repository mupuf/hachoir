#ifndef SENSINGNODE_H
#define SENSINGNODE_H

#include <QObject>
#include <QMutex>
#include <QVariant>
#include <QMap>
#include <QTcpSocket>
#include <QSharedPointer>
#include <QAtomicInt>

#include "powerspectrum.h"
#include "absoluteringbuffer.h"

class SensingNode : public QObject
{
	Q_OBJECT

private:
	QTcpSocket *clientSocket;
	int clientID;

	QMutex _ringbufferMutex;
	AbsoluteRingBuffer< PowerSpectrum > _ringbuffer;

	/* qml's view */
	QVector< QSharedPointer<PowerSpectrum> > _entries;
	uint64_t _entries_start, _entries_end;

	QAtomicInt updatesPaused;

	/* hold the maximum and the minimum received power */
	char pwr_min;
	char pwr_max;

	QByteArray readExactlyNBytes(QTcpSocket *socket, qint64 n);
	void readPowerSpectrumMessage();
public:
	explicit SensingNode(QTcpSocket *socket, int clientID, QObject *parent = 0);

	Q_INVOKABLE void pauseUpdates(bool pause);
	Q_INVOKABLE bool areUpdatesPaused();

	Q_INVOKABLE bool fetchEntries(qreal start, qreal end);
	Q_INVOKABLE QMap<QString, QVariant> getEntriesRange();

	Q_INVOKABLE QObject * selectEntry(qreal pos);

	Q_INVOKABLE void freeEntries(); /* unreference the entries */

signals:
	void dataChanged();
	void powerRangeChanged(qreal high, qreal low);
	void frequencyRangeChanged(qreal low, qreal high);
	void timeChanged(qreal timeNs);
	void requestDestroy(int clientID);

private slots:
	void clientDisconnected();
	void dataReady();
};

#endif // SENSINGNODE_H

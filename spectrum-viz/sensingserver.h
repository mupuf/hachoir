#ifndef SENSINGSERVER_H
#define SENSINGSERVER_H

#include <QTcpServer>
#include <QVector>
#include <QSharedPointer>

#include "sensingnode.h"

class SensingServer : public QTcpServer
{
	Q_OBJECT

	QVector< QSharedPointer<SensingNode> > nodes;
public:
	explicit SensingServer(QObject *parent = 0);

	/// returns a QObject because it is used in the qml
	Q_INVOKABLE QObject * sensingNode(int clientID);

	void addNewClient(QTcpSocket *client);

signals:
	void newClientArrived(int clientID);

private slots:
	void newClient();
	void destroyClient(int clientID);
};

#endif // SENSINGSERVER_H

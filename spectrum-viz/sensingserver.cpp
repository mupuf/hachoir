#include "sensingserver.h"
#include "sensingnode.h"

#include <iostream>

#include <QQmlEngine>

SensingServer::SensingServer(QObject *parent) :
	QTcpServer(parent)
{
	connect(this, SIGNAL(newConnection()), this, SLOT(newClient()));
}

QObject * SensingServer::sensingNode(int clientID)
{
	QQmlEngine::setObjectOwnership(nodes[clientID].data(), QQmlEngine::CppOwnership);
	return nodes[clientID].data();
}

void SensingServer::addNewClient(QTcpSocket *client)
{
	SensingNode *node;

	/* add the node to the list */
	int i = 0, clientID = -1;
	do {
		if (i <= nodes.size() - 1) {
			if (nodes[i].isNull()) {
				clientID = i;
				node = new SensingNode(client, clientID);
				nodes[clientID].reset(node);
			}
		} else {
			clientID = i;
			node = new SensingNode(client, clientID);
			nodes.append(QSharedPointer<SensingNode>(node));
		}
		i++;
	} while(clientID < 0);

	connect(node, SIGNAL(requestDestroy(int)), this, SLOT(destroyClient(int)));

	/* tell the QMLEngine that we own this object */
	QQmlEngine::setObjectOwnership(node, QQmlEngine::CppOwnership);

	/* send a signal that a new node arrived */
	emit newClientArrived(clientID);
}

void SensingServer::newClient()
{
	QTcpSocket *client = nextPendingConnection();
	if (client == 0)
		return;

	addNewClient(client);
}

void SensingServer::destroyClient(int clientID)
{
	nodes[clientID].clear();
}

#include <QtGui/QGuiApplication>
#include "qtquick2applicationviewer.h"

#include <iostream>

#include <QQmlContext>
#include <QTcpSocket>
#include <sensingserver.h>

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);

	SensingServer server;
	if (!server.listen(QHostAddress::Any, 21334)) { // spectrum-viz -> SV --> 0x53 + 0x56 --> 21334
		std::cerr << "Listening failed: " << server.errorString().toStdString() << std::endl;
		return -1;
	}

	QtQuick2ApplicationViewer viewer;
	viewer.rootContext()->setContextProperty("SensingServer", &server);
	viewer.setMainQmlFile(QStringLiteral("qml/spectrum-viz/main.qml"));
	viewer.showExpanded();

	/* HACK */
	QTcpSocket socket;
	socket.connectToHost("127.0.0.1", 21333);
	if (!socket.waitForConnected(1000)) {
		qDebug("Couldn't connect to the sensing Server!");
		return 1;
	}
	server.addNewClient(&socket);

	return app.exec();
}

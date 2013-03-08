#include <QtGui/QGuiApplication>
#include "qtquick2applicationviewer.h"

#include <QQmlContext>
#include <sensingnode.h>

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);

	SensingNode node;

	QtQuick2ApplicationViewer viewer;
	viewer.rootContext()->setContextProperty("SensingNode", &node);
	viewer.setMainQmlFile(QStringLiteral("qml/spectrum-viz/main.qml"));
	viewer.showExpanded();

	return app.exec();
}

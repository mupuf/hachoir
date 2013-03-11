import QtQuick 2.0

Rectangle {
	width: 1024
	height: 650

	Connections {
		target: SensingServer
		onNewClientArrived: {
			var component = Qt.createComponent("Spectrometer.qml");
			if (component.status === Component.Ready)
				component.createObject(spectrometers, {"width": spectrometers.width, "height": 300, "clientID": clientID});
		}
	}

	Column {
		id: spectrometers
		spacing: 5
		anchors.fill: parent

		/* components here will be allocated dynamically */
	}
}

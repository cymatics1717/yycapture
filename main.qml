import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    property real r: .8
    width: Screen.desktopAvailableWidth *r
    height: Screen.desktopAvailableHeight *r
    visible: true
    id: preview
    objectName: "preview"
    Rectangle {
        anchors.fill: parent
        color: "#333333"
    }

    Image {
        id: media
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit

        focus: true
        Keys.onPressed: {
            console.log('pressed Key '+ event.text);
            if (event.key === Qt.Key_Return || event.key === Qt.Key_F) {
                preview.visibility === Window.FullScreen ? preview.showNormal() : preview.showFullScreen()
            } else if (event.key === Qt.Key_Q) {
                Qt.quit();
            } else if (event.key === Qt.Key_S) {
                txt.text =Date().toString()
                txt.z = 10
                popup.open()
            }
        }
    }

    Text {
        z:1
        id: txt
        anchors.centerIn: parent
        text: "Hello World!"
        font.pointSize: 50
        color: "steelblue"
    }

    Dialog {
        id: popup
        x: 100
        y: 100
        z:100
        width: 200
        height: 300
        modal: true
        anchors.centerIn: Overlay.overlay
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
    }

    Timer {
        running: true
        interval: 1000
        repeat: true
        onTriggered: {
            txt.text = Date().toString()
        }
    }

}

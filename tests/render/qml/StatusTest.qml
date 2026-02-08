import QtQuick
import QtQuick.Window
import QtQuick.Controls

import org.freedesktop.gstreamer.Qt6GLVideoItem 1.0

Window {
    id: window
    width: 1280
    height: 720
    visible: true
    visibility: Window.FullScreen
    color: "black"

    Item {
        anchors.fill: parent

        GstGLQt6VideoItem {
            id: video
            objectName: "videoItem"
            anchors.fill: parent
        }

        // 按钮控制栏
        Rectangle {
            height: 60
            width: 500
            color: '#804c4646' // 半透明黑色背景
            radius: 10
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 20

            Row {
                anchors.centerIn: parent
                spacing: 15

                Button { 
                    text: "Play"
                    onClicked: videoController.play() 
                }
                Button { 
                    text: "Pause"
                    onClicked: videoController.pause() 
                }
                Button { 
                    text: "Flush"
                    onClicked: videoController.flush() 
                    palette.buttonText: "orange"
                }
                Button { 
                    text: "Reset"
                    onClicked: videoController.reset()
                    palette.buttonText: "yellow"
                }
                Button { 
                    text: "Stop"
                    onClicked: videoController.stop() 
                    palette.buttonText: "red"
                }
            }
        }
    }

    Shortcut {
        sequence: "Escape"
        onActivated: Qt.quit()
    }
}
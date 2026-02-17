import QtQuick 6.5
import QtQuick.Controls 6.5
import QtQuick.Layouts 6.5

ApplicationWindow {
    visible: true
    width: 900
    height: 600
    title: "Tux-TI83"
    color: "#2E3440"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.65
            color: "#2E3440"
            
            ColumnLayout {
                anchors.fill: parent
                padding: 20
                
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    color: "#3B4252"
                    Text {
                        anchors.centerIn: parent
                        text: uiController.currentDisplay
                        color: "#ECEFF4"
                        font.pixelSize: 24
                    }
                }

                Canvas {
                    id: graphCanvas
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.clearRect(0, 0, width, height);
                        ctx.strokeStyle = "#88C0D0";
                        ctx.beginPath();
                        ctx.moveTo(0, height/2); ctx.lineTo(width, height/2);
                        ctx.stroke();
                    }
                }
            }
        }
        
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.35
            color: "#3B4252"
            Text { text: "History"; color: "#81A1C1"; anchors.horizontalCenter: parent.horizontalCenter }
        }
    }
}

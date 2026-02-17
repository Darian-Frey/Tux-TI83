import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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
            id: workspacePane
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.65
            color: "#2E3440"

            Item {
                anchors.fill: parent
                anchors.margins: 16

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    Rectangle {
                        id: display
                        Layout.fillWidth: true
                        Layout.preferredHeight: 80
                        color: "#3B4252"
                        radius: 6

                        Text {
                            anchors.centerIn: parent
                            text: uiController.currentDisplay
                            font.pixelSize: 28
                            color: "#ECEFF4"
                        }
                    }

                    GridLayout {
                        id: keypad
                        columns: 4
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        rowSpacing: 10
                        columnSpacing: 10

                        Repeater {
                            model: [
                                "7","8","9","÷",
                                "4","5","6","×",
                                "1","2","3","−",
                                "0",".","π","+",
                                "C", "(", ")", "ENTER"
                            ]

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 6
                                // Style ENTER button differently (Nord Frost)
                                color: modelData === "ENTER" ? "#88C0D0" : "#4C566A"

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.pixelSize: 22
                                    color: modelData === "ENTER" ? "#2E3440" : "#ECEFF4"
                                    font.bold: modelData === "ENTER"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onEntered: parent.color = modelData === "ENTER" ? "#8FBCBB" : "#81A1C1"
                                    onExited: parent.color = modelData === "ENTER" ? "#88C0D0" : "#4C566A"
                                    onClicked: {
                                        uiController.processInput(index)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: historyPane
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.35
            color: "#2E3440"
            border.color: "#4C566A"
            border.width: 1

            Text {
                anchors.centerIn: parent
                text: "History"
                color: "#4C566A"
            }
        }
    }
}

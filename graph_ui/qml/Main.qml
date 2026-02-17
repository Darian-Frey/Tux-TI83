import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    visible: true; width: 900; height: 600; title: "Tux-TI83"; color: "#2E3440"
    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            id: workspacePane; Layout.fillHeight: true; Layout.preferredWidth: parent.width * 0.65; color: "#2E3440"
            Item {
                anchors.fill: parent; anchors.margins: 16
                ColumnLayout {
                    anchors.fill: parent; spacing: 12
                    RowLayout {
                        Layout.fillWidth: true
                        Rectangle {
                            Layout.fillWidth: true; Layout.preferredHeight: 60; color: "#3B4252"; radius: 6
                            Text { anchors.centerIn: parent; text: uiController.currentDisplay; font.pixelSize: 24; color: "#ECEFF4" }
                        }
                        Button { text: uiController.isGraphMode ? "KEYS" : "GRAPH"; onClicked: uiController.toggleGraphMode() }
                    }
                    StackLayout {
                        currentIndex: uiController.isGraphMode ? 1 : 0
                        Layout.fillWidth: true; Layout.fillHeight: true
                        GridLayout {
                            columns: 4; rowSpacing: 10; columnSpacing: 10
                            Repeater {
                                model: ["7","8","9","÷","4","5","6","×","1","2","3","−","0",".","π","+","C", "(", ")", "ENTER", "X"]
                                delegate: Rectangle {
                                    Layout.fillWidth: true; Layout.fillHeight: true; radius: 6
                                    color: modelData === "ENTER" ? "#88C0D0" : (modelData === "X" ? "#A3BE8C" : "#4C566A")
                                    Text { anchors.centerIn: parent; text: modelData; font.pixelSize: 20; color: modelData === "ENTER" ? "#2E3440" : "#ECEFF4" }
                                    MouseArea { anchors.fill: parent; onClicked: (mouse) => uiController.processInput(index) }
                                }
                            }
                        }
                        Rectangle {
                            color: "#3B4252"; radius: 6; clip: true
                            Canvas {
                                id: graphCanvas; anchors.fill: parent
                                onPaint: {
                                    var ctx = getContext("2d"); ctx.clearRect(0, 0, width, height);
                                    function toPx(x, y) {
                                        return {
                                            x: (x - uiController.xMin) * (width / (uiController.xMax - uiController.xMin)),
                                            y: height - (y - uiController.yMin) * (height / (uiController.yMax - uiController.yMin))
                                        };
                                    }
                                    ctx.strokeStyle = "#4C566A"; ctx.beginPath();
                                    var origin = toPx(0, 0); ctx.moveTo(0, origin.y); ctx.lineTo(width, origin.y);
                                    ctx.moveTo(origin.x, 0); ctx.lineTo(origin.x, height); ctx.stroke();
                                    var pts = uiController.getGraphPoints(400);
                                    ctx.strokeStyle = "#88C0D0"; ctx.lineWidth = 2; ctx.beginPath();
                                    for (var i=0; i<pts.length; i++) {
                                        var p = toPx(pts[i].x, pts[i].y);
                                        if (i === 0) ctx.moveTo(p.x, p.y); else ctx.lineTo(p.x, p.y);
                                    }
                                    ctx.stroke();
                                }
                                MouseArea {
                                    anchors.fill: parent; property real lastX: 0; property real lastY: 0
                                    onPressed: (mouse) => { lastX = mouse.x; lastY = mouse.y; }
                                    onPositionChanged: (mouse) => { if (pressed) { uiController.pan(mouse.x - lastX, mouse.y - lastY, width, height); lastX = mouse.x; lastY = mouse.y; } }
                                    onWheel: (wheel) => { var factor = wheel.angleDelta.y > 0 ? 0.9 : 1.1; uiController.zoom(factor, wheel.x, wheel.y, width, height); }
                                }
                                Connections {
                                    target: uiController
                                    function onViewportChanged() { graphCanvas.requestPaint(); }
                                    function onGraphModeChanged() { graphCanvas.requestPaint(); }
                                }
                            }
                        }
                    }
                }
            }
        }
        Rectangle {
            Layout.fillHeight: true; Layout.preferredWidth: parent.width * 0.35; color: "#2E3440"; border.color: "#4C566A"; border.width: 1
            ListView {
                anchors.fill: parent; anchors.margins: 10
                header: Text { text: "HISTORY"; color: "#81A1C1"; horizontalAlignment: Text.AlignHCenter; width: parent.width; font.bold: true }
                model: uiController.history; spacing: 4
                delegate: Rectangle {
                    width: parent.width; height: 35; color: "#3B4252"; radius: 4
                    Text { anchors.centerIn: parent; text: modelData; color: "#D8DEE9" }
                    MouseArea { anchors.fill: parent; onClicked: (mouse) => uiController.restoreFromHistory(index) }
                }
            }
        }
    }
}

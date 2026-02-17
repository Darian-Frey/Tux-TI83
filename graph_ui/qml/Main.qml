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
                    
                    // High Contrast Function Selectors
                    RowLayout {
                        spacing: 10
                        Repeater {
                            model: ["Y1", "Y2", "Y3"]
                            delegate: Button {
                                contentItem: Text {
                                    text: modelData
                                    font.pixelSize: 14; font.bold: true
                                    color: uiController.activeFunctionIndex === index ? "#2E3440" : "#ECEFF4"
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    implicitWidth: 60; implicitHeight: 35
                                    color: uiController.activeFunctionIndex === index ? "#88C0D0" : "#434C5E"
                                    radius: 4
                                }
                                onClicked: uiController.setActiveFunction(index)
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            contentItem: Text {
                                text: uiController.isGraphMode ? "KEYS" : "GRAPH"
                                font.pixelSize: 14; font.bold: true; color: "#2E3440"
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle { implicitWidth: 80; implicitHeight: 35; color: "#EBCB8B"; radius: 4 }
                            onClicked: uiController.toggleGraphMode()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 60; color: "#3B4252"; radius: 6
                        Text { anchors.centerIn: parent; text: uiController.currentDisplay; font.pixelSize: 24; color: "#ECEFF4" }
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
                                    Text { 
                                        anchors.centerIn: parent; text: modelData; font.bold: true
                                        color: (modelData === "ENTER" || modelData === "X") ? "#2E3440" : "#ECEFF4" 
                                    }
                                    MouseArea { anchors.fill: parent; onClicked: (mouse) => uiController.processInput(index) }
                                }
                            }
                        }

                        Rectangle {
                            color: "#1A1D23"; radius: 6; clip: true
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

                                    // RESTORED: Grid Logic
                                    var rangeX = uiController.xMax - uiController.xMin;
                                    var step = 1;
                                    if (rangeX > 50) step = 10; else if (rangeX < 5) step = 0.5;

                                    ctx.lineWidth = 1; ctx.font = "10px sans-serif";
                                    
                                    // Vertical Grid & Labels
                                    for (var x = Math.floor(uiController.xMin / step) * step; x <= uiController.xMax; x += step) {
                                        var p = toPx(x, 0);
                                        ctx.strokeStyle = (Math.abs(x) < 0.0001) ? "#D8DEE9" : "#2E3440";
                                        ctx.beginPath(); ctx.moveTo(p.x, 0); ctx.lineTo(p.x, height); ctx.stroke();
                                        if (Math.abs(x) > 0.0001) { ctx.fillStyle = "#4C566A"; ctx.fillText(x.toFixed(1), p.x + 2, height - 5); }
                                    }
                                    // Horizontal Grid & Labels
                                    for (var y = Math.floor(uiController.yMin / step) * step; y <= uiController.yMax; y += step) {
                                        var p = toPx(0, y);
                                        ctx.strokeStyle = (Math.abs(y) < 0.0001) ? "#D8DEE9" : "#2E3440";
                                        ctx.beginPath(); ctx.moveTo(0, p.y); ctx.lineTo(width, p.y); ctx.stroke();
                                        if (Math.abs(y) > 0.0001) { ctx.fillStyle = "#4C566A"; ctx.fillText(y.toFixed(1), 5, p.y - 2); }
                                    }

                                    // Multi-Function Plotting
                                    var colors = ["#88C0D0", "#BF616A", "#A3BE8C"];
                                    var multiPts = uiController.getMultiGraphPoints(400);
                                    for (var f=0; f < multiPts.length; f++) {
                                        var pts = multiPts[f];
                                        ctx.strokeStyle = colors[f % colors.length]; ctx.lineWidth = 2.5; ctx.beginPath();
                                        for (var i=0; i<pts.length; i++) {
                                            var pt = toPx(pts[i].x, pts[i].y);
                                            if (i === 0) ctx.moveTo(pt.x, pt.y); else ctx.lineTo(pt.x, pt.y);
                                        }
                                        ctx.stroke();
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent; property real lastX; property real lastY
                                    onPressed: (mouse) => { lastX = mouse.x; lastY = mouse.y; }
                                    onPositionChanged: (mouse) => { if (pressed) { uiController.pan(mouse.x - lastX, mouse.y - lastY, width, height); lastX = mouse.x; lastY = mouse.y; } }
                                    onWheel: (wheel) => { uiController.zoom(wheel.angleDelta.y > 0 ? 0.9 : 1.1, wheel.x, wheel.y, width, height); }
                                }
                                Connections {
                                    target: uiController
                                    function onViewportChanged() { graphCanvas.requestPaint(); }
                                    function onGraphModeChanged() { graphCanvas.requestPaint(); }
                                }
                            }
                            // Reset Viewport Target
                            Rectangle {
                                anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 15
                                width: 40; height: 40; radius: 20; color: "#EBCB8B"
                                Text { anchors.centerIn: parent; text: "⌖"; color: "#2E3440"; font.pixelSize: 24; font.bold: true }
                                MouseArea { anchors.fill: parent; onClicked: uiController.resetViewport() }
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
                header: Text { text: "HISTORY"; color: "#81A1C1"; horizontalAlignment: Text.AlignHCenter; width: parent.width; font.bold: true; bottomPadding: 10 }
                model: uiController.history; spacing: 4
                delegate: Rectangle {
                    width: parent.width; height: 35; color: "#3B4252"; radius: 4
                    Text { anchors.centerIn: parent; text: modelData; color: "#D8DEE9"; font.bold: true }
                }
            }
        }
    }
}

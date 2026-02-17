import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    visible: true; width: 900; height: 600; title: "Tux-TI83"; color: "#2E3440"
    
    // Completely custom Popup instead of Menu to bypass system themes
    Popup {
        id: logicPopup
        width: 140; height: 320
        padding: 5
        background: Rectangle { 
            color: "#88C0D0" // Bright Frost Blue background
            radius: 8
            border.color: "#ECEFF4"
            border.width: 2
        }

        ColumnLayout {
            anchors.fill: parent; spacing: 2
            Repeater {
                model: ["=", "≠", "<", ">", "and", "or", "not"]
                delegate: Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 40
                    color: mouseArea.containsMouse ? "#4C566A" : "transparent"
                    radius: 4
                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        font.pixelSize: 18; font.bold: true
                        // Dark text on Light Blue background
                        color: mouseArea.containsMouse ? "#ECEFF4" : "#2E3440"
                    }
                    MouseArea {
                        id: mouseArea; anchors.fill: parent; hoverEnabled: true
                        onClicked: {
                            uiController.processInput(modelData)
                            logicPopup.close()
                        }
                    }
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            id: workspacePane; Layout.fillHeight: true; Layout.preferredWidth: parent.width * 0.65; color: "#2E3440"
            Item {
                anchors.fill: parent; anchors.margins: 16
                ColumnLayout {
                    anchors.fill: parent; spacing: 12
                    
                    RowLayout {
                        spacing: 10
                        Repeater {
                            model: ["Y1", "Y2", "Y3"]
                            delegate: Button {
                                contentItem: Text { text: modelData; font.bold: true; color: uiController.activeFunctionIndex === index ? "#2E3440" : "#ECEFF4" }
                                background: Rectangle { implicitWidth: 60; implicitHeight: 35; color: uiController.activeFunctionIndex === index ? "#88C0D0" : "#434C5E"; radius: 4 }
                                onClicked: uiController.setActiveFunction(index)
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            background: Rectangle { implicitWidth: 80; implicitHeight: 35; color: "#EBCB8B"; radius: 4 }
                            contentItem: Text { text: uiController.isGraphMode ? "KEYS" : "GRAPH"; font.bold: true; color: "#2E3440" }
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
                            id: keypadGrid; columns: 6; rowSpacing: 8; columnSpacing: 8
                            Repeater {
                                model: ["7","8","9","÷","sin","asin","4","5","6","×","cos","acos","1","2","3","−","tan","atan","0",".","π","+","√","^","X","log","ln","(","LOGIC",")","C","ENTER"]
                                delegate: Rectangle {
                                    id: buttonRect
                                    Layout.fillWidth: true; Layout.fillHeight: true; radius: 6
                                    Layout.columnSpan: modelData === "ENTER" ? 5 : 1
                                    color: (modelData === "ENTER") ? "#88C0D0" : (modelData === "X") ? "#A3BE8C" : (modelData === "LOGIC") ? "#EBCB8B" : (["sin","cos","tan","asin","acos","atan","√","log","ln","^"].indexOf(modelData) !== -1) ? "#B48EAD" : "#4C566A"
                                    Text { anchors.centerIn: parent; text: modelData; font.bold: true; color: (modelData === "ENTER" || modelData === "X" || modelData === "LOGIC") ? "#2E3440" : "#ECEFF4" }
                                    MouseArea { 
                                        anchors.fill: parent
                                        onClicked: {
                                            if (modelData === "LOGIC") {
                                                // Center the popup over the workspacePane for visibility
                                                logicPopup.x = (workspacePane.width - logicPopup.width) / 2
                                                logicPopup.y = (workspacePane.height - logicPopup.height) / 2
                                                logicPopup.open()
                                            } else {
                                                uiController.processInput(modelData)
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            color: "#1A1D23"; radius: 6; clip: true
                            Canvas {
                                id: graphCanvas; anchors.fill: parent
                                onWidthChanged: requestPaint(); onHeightChanged: requestPaint()
                                onPaint: {
                                    var ctx = getContext("2d"); ctx.clearRect(0, 0, width, height);
                                    function toPx(x, y) { return { x: (x - uiController.xMin) * (width / (uiController.xMax - uiController.xMin)), y: height - (y - uiController.yMin) * (height / (uiController.yMax - uiController.yMin)) }; }
                                    var rangeX = uiController.xMax - uiController.xMin;
                                    var step = rangeX > 50 ? 10 : (rangeX < 5 ? 0.5 : 1);
                                    
                                    for (var x = Math.floor(uiController.xMin / step) * step; x <= uiController.xMax; x += step) {
                                        var px = toPx(x, 0); ctx.strokeStyle = (Math.abs(x) < 0.0001) ? "#D8DEE9" : "#2E3440";
                                        ctx.beginPath(); ctx.moveTo(px.x, 0); ctx.lineTo(px.x, height); ctx.stroke();
                                        if (Math.abs(x) > 0.0001) { ctx.fillStyle = "#4C566A"; ctx.fillText(x.toFixed(1), px.x + 2, height - 5); }
                                    }
                                    for (var y = Math.floor(uiController.yMin / step) * step; y <= uiController.yMax; y += step) {
                                        var py = toPx(0, y); ctx.strokeStyle = (Math.abs(y) < 0.0001) ? "#D8DEE9" : "#2E3440";
                                        ctx.beginPath(); ctx.moveTo(0, py.y); ctx.lineTo(width, py.y); ctx.stroke();
                                        if (Math.abs(y) > 0.0001) { ctx.fillStyle = "#4C566A"; ctx.fillText(y.toFixed(1), 5, py.y - 2); }
                                    }
                                    
                                    var colors = ["#88C0D0", "#BF616A", "#A3BE8C"];
                                    var multiPts = uiController.getMultiGraphPoints(600);
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
                header: Text { text: "HISTORY"; color: "#81A1C1"; font.bold: true; bottomPadding: 10; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                model: uiController.history; spacing: 4
                delegate: Rectangle {
                    width: parent.width; height: 35; color: "#3B4252"; radius: 4
                    Text { anchors.centerIn: parent; text: modelData; color: "#D8DEE9"; font.bold: true }
                }
            }
        }
    }
}

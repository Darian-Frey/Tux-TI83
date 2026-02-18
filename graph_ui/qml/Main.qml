import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    visible: true
    width: 900
    height: 600
    title: "Tux-TI83"
    color: "#2E3440"

    // MATRIX POPUP
    Popup {
        id: matrixPopup
        width: 380
        height: 480
        modal: true
        focus: true
        x: (workspacePane.width - width) / 2
        y: (workspacePane.height - height) / 2
        background: Rectangle { 
            color: "#3B4252"
            radius: 8
            border.color: "#88C0D0"
            border.width: 2 
        }
        
        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            TabBar {
                id: matrixTabs
                Layout.fillWidth: true
                TabButton { 
                    id: namesTab
                    text: "NAMES"
                    contentItem: Text { 
                        text: namesTab.text
                        color: namesTab.checked ? "#2E3440" : "#D8DEE9"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.bold: true 
                    }
                    background: Rectangle { color: namesTab.checked ? "#88C0D0" : "#2E3440" }
                }
                TabButton { 
                    id: editTab
                    text: "EDIT"
                    contentItem: Text { 
                        text: editTab.text
                        color: editTab.checked ? "#2E3440" : "#D8DEE9"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.bold: true 
                    }
                    background: Rectangle { color: editTab.checked ? "#88C0D0" : "#2E3440" }
                }
            }

            StackLayout {
                currentIndex: matrixTabs.currentIndex
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 15
                
                ColumnLayout {
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: ["[A]", "[B]", "[C]"]
                        delegate: ItemDelegate { 
                            id: matDelegate
                            width: parent.width
                            height: 45
                            background: Rectangle { color: matDelegate.hovered ? "#4C566A" : "transparent"; radius: 4 }
                            contentItem: Text { 
                                text: modelData
                                color: "#88C0D0"
                                font.pixelSize: 20
                                font.bold: true
                                verticalAlignment: Text.AlignVCenter 
                            }
                            onClicked: {
                                uiController.processInput(modelData)
                                matrixPopup.close()
                            }
                        }
                    }
                }
                
                ColumnLayout {
                    spacing: 10
                    Text { text: "Edit Matrix [A] (3x3)"; color: "#88C0D0"; font.bold: true; font.pixelSize: 16 }
                    GridLayout {
                        id: matrixGrid
                        columns: 3
                        Layout.fillWidth: true
                        rowSpacing: 5
                        columnSpacing: 5
                        Repeater {
                            model: 9
                            TextField { 
                                placeholderText: "0"
                                Layout.fillWidth: true
                                color: "#88C0D0"
                                font.bold: true
                                horizontalAlignment: TextInput.AlignHCenter
                                background: Rectangle { 
                                    color: "#2E3440"
                                    radius: 4
                                    border.color: parent.activeFocus ? "#88C0D0" : "#4C566A"
                                }
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                    Button {
                        text: "SAVE TO [A]"
                        Layout.fillWidth: true
                        height: 45
                        background: Rectangle { color: "#A3BE8C"; radius: 4 }
                        contentItem: Text { 
                            text: "SAVE TO [A]"
                            color: "#2E3440"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter 
                        }
                        onClicked: {
                            var vals = []
                            for(var i=0; i<9; i++) {
                                vals.push(parseFloat(matrixGrid.children[i].text || "0"))
                            }
                            uiController.updateMatrix("[A]", 3, 3, vals)
                            matrixPopup.close()
                        }
                    }
                }
            }
        }
    }

    // WINDOW POPUP
    Popup {
        id: windowPopup
        width: 220
        height: 420
        modal: true
        focus: true
        x: (workspacePane.width - width) / 2
        y: (workspacePane.height - height) / 2
        background: Rectangle { 
            color: "#3B4252"
            radius: 8
            border.color: "#81A1C1"
            border.width: 2 
        }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 10
            Text { 
                text: "WINDOW SETTINGS"
                color: "#88C0D0"
                font.bold: true
                Layout.alignment: Qt.AlignHCenter 
            }
            Repeater {
                model: [
                    {l: "Xmin:", p: "xMin"}, 
                    {l: "Xmax:", p: "xMax"}, 
                    {l: "Ymin:", p: "yMin"}, 
                    {l: "Ymax:", p: "yMax"}
                ]
                delegate: RowLayout {
                    Text { text: modelData.l; color: "#ECEFF4"; Layout.preferredWidth: 60 }
                    TextField {
                        text: uiController[modelData.p].toFixed(2)
                        Layout.fillWidth: true
                        color: "white"
                        background: Rectangle { color: "#2E3440"; radius: 4 }
                        onEditingFinished: {
                            uiController[modelData.p] = parseFloat(text)
                        }
                    }
                }
            }
            Button {
                Layout.fillWidth: true
                text: "ZSTANDARD"
                background: Rectangle { color: "#4C566A"; radius: 4 }
                contentItem: Text { text: "ZSTANDARD"; color: "white"; horizontalAlignment: Text.AlignHCenter; font.bold: true }
                onClicked: { uiController.resetViewport() }
            }
            Button {
                Layout.fillWidth: true
                text: "ZOOM FIT"
                background: Rectangle { color: "#4C566A"; radius: 4 }
                contentItem: Text { text: "ZOOM FIT"; color: "white"; horizontalAlignment: Text.AlignHCenter; font.bold: true }
                onClicked: { uiController.zoomFit() }
            }
            Button {
                Layout.fillWidth: true
                text: "DONE"
                background: Rectangle { color: "#88C0D0"; radius: 4 }
                contentItem: Text { text: "DONE"; color: "#2E3440"; horizontalAlignment: Text.AlignHCenter; font.bold: true }
                onClicked: { windowPopup.close() }
            }
        }
    }

    // LOGIC POPUP
    Popup {
        id: logicPopup
        width: 170
        height: 380
        modal: true
        focus: true
        x: (workspacePane.width - width) / 2
        y: (workspacePane.height - height) / 2
        background: Rectangle { 
            color: "#88C0D0"
            radius: 8
            border.color: "#ECEFF4"
            border.width: 3 
        }
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 5
            Repeater {
                model: ["=", "≠", "<", ">", "and", "or", "not", "▶Frac"]
                delegate: Button {
                    id: logicBtn
                    Layout.fillWidth: true
                    text: modelData
                    background: Rectangle { 
                        color: logicBtn.pressed ? "#2E3440" : (logicBtn.hovered ? "#4C566A" : "#3B4252")
                        radius: 4 
                    }
                    contentItem: Text { 
                        text: modelData
                        color: "#ECEFF4"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter 
                    }
                    onClicked: {
                        uiController.processInput(text)
                        logicPopup.close()
                    }
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0
        
        Rectangle {
            id: workspacePane
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: "#2E3440"
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12
                
                RowLayout {
                    spacing: 10
                    Repeater {
                        model: ["Y1", "Y2", "Y3"]
                        delegate: Button {
                            contentItem: Text { 
                                text: modelData
                                font.bold: true
                                color: uiController.activeFunctionIndex === index ? "#2E3440" : "#ECEFF4" 
                            }
                            background: Rectangle { 
                                implicitWidth: 60
                                implicitHeight: 35
                                color: uiController.activeFunctionIndex === index ? "#88C0D0" : "#434C5E"
                                radius: 4 
                            }
                            onClicked: { uiController.setActiveFunction(index) }
                        }
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "WINDOW"
                        background: Rectangle { implicitWidth: 80; implicitHeight: 35; color: "#81A1C1"; radius: 4 }
                        contentItem: Text { text: "WINDOW"; color: "#2E3440"; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                        onClicked: { windowPopup.open() }
                    }
                    Button {
                        text: uiController.isGraphMode ? "KEYS" : "GRAPH"
                        background: Rectangle { implicitWidth: 80; implicitHeight: 35; color: "#EBCB8B"; radius: 4 }
                        contentItem: Text { text: uiController.isGraphMode ? "KEYS" : "GRAPH"; color: "#2E3440"; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                        onClicked: { uiController.toggleGraphMode() }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    color: "#3B4252"
                    radius: 6
                    Text { anchors.centerIn: parent; text: uiController.currentDisplay; font.pixelSize: 24; color: "#ECEFF4"; font.bold: true }
                }

                StackLayout {
                    currentIndex: uiController.isGraphMode ? 1 : 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    
                    GridLayout {
                        columns: 6
                        rowSpacing: 8
                        columnSpacing: 8
                        Repeater {
                            model: ["7","8","9","÷","sin","asin","4","5","6","×","cos","acos","1","2","3","−","tan","atan","0",".","MATRX","+","√","^","X","log","ln","(","LOGIC",")","DEL","C","ENTER"]
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 6
                                Layout.columnSpan: modelData === "ENTER" ? 4 : 1
                                color: (modelData === "ENTER") ? "#88C0D0" : (modelData === "DEL" || modelData === "C") ? "#BF616A" : (modelData === "X") ? "#A3BE8C" : (modelData === "LOGIC" || modelData === "MATRX") ? "#EBCB8B" : (["sin","cos","tan","asin","acos","atan","√","log","ln","^"].indexOf(modelData) !== -1) ? "#B48EAD" : "#4C566A"
                                Text { 
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.bold: true
                                    color: (["ENTER","X","LOGIC","MATRX","sin","cos","tan","asin","acos","atan","√","log","ln","^"].indexOf(modelData) !== -1) ? "#2E3440" : "#ECEFF4" 
                                }
                                MouseArea { 
                                    anchors.fill: parent
                                    onClicked: { 
                                        if (modelData === "LOGIC") logicPopup.open()
                                        else if (modelData === "MATRX") matrixPopup.open()
                                        else uiController.processInput(modelData) 
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        color: "#1A1D23"
                        radius: 6
                        clip: true
                        Canvas {
                            id: graphCanvas
                            anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                function toPx(x, y) { return { x: (x - uiController.xMin) * (width / (uiController.xMax - uiController.xMin)), y: height - (y - uiController.yMin) * (height / (uiController.yMax - uiController.yMin)) }; }
                                
                                var rangeX = uiController.xMax - uiController.xMin
                                var step = rangeX > 50 ? 10 : (rangeX < 5 ? 0.5 : 1)
                                ctx.font = "10px sans-serif"
                                ctx.fillStyle = "#4C566A"

                                // Grid Lines & Labels
                                for (var x = Math.floor(uiController.xMin / step) * step; x <= uiController.xMax; x += step) {
                                    var px = toPx(x, 0)
                                    ctx.strokeStyle = (Math.abs(x) < 0.0001) ? "#D8DEE9" : "#2E3440"
                                    ctx.beginPath()
                                    ctx.moveTo(px.x, 0)
                                    ctx.lineTo(px.x, height)
                                    ctx.stroke()
                                    if (Math.abs(x) > 0.0001) ctx.fillText(x.toFixed(1), px.x + 2, height - 5)
                                }
                                for (var y = Math.floor(uiController.yMin / step) * step; y <= uiController.yMax; y += step) {
                                    var py = toPx(0, y)
                                    ctx.strokeStyle = (Math.abs(y) < 0.0001) ? "#D8DEE9" : "#2E3440"
                                    ctx.beginPath()
                                    ctx.moveTo(0, py.y)
                                    ctx.lineTo(width, py.y)
                                    ctx.stroke()
                                    if (Math.abs(y) > 0.0001) ctx.fillText(y.toFixed(1), 5, py.y - 2)
                                }

                                // Function Lines
                                var colors = ["#88C0D0", "#BF616A", "#A3BE8C"]
                                var multiPts = uiController.getMultiGraphPoints(600)
                                for (var f=0; f < multiPts.length; f++) {
                                    var pts = multiPts[f]
                                    ctx.beginPath()
                                    ctx.strokeStyle = colors[f % colors.length]
                                    ctx.lineWidth = 2.5
                                    for (var i=0; i < pts.length; i++) { 
                                        var pt = toPx(pts[i].x, pts[i].y)
                                        if (i === 0) ctx.moveTo(pt.x, pt.y); else ctx.lineTo(pt.x, pt.y)
                                    }
                                    ctx.stroke()
                                }
                            }
                            
                            MouseArea {
                                anchors.fill: parent
                                property real lastX
                                property real lastY
                                onPressed: (mouse) => { 
                                    lastX = mouse.x
                                    lastY = mouse.y 
                                }
                                onPositionChanged: (mouse) => { 
                                    if (pressed) { 
                                        uiController.pan(mouse.x - lastX, mouse.y - lastY, width, height)
                                        lastX = mouse.x
                                        lastY = mouse.y 
                                    } 
                                }
                                onWheel: (wheel) => { 
                                    uiController.zoom(wheel.angleDelta.y > 0 ? 0.9 : 1.1, wheel.x, wheel.y, width, height) 
                                }
                            }

                            Connections { 
                                target: uiController
                                function onViewportChanged() { graphCanvas.requestPaint() } 
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: historyPane
            Layout.fillHeight: true
            Layout.preferredWidth: 300
            color: "#2E3440"
            border.color: "#4C566A"
            border.width: 1
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                Text { 
                    text: "HISTORY"
                    color: "#81A1C1"
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                    bottomPadding: 5 
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: uiController.history
                    spacing: 5
                    clip: true
                    delegate: Rectangle { 
                        width: parent.width
                        height: 40
                        color: "#3B4252"
                        radius: 4
                        Text { 
                            anchors.centerIn: parent
                            text: modelData
                            color: "#D8DEE9"
                            font.bold: true
                            font.pixelSize: 12 
                        } 
                    }
                }
            }
        }
    }
}

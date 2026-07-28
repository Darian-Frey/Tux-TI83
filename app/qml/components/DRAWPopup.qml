import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// DRAW menu (Phase D) — opened via 2ND+TRACE. Pick a command, fill in
// its arguments, and Draw adds a persistent overlay to the graph.
// ClrDraw removes every overlay. Args are plain numbers (in graph data
// coordinates).
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 360
    height: 480
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    readonly property var commands: [
        { name: "Line",   labels: ["x1", "y1", "x2", "y2"], text: false },
        { name: "Circle", labels: ["x", "y", "r"],          text: false },
        { name: "Horiz",  labels: ["y"],                     text: false },
        { name: "Vert",   labels: ["x"],                     text: false },
        { name: "Pt-On",  labels: ["x", "y"],                text: false },
        { name: "Text",   labels: ["x", "y"],               text: true  }
    ]
    property int cmd: 0
    readonly property var cur: commands[cmd]
    property var vals: ["", "", "", ""]
    property string txt: ""

    // Bumped on any draw-object change to refresh the drawings list.
    property int rev: 0
    Connections {
        target: uiController
        function onDrawObjectsChanged() { root.rev++ }
    }

    onCmdChanged: { vals = ["", "", "", ""]; txt = "" }

    // Short human label for an overlay in the drawings list.
    function describe(o) {
        var t = o.type
        if (t === "line")   return "Line (" + o.a + "," + o.b + ")→(" + o.c + "," + o.d + ")"
        if (t === "circle") return "Circle (" + o.a + "," + o.b + ") r=" + o.c
        if (t === "hline")  return "Horiz y=" + o.a
        if (t === "vline")  return "Vert x=" + o.a
        if (t === "point")  return "Pt (" + o.a + "," + o.b + ")"
        if (t === "text")   return "Text \"" + o.text + "\""
        return t
    }

    function doDraw() {
        var v = []
        for (var i = 0; i < cur.labels.length; i++) {
            var f = parseFloat(vals[i])
            v.push(Number.isFinite(f) ? f : 0)
        }
        var n = cur.name
        if (n === "Line") uiController.drawLine(v[0], v[1], v[2], v[3])
        else if (n === "Circle") uiController.drawCircle(v[0], v[1], v[2])
        else if (n === "Horiz") uiController.drawHorizontal(v[0])
        else if (n === "Vert") uiController.drawVertical(v[0])
        else if (n === "Pt-On") uiController.drawPoint(v[0], v[1])
        else if (n === "Text") uiController.drawText(v[0], v[1], root.txt)
        // Stay open so several overlays can be added and managed in the
        // list below; DONE shows the result on the graph.
    }

    contentItem: ColumnLayout {
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: "DRAW"
            color: Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.sectionLabelPixelSize
            font.letterSpacing: Style.sectionLabelPixelSize * Style.sectionLabelLetterSpacing
            font.capitalization: Font.AllUppercase
            horizontalAlignment: Text.AlignHCenter
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Style.bgSection
        }

        // ── Command selector ──
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: root.commands
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    radius: 4
                    property bool sel: root.cmd === index
                    color: sel ? Style.secondBg
                               : (cmdArea.containsMouse
                                  ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                                  : Style.bgSection)
                    border.width: sel ? 1 : Style.keyBorderWidth
                    border.color: sel ? Style.secondBorder : Style.keyBorderNeutral
                    Text {
                        anchors.centerIn: parent
                        text: modelData.name
                        color: Style.textPrimary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                    }
                    MouseArea {
                        id: cmdArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.cmd = index
                    }
                }
            }
        }

        // ── Argument fields ──
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 10
            rowSpacing: 6
            Repeater {
                model: root.cur.labels
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: modelData
                        color: Style.textSecondary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.exprPixelSize
                        Layout.preferredWidth: 24
                    }
                    TextField {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        placeholderText: "0"
                        color: Style.textDisplay
                        font.family: Style.monoFamily
                        font.pixelSize: Style.keyLabelPixelSize
                        horizontalAlignment: TextInput.AlignHCenter
                        selectByMouse: true
                        text: (root.cmd, "")  // reset when command changes
                        onTextChanged: root.vals[index] = text
                        background: Rectangle {
                            color: Style.bgDisplay
                            radius: 4
                            border.color: parent.activeFocus ? Style.textExpr : Style.keyBorderNeutral
                            border.width: 1
                        }
                    }
                }
            }
        }
        // Text string (only for the Text command).
        RowLayout {
            visible: root.cur.text
            Layout.fillWidth: true
            spacing: 6
            Text {
                text: "text"
                color: Style.textSecondary
                font.family: Style.monoFamily
                font.pixelSize: Style.exprPixelSize
                Layout.preferredWidth: 34
            }
            TextField {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                placeholderText: "label"
                color: Style.textDisplay
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
                selectByMouse: true
                text: (root.cmd, "")
                onTextChanged: root.txt = text
            }
        }

        // ── Current drawings (delete one at a time) ──
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Style.bgSection
        }
        Text {
            text: "DRAWINGS  (" + (root.rev, uiController.getDrawObjects().length) + ")"
            color: Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.funcKeyLabelPixelSize
        }
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 3
            boundsBehavior: Flickable.StopAtBounds
            model: (root.rev, uiController.getDrawObjects())
            delegate: RowLayout {
                width: ListView.view.width
                height: 26
                spacing: 6
                Text {
                    Layout.fillWidth: true
                    text: root.describe(modelData)
                    color: Style.textDisplay
                    font.family: Style.monoFamily
                    font.pixelSize: Style.funcKeyLabelPixelSize
                    elide: Text.ElideRight
                }
                Rectangle {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 22
                    radius: 4
                    color: delArea.containsMouse
                           ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                           : Style.bgSection
                    border.width: 1
                    border.color: Style.keyBorderNeutral
                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: Style.textError
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                    }
                    MouseArea {
                        id: delArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: uiController.deleteDrawObject(index)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            CalcKey {
                Layout.fillWidth: true
                label: "DRAW"
                keyType: "enter"
                onPressed: root.doDraw()
            }
            CalcKey {
                Layout.fillWidth: true
                label: "CLRDRAW"
                keyType: "control"
                onPressed: uiController.clrDraw()
            }
            CalcKey {
                Layout.fillWidth: true
                label: "DONE"
                keyType: "function"
                onPressed: {
                    // Show the overlays on the graph.
                    if (!uiController.isGraphMode)
                        uiController.toggleGraphMode()
                    root.close()
                }
            }
        }
    }
}

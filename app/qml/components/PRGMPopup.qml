import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tux 1.0
import ".."

// PRGM — TI-BASIC program manager + editor (P1). Two modes:
//   • list  — browse programs; EDIT / RUN each, NEW, or delete (✕).
//   • edit  — name field (for NEW) + a multi-line source editor + SAVE.
//
// Programs are stored as source text; the interpreter re-tokenises each
// line at run time (see docs/TIBASIC.md). RUN currently just confirms
// completion in history — real Disp output + a run view arrive in P2.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 360
    height: 500
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    // State.
    property bool editing: false
    property bool isNew: false
    property string editName: ""
    // Program list, refreshed from the controller.
    property var names: []
    // Pending "open editor at line" request (applied in onOpened so it
    // survives the open transition). -1 line = just edit, no positioning.
    property string pendingEditName: ""
    property int pendingEditLine: -1

    // ── Command-paste palette (insert keywords without hand-typing) ──
    property bool cmdPaletteOpen: false
    property string cmdCategory: "CTL"
    // Category → the exact text each button inserts (trailing space on
    // statement words; a bare "(" left open for the user to fill args).
    readonly property var cmdSet: ({
        "CTL": ["If ", "Then", "Else", "For(", "While ", "Repeat ", "End",
                "break", "continue", "Try", "Lbl ", "Goto ", "Pause", "Stop",
                "Define ", "Return ", "Local ", "DelVar ", "Menu(", "prgm",
                "# "],
        "I/O": ["Disp ", "Input ", "Prompt ", "Output(", "ClrHome", "getKey"],
        "STR": ["Str1", "Str2", "Str3", "sub(", "length(", "inString(",
                "expr(", "toString(", "\"\""],
        "FN":  ["→", ":", "sin(", "cos(", "tan(", "√(", "ln(", "log(", "abs(",
                "round(", "int(", "nCr(", "nPr(", "SortA(", "SortD("],
        "GFX": ["DispGraph", "ClrDraw", "Line(", "Circle(", "Horizontal ",
                "Vertical ", "Pt-On(", "Pt-Off(", "Pt-Change(", "Pxl-On(",
                "Pxl-Off(", "Pxl-Test(", "Text(", "FnOn ", "FnOff ",
                "ZStandard", "→Y1"]
    })

    // Insert `text` at the editor's cursor and keep focus there. For the
    // quote pair, drop the cursor between the quotes ready for the string.
    function insertCmd(text) {
        var pos = bodyArea.cursorPosition
        bodyArea.insert(pos, text)
        bodyArea.cursorPosition = (text === "\"\"") ? pos + 1 : pos + text.length
        bodyArea.forceActiveFocus()
    }

    function refresh() { names = uiController.programNames() }

    function startNew() {
        isNew = true
        editName = ""
        nameField.text = ""
        bodyArea.text = ""
        editing = true
    }
    function startEdit(name) {
        isNew = false
        editName = name
        nameField.text = name
        bodyArea.text = uiController.programText(name)
        editing = true
    }
    function doSave() {
        var target = isNew ? nameField.text : editName
        var saved = uiController.saveProgram(target, bodyArea.text)
        if (saved.length > 0) {
            editing = false
            refresh()
        }
    }
    // Open the editor for `name` with the cursor on `line` (0-based). Defers
    // to onOpened so it works whether the popup is already open or not.
    function openAtLine(name, line) {
        pendingEditName = name
        pendingEditLine = line
        if (visible) applyPendingEdit()
        else open()
    }
    function applyPendingEdit() {
        if (pendingEditName.length === 0)
            return
        startEdit(pendingEditName)
        if (pendingEditLine >= 0) {
            var lines = bodyArea.text.split("\n")
            var pos = 0
            for (var i = 0; i < pendingEditLine && i < lines.length; i++)
                pos += lines[i].length + 1  // +1 for the newline
            var endPos = (pendingEditLine < lines.length)
                         ? pos + lines[pendingEditLine].length : pos
            bodyArea.cursorPosition = pos
            bodyArea.select(pos, endPos)  // highlight the offending line
            bodyArea.forceActiveFocus()
        }
        pendingEditName = ""
        pendingEditLine = -1
    }

    onOpened: {
        refresh()
        if (pendingEditName.length > 0)
            applyPendingEdit()
        else
            editing = false
    }

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    Connections {
        target: uiController
        function onProgramsChanged() { root.refresh() }
    }

    contentItem: ColumnLayout {
        spacing: 10

        // ── Header ──
        Text {
            Layout.fillWidth: true
            text: root.editing ? (root.isNew ? "PRGM · NEW" : "PRGM · EDIT " + root.editName)
                               : "PRGM"
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

        // ── LIST MODE ──
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            visible: !root.editing

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.names
                spacing: 4
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: RowLayout {
                    width: ListView.view.width
                    spacing: 6

                    Text {
                        Layout.fillWidth: true
                        text: "prgm" + modelData
                        color: Style.textPrimary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.keyLabelPixelSize
                        elide: Text.ElideRight
                    }
                    CalcKey {
                        Layout.preferredWidth: 56
                        Layout.fillWidth: false
                        label: "RUN"
                        keyType: "enter"
                        onPressed: { root.close(); uiController.runProgram(modelData) }
                    }
                    CalcKey {
                        Layout.preferredWidth: 56
                        Layout.fillWidth: false
                        label: "EDIT"
                        keyType: "function"
                        onPressed: root.startEdit(modelData)
                    }
                    CalcKey {
                        Layout.preferredWidth: 34
                        Layout.fillWidth: false
                        label: "✕"
                        keyType: "control"
                        onPressed: uiController.deleteProgram(modelData)
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.names.length === 0
                text: "No programs yet — press NEW."
                color: Style.textMuted
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
                horizontalAlignment: Text.AlignHCenter
            }

            CalcKey {
                Layout.fillWidth: true
                label: "NEW"
                keyType: "function"
                onPressed: root.startNew()
            }
        }

        // ── EDIT MODE ──
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            visible: root.editing

            // Name field (editable only when creating a new program).
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    text: "Name:"
                    color: Style.textPrimary
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                }
                TextField {
                    id: nameField
                    Layout.fillWidth: true
                    enabled: root.isNew
                    placeholderText: "A–Z, 0–9 · max 8"
                    color: Style.textDisplay
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                    maximumLength: 8
                    background: Rectangle {
                        color: Style.bgDisplay
                        radius: 4
                        border.color: nameField.activeFocus ? Style.textExpr : Style.keyBorderNeutral
                        border.width: 1
                    }
                }
            }

            // Program source editor (dark, monospace, scrollable).
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                background: Rectangle {
                    color: Style.bgDisplay
                    radius: 4
                    border.color: Style.keyBorderNeutral
                    border.width: 1
                }
                TextArea {
                    id: bodyArea
                    wrapMode: TextArea.NoWrap
                    placeholderText: "one statement per line\ne.g.  5→A   (type -> for →)\n      Disp A"
                    color: Style.textDisplay
                    selectByMouse: true
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                    padding: 8

                    // Syntax highlighting (keywords / variables / strings /
                    // numbers / # comments), attached to the text document.
                    ProgramHighlighter { textDocument: bodyArea.textDocument }
                }
            }

            // ── Command-paste palette ──
            CalcKey {
                Layout.fillWidth: true
                label: root.cmdPaletteOpen ? "⌨ COMMANDS ▴" : "⌨ COMMANDS ▾"
                keyType: "function"
                onPressed: root.cmdPaletteOpen = !root.cmdPaletteOpen
            }
            ColumnLayout {
                Layout.fillWidth: true
                visible: root.cmdPaletteOpen
                spacing: 6

                // Category tabs.
                Row {
                    Layout.fillWidth: true
                    spacing: 4
                    Repeater {
                        model: ["CTL", "I/O", "STR", "FN", "GFX"]
                        Rectangle {
                            width: tabText.width + 16
                            height: 26
                            radius: 4
                            color: root.cmdCategory === modelData ? Style.opBg : Style.bgSurface
                            border.width: 1
                            border.color: root.cmdCategory === modelData
                                          ? Style.textExpr : Style.keyBorderNeutral
                            Text {
                                id: tabText
                                anchors.centerIn: parent
                                text: modelData
                                color: Style.textPrimary
                                font.family: Style.monoFamily
                                font.pixelSize: Style.keyLabelPixelSize
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: root.cmdCategory = modelData
                            }
                        }
                    }
                }

                // Commands in the selected category (wrapping, scrollable).
                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                    clip: true
                    Flow {
                        width: parent.width
                        spacing: 4
                        Repeater {
                            model: root.cmdSet[root.cmdCategory]
                            Rectangle {
                                width: cmdText.width + 14
                                height: 28
                                radius: 4
                                color: cmdMouse.pressed ? Style.bgSection : Style.bgSurface
                                border.color: Style.keyBorderNeutral
                                border.width: 1
                                Text {
                                    id: cmdText
                                    anchors.centerIn: parent
                                    text: modelData.trim().length ? modelData.trim() : modelData
                                    color: Style.textPrimary
                                    font.family: Style.monoFamily
                                    font.pixelSize: Style.keyLabelPixelSize
                                }
                                MouseArea {
                                    id: cmdMouse
                                    anchors.fill: parent
                                    onClicked: root.insertCmd(modelData)
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                CalcKey {
                    label: "SAVE"
                    keyType: "enter"
                    onPressed: root.doSave()
                }
                CalcKey {
                    label: "CANCEL"
                    keyType: "control"
                    onPressed: { root.editing = false; root.refresh() }
                }
            }
        }

        // ── DONE — dismiss ──
        CalcKey {
            label: "DONE"
            keyType: "control"
            onPressed: root.close()
        }
    }
}

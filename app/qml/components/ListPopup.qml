import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Stat list editor (Phase C Wave 2). A 1-D analogue of the matrix editor
// v2: a selector for L1–L6, a length stepper, an editable column of
// cells, and read-back of the stored list on open / selection change so
// editing an existing list doesn't require retyping.
//
// Values are held in a flat `cells` string array that each TextField
// initialises from (Component.onCompleted) and writes back to
// (onTextChanged); reloads push fresh values in via Qt.callLater(sync).
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 300
    height: 520
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    property string selectedList: "L1"
    property int mLen: 3
    property var cells: []
    readonly property int maxLen: 10

    // Emitted when the user asks for 1-Var Stats on the selected list;
    // Main.qml computes the bundle and shows the results popup.
    signal statsRequested(string listName)
    // Emitted for 2-Var Stats + LinReg over L1 (Xlist) and L2 (Ylist) —
    // the TI-83 defaults.
    signal twoVarRequested()
    // Emitted to open the regression-model picker (also L1/L2).
    signal regMenuRequested()

    function cellText(i) { return (i >= 0 && i < cells.length) ? cells[i] : "" }
    function setCell(i, t) { if (i >= 0 && i < cells.length) cells[i] = t }

    function loadList(name) {
        selectedList = name
        var data = uiController.getList(name)
        var n = (data && data.length > 0) ? Math.min(data.length, maxLen) : 3
        var arr = []
        for (var i = 0; i < n; i++)
            arr.push(i < data.length ? String(data[i]) : "")
        cells = arr
        mLen = n
        Qt.callLater(syncFields)
    }

    function resizeCells() {
        var arr = []
        for (var i = 0; i < mLen; i++)
            arr.push(i < cells.length ? cells[i] : "")
        cells = arr
        Qt.callLater(syncFields)
    }

    function syncFields() {
        for (var i = 0; i < cellCol.children.length; i++) {
            var ch = cellCol.children[i]
            if (ch && ch.hasOwnProperty("cellIndex") && ch.hasOwnProperty("text"))
                ch.text = cellText(ch.cellIndex)
        }
    }

    // Persist the filled cells (blanks skipped) of the selected list.
    // Shared by SAVE and the stat buttons so what you see is what's
    // computed; an untouched list stays undefined.
    function commitCells() {
        var vals = []
        for (var i = 0; i < mLen; i++) {
            var raw = cells[i]
            if (raw === undefined || raw === "")
                continue
            var v = parseFloat(raw)
            if (Number.isFinite(v))
                vals.push(v)
        }
        if (vals.length > 0)
            uiController.updateList(selectedList, vals)
    }

    onOpened: loadList(selectedList)

    contentItem: ColumnLayout {
        spacing: 10

        // ── Header ──
        Text {
            Layout.fillWidth: true
            text: "LIST"
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

        // ── Selector L1–L6 ──
        Row {
            Layout.fillWidth: true
            spacing: 5
            Repeater {
                model: ["L1", "L2", "L3", "L4", "L5", "L6"]
                Rectangle {
                    width: (root.width - 2 * root.padding - 5 * 5) / 6
                    height: 30
                    radius: 4
                    property bool sel: root.selectedList === modelData
                    color: selArea.containsMouse
                           ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                           : Style.bgSection
                    border.width: 1
                    border.color: sel ? Style.textExpr : Style.keyBorderNeutral
                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        color: parent.sel ? Style.textExpr : Style.textSecondary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                    }
                    MouseArea {
                        id: selArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.loadList(modelData)
                    }
                }
            }
        }

        // ── Header label + length stepper ──
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
                text: "Edit " + root.selectedList
                color: Style.textSecondary
                font.family: Style.monoFamily
                font.pixelSize: Style.exprPixelSize
            }
            Item { Layout.fillWidth: true }
            Text {
                Layout.alignment: Qt.AlignVCenter
                text: "len"
                color: Style.textMuted
                font.family: Style.monoFamily
                font.pixelSize: Style.funcKeyLabelPixelSize
            }
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                width: 26; height: 26; radius: 4
                color: decArea.containsMouse
                       ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                       : Style.bgSection
                border.width: 1
                border.color: Style.keyBorderNeutral
                Text {
                    anchors.centerIn: parent; text: "−"
                    color: Style.textPrimary
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                }
                MouseArea {
                    id: decArea; anchors.fill: parent; hoverEnabled: true
                    onClicked: if (root.mLen > 1) { root.mLen--; root.resizeCells() }
                }
            }
            Text {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 18
                horizontalAlignment: Text.AlignHCenter
                text: root.mLen
                color: Style.textDisplay
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
            }
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                width: 26; height: 26; radius: 4
                color: incArea.containsMouse
                       ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                       : Style.bgSection
                border.width: 1
                border.color: Style.keyBorderNeutral
                Text {
                    anchors.centerIn: parent; text: "+"
                    color: Style.textPrimary
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                }
                MouseArea {
                    id: incArea; anchors.fill: parent; hoverEnabled: true
                    onClicked: if (root.mLen < root.maxLen) { root.mLen++; root.resizeCells() }
                }
            }
        }

        // ── Editable cells (one column, index labels on the left) ──
        ColumnLayout {
            id: cellCol
            Layout.fillWidth: true
            spacing: 5
            Repeater {
                id: cellRepeater
                model: root.mLen
                RowLayout {
                    property int cellIndex: index
                    property alias text: field.text
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        text: (index + 1) + ":"
                        color: Style.textMuted
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                        Layout.preferredWidth: 22
                        horizontalAlignment: Text.AlignRight
                    }
                    TextField {
                        id: field
                        placeholderText: "0"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        color: Style.textDisplay
                        font.family: Style.monoFamily
                        font.pixelSize: Style.keyLabelPixelSize
                        horizontalAlignment: TextInput.AlignHCenter
                        selectByMouse: true
                        Component.onCompleted: text = root.cellText(index)
                        onTextChanged: root.setCell(index, text)
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

        Item { Layout.fillHeight: true }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            CalcKey {
                Layout.fillWidth: true
                label: "SAVE TO " + root.selectedList
                keyType: "enter"
                onPressed: { root.commitCells(); root.close() }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                // 1-Var Stats over the selected list.
                CalcKey {
                    Layout.fillWidth: true
                    label: "1-VAR STATS"
                    keyType: "function"
                    onPressed: {
                        root.commitCells()
                        var name = root.selectedList
                        root.close()
                        root.statsRequested(name)
                    }
                }
                // 2-Var Stats + LinReg over L1 (X) and L2 (Y).
                CalcKey {
                    Layout.fillWidth: true
                    label: "2-VAR L1,L2"
                    keyType: "function"
                    onPressed: {
                        root.commitCells()
                        root.close()
                        root.twoVarRequested()
                    }
                }
            }
            // Other regression models (Quad/Cubic/Exp/Ln/Pwr) over L1,L2.
            CalcKey {
                Layout.fillWidth: true
                label: "REGRESSIONS ▸"
                keyType: "function"
                onPressed: {
                    root.commitCells()
                    root.close()
                    root.regMenuRequested()
                }
            }
        }
    }
}

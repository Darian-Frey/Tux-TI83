import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Vertical inequalities manager (Inequalz `X <rel> value`). Lists the
// current X inequalities (each: a relation chip + value + ✕ delete) and an
// "add" row (relation cycle + value field + ADD). Each entry shades a
// vertical half-plane on the graph, combining with the Y inequalities under
// the FORMAT → Ineq union / intersect mode. Opened from the Y= editor.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 320
    // Size to content so a growing list never clips DONE.
    height: Math.min(col.implicitHeight + topPadding + bottomPadding, 460)
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    // Relations are 1..4 (< > ≤ ≥); index into this with rel-1.
    readonly property var relGlyphs: ["<", ">", "≤", "≥"]
    // The relation currently selected in the add row (1-based).
    property int newRel: 1

    property int rev: 0
    Connections {
        target: uiController
        function onFunctionsChanged() { root.rev++ }
    }

    contentItem: ColumnLayout {
        id: col
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: "X INEQUALITIES"
            color: Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.popupTitlePixelSize
            font.letterSpacing: Style.popupTitlePixelSize * Style.sectionLabelLetterSpacing
            font.capitalization: Font.AllUppercase
            horizontalAlignment: Text.AlignHCenter
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Style.bgSection }

        // ── Current inequalities ──
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 180)
            clip: true
            model: (root.rev, uiController.getXIneqs())
            spacing: 4
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: RowLayout {
                width: ListView.view.width
                spacing: 6
                Text {
                    Layout.fillWidth: true
                    text: "X " + root.relGlyphs[modelData.rel - 1] + " " +
                          uiController.formatScalar(modelData.val)
                    color: Style.textExpr
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                }
                CalcKey {
                    Layout.preferredWidth: 34
                    Layout.fillWidth: false
                    label: "✕"
                    keyType: "control"
                    onPressed: uiController.removeXIneq(index)
                }
            }
        }
        Text {
            Layout.fillWidth: true
            visible: (root.rev, uiController.getXIneqs().length === 0)
            text: "None yet — add one below."
            color: Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.keyLabelPixelSize
            horizontalAlignment: Text.AlignHCenter
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Style.bgSection }

        // ── Add row: X [rel] [value] ADD ──
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                text: "X"
                color: Style.textPrimary
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
            }
            // Relation cycle chip.
            Rectangle {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                radius: 4
                color: relArea.containsMouse
                       ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                       : Style.bgSection
                border.width: 1
                border.color: Style.textExpr
                Text {
                    anchors.centerIn: parent
                    text: root.relGlyphs[root.newRel - 1]
                    color: Style.textExpr
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                }
                MouseArea {
                    id: relArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.newRel = (root.newRel % 4) + 1
                }
            }
            TextField {
                id: valField
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                placeholderText: "value"
                color: Style.textDisplay
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
                selectByMouse: true
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                background: Rectangle {
                    color: Style.bgDisplay
                    radius: 4
                    border.color: valField.activeFocus ? Style.textExpr : Style.keyBorderNeutral
                    border.width: 1
                }
                onAccepted: addBtn.commit()
            }
            CalcKey {
                id: addBtn
                Layout.preferredWidth: 60
                Layout.fillWidth: false
                label: "ADD"
                keyType: "enter"
                function commit() {
                    const v = parseFloat(valField.text)
                    if (!isNaN(v)) {
                        uiController.addXIneq(root.newRel, v)
                        valField.text = ""
                    }
                }
                onPressed: commit()
            }
        }

        CalcKey {
            label: "DONE"
            keyType: "control"
            onPressed: root.close()
        }
    }
}

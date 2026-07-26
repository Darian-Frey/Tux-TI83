import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Memory management (2ND++). Shows what's currently in use and offers
// targeted clearing per category, plus the full factory RESET. Counts
// refresh on open and after every action.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 320
    height: 460
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    property var info: ({})
    function refresh() { info = uiController.memInfo() }
    onOpened: refresh()

    contentItem: ColumnLayout {
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: "MEMORY"
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

        // ── In-use summary ──
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 16
            rowSpacing: 3
            Repeater {
                model: [
                    { k: "vars",      label: "Vars A–Z" },
                    { k: "matrices",  label: "Matrices" },
                    { k: "lists",     label: "Lists" },
                    { k: "functions", label: "Y= funcs" },
                    { k: "entries",   label: "History" }
                ]
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: modelData.label
                        color: Style.textSecondary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                        Layout.fillWidth: true
                    }
                    Text {
                        text: (root.info && root.info[modelData.k] !== undefined)
                              ? root.info[modelData.k] : "0"
                        color: Style.textDisplay
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Style.bgSection
        }

        // ── Targeted clears ──
        CalcKey {
            Layout.fillWidth: true
            label: "Clear All Lists"
            keyType: "function"
            onPressed: { uiController.clearAllLists(); root.refresh() }
        }
        CalcKey {
            Layout.fillWidth: true
            label: "Clear All Matrices"
            keyType: "function"
            onPressed: { uiController.clearAllMatrices(); root.refresh() }
        }
        CalcKey {
            Layout.fillWidth: true
            label: "Clear Vars A–Z"
            keyType: "function"
            onPressed: { uiController.clearAllVars(); root.refresh() }
        }
        CalcKey {
            Layout.fillWidth: true
            label: "Clear Entries"
            keyType: "function"
            onPressed: { uiController.clearEntries(); root.refresh() }
        }
        CalcKey {
            Layout.fillWidth: true
            label: "RESET (everything)"
            keyType: "control"
            onPressed: { uiController.resetAll(); root.close() }
        }

        Item { Layout.fillHeight: true }

        CalcKey {
            label: "DONE"
            keyType: "enter"
            onPressed: root.close()
        }
    }
}

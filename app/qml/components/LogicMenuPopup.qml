import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Modal popup for the TI-83's TEST + LOGIC operator catalogue (opened
// via 2ND + MATH). Replacement for the legacy LOGIC popup that was
// deleted during Phase A, now that all target operators are actually
// wired on the engine side.
//
// Visual contract: two section headers ("TEST" for comparators,
// "LOGIC" for boolean ops) each followed by a numbered list of entries.
// Same interaction pattern as MathMenuPopup — click an entry to insert
// the token and close the popup.
//
// Behavioural contract: each entry's `input` string goes through
// `uiController.processInput()` (single-token) so the display's
// rebuild uses the canonical kTokens displayStr. Operators with an
// ASCII alias in kTokens (<=, >=) still go through the Unicode form
// here — the popup is for discoverability, the aliases are for typing.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Height sized for 6 TEST rows + "LOGIC" section header + 4 LOGIC
    // rows + outer padding, so the `not` row at the bottom renders in
    // full. If any more ops land here, bump this again.
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

    // Two parallel arrays, one per sub-menu. Keeping them separate
    // lets us render section labels without threading sentinels through
    // a single model.
    readonly property var testEntries: [
        { label: "=",  input: "=" },
        { label: "≠",  input: "≠" },
        { label: "<",  input: "<" },
        { label: "≤",  input: "≤" },
        { label: ">",  input: ">" },
        { label: "≥",  input: "≥" }
    ]
    readonly property var logicEntries: [
        { label: "and", input: "and" },
        { label: "or",  input: "or"  },
        { label: "xor", input: "xor" },
        { label: "not", input: "not" }
    ]

    // Inline component for an entry row. Both sub-menus use identical
    // formatting so this factors the delegate out of the two Repeaters
    // below.
    component EntryRow: Rectangle {
        id: row
        property int    indexLabel: 1
        property string labelText: ""
        property string inputText: ""

        Layout.fillWidth: true
        Layout.preferredHeight: 30
        radius: 4
        color: rowArea.containsMouse
               ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
               : Style.bgSection

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 8

            Text {
                text: row.indexLabel + ":"
                color: Style.textMuted
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
                Layout.preferredWidth: 20
            }
            Text {
                text: row.labelText
                color: Style.textDisplay
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
                Layout.fillWidth: true
            }
        }

        MouseArea {
            id: rowArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                uiController.processInput(row.inputText)
                root.close()
            }
        }
    }

    // Small component for the section headers (TEST / LOGIC).
    component SectionLabel: ColumnLayout {
        property string label: ""
        Layout.fillWidth: true
        spacing: 4
        Text {
            text: parent.label
            color: Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.sectionLabelPixelSize
            font.letterSpacing: Style.sectionLabelPixelSize * Style.sectionLabelLetterSpacing
            font.capitalization: Font.AllUppercase
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Style.bgSection
        }
    }

    contentItem: ColumnLayout {
        spacing: 8

        // ── Header ──
        Text {
            Layout.fillWidth: true
            text: "TEST"
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

        // Comparators (TEST on a real TI-83). Numbered 1..6.
        Repeater {
            model: root.testEntries
            delegate: EntryRow {
                indexLabel: index + 1
                labelText: modelData.label
                inputText: modelData.input
            }
        }

        // Section break before the boolean ops.
        SectionLabel { label: "LOGIC" }

        // Boolean ops (LOGIC on a real TI-83). Numbered 1..4.
        Repeater {
            model: root.logicEntries
            delegate: EntryRow {
                indexLabel: index + 1
                labelText: modelData.label
                inputText: modelData.input
            }
        }

        Item { Layout.fillHeight: true }
    }
}

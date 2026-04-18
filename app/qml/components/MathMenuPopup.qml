import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Modal popup listing the MATH-menu functions available to insert into
// the current expression.
//
// Visual contract: a centred dialog with a scrollable ListView of
// functions. Each entry shows a TI-83-style numbered label
// (`1: abs(`, `2: int(`, ...) and inserts the function token when
// clicked. Same interaction pattern as the MatrixPopup's MATH tab.
//
// Behavioural contract: each entry's `input` string is passed to
// `uiController.processInput()` when clicked; the popup auto-closes
// after insertion. Binary functions (round, min, max, mod) are
// included with "(needs binary args)" placeholders until Wave 2
// binary-function infrastructure lands.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 320
    height: 360
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    // Single source of truth for the menu. Adding a new MATH entry =
    // adding one row here. `input` is what gets sent to processInput();
    // `available` is the gate for Wave 2 features that aren't wired yet.
    readonly property var entries: [
        { label: "abs(",   input: "abs(",   available: true  },
        { label: "int(",   input: "int(",   available: true  },
        { label: "iPart(", input: "iPart(", available: true  },
        { label: "fPart(", input: "fPart(", available: true  },
        { label: "round(", input: "round(", available: false },
        { label: "min(",   input: "min(",   available: false },
        { label: "max(",   input: "max(",   available: false },
        { label: "mod(",   input: "mod(",   available: false },
        { label: "▶Frac",  input: "▶Frac",  available: true  },
        { label: "▶Dec",   input: "▶Dec",   available: true  }
    ]

    contentItem: ColumnLayout {
        spacing: 10

        // ── Header ──
        Text {
            Layout.fillWidth: true
            text: "MATH"
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

        // ── Entry list ──
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            model: root.entries
            spacing: 4
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: list.width
                height: 32
                radius: 4
                color: modelData.available
                       ? (entryArea.containsMouse
                           ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                           : Style.bgSection)
                       : Style.bgSection
                opacity: modelData.available ? 1.0 : 0.4

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Text {
                        text: (index + 1) + ":"
                        color: Style.textMuted
                        font.family: Style.monoFamily
                        font.pixelSize: Style.keyLabelPixelSize
                        Layout.preferredWidth: 20
                    }
                    Text {
                        text: modelData.label
                        color: Style.textDisplay
                        font.family: Style.monoFamily
                        font.pixelSize: Style.keyLabelPixelSize
                        Layout.fillWidth: true
                    }
                    Text {
                        visible: !modelData.available
                        text: "(binary — Wave 2)"
                        color: Style.textMuted
                        font.family: Style.monoFamily
                        font.pixelSize: Style.exprPixelSize
                        font.italic: true
                    }
                }

                MouseArea {
                    id: entryArea
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: modelData.available
                    onClicked: {
                        uiController.processInput(modelData.input)
                        root.close()
                    }
                }
            }
        }
    }
}

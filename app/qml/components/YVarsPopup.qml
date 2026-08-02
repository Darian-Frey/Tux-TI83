import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Y-VARS picker — inserts a Y-function token (Y1..Y0) into the current
// expression at the cursor. Opened via 2ND+X.
//
// Fixes BUG-023: the on-screen keypad otherwise has no way to enter a
// Y-function token — ALPHA+1 inserts the *letter-Y* scalar variable, so
// "Y1(3)" typed on the keypad parses as Y·1·(3) = 0 instead of recalling
// the Y1 function. processExpression("Yn") tokenises to the fused Yn
// token (longest match in the token table), so each button inserts the
// real function reference. Pair with "(3)" for a call, or use bare.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 260
    height: 340
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 12

        // ── Header ──
        Text {
            Layout.fillWidth: true
            text: "Y-VARS"
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

        // ── Y1..Y0 grid (2 columns). Each button inserts the fused token. ──
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 6
            columnSpacing: 6

            Repeater {
                model: ["Y1", "Y2", "Y3", "Y4", "Y5",
                        "Y6", "Y7", "Y8", "Y9", "Y0"]
                delegate: CalcKey {
                    label: modelData
                    keyType: "function"
                    onPressed: {
                        uiController.processExpression(modelData)
                        root.close()
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        CalcKey {
            label: "DONE"
            keyType: "enter"
            onPressed: root.close()
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Modal popup for global calculator settings — the TI-83's MODE screen.
//
// Visual contract: a centred dialog with one row per setting. Each row
// shows the label on the left and a pair of selectable segments on the
// right; the active segment is highlighted in the 2ND-amber accent so
// the UI reads as "armed with this choice". Rows that haven't been
// wired yet render their segments with reduced opacity and don't
// respond to clicks — this preserves the real TI-83's MODE layout as a
// roadmap of what's coming without pretending settings exist.
//
// Behavioural contract: the Angle row writes to `uiController.angleMode`
// (0 = Radian, 1 = Degree); the header's angle indicator binds to the
// same property, so flipping the setting updates the badge immediately.
// All other rows are visual placeholders — no backing property yet.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // The Decimal row has 11 segments (Float, 0..9). 340px was fine for
    // ≤4-option rows but squeezed each segment to ~28px here — bumped
    // to 420 so "Float" and the single-digit segments both breathe.
    // Height bumped a second time (was 420) to fit the RESET button
    // added below the option rows — DONE was clipped off the bottom.
    width: 420
    height: 500
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    // Each row is a single-select group of segments. `active` controls
    // whether clicks route to the provided onSelected; inactive rows
    // still render so users see the full TI-83 MODE layout.
    component ModeRow: RowLayout {
        id: row
        property string label: ""
        property var options: []            // list of display strings
        property int  selectedIndex: 0
        property bool active: false
        signal selected(int index)

        Layout.fillWidth: true
        spacing: 8

        Text {
            text: row.label
            color: row.active ? Style.textSecondary : Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.exprPixelSize
            Layout.preferredWidth: 70
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: row.options
                delegate: Rectangle {
                    // `fillWidth` with a `minimumWidth` tied to the text's
                    // implicit width keeps short digit segments modest and
                    // lets long labels (e.g. "Float", "Connected") expand
                    // to fit their text without being clipped. +12 is the
                    // horizontal padding around the centred label.
                    Layout.fillWidth: true
                    Layout.minimumWidth: segLabel.implicitWidth + 12
                    Layout.preferredHeight: 26
                    radius: 4
                    opacity: row.active ? 1.0 : 0.4
                    color: index === row.selectedIndex
                           ? Style.secondBg
                           : (segMouse.containsMouse
                              ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                              : Style.bgSection)
                    border.color: index === row.selectedIndex
                                  ? Style.secondBorder
                                  : Style.keyBorderNeutral
                    border.width: index === row.selectedIndex ? 1 : Style.keyBorderWidth
                    Text {
                        id: segLabel
                        anchors.centerIn: parent
                        text: modelData
                        color: Style.textPrimary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                        font.weight: Style.keyLabelFontWeight
                    }
                    MouseArea {
                        id: segMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: row.active
                        onClicked: row.selected(index)
                    }
                }
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 12

        // ── Header ──
        Text {
            Layout.fillWidth: true
            text: "MODE"
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

        // Angle — wired. Changing this immediately affects the engine's
        // trig functions for every subsequent evaluation.
        ModeRow {
            label: "Angle"
            options: ["Radian", "Degree"]
            selectedIndex: uiController.angleMode
            active: true
            onSelected: (index) => uiController.angleMode = index
        }

        // Notation: Normal / Sci / Eng — wired. Feeds formatScalar, so
        // the next evaluation reflects the selection.
        ModeRow {
            label: "Notation"
            options: ["Normal", "Sci", "Eng"]
            selectedIndex: uiController.notation
            active: true
            onSelected: (index) => uiController.notation = index
        }

        // Decimal: Float / 0..9 — wired. Selection index 0 maps to the
        // Float sentinel (-1 on the controller); 1..10 map to Fix 0..9.
        ModeRow {
            label: "Decimal"
            options: ["Float", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"]
            selectedIndex: uiController.fixDecimals < 0
                           ? 0
                           : uiController.fixDecimals + 1
            active: true
            onSelected: (index) => {
                uiController.fixDecimals = (index === 0) ? -1 : (index - 1)
            }
        }
        // Base: Dec / Hex / Oct / Bin — wired. Affects formatScalar for
        // integer-valued results; non-integers always render in decimal.
        ModeRow {
            label: "Base"
            options: ["Dec", "Hex", "Oct", "Bin"]
            selectedIndex: uiController.numberBase
            active: true
            onSelected: (index) => uiController.numberBase = index
        }
        ModeRow {
            label: "Graph"
            options: ["Func", "Par", "Pol", "Seq"]
            selectedIndex: 0
        }
        ModeRow {
            label: "Draw"
            options: ["Connected", "Dot"]
            selectedIndex: uiController.drawMode
            active: true
            onSelected: (index) => uiController.drawMode = index
        }
        ModeRow {
            label: "Plot"
            options: ["Sequential", "Simul"]
            selectedIndex: 0
        }
        ModeRow {
            label: "Complex"
            options: ["Real", "a+bi", "re^θi"]
            selectedIndex: 0
        }
        ModeRow {
            label: "Screen"
            options: ["Full", "Horiz", "G-T"]
            selectedIndex: 0
        }

        Item { Layout.fillHeight: true }

        // Factory reset — wipes scalars / matrices / Y= buffers /
        // history / MODE / viewport and removes the state.json so
        // the next launch starts truly clean. Red CLEAR colour to
        // signal "destructive"; no confirmation prompt since you
        // had to deliberately open MODE first.
        CalcKey {
            label: "RESET"
            keyType: "control"
            onPressed: {
                uiController.resetAll()
                root.close()
            }
        }

        CalcKey {
            label: "DONE"
            keyType: "enter"
            onPressed: root.close()
        }
    }
}

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
// Behavioural contract: wired rows write straight to a controller
// property (Angle, Notation, Decimal, Base, Graph, Draw, Complex) and
// the header indicator binds to the same properties. Plot and Screen
// remain full placeholders (active:false) — their features aren't built,
// so they render greyed rather than pretending the settings exist.
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
    // Size to content so adding an option row never clips DONE off the
    // bottom (was a fixed 540 sized to the exact row count).
    height: col.implicitHeight + topPadding + bottomPadding
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
        // Indices within `options` that render greyed and ignore clicks
        // even when the row is active — used for options that front
        // unimplemented features (e.g. Graph → Par/Seq).
        property var disabledIndices: []
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
                    // A segment is interactive only when its row is active
                    // AND it isn't in the row's disabledIndices set. Greyed
                    // segments still render so the full TI-83 MODE layout
                    // shows as a roadmap of what's coming.
                    property bool segEnabled: row.active
                                              && row.disabledIndices.indexOf(index) < 0
                    // `fillWidth` with a `minimumWidth` tied to the text's
                    // implicit width keeps short digit segments modest and
                    // lets long labels (e.g. "Float", "Connected") expand
                    // to fit their text without being clipped. +12 is the
                    // horizontal padding around the centred label.
                    Layout.fillWidth: true
                    Layout.minimumWidth: segLabel.implicitWidth + 12
                    Layout.preferredHeight: 26
                    radius: 4
                    opacity: segEnabled ? 1.0 : 0.4
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
                        enabled: parent.segEnabled
                        onClicked: row.selected(index)
                    }
                }
            }
        }
    }

    // Zoom presets for the "UI Size" row (values map to uiController.uiZoom).
    readonly property var zoomLevels: [0.75, 1.0, 1.25, 1.5, 2.0]
    function zoomIndex() {
        var best = 0, bestD = 1e9
        for (var i = 0; i < zoomLevels.length; i++) {
            var d = Math.abs(zoomLevels[i] - uiController.uiZoom)
            if (d < bestD) { bestD = d; best = i }
        }
        return best
    }

    contentItem: ColumnLayout {
        id: col
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
        // Graph: all four modes wired — Func / Par / Pol / Seq.
        // selectedIndex/onSelected use the option index directly, matching
        // the controller's graphMode encoding (0 Func, 1 Par, 2 Pol, 3 Seq).
        ModeRow {
            label: "Graph"
            options: ["Func", "Par", "Pol", "Seq"]
            selectedIndex: uiController.graphMode
            active: true
            onSelected: (index) => uiController.graphMode = index
        }
        ModeRow {
            label: "Draw"
            options: ["Connected", "Dot"]
            selectedIndex: uiController.drawMode
            active: true
            onSelected: (index) => uiController.drawMode = index
        }
        // Plot: Sequential / Simul — wired. Governs the GraphCanvas draw
        // animation order (each curve fully, vs all curves in lockstep).
        ModeRow {
            label: "Plot"
            options: ["Sequential", "Simul"]
            selectedIndex: uiController.plotMode
            active: true
            onSelected: (index) => uiController.plotMode = index
        }
        // Complex: Real / a+bi / re^θi — wired. Governs whether √ of a
        // negative (etc.) yields a complex result, and the display form.
        ModeRow {
            label: "Complex"
            options: ["Real", "a+bi", "re^θi"]
            selectedIndex: uiController.complexMode
            active: true
            onSelected: (index) => uiController.complexMode = index
        }
        // Screen: Full / Horiz / G-T — wired. Governs the main view layout
        // (single view vs graph-over-keypad vs graph-beside-table).
        ModeRow {
            label: "Screen"
            options: ["Full", "Horiz", "G-T"]
            selectedIndex: uiController.screenMode
            active: true
            onSelected: (index) => uiController.screenMode = index
        }
        // Theme: Dark / Light / Amber — app-wide UI theme (not a TI-83
        // setting, but the MODE screen is our settings hub). Amber is an
        // orange-on-black terminal look. The LCD panel stays dark in all.
        ModeRow {
            label: "Theme"
            options: ["Dark", "Light", "Amber"]
            selectedIndex: uiController.theme
            active: true
            onSelected: (index) => uiController.theme = index
        }
        // UI Size — a persisted zoom multiplier on the whole calculator
        // (window + popups). Not a TI-83 setting; the MODE screen is our
        // settings hub. The comma forces the binding to track uiZoom.
        ModeRow {
            label: "UI Size"
            options: ["75%", "100%", "125%", "150%", "200%"]
            selectedIndex: (uiController.uiZoom, root.zoomIndex())
            active: true
            onSelected: (index) => uiController.uiZoom = root.zoomLevels[index]
        }

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

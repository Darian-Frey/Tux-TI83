import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Graph FORMAT menu (2ND+ZOOM). Toggles for grid lines, axes, the trace
// coordinate readout, and the tick-number labels. Each row writes
// straight to a uiController flag; the graph canvas repaints on change.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 340
    height: 300
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    // Two-segment On/Off row bound to a bool. `onValue` is the segment
    // index (0 or 1) that means "on".
    component ToggleRow: RowLayout {
        id: tr
        property string label: ""
        property bool value: true
        property var options: ["Off", "On"]  // index 1 = on
        signal picked(bool on)

        Layout.fillWidth: true
        spacing: 8

        Text {
            text: tr.label
            color: Style.textSecondary
            font.family: Style.monoFamily
            font.pixelSize: Style.exprPixelSize
            Layout.preferredWidth: 70
            Layout.alignment: Qt.AlignVCenter
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: tr.options
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    radius: 4
                    // index 1 == "On"; selected when value matches.
                    property bool sel: (index === 1) === tr.value
                    color: sel ? Style.secondBg
                               : (segMouse.containsMouse
                                  ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                                  : Style.bgSection)
                    border.width: sel ? 1 : Style.keyBorderWidth
                    border.color: sel ? Style.secondBorder : Style.keyBorderNeutral
                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        color: Style.textPrimary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                    }
                    MouseArea {
                        id: segMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: tr.picked(index === 1)
                    }
                }
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 12

        Text {
            Layout.fillWidth: true
            text: "FORMAT"
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

        ToggleRow {
            label: "Grid"
            value: uiController.gridOn
            onPicked: (on) => uiController.gridOn = on
        }
        ToggleRow {
            label: "Axes"
            value: uiController.axesOn
            onPicked: (on) => uiController.axesOn = on
        }
        ToggleRow {
            label: "Coord"
            value: uiController.coordOn
            onPicked: (on) => uiController.coordOn = on
        }
        ToggleRow {
            label: "Label"
            value: uiController.labelOn
            onPicked: (on) => uiController.labelOn = on
        }
        // Trace coordinate readout mode: RectGC (X/Y) vs PolarGC (R/θ).
        // Reuses the two-segment ToggleRow: "on" == PolarGC.
        ToggleRow {
            label: "GC"
            options: ["Rect", "Polar"]
            value: uiController.coordMode === 1
            onPicked: (polar) => uiController.coordMode = polar ? 1 : 0
        }
        // ExprOn/ExprOff — show the traced function's equation.
        ToggleRow {
            label: "Expr"
            value: uiController.exprOn
            onPicked: (on) => uiController.exprOn = on
        }

        Item { Layout.fillHeight: true }

        CalcKey {
            label: "DONE"
            keyType: "enter"
            onPressed: root.close()
        }
    }
}

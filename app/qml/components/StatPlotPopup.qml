import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// STAT PLOT setup (Phase C Wave 5b) — opened via 2ND+Y=. Configures a
// single Plot1: on/off, type, and the source lists. DONE shows the plot
// (switches to graph mode when the plot is on).
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 380
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

    // A single-select row of segments. `enabled` greys the whole row.
    component SegRow: RowLayout {
        id: sr
        property string label: ""
        property var options: []
        property int selectedIndex: 0
        property bool rowEnabled: true
        signal picked(int index)

        Layout.fillWidth: true
        spacing: 8

        Text {
            text: sr.label
            color: sr.rowEnabled ? Style.textSecondary : Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.exprPixelSize
            Layout.preferredWidth: 60
            Layout.alignment: Qt.AlignVCenter
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: sr.options
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.minimumWidth: segText.implicitWidth + 12
                    Layout.preferredHeight: 28
                    radius: 4
                    opacity: sr.rowEnabled ? 1.0 : 0.4
                    color: index === sr.selectedIndex
                           ? Style.secondBg
                           : (segMouse.containsMouse
                              ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                              : Style.bgSection)
                    border.width: index === sr.selectedIndex ? 1 : Style.keyBorderWidth
                    border.color: index === sr.selectedIndex ? Style.secondBorder
                                                             : Style.keyBorderNeutral
                    Text {
                        id: segText
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
                        enabled: sr.rowEnabled
                        onClicked: sr.picked(index)
                    }
                }
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 12

        Text {
            Layout.fillWidth: true
            text: "STAT PLOT — Plot1"
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

        SegRow {
            label: "Plot1"
            options: ["Off", "On"]
            selectedIndex: uiController.statPlotOn ? 1 : 0
            onPicked: (i) => uiController.statPlotOn = (i === 1)
        }
        SegRow {
            label: "Type"
            options: ["Scatter", "xyLine", "Hist", "Box"]
            selectedIndex: uiController.statPlotType
            onPicked: (i) => uiController.statPlotType = i
        }
        SegRow {
            label: "Xlist"
            options: ["L1", "L2", "L3", "L4", "L5", "L6"]
            selectedIndex: parseInt(uiController.statPlotXList.slice(1)) - 1
            onPicked: (i) => uiController.statPlotXList = "L" + (i + 1)
        }
        SegRow {
            label: "Ylist"
            // Only scatter (0) and xyLine (1) use a Ylist.
            rowEnabled: uiController.statPlotType < 2
            options: ["L1", "L2", "L3", "L4", "L5", "L6"]
            selectedIndex: parseInt(uiController.statPlotYList.slice(1)) - 1
            onPicked: (i) => uiController.statPlotYList = "L" + (i + 1)
        }

        Item { Layout.fillHeight: true }

        CalcKey {
            label: "DONE"
            keyType: "enter"
            onPressed: {
                // Show the result: entering graph mode if the plot is on.
                if (uiController.statPlotOn && !uiController.isGraphMode)
                    uiController.toggleGraphMode()
                root.close()
            }
        }
    }
}

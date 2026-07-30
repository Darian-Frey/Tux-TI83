import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Y= editor (Phase D). Lists the 10 function slots Y1..Y9, Y0 (r1..r0 in
// polar mode). Each row: the Yn label in its curve colour, the current
// expression, an on/off toggle, and a line-style cycle. Tapping the
// expression makes that slot active for keypad editing and closes the
// popup. Opened by the Y= soft-key.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 360
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

    // Bumped whenever the controller reports a change, to re-pull the
    // per-slot expr / on / style through the function accessors.
    property int rev: 0
    Connections {
        target: uiController
        function onFunctionsChanged() { root.rev++ }
        function onDisplayChanged() { root.rev++ }
        function onActiveFunctionIndexChanged() { root.rev++ }
        function onGraphModeSettingChanged() { root.rev++ }
    }

    readonly property var styleGlyphs: ["―", "█", "⋯"]  // thin / thick / dotted

    contentItem: ColumnLayout {
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: (uiController.graphMode === 2 ? "r=" :
                   uiController.graphMode === 1 ? "PARAM" : "Y=") + " EDITOR"
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

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: uiController.functionCount()
            spacing: 4
            boundsBehavior: Flickable.StopAtBounds
            delegate: Rectangle {
                id: slotRow
                width: ListView.view.width
                height: 38
                radius: 4
                // `root.rev` referenced so these refresh on change.
                readonly property bool isActive: (root.rev, uiController.activeFunctionIndex === index)
                readonly property bool on: (root.rev, uiController.functionEnabled(index))
                readonly property color slotColor: Style.graphColors[index % Style.graphColors.length]
                color: isActive ? Style.bgSection : Style.bgDisplay
                border.width: isActive ? 1 : Style.keyBorderWidth
                border.color: isActive ? Style.textExpr : Style.keyBorderNeutral

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 6
                    spacing: 6

                    Text {
                        text: (root.rev, uiController.functionLabel(index))
                        color: slotRow.slotColor
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                        Layout.preferredWidth: 34
                    }
                    Text {
                        Layout.fillWidth: true
                        text: (root.rev, uiController.functionExpr(index)) || "—"
                        color: slotRow.on ? Style.textDisplay : Style.textMuted
                        font.family: Style.monoFamily
                        font.pixelSize: Style.funcKeyLabelPixelSize
                        elide: Text.ElideRight
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                uiController.setActiveFunction(index)
                                root.close()
                            }
                        }
                    }
                    Rectangle {
                        Layout.preferredWidth: 34
                        Layout.preferredHeight: 24
                        radius: 4
                        color: onArea.containsMouse
                               ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                               : Style.bgSection
                        border.width: 1
                        border.color: slotRow.on ? Style.enterBorder : Style.keyBorderNeutral
                        Text {
                            anchors.centerIn: parent
                            text: slotRow.on ? "on" : "off"
                            color: slotRow.on ? Style.textResult : Style.textMuted
                            font.family: Style.monoFamily
                            font.pixelSize: Style.funcKeyLabelPixelSize
                        }
                        MouseArea {
                            id: onArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: uiController.toggleFunctionEnabled(index)
                        }
                    }
                    Rectangle {
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 24
                        radius: 4
                        color: stArea.containsMouse
                               ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                               : Style.bgSection
                        border.width: Style.keyBorderWidth
                        border.color: Style.keyBorderNeutral
                        Text {
                            anchors.centerIn: parent
                            text: (root.rev, root.styleGlyphs[uiController.functionStyle(index)])
                            color: slotRow.slotColor
                            font.family: Style.monoFamily
                            font.pixelSize: Style.funcKeyLabelPixelSize
                        }
                        MouseArea {
                            id: stArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: uiController.cycleFunctionStyle(index)
                        }
                    }
                }
            }
        }

        CalcKey {
            label: "DONE"
            keyType: "enter"
            onPressed: root.close()
        }
    }
}

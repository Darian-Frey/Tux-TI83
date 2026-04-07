import QtQuick
import QtQuick.Layouts
import ".."

// Three-way selector for the active Y function (Y1 / Y2 / Y3).
//
// Visual contract: a horizontal row of three small buttons indicating
// which function buffer the keypad is currently editing. The active
// button uses the operator-blue palette so it visually echoes the
// graph curve colour for that slot.
//
// Behavioural contract: clicking a button calls
// `uiController.setActiveFunction(index)`. The active state is bound
// to `uiController.activeFunctionIndex` so external mode changes
// reflect immediately.
RowLayout {
    id: root

    spacing: 4

    Repeater {
        model: ["Y1", "Y2", "Y3"]
        delegate: Rectangle {
            id: cell
            Layout.fillWidth: true
            Layout.preferredHeight: 22

            readonly property bool active: uiController.activeFunctionIndex === index

            radius: 4
            border.width: Style.keyBorderWidth
            color: active ? Style.opBg
                          : (mouseArea.containsMouse
                              ? Qt.lighter(Style.bgSurface, 1.0 + Style.keyHoverLighten)
                              : Style.bgSurface)
            border.color: active ? Style.opBorder : Style.keyBorderNeutral

            Text {
                anchors.centerIn: parent
                text: modelData
                color: active ? Style.textPrimary : Style.textSecondary
                font.family: Style.monoFamily
                font.pixelSize: Style.funcKeyLabelPixelSize
                font.weight: Style.keyLabelFontWeight
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: uiController.setActiveFunction(index)
            }
        }
    }
}

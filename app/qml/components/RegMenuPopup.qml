import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Regression model picker (Phase C Wave 4c). Lists the regression models
// that operate on L1 (X) / L2 (Y); selecting one emits `picked(type,
// label)` so Main.qml can compute and show the results.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 300
    height: 320
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    signal picked(string type, string label)

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    readonly property var models: [
        { type: "quad",  label: "QuadReg  ax²+bx+c" },
        { type: "cubic", label: "CubicReg  ax³+bx²+cx+d" },
        { type: "exp",   label: "ExpReg  a·bˣ" },
        { type: "ln",    label: "LnReg  a+b·ln x" },
        { type: "pwr",   label: "PwrReg  a·xᵇ" }
    ]

    contentItem: ColumnLayout {
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: "REGRESSIONS  L1,L2"
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
            model: root.models
            spacing: 4
            boundsBehavior: Flickable.StopAtBounds
            delegate: Rectangle {
                width: ListView.view.width
                height: 40
                radius: 4
                color: area.containsMouse
                       ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                       : Style.bgSection
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.label
                    color: Style.textDisplay
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        var ty = modelData.type
                        var lb = modelData.label
                        root.close()
                        root.picked(ty, lb)
                    }
                }
            }
        }

        CalcKey {
            label: "CANCEL"
            keyType: "control"
            onPressed: root.close()
        }
    }
}

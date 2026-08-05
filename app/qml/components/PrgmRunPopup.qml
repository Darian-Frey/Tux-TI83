import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Program run / output view (P2) — the program's "home screen". Shows the
// Disp / echo output on a dark LCD-style panel; opened automatically when a
// program finishes (uiController.programRunFinished). ClrHome clears the
// buffer during a run. Interactive I/O (Input/Prompt/Pause) arrives in P4.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 360
    height: 460
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
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: "PROGRAM OUTPUT"
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

        // Output screen — dark LCD, monospace, scrollable.
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Style.bgDisplay
            radius: 4
            border.color: Style.keyBorderNeutral
            border.width: 1
            clip: true

            ListView {
                id: outView
                anchors.fill: parent
                anchors.margins: 8
                model: uiController.programOutput
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                delegate: Text {
                    width: ListView.view.width
                    text: modelData
                    color: modelData.indexOf("ERR:") === 0 ? Style.textError : Style.textDisplay
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                    wrapMode: Text.WrapAnywhere
                }
            }

            Text {
                anchors.centerIn: parent
                visible: uiController.programOutput.length === 0
                text: "(no output)"
                color: Style.textMuted
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
            }
        }

        CalcKey {
            label: "DONE"
            keyType: "enter"
            onPressed: root.close()
        }
    }
}

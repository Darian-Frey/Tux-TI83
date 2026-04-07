import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Side panel showing the history of evaluated expressions.
//
// Visual contract: a vertical column anchored to the right side of the
// calculator window, with a section-style "HISTORY" header and a
// scrollable list of past evaluations (most recent at top — the
// controller `prepend`s to its history list).
//
// Behavioural contract: read-only display, bound to
// `uiController.history`. Tapping a history entry could one day re-load
// the expression — for now it's purely informational.
Rectangle {
    id: root

    color: Style.bgSection

    // Left edge separator from the calculator column.
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: Style.bgShell
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        anchors.topMargin: 14
        anchors.bottomMargin: 14
        spacing: 6

        // Section header label (matches the CONTROL/SCIENTIFIC/NUMERIC style)
        Text {
            Layout.fillWidth: true
            text: "HISTORY"
            color: Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.sectionLabelPixelSize
            font.letterSpacing: Style.sectionLabelPixelSize * Style.sectionLabelLetterSpacing
            font.capitalization: Font.AllUppercase
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Style.bgShell
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            model: uiController.history
            spacing: 4
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: list.width
                height: entryText.implicitHeight + 12
                color: Style.bgSurface
                radius: 4

                Text {
                    id: entryText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    text: modelData
                    color: Style.textSecondary
                    font.family: Style.monoFamily
                    font.pixelSize: Style.exprPixelSize
                    wrapMode: Text.Wrap
                }
            }

            // Empty-state placeholder shown when history is empty.
            Text {
                anchors.centerIn: parent
                visible: list.count === 0
                text: "No history yet"
                color: Style.textMuted
                font.family: Style.monoFamily
                font.pixelSize: Style.funcKeyLabelPixelSize
                font.italic: true
            }
        }
    }
}

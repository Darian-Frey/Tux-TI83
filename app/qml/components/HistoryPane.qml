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
// `uiController.history`. Right-clicking an entry opens a context menu
// with "Copy" — copies that single entry's full text (expression =
// result) to the system clipboard.
Rectangle {
    id: root

    color: Style.bgSection

    // Hidden TextEdit used as a clipboard proxy. Qt6 QML doesn't expose
    // the system clipboard directly, but a TextEdit's `copy()` method
    // writes its selected text to it — so we set the text, selectAll,
    // and copy. Zero-sized and invisible so it doesn't affect layout.
    TextEdit {
        id: clipboardProxy
        visible: false
        width: 0
        height: 0
    }

    function copyToClipboard(text) {
        clipboardProxy.text = text
        clipboardProxy.selectAll()
        clipboardProxy.copy()
    }

    // Shared context menu — one instance, opened with the target entry's
    // text stashed in `entryText`.
    Menu {
        id: contextMenu
        property string entryText: ""
        MenuItem {
            text: "Copy"
            onTriggered: root.copyToClipboard(contextMenu.entryText)
        }
    }

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
            font.pixelSize: Style.popupTitlePixelSize
            font.letterSpacing: Style.popupTitlePixelSize * Style.sectionLabelLetterSpacing
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
                id: entryRect
                width: list.width
                height: entryText.implicitHeight + 12
                color: entryMouse.containsMouse
                       ? Qt.lighter(Style.bgSurface, 1.0 + Style.keyHoverLighten)
                       : Style.bgSurface
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

                // Right-click opens the context menu scoped to THIS entry
                // only (contextMenu.entryText holds the one we clicked).
                // Left-click is a no-op for now — future: re-load the
                // expression into the display.
                MouseArea {
                    id: entryMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.RightButton
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            contextMenu.entryText = modelData
                            contextMenu.popup()
                        }
                    }
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

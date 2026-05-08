import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Modal popup for the ZOOM soft-key. Mirrors the most-used presets
// from the TI-83's ZOOM menu: ZStandard (back to the default
// -10..10 viewport), Zoom In / Zoom Out (2× around the current
// centre), ZFit (autoscale Y to the visible curves).
//
// Behavioural contract: each entry calls a Q_INVOKABLE on
// `uiController` and closes. No state stored here — the popup is a
// thin dispatcher.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 280
    height: 260
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    readonly property var entries: [
        { label: "1: ZStandard", action: "zstd" },
        { label: "2: Zoom In",   action: "zin"  },
        { label: "3: Zoom Out",  action: "zout" },
        { label: "4: ZFit",      action: "zfit" }
    ]

    function dispatch(action) {
        if (action === "zstd")      uiController.resetViewport()
        else if (action === "zin")  uiController.zoomIn()
        else if (action === "zout") uiController.zoomOut()
        else if (action === "zfit") uiController.zoomFit()
        root.close()
    }

    contentItem: ColumnLayout {
        spacing: 10

        // ── Header ──
        Text {
            Layout.fillWidth: true
            text: "ZOOM"
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

        Repeater {
            model: root.entries
            delegate: Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                radius: 4
                color: rowArea.containsMouse
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
                    id: rowArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.dispatch(modelData.action)
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}

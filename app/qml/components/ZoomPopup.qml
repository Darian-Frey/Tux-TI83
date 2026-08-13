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

    // Scrollable list of the 13 zoom actions.
    width: 280
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

    readonly property var entries: [
        { label: "1: ZStandard", action: "zstd" },
        { label: "2: Zoom In",   action: "zin"  },
        { label: "3: Zoom Out",  action: "zout" },
        { label: "4: ZSquare",   action: "zsqr" },
        { label: "5: ZTrig",     action: "ztrg" },
        { label: "6: ZDecimal",  action: "zdec" },
        { label: "7: ZInteger",   action: "zint"  },
        { label: "8: ZFit",       action: "zfit"  },
        { label: "9: ZoomStat",   action: "zstat" },
        { label: "10: ZoomPrev",  action: "zprev" },
        { label: "11: ZBox",      action: "zbox"  },
        { label: "12: ZoomSto",   action: "zsto"  },
        { label: "13: ZoomRcl",   action: "zrcl"  }
    ]

    function dispatch(action) {
        // Snapshot the current window so ZoomPrevious can undo this zoom
        // (ZoomPrevious itself must not overwrite the snapshot).
        if (action !== "zprev")
            uiController.savePrevViewport()

        if (action === "zstd")       uiController.resetViewport()
        else if (action === "zin")   uiController.zoomIn()
        else if (action === "zout")  uiController.zoomOut()
        else if (action === "zsqr")  uiController.zoomSquare()
        else if (action === "ztrg")  uiController.zoomTrig()
        else if (action === "zdec")  uiController.zoomDecimal()
        else if (action === "zint")  uiController.zoomInteger()
        else if (action === "zfit")  uiController.zoomFit()
        else if (action === "zstat") uiController.zoomStat()
        else if (action === "zprev") uiController.zoomPrevious()
        else if (action === "zbox")  uiController.armZoomBox()
        else if (action === "zsto")  uiController.zoomStore()
        else if (action === "zrcl")  uiController.zoomRecall()
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
            font.pixelSize: Style.popupTitlePixelSize
            font.letterSpacing: Style.popupTitlePixelSize * Style.sectionLabelLetterSpacing
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
            model: root.entries
            spacing: 6
            boundsBehavior: Flickable.StopAtBounds
            delegate: Rectangle {
                width: ListView.view.width
                height: 32
                radius: 4
                color: rowArea.containsMouse
                       ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                       : Style.bgSection

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.label
                    color: Style.textPrimary
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
    }
}

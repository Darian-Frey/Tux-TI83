import QtQuick
import QtQuick.Layouts
import ".."

// TABLE mode renderer — counterpart to GraphCanvas.qml. Shows the
// values of Y1/Y2/Y3 at a sequence of X positions stepped by ΔTbl
// starting from TblStart. Built from `uiController.getTableRows()`
// requested with a window of `visibleRows` rows offset by
// `scrollOffsetSteps`.
//
// Visual contract: LCD-style dark Rectangle. Top row is the column
// header (X | Y1 | Y2 | Y3); the rest are data rows. Numbers go
// through `uiController.formatScalar` so they respect the user's
// Notation/Decimal MODE settings. Missing cells (empty function
// slots, eval errors) render as "—".
//
// Behavioural contract: scrollOffsetSteps is the number of ΔTbl
// steps the viewport has scrolled from TblStart. Main.qml's keyboard
// handler nudges this when ↑/↓ fire in table mode.
Rectangle {
    id: root

    Layout.fillWidth: true
    Layout.fillHeight: true

    // How many rows we show in the visible region — also how many
    // rows we request from getTableRows().
    property int visibleRows: 14
    // Scroll offset in steps. 0 = TblStart at the top row.
    property int scrollOffsetSteps: 0

    color: Style.bgDisplay
    radius: 4
    border.color: Style.bgSection
    border.width: 1
    clip: true

    // Row data — recomputed whenever the offset, the table settings,
    // any function buffer, or a relevant MODE setting changes. Each
    // entry is a {x, y1?, y2?, y3?} map.
    property var rows: []
    function refresh() {
        const startX = uiController.tblStart +
                       root.scrollOffsetSteps * uiController.tblStep
        root.rows = uiController.getTableRows(root.visibleRows, startX)
    }

    Connections {
        target: uiController
        function onTableSettingsChanged() { root.refresh() }
        function onDisplayChanged()       { root.refresh() }
        function onActiveFunctionIndexChanged() { root.refresh() }
        function onAngleModeChanged()     { root.refresh() }
        function onNotationChanged()      { root.refresh() }
        function onFixDecimalsChanged()   { root.refresh() }
        function onTableModeChanged() {
            // Whenever we enter table mode, reset scroll so users
            // see TblStart at the top — matches TI-83 behaviour.
            if (uiController.isTableMode) {
                root.scrollOffsetSteps = 0
                root.refresh()
            }
        }
    }
    onScrollOffsetStepsChanged: root.refresh()
    Component.onCompleted: refresh()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 0

        // ── Header row ──
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Repeater {
                model: ["X", "Y1", "Y2", "Y3"]
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 22
                    color: Style.bgSection
                    radius: 3
                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        color: Style.textSecondary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.keyLabelPixelSize
                        font.weight: Style.keyLabelFontWeight
                    }
                }
            }
        }

        // Thin separator under the header.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            color: Style.bgSection
        }

        // ── Data rows ──
        Repeater {
            model: root.rows
            delegate: RowLayout {
                id: dataRow
                // Capture the row map so the inner Repeater can read
                // it without `index`/`modelData` collisions.
                property var rowData: modelData
                Layout.fillWidth: true
                spacing: 4

                // X cell. Highlighted slightly so the index column
                // stands out from the function values.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 22
                    color: Style.bgSection
                    radius: 3
                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        text: uiController.formatScalar(dataRow.rowData.x)
                        color: Style.textSecondary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.keyLabelPixelSize
                    }
                }
                // Y1 / Y2 / Y3 cells. Empty slots render as "—" so
                // the row alignment stays consistent.
                Repeater {
                    model: ["y1", "y2", "y3"]
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 22
                        color: "transparent"
                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: (modelData in dataRow.rowData)
                                  ? uiController.formatScalar(dataRow.rowData[modelData])
                                  : "—"
                            color: Style.textDisplay
                            font.family: Style.monoFamily
                            font.pixelSize: Style.keyLabelPixelSize
                        }
                    }
                }
            }
        }

        // Footer spacer — pushes rows to the top of the panel.
        Item { Layout.fillHeight: true }

        // Small footer with TblStart / ΔTbl + scroll hint.
        Text {
            Layout.fillWidth: true
            text: "TblStart=" + uiController.formatScalar(uiController.tblStart) +
                  "  ΔTbl="  + uiController.formatScalar(uiController.tblStep) +
                  "    ↑/↓ to scroll · 2ND+WINDOW for TBLSET"
            color: Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.exprPixelSize
            horizontalAlignment: Text.AlignHCenter
        }
    }
}

import QtQuick
import QtQuick.Layouts
import ".."

// The soft-key row above the main keypad — Y= · WINDOW · ZOOM · TRACE · GRAPH.
//
// Visual contract: a single rounded Rectangle background containing five
// equal-width key cells separated by 1px gaps. Cells share the row's
// background colour and read as a single unit rather than discrete keys
// (no individual borders). Hover and press states match the main CalcKey
// treatment so the row feels coherent with the rest of the keypad.
//
// Behavioural contract: emits `pressed(label)` when any cell is clicked,
// passing the cell's label string. The owner decides what each label
// means; this component has no opinion on routing.
Rectangle {
    id: root

    // ── Public API ────────────────────────────────────────
    property var labels: ["Y=", "WINDOW", "ZOOM", "TRACE", "GRAPH"]
    signal pressed(string label)

    // ── Layout defaults ───────────────────────────────────
    Layout.fillWidth: true
    Layout.preferredHeight: Style.keyHeight

    // ── Visual ────────────────────────────────────────────
    color: Style.bgSection
    radius: Style.keyRadius
    clip: true

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Repeater {
            model: root.labels
            delegate: Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true

                color: pressArea.containsMouse
                       ? Qt.lighter(Style.bgSurface, 1.0 + Style.keyHoverLighten)
                       : Style.bgSurface

                scale: pressArea.pressed ? Style.keyPressScale : 1.0
                Behavior on scale {
                    NumberAnimation { duration: Style.keyPressDurationMs }
                }

                Text {
                    anchors.centerIn: parent
                    text: modelData
                    color: Style.textSecondary
                    font.family: Style.monoFamily
                    font.pixelSize: Style.funcKeyLabelPixelSize
                    font.weight: Style.keyLabelFontWeight
                }

                MouseArea {
                    id: pressArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.pressed(modelData)
                }
            }
        }
    }
}

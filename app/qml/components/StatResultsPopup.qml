import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// 1-Var Stats results screen (Phase C Wave 4a). Displays the stat bundle
// returned by uiController.oneVarStats() in the TI-83 row order. The
// `results` map is set by the opener (the Stat editor's 1-VAR STATS
// button) just before open().
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 300
    height: 500
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    property var results: ({})
    property string sourceLabel: ""

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    // (key, label) in TI-83 display order. `n` renders as an integer;
    // everything else routes through formatScalar so the readout honours
    // the active Notation / Decimal MODE settings.
    readonly property var rows: [
        { key: "mean",   label: "x̄" },
        { key: "sumX",   label: "Σx" },
        { key: "sumX2",  label: "Σx²" },
        { key: "Sx",     label: "Sx" },
        { key: "sigmaX", label: "σx" },
        { key: "n",      label: "n" },
        { key: "minX",   label: "minX" },
        { key: "Q1",     label: "Q1" },
        { key: "median", label: "Med" },
        { key: "Q3",     label: "Q3" },
        { key: "maxX",   label: "maxX" }
    ]

    contentItem: ColumnLayout {
        spacing: 10

        // ── Header ──
        Text {
            Layout.fillWidth: true
            text: "1-VAR STATS" + (root.sourceLabel ? "  " + root.sourceLabel : "")
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

        // ── Error state ──
        Text {
            visible: !!(root.results && root.results.error)
            Layout.fillWidth: true
            text: "ERR:UNDEFINED — list is empty"
            color: Style.textError
            font.family: Style.monoFamily
            font.pixelSize: Style.exprPixelSize
            horizontalAlignment: Text.AlignHCenter
        }

        // ── Stat rows ──
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4
            visible: !(root.results && root.results.error)
            // (kept as-is: `!(...)` already coerces to a proper bool)
            Repeater {
                model: root.rows
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: modelData.label
                        color: Style.textSecondary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.keyLabelPixelSize
                        Layout.preferredWidth: 70
                    }
                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignRight
                        text: {
                            if (!root.results || root.results[modelData.key] === undefined)
                                return "—"
                            if (modelData.key === "n")
                                return "" + root.results.n
                            return uiController.formatScalar(root.results[modelData.key])
                        }
                        color: Style.textDisplay
                        font.family: Style.monoFamily
                        font.pixelSize: Style.keyLabelPixelSize
                        elide: Text.ElideRight
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        CalcKey {
            label: "DONE"
            keyType: "enter"
            onPressed: root.close()
        }
    }
}

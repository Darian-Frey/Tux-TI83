import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Stat results screen (Phase C Wave 4a/4b). Renders the bundle returned
// by oneVarStats()/twoVarStats() in TI-83 row order. The opener sets
// `mode` ("oneVar" | "twoVar"), `results`, and `sourceLabel`, then
// open(). The rows scroll so the longer 2-Var/LinReg set fits.
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
    property string mode: "oneVar"

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    readonly property var oneVarRows: [
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
    readonly property var twoVarRows: [
        { key: "n",     label: "n" },
        { key: "meanX", label: "x̄" },
        { key: "meanY", label: "ȳ" },
        { key: "sumX",  label: "Σx" },
        { key: "sumY",  label: "Σy" },
        { key: "sumXY", label: "Σxy" },
        { key: "sumX2", label: "Σx²" },
        { key: "sumY2", label: "Σy²" },
        { key: "Sx",    label: "Sx" },
        { key: "Sy",    label: "Sy" },
        { key: "a",     label: "a (slope)" },
        { key: "b",     label: "b (y-int)" },
        { key: "r",     label: "r" },
        { key: "r2",    label: "r²" }
    ]
    // Regression coefficient template — filtered to the keys actually
    // present so e.g. an ExpReg (a, b) doesn't show empty c/d rows.
    readonly property var regRowsAll: [
        { key: "n",  label: "n" },
        { key: "a",  label: "a" },
        { key: "b",  label: "b" },
        { key: "c",  label: "c" },
        { key: "d",  label: "d" },
        { key: "r",  label: "r" },
        { key: "r2", label: "R²" }
    ]
    readonly property var regRows: {
        var out = []
        for (var i = 0; i < regRowsAll.length; ++i) {
            var k = regRowsAll[i].key
            if (results && results[k] !== undefined)
                out.push(regRowsAll[i])
        }
        return out
    }
    readonly property var rows: mode === "twoVar" ? twoVarRows
                              : mode === "reg"    ? regRows
                                                  : oneVarRows
    readonly property string headerText: mode === "twoVar" ? "2-VAR / LINREG"
                              : mode === "reg"    ? "REGRESSION"
                                                  : "1-VAR STATS"
    readonly property string errorText: {
        var e = (results && results.error) ? results.error : ""
        if (e === "DIM")
            return "ERR:INVALID DIM — Xlist and Ylist differ in length"
        if (e === "DOMAIN")
            return "ERR:DOMAIN — model needs enough points / positive data"
        if (e)
            return "ERR:UNDEFINED — empty or undefined list"
        return ""
    }

    contentItem: ColumnLayout {
        spacing: 10

        // ── Header ──
        Text {
            Layout.fillWidth: true
            text: root.headerText + (root.sourceLabel ? "  " + root.sourceLabel : "")
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
            visible: root.errorText.length > 0
            Layout.fillWidth: true
            text: root.errorText
            color: Style.textError
            font.family: Style.monoFamily
            font.pixelSize: Style.exprPixelSize
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        // ── Stat rows (scrollable) ──
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.errorText.length === 0
            clip: true
            model: root.rows
            spacing: 3
            boundsBehavior: Flickable.StopAtBounds
            delegate: RowLayout {
                width: ListView.view.width
                height: 24
                Text {
                    text: modelData.label
                    color: Style.textSecondary
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                    Layout.preferredWidth: 80
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
                    color: Style.textPrimary
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                    elide: Text.ElideRight
                }
            }
        }

        CalcKey {
            label: "DONE"
            keyType: "enter"
            onPressed: root.close()
        }
    }
}

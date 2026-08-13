import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Modal popup listing the MATH-menu functions available to insert into
// the current expression.
//
// Visual contract: a centred dialog with a scrollable ListView of
// functions. Each entry shows a TI-83-style numbered label
// (`1: abs(`, `2: int(`, ...) and inserts the function token when
// clicked. Same interaction pattern as the MatrixPopup's MATH tab.
//
// Behavioural contract: each entry's `input` string is passed to
// `uiController.processInput()` when clicked; the popup auto-closes
// after insertion. Binary functions (round, min, max, mod) are
// included with "(needs binary args)" placeholders until Wave 2
// binary-function infrastructure lands.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 320
    height: 360
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    // Single source of truth for the menu. Adding a new MATH entry =
    // adding one row here. `input` is what gets sent to processInput();
    // `available` is the gate for Wave 2 features that aren't wired yet.
    readonly property var entries: [
        { label: "abs(",   input: "abs(",   available: true  },
        { label: "int(",   input: "int(",   available: true  },
        { label: "iPart(", input: "iPart(", available: true  },
        { label: "fPart(", input: "fPart(", available: true  },
        { label: "e^(",    input: "e^(",    available: true  },
        { label: "sgn(",   input: "sgn(",   available: true  },
        { label: "i (imaginary)", input: "i",     available: true },
        { label: "conj(",  input: "conj(",  available: true },
        { label: "real(",  input: "real(",  available: true },
        { label: "imag(",  input: "imag(",  available: true },
        { label: "angle(", input: "angle(", available: true },
        { label: "round(", input: "round(", available: true },
        { label: "min(",   input: "min(",   available: true },
        { label: "max(",   input: "max(",   available: true },
        { label: "mod(",   input: "mod(",   available: true },
        { label: "nCr(",   input: "nCr(",   available: true },
        { label: "nPr(",   input: "nPr(",   available: true },
        { label: "! (factorial)", input: "!", available: true },
        { label: "fnInt(",  input: "fnInt(",  available: true },
        { label: "nDeriv(", input: "nDeriv(", available: true },
        { label: "sum(",    input: "sum(",    available: true },
        { label: "prod(",   input: "prod(",   available: true },
        { label: "mean(",     input: "mean(",     available: true },
        { label: "stdDev(",   input: "stdDev(",   available: true },
        { label: "variance(", input: "variance(", available: true },
        { label: "median(",   input: "median(",   available: true },
        { label: "seq(",      input: "seq(",      available: true },
        { label: "rand",      input: "rand",      available: true },
        { label: "randInt(",  input: "randInt(",  available: true },
        { label: "randNorm(", input: "randNorm(", available: true },
        { label: "randBin(",  input: "randBin(",  available: true },
        { label: "normalpdf(", input: "normalpdf(", available: true },
        { label: "normalcdf(", input: "normalcdf(", available: true },
        { label: "invNorm(",   input: "invNorm(",   available: true },
        { label: "binompdf(",   input: "binompdf(",   available: true },
        { label: "binomcdf(",   input: "binomcdf(",   available: true },
        { label: "poissonpdf(", input: "poissonpdf(", available: true },
        { label: "poissoncdf(", input: "poissoncdf(", available: true },
        { label: "geometpdf(",  input: "geometpdf(",  available: true },
        { label: "geometcdf(",  input: "geometcdf(",  available: true },
        { label: "tpdf(",    input: "tpdf(",    available: true },
        { label: "tcdf(",    input: "tcdf(",    available: true },
        { label: "χ²pdf(",   input: "χ²pdf(",   available: true },
        { label: "χ²cdf(",   input: "χ²cdf(",   available: true },
        { label: "Fpdf(",    input: "Fpdf(",    available: true },
        { label: "Fcdf(",    input: "Fcdf(",    available: true },
        { label: "sinh(",  input: "sinh(",  available: true },
        { label: "cosh(",  input: "cosh(",  available: true },
        { label: "tanh(",  input: "tanh(",  available: true },
        { label: "asinh(", input: "asinh(", available: true },
        { label: "acosh(", input: "acosh(", available: true },
        { label: "atanh(", input: "atanh(", available: true },
        { label: "▶Frac",  input: "▶Frac",  available: true },
        { label: "▶Dec",   input: "▶Dec",   available: true },
        // STO → lives here until there's a dedicated key. Inserting it
        // mid-expression lets the user type `5 → A` from the GUI without
        // needing the keyboard `->` shortcut.
        { label: "→ (STO)", input: "→",      available: true }
    ]

    contentItem: ColumnLayout {
        spacing: 10

        // ── Header ──
        Text {
            Layout.fillWidth: true
            text: "MATH"
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

        // ── Entry list ──
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 4
            model: root.entries
            spacing: 4
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: list.width
                height: 32
                radius: 4
                color: modelData.available
                       ? (entryArea.containsMouse
                           ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                           : Style.bgSection)
                       : Style.bgSection
                opacity: modelData.available ? 1.0 : 0.4

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Text {
                        text: (index + 1) + ":"
                        color: Style.textMuted
                        font.family: Style.monoFamily
                        font.pixelSize: Style.keyLabelPixelSize
                        Layout.preferredWidth: 20
                    }
                    Text {
                        text: modelData.label
                        color: Style.textPrimary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.keyLabelPixelSize
                        Layout.fillWidth: true
                    }
                    Text {
                        visible: !modelData.available
                        text: "(unavailable)"
                        color: Style.textMuted
                        font.family: Style.monoFamily
                        font.pixelSize: Style.exprPixelSize
                        font.italic: true
                    }
                }

                MouseArea {
                    id: entryArea
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: modelData.available
                    onClicked: {
                        // processExpression tokenises multi-char inputs
                        // (e.g. "^-1") so entries can insert composite
                        // sequences. Single-token entries like "abs("
                        // work identically to processInput.
                        uiController.processExpression(modelData.input)
                        root.close()
                    }
                }
            }
        }
    }
}

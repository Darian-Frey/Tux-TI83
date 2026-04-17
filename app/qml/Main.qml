import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

ApplicationWindow {
    id: root
    visible: true
    width: 720
    height: 760
    title: "Tux-TI83"
    color: Style.bgShell

    // ─────────────────────────────────────────────────────
    // Inline component: section header (label + hairline)
    // Temporary — will be folded into a dedicated abstraction
    // when component extraction stabilises.
    // ─────────────────────────────────────────────────────
    component SectionHeader: ColumnLayout {
        property string label
        Layout.fillWidth: true
        spacing: 4
        Text {
            text: parent.label
            color: Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.sectionLabelPixelSize
            font.letterSpacing: Style.sectionLabelPixelSize * Style.sectionLabelLetterSpacing
            font.capitalization: Font.AllUppercase
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Style.bgSection
        }
    }

    // ─────────────────────────────────────────────────────
    // Layout — calculator column on the left, history on the right.
    // ─────────────────────────────────────────────────────
    RowLayout {
        anchors.fill: parent
        spacing: 0
        focus: true

        // ── Keyboard shortcuts (CLAUDE.md key map, literal spec) ──
        // Routes physical keystrokes through the same `processInput`
        // entry point the on-screen keys use, so the controller's state
        // machine handles them identically.
        //
        // Modified events (Ctrl/Alt/Meta) are passed through unchanged
        // so future copy/paste / shortcut handling stays available.
        // Bare-letter shortcuts (s/c/t/l/n/r/p) are unconditional — see
        // IMP-003 for the eventual ALPHA-modifier gate.
        Keys.onPressed: function(event) {
            if (event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))
                return

            // Special keys (handled by Qt key code, not by text)
            switch (event.key) {
            case Qt.Key_Return:
            case Qt.Key_Enter:
            case Qt.Key_Equal:
                uiController.processInput("ENTER")
                event.accepted = true
                return
            case Qt.Key_Backspace:
                uiController.processInput("DEL")
                event.accepted = true
                return
            case Qt.Key_Escape:
                uiController.processInput("C")
                event.accepted = true
                return
            }

            // Printable characters routed by event.text. Using text
            // (not key code) keeps the mapping layout-agnostic — Shift,
            // numpad, and non-US layouts all just work.
            if (event.text.length === 0)
                return
            const ch = event.text.charAt(0)
            const map = {
                "0": "0", "1": "1", "2": "2", "3": "3", "4": "4",
                "5": "5", "6": "6", "7": "7", "8": "8", "9": "9",
                ".": ".",
                "+": "+", "-": "−", "*": "×", "/": "÷",
                "^": "^", "(": "(", ")": ")",
                "s": "sin", "c": "cos", "t": "tan",
                "l": "log", "n": "ln", "r": "√", "p": "π"
            }
            if (map.hasOwnProperty(ch)) {
                uiController.processInput(map[ch])
                event.accepted = true
            }
        }

        // ── Calculator column ───────────────────────────
        Item {
            Layout.preferredWidth: 420
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

        // ── 1. Header strip ─────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 16
            Text {
                text: Style.brandText
                color: Style.textSecondary
                font.family: Style.monoFamily
                font.pixelSize: Style.headerBrandPixelSize
                font.letterSpacing: Style.headerBrandPixelSize * Style.headerBrandLetterSpacing
            }
            Item { Layout.fillWidth: true }
            Text {
                text: "NORMAL  DEG"
                color: Style.textMuted
                font.family: Style.monoFamily
                font.pixelSize: Style.headerBrandPixelSize
                font.letterSpacing: Style.headerBrandPixelSize * 0.10
            }
        }

        // ── Active-function selector (Y1 / Y2 / Y3) ─────
        FunctionSelector {
            Layout.fillWidth: true
        }

        // ── 2. LCD display panel ────────────────────────
        Display {
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            currentState: uiController.displayState
            expressionText: uiController.displayState !== 0
                            ? (uiController.displayExpression + " =")
                            : ""
            mainText: uiController.currentDisplay
        }

        // ── 3. Soft-key row ─────────────────────────────
        SoftKeyRow {
            onPressed: function(label) {
                if (label === "WINDOW") {
                    windowPopup.open()
                } else if (label === "GRAPH") {
                    if (!uiController.isGraphMode)
                        uiController.toggleGraphMode()
                } else if (label === "Y=") {
                    if (uiController.isGraphMode)
                        uiController.toggleGraphMode()
                }
                // TODO: ZOOM and TRACE remain no-op until those features
                // land (zoom menu, trace cursor, etc.)
            }
        }

        // ── Mode switcher: keypad ↔ graph ────────────────
        // The bottom half of the calculator column swaps between the
        // keypad sections (page 0) and the graph canvas (page 1).
        // Toggled by the GRAPH / Y= soft keys above.
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: uiController.isGraphMode ? 1 : 0

            // ── Page 0: keypad (CONTROL / SCIENTIFIC / NUMERIC) ──
            ColumnLayout {
                spacing: 10

        // ── 4. CONTROL section ──────────────────────────
        SectionHeader { label: "CONTROL" }
        GridLayout {
            Layout.fillWidth: true
            columns: 5
            rowSpacing: 6
            columnSpacing: 6

            CalcKey { label: "2ND";   keyType: "second";  onPressed: { /* TODO: 2ND modifier */ } }
            CalcKey { label: "MODE";  keyType: "control"; onPressed: { /* TODO: mode menu */ } }
            CalcKey { label: "⌫";     keyType: "control"; onPressed: uiController.processInput("DEL") }
            CalcKey { label: "ALPHA"; keyType: "control"; onPressed: { /* TODO: alpha modifier */ } }
            CalcKey { label: "CLEAR"; keyType: "control"; onPressed: uiController.processInput("C") }
        }

        // ── 5. SCIENTIFIC section ───────────────────────
        SectionHeader { label: "SCIENTIFIC" }
        GridLayout {
            Layout.fillWidth: true
            columns: 5
            rowSpacing: 6
            columnSpacing: 6

            CalcKey { label: "MATH"; keyType: "function"; onPressed: { /* TODO: MATH menu */ } }
            CalcKey { label: "sin(";  keyType: "function"; onPressed: uiController.processInput("sin")  }
            CalcKey { label: "cos(";  keyType: "function"; onPressed: uiController.processInput("cos")  }
            CalcKey { label: "tan(";  keyType: "function"; onPressed: uiController.processInput("tan")  }
            CalcKey { label: "^";     keyType: "function"; onPressed: uiController.processInput("^")    }

            CalcKey { label: "√(";    keyType: "function"; onPressed: uiController.processInput("√")    }
            CalcKey { label: "ln(";   keyType: "function"; onPressed: uiController.processInput("ln")   }
            CalcKey { label: "log(";  keyType: "function"; onPressed: uiController.processInput("log")  }
            CalcKey { label: "π";     keyType: "function"; onPressed: uiController.processInput("π")    }
            CalcKey { label: "e";     keyType: "function"; onPressed: uiController.processInput("e")    }
        }

        // ── 6. NUMERIC section ──────────────────────────
        SectionHeader { label: "NUMERIC" }
        GridLayout {
            Layout.fillWidth: true
            columns: 5
            rowSpacing: 6
            columnSpacing: 6

            // Row 1
            CalcKey { label: "(";  keyType: "function"; onPressed: uiController.processInput("(") }
            CalcKey { label: ")";  keyType: "function"; onPressed: uiController.processInput(")") }
            CalcKey { label: ",";  keyType: "function"; onPressed: uiController.processInput(",") }
            CalcKey { label: "X";  keyType: "function"; onPressed: uiController.processInput("X") }
            CalcKey { label: "÷";  keyType: "operator"; onPressed: uiController.processInput("÷") }

            // Row 2
            CalcKey { label: "7";  keyType: "numeric"; onPressed: uiController.processInput("7") }
            CalcKey { label: "8";  keyType: "numeric"; onPressed: uiController.processInput("8") }
            CalcKey { label: "9";  keyType: "numeric"; onPressed: uiController.processInput("9") }
            CalcKey { label: "MATRX"; keyType: "function"; onPressed: matrixPopup.open() }
            CalcKey { label: "×";  keyType: "operator"; onPressed: uiController.processInput("×") }

            // Row 3
            CalcKey { label: "4";  keyType: "numeric"; onPressed: uiController.processInput("4") }
            CalcKey { label: "5";  keyType: "numeric"; onPressed: uiController.processInput("5") }
            CalcKey { label: "6";  keyType: "numeric"; onPressed: uiController.processInput("6") }
            CalcKey { label: "x²"; keyType: "function"; onPressed: { uiController.processInput("^"); uiController.processInput("2") } }
            CalcKey { label: "−";  keyType: "operator"; onPressed: uiController.processInput("−") }

            // Row 4
            CalcKey { label: "1";  keyType: "numeric"; onPressed: uiController.processInput("1") }
            CalcKey { label: "2";  keyType: "numeric"; onPressed: uiController.processInput("2") }
            CalcKey { label: "3";  keyType: "numeric"; onPressed: uiController.processInput("3") }
            CalcKey { label: "Ans"; keyType: "function"; onPressed: uiController.processInput("Ans") }
            CalcKey { label: "+";  keyType: "operator"; onPressed: uiController.processInput("+") }

            // Row 5
            CalcKey { label: "0";   keyType: "numeric"; onPressed: uiController.processInput("0") }
            CalcKey { label: ".";   keyType: "numeric"; onPressed: uiController.processInput(".") }
            CalcKey { label: "(-)"; keyType: "numeric"; onPressed: uiController.processInput("-") }
            CalcKey {
                label: "ENTER"
                keyType: "enter"
                Layout.columnSpan: 2
                onPressed: uiController.processInput("ENTER")
            }
        }

        Item { Layout.fillHeight: true }
            }

            // ── Page 1: graph canvas ─────────────────────
            GraphCanvas { }
        }
            }
        }

        // ── History side panel ──────────────────────────
        HistoryPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    // ── Modal popups (overlay layer) ────────────────────
    WindowPopup {
        id: windowPopup
    }

    MatrixPopup {
        id: matrixPopup
    }
}

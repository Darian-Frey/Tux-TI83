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
    // Modifier state (2ND / ALPHA)
    //
    // Mirrors TI-83 hardware: pressing 2ND or ALPHA arms a one-shot
    // modifier that transforms the NEXT keypress, then auto-clears.
    // They're mutually exclusive — arming one disarms the other — and
    // pressing an already-armed modifier disarms it (toggle behaviour).
    //
    // The dispatcher `handleKey(primary)` is the single entry point for
    // both on-screen CalcKeys and physical keyboard events. It inspects
    // the armed state, looks up a 2ND/ALPHA variant if applicable, and
    // routes the resolved token through the controller. ALPHA has no
    // wired variants yet — the flag just clears silently on any key
    // until we have a variable registry to bind letters to.
    // ─────────────────────────────────────────────────────
    property bool secondArmed: false
    property bool alphaArmed:  false

    // Primary-label → 2ND-variant token sequence. Values are fed to
    // `processExpression` (tokenising), so multi-token variants like
    // "e^(" just work.
    readonly property var secondMap: ({
        "sin(": "asin(",
        "cos(": "acos(",
        "tan(": "atan(",
        "x²":   "√(",
        "ln(":  "e^(",
        "(-)":  "Ans"
    })

    function armSecond() {
        alphaArmed  = false
        secondArmed = !secondArmed
    }
    function armAlpha() {
        secondArmed = false
        alphaArmed  = !alphaArmed
    }
    function clearModifiers() {
        secondArmed = false
        alphaArmed  = false
    }

    // Central key dispatcher. `primary` is the key's un-modified label
    // ("sin(", "7", "ENTER", "DEL", "C", …). For modifier-free presses
    // this is identical to the old inline routing — the dispatcher just
    // forwards to `processInput`. Modifier-armed presses look up the
    // variant, call `processExpression` (to handle multi-token variants
    // like "e^("), and then clear the modifier.
    function handleKey(primary) {
        if (secondArmed) {
            secondArmed = false
            if (secondMap.hasOwnProperty(primary)) {
                uiController.processExpression(secondMap[primary])
                return
            }
            // Fallthrough: 2ND + <unmapped key> cancels silently and
            // sends the primary — matches TI-83 behaviour.
        }
        if (alphaArmed) {
            alphaArmed = false
            // No ALPHA variants wired yet; primary still runs.
        }
        uiController.processInput(primary)
    }

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
        // Physical keystrokes route through `handleKey` — the same
        // dispatcher the on-screen CalcKeys use — so the 2ND/ALPHA
        // modifier state applies identically to both input paths.
        //
        // Modified events (Ctrl/Alt/Meta) are passed through unchanged
        // so future copy/paste / shortcut handling stays available.
        //
        // Physical-key modifier shortcuts: `Tab` arms/disarms ALPHA,
        // `\` arms/disarms 2ND. These are chosen to avoid conflicting
        // with any existing literal keymap entry.
        Keys.onPressed: function(event) {
            if (event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier))
                return

            // Modifier-arming shortcuts
            if (event.key === Qt.Key_Tab) {
                root.armAlpha()
                event.accepted = true
                return
            }
            if (event.key === Qt.Key_Backslash) {
                root.armSecond()
                event.accepted = true
                return
            }

            // Special keys (handled by Qt key code, not by text)
            switch (event.key) {
            case Qt.Key_Return:
            case Qt.Key_Enter:
            case Qt.Key_Equal:
                root.handleKey("ENTER")
                event.accepted = true
                return
            case Qt.Key_Backspace:
                root.handleKey("DEL")
                event.accepted = true
                return
            case Qt.Key_Escape:
                root.clearModifiers()
                root.handleKey("C")
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
                "s": "sin(", "c": "cos(", "t": "tan(",
                "l": "log(", "n": "ln(", "r": "√(", "p": "π",
                "!": "!"
            }
            if (map.hasOwnProperty(ch)) {
                root.handleKey(map[ch])
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

            // Modifier badges — visible only when the matching modifier
            // is armed. Small and right-justified so they sit alongside
            // the mode indicator without crowding it.
            Text {
                visible: root.secondArmed
                text: "2ND"
                color: Style.armedBadge2nd
                font.family: Style.monoFamily
                font.pixelSize: Style.headerBrandPixelSize
                font.letterSpacing: Style.headerBrandPixelSize * 0.10
                font.weight: Font.Bold
            }
            Text {
                visible: root.alphaArmed
                text: "α"
                color: Style.armedBadgeAlpha
                font.family: Style.monoFamily
                font.pixelSize: Style.headerBrandPixelSize
                font.letterSpacing: Style.headerBrandPixelSize * 0.10
                font.weight: Font.Bold
            }

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

            CalcKey { label: "2ND";   keyType: "second";  armed: root.secondArmed; onPressed: root.armSecond() }
            CalcKey { label: "MODE";  keyType: "control"; onPressed: { /* TODO: mode menu */ } }
            CalcKey { label: "⌫";     keyType: "control"; onPressed: root.handleKey("DEL") }
            CalcKey { label: "ALPHA"; keyType: "control"; armed: root.alphaArmed;  onPressed: root.armAlpha() }
            CalcKey { label: "CLEAR"; keyType: "control"; onPressed: { root.clearModifiers(); uiController.processInput("C") } }
        }

        // ── 5. SCIENTIFIC section ───────────────────────
        SectionHeader { label: "SCIENTIFIC" }
        GridLayout {
            Layout.fillWidth: true
            columns: 5
            rowSpacing: 6
            columnSpacing: 6

            CalcKey { label: "MATH"; keyType: "function"; onPressed: { root.clearModifiers(); mathMenuPopup.open() } }
            CalcKey { label: "sin(";  keyType: "function"; onPressed: root.handleKey("sin(")  }
            CalcKey { label: "cos(";  keyType: "function"; onPressed: root.handleKey("cos(")  }
            CalcKey { label: "tan(";  keyType: "function"; onPressed: root.handleKey("tan(")  }
            CalcKey { label: "^";     keyType: "function"; onPressed: root.handleKey("^")    }

            CalcKey { label: "√(";    keyType: "function"; onPressed: root.handleKey("√(")    }
            CalcKey { label: "ln(";   keyType: "function"; onPressed: root.handleKey("ln(")   }
            CalcKey { label: "log(";  keyType: "function"; onPressed: root.handleKey("log(")  }
            CalcKey { label: "π";     keyType: "function"; onPressed: root.handleKey("π")    }
            CalcKey { label: "e";     keyType: "function"; onPressed: root.handleKey("e")    }
        }

        // ── 6. NUMERIC section ──────────────────────────
        SectionHeader { label: "NUMERIC" }
        GridLayout {
            Layout.fillWidth: true
            columns: 5
            rowSpacing: 6
            columnSpacing: 6

            // Row 1
            CalcKey { label: "(";  keyType: "function"; onPressed: root.handleKey("(") }
            CalcKey { label: ")";  keyType: "function"; onPressed: root.handleKey(")") }
            CalcKey { label: ",";  keyType: "function"; onPressed: root.handleKey(",") }
            CalcKey { label: "X";  keyType: "function"; onPressed: root.handleKey("X") }
            CalcKey { label: "÷";  keyType: "operator"; onPressed: root.handleKey("÷") }

            // Row 2
            CalcKey { label: "7";  keyType: "numeric"; onPressed: root.handleKey("7") }
            CalcKey { label: "8";  keyType: "numeric"; onPressed: root.handleKey("8") }
            CalcKey { label: "9";  keyType: "numeric"; onPressed: root.handleKey("9") }
            CalcKey { label: "MATRX"; keyType: "function"; onPressed: { root.clearModifiers(); matrixPopup.open() } }
            CalcKey { label: "×";  keyType: "operator"; onPressed: root.handleKey("×") }

            // Row 3
            CalcKey { label: "4";  keyType: "numeric"; onPressed: root.handleKey("4") }
            CalcKey { label: "5";  keyType: "numeric"; onPressed: root.handleKey("5") }
            CalcKey { label: "6";  keyType: "numeric"; onPressed: root.handleKey("6") }
            // x² routes through handleKey so 2ND + x² can intercept and
            // send √( instead of the default "^ then 2" sequence.
            CalcKey { label: "x²"; keyType: "function"; onPressed: {
                if (root.secondArmed) {
                    root.handleKey("x²")
                } else {
                    uiController.processInput("^")
                    uiController.processInput("2")
                }
            } }
            CalcKey { label: "−";  keyType: "operator"; onPressed: root.handleKey("−") }

            // Row 4
            CalcKey { label: "1";  keyType: "numeric"; onPressed: root.handleKey("1") }
            CalcKey { label: "2";  keyType: "numeric"; onPressed: root.handleKey("2") }
            CalcKey { label: "3";  keyType: "numeric"; onPressed: root.handleKey("3") }
            CalcKey { label: "Ans"; keyType: "function"; onPressed: root.handleKey("Ans") }
            CalcKey { label: "+";  keyType: "operator"; onPressed: root.handleKey("+") }

            // Row 5
            CalcKey { label: "0";   keyType: "numeric"; onPressed: root.handleKey("0") }
            CalcKey { label: ".";   keyType: "numeric"; onPressed: root.handleKey(".") }
            // (-) routes through handleKey so 2ND + (-) → Ans (TI-83
            // convention). Default primary is "neg" for unary negation.
            CalcKey { label: "(-)"; keyType: "numeric"; onPressed: {
                if (root.secondArmed) {
                    root.handleKey("(-)")
                } else {
                    uiController.processInput("neg")
                }
            } }
            CalcKey {
                label: "ENTER"
                keyType: "enter"
                Layout.columnSpan: 2
                onPressed: root.handleKey("ENTER")
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

    MathMenuPopup {
        id: mathMenuPopup
    }
}

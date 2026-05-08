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
    // ALPHA-lock: persistent mode where letters stay accessible across
    // multiple keypresses. Entered via 2ND + ALPHA, released by another
    // ALPHA press or CLEAR. `alphaArmed` is the one-shot flag; `alphaLocked`
    // is the persistent flag. Visual + handleKey treat either-or as "ALPHA
    // is active" for the next keypress. After a key fires its ALPHA
    // variant, alphaArmed is cleared but alphaLocked stays.
    property bool alphaLocked: false
    // Derived "ALPHA is in effect for the next keypress" flag. CalcKeys
    // that bypass handleKey (MATH, MATRX, x², (-)) need this explicit
    // combination — checking only alphaArmed would miss lock mode.
    readonly property bool alphaActive: alphaArmed || alphaLocked

    // Primary-label → 2ND-variant token sequence. Values are fed to
    // `processExpression` (tokenising), so multi-token variants like
    // "e^(" just work.
    readonly property var secondMap: ({
        "sin(": "asin(",
        "cos(": "acos(",
        "tan(": "atan(",
        "^":    "ˣ√",
        "x²":   "√(",
        "ln(":  "e^(",
        "(-)":  "Ans"
    })

    // Primary-label → ALPHA-variant token. Backed by the VarA..VarZ
    // tokens that resolve via `MathStateMachine::varRegistry`. Mirrors
    // the on-key `alphaLabel` annotations so the visual layout matches
    // the functional mapping one-to-one.
    // Only letters A..Z are routed — ":", "?" and '"' on the `.` / (-)
    // / `+` keys are layout-accurate labels (they show what those keys
    // *would* send under ALPHA on a real TI-83) but aren't tokens in
    // our vocabulary yet, so pressing ALPHA + those keys just clears
    // the modifier without inserting anything.
    readonly property var alphaMap: ({
        "MATH":  "A", "MATRX": "B",
        "sin(":  "E", "cos(":  "F", "tan(": "G", "^": "H",
        "ln(":   "S", "log(":  "N",
        "(":     "K", ")":     "L", ",":    "J",
        "÷":     "M", "×":     "R", "−":    "W",
        "x²":    "I",
        "7":     "O", "8":     "P", "9":    "Q",
        "4":     "T", "5":     "U", "6":    "V",
        "1":     "Y", "2":     "Z",
        ".":     ":"
    })

    function armSecond() {
        // Arm 2ND, clear one-shot alphaArmed. `alphaLocked` deliberately
        // stays so "2ND + SIN" mid-typing doesn't unlock the letter
        // mode — the lock persists to serve the next keypress too.
        alphaArmed  = false
        secondArmed = !secondArmed
    }
    function armAlpha() {
        // Three-way: 2ND + ALPHA toggles lock, ALPHA-while-locked
        // releases the lock, otherwise single-press toggles the
        // one-shot alphaArmed flag.
        if (secondArmed) {
            secondArmed = false
            alphaLocked = !alphaLocked
            alphaArmed  = false
            return
        }
        if (alphaLocked) {
            alphaLocked = false
            alphaArmed  = false
            return
        }
        alphaArmed = !alphaArmed
    }
    function clearModifiers() {
        secondArmed = false
        alphaArmed  = false
        alphaLocked = false
    }

    // Central key dispatcher. `primary` is the key's un-modified label
    // ("sin(", "7", "ENTER", "DEL", "CLEAR", …). For modifier-free presses
    // this is identical to the old inline routing — the dispatcher just
    // forwards to `processInput`. Modifier-armed presses look up the
    // variant, call `processExpression` (to handle multi-token variants
    // like "e^("), and then clear the modifier.
    function handleKey(primary) {
        if (secondArmed) {
            secondArmed = false
            // 2ND + ENTER is last-entry recall. Dedicated path because
            // it calls a controller method rather than inserting tokens,
            // and ENTER itself is a control sentinel (not a kTokens
            // entry) — so it can't live in secondMap cleanly.
            if (primary === "ENTER") {
                uiController.recallLastEntry()
                return
            }
            // 2ND + MATH opens the TEST/LOGIC operator popup. Like
            // recall above, it's a popup trigger rather than a token
            // insertion, so it sits outside secondMap.
            if (primary === "MATH") {
                logicMenuPopup.open()
                return
            }
            // 2ND + DEL toggles insert / overwrite mode (TI-83 INS).
            // Controller method, not a token insertion.
            if (primary === "DEL") {
                uiController.toggleInsertMode()
                return
            }
            // 2ND + 0 opens the alphabetical CATALOG browser. Popup
            // trigger, not a token insertion — same shape as the
            // other special cases above.
            if (primary === "0") {
                catalogPopup.open()
                return
            }
            if (secondMap.hasOwnProperty(primary)) {
                uiController.processExpression(secondMap[primary])
                return
            }
            // Fallthrough: 2ND + <unmapped key> cancels silently and
            // sends the primary — matches TI-83 behaviour.
        }
        if (alphaArmed || alphaLocked) {
            // One-shot arming always clears; the persistent lock
            // survives so the next keypress is still ALPHA-modified.
            alphaArmed = false
            if (alphaMap.hasOwnProperty(primary)) {
                // ALPHA variants are single letters (A..Z) or literal
                // punctuation (:, ?, "). processExpression handles all
                // of them via the unified kTokens table. For tokens
                // that tokenise as multiple inputs (none of ours
                // currently do) it would still Just Work.
                uiController.processExpression(alphaMap[primary])
                return
            }
            // Fallthrough: ALPHA + unmapped key just sends the primary.
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
                root.handleKey("CLEAR")
                event.accepted = true
                return
            case Qt.Key_Left:
                uiController.moveCursorLeft()
                event.accepted = true
                return
            case Qt.Key_Right:
                uiController.moveCursorRight()
                event.accepted = true
                return
            case Qt.Key_Home:
                uiController.moveCursorHome()
                event.accepted = true
                return
            case Qt.Key_End:
                uiController.moveCursorEnd()
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
                "!": "!",
                "|": "→"  // STO assignment arrow — Shift-\\ on US layout.
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
                visible: root.alphaArmed || root.alphaLocked
                // "α" for a one-shot arm; "A-LOCK" once the lock has
                // been engaged (matches the TI-83 convention for
                // alpha-lock).
                text: root.alphaLocked ? "A-LOCK" : "α"
                color: Style.armedBadgeAlpha
                font.family: Style.monoFamily
                font.pixelSize: Style.headerBrandPixelSize
                font.letterSpacing: Style.headerBrandPixelSize * 0.10
                font.weight: Font.Bold
            }

            // Overwrite-mode badge (TI-83 OVR). Visible only when the
            // controller's insertMode is false — INS is the default
            // and doesn't need its own badge.
            Text {
                visible: !uiController.insertMode
                text: "OVR"
                color: Style.armedBadge2nd
                font.family: Style.monoFamily
                font.pixelSize: Style.headerBrandPixelSize
                font.letterSpacing: Style.headerBrandPixelSize * 0.10
                font.weight: Font.Bold
            }

            Text {
                // Notation slot is a placeholder ("NORMAL") until the
                // Sci/Eng modes are wired; angle slot binds to the live
                // controller property so flipping the MODE popup updates
                // the badge immediately.
                text: "NORMAL  " + (uiController.angleMode === 1 ? "DEG" : "RAD")
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
            cursorCharOffset: uiController.cursorOffset
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
            CalcKey { label: "MODE";  keyType: "control"; onPressed: { root.clearModifiers(); modePopup.open() } }
            CalcKey { label: "⌫";     keyType: "control"; secondLabel: "INS"; onPressed: root.handleKey("DEL") }
            CalcKey { label: "ALPHA"; keyType: "control"; armed: root.alphaArmed || root.alphaLocked; onPressed: root.armAlpha() }
            CalcKey { label: "CLEAR"; keyType: "control"; onPressed: { root.clearModifiers(); uiController.processInput("CLEAR") } }
            // Note: corner labels intentionally omitted from CONTROL row
            // — the TI-83 equivalents (QUIT, INS, A-LOCK, RESET) aren't
            // wired yet. Labelling them would advertise behaviour the
            // keys don't deliver.
        }

        // ── CURSOR section ──────────────────────────────
        // On-screen complement to the Left/Right/Home/End keyboard
        // shortcuts. ↑ / ↓ are intentionally omitted — we don't have
        // multi-line edit semantics, and 2ND+ENTER already covers
        // history recall.
        SectionHeader { label: "CURSOR" }
        GridLayout {
            Layout.fillWidth: true
            columns: 5
            rowSpacing: 6
            columnSpacing: 6

            CalcKey { label: "HOME"; keyType: "function"; onPressed: uiController.moveCursorHome() }
            CalcKey { label: "←";    keyType: "function"; onPressed: uiController.moveCursorLeft() }
            CalcKey { label: "→";    keyType: "function"; onPressed: uiController.moveCursorRight() }
            CalcKey { label: "END";  keyType: "function"; onPressed: uiController.moveCursorEnd() }
            // 5th column intentionally empty so the four keys span
            // most of the row width without crowding.
            Item { Layout.fillWidth: true }
        }

        // ── 5. SCIENTIFIC section ───────────────────────
        SectionHeader { label: "SCIENTIFIC" }
        GridLayout {
            Layout.fillWidth: true
            columns: 5
            rowSpacing: 6
            columnSpacing: 6

            // MATH: ALPHA-armed → insert letter A via handleKey; otherwise
            // open the MATH menu (and always clear 2ND if it was armed,
            // since 2ND+MATH is unwired).
            CalcKey { label: "MATH"; keyType: "function"; secondLabel: "TEST"; alphaLabel: "A"; onPressed: {
                // 2ND → route through handleKey so 2ND+MATH opens the
                // TEST popup; ALPHA → route through so A gets inserted;
                // unmodified → open the MATH menu directly.
                if (root.secondArmed || root.alphaActive) {
                    root.handleKey("MATH")
                } else {
                    root.clearModifiers()
                    mathMenuPopup.open()
                }
            } }
            CalcKey { label: "sin(";  keyType: "function"; secondLabel: "sin⁻¹"; alphaLabel: "E"; onPressed: root.handleKey("sin(")  }
            CalcKey { label: "cos(";  keyType: "function"; secondLabel: "cos⁻¹"; alphaLabel: "F"; onPressed: root.handleKey("cos(")  }
            CalcKey { label: "tan(";  keyType: "function"; secondLabel: "tan⁻¹"; alphaLabel: "G"; onPressed: root.handleKey("tan(")  }
            CalcKey { label: "^";     keyType: "function"; secondLabel: "ˣ√"; alphaLabel: "H"; onPressed: root.handleKey("^") }

            CalcKey { label: "√(";    keyType: "function"; onPressed: root.handleKey("√(")    }
            CalcKey { label: "ln(";   keyType: "function"; secondLabel: "eˣ"; alphaLabel: "S"; onPressed: root.handleKey("ln(")   }
            CalcKey { label: "log(";  keyType: "function"; alphaLabel: "N"; onPressed: root.handleKey("log(")  }
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
            CalcKey { label: "(";  keyType: "function"; alphaLabel: "K"; onPressed: root.handleKey("(") }
            CalcKey { label: ")";  keyType: "function"; alphaLabel: "L"; onPressed: root.handleKey(")") }
            CalcKey { label: ",";  keyType: "function"; alphaLabel: "J"; onPressed: root.handleKey(",") }
            CalcKey { label: "X";  keyType: "function"; onPressed: root.handleKey("X") }
            CalcKey { label: "÷";  keyType: "operator"; alphaLabel: "M"; onPressed: root.handleKey("÷") }

            // Row 2
            CalcKey { label: "7";  keyType: "numeric"; alphaLabel: "O"; onPressed: root.handleKey("7") }
            CalcKey { label: "8";  keyType: "numeric"; alphaLabel: "P"; onPressed: root.handleKey("8") }
            CalcKey { label: "9";  keyType: "numeric"; alphaLabel: "Q"; onPressed: root.handleKey("9") }
            // MATRX: ALPHA-armed → insert letter B via handleKey;
            // otherwise open the matrix popup.
            CalcKey { label: "MATRX"; keyType: "function"; alphaLabel: "B"; onPressed: {
                if (root.alphaActive) {
                    root.handleKey("MATRX")
                } else {
                    root.clearModifiers()
                    matrixPopup.open()
                }
            } }
            CalcKey { label: "×";  keyType: "operator"; alphaLabel: "R"; onPressed: root.handleKey("×") }

            // Row 3
            CalcKey { label: "4";  keyType: "numeric"; alphaLabel: "T"; onPressed: root.handleKey("4") }
            CalcKey { label: "5";  keyType: "numeric"; alphaLabel: "U"; onPressed: root.handleKey("5") }
            CalcKey { label: "6";  keyType: "numeric"; alphaLabel: "V"; onPressed: root.handleKey("6") }
            // x² routes through handleKey when any modifier is armed so
            // 2ND + x² → √( and ALPHA + x² → I get intercepted there.
            // Default (no modifier) inserts the composite "^ 2" sequence,
            // which handleKey doesn't support (it sends a single token).
            CalcKey { label: "x²"; keyType: "function"; secondLabel: "√"; alphaLabel: "I"; onPressed: {
                if (root.secondArmed || root.alphaActive) {
                    root.handleKey("x²")
                } else {
                    uiController.processInput("^")
                    uiController.processInput("2")
                }
            } }
            CalcKey { label: "−";  keyType: "operator"; alphaLabel: "W"; onPressed: root.handleKey("−") }

            // Row 4
            CalcKey { label: "1";  keyType: "numeric"; alphaLabel: "Y"; onPressed: root.handleKey("1") }
            CalcKey { label: "2";  keyType: "numeric"; alphaLabel: "Z"; onPressed: root.handleKey("2") }
            CalcKey { label: "3";  keyType: "numeric"; onPressed: root.handleKey("3") }
            CalcKey { label: "Ans"; keyType: "function"; onPressed: root.handleKey("Ans") }
            CalcKey { label: "+";  keyType: "operator"; alphaLabel: "\""; onPressed: root.handleKey("+") }

            // Row 5
            CalcKey { label: "0";   keyType: "numeric"; secondLabel: "CATALOG"; onPressed: root.handleKey("0") }
            CalcKey { label: ".";   keyType: "numeric"; alphaLabel: ":"; onPressed: root.handleKey(".") }
            // (-) routes through handleKey when 2ND is armed (→ Ans).
            // ALPHA label "?" is aspirational — not a real token — so
            // ALPHA + (-) just disarms silently. Default primary is
            // "neg" for unary negation.
            CalcKey { label: "(-)"; keyType: "numeric"; secondLabel: "ANS"; alphaLabel: "?"; onPressed: {
                if (root.secondArmed) {
                    root.handleKey("(-)")
                } else if (root.alphaActive) {
                    // "?" isn't a real token yet, so pressing ALPHA+(-)
                    // clears the one-shot arm but preserves the lock —
                    // matches the "unmapped ALPHA key is a silent
                    // passthrough" rule in handleKey.
                    root.alphaArmed = false
                } else {
                    uiController.processInput("neg")
                }
            } }
            CalcKey {
                label: "ENTER"
                keyType: "enter"
                secondLabel: "ENTRY"
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

    MODEPopup {
        id: modePopup
    }

    LogicMenuPopup {
        id: logicMenuPopup
    }

    CatalogPopup {
        id: catalogPopup
    }
}

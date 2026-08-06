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

    // Keep the Style singleton's theme flag in sync with the persisted
    // controller setting, so the whole UI restyles when the theme is
    // toggled in MODE. The LCD panel stays dark either way (see Style).
    Binding {
        target: Style
        property: "theme"
        value: uiController.theme
    }

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
    // One-keystroke lookahead for `Y` so the keyboard can type the
    // multi-char Y1/Y2/Y3 tokens directly. The `Y` keystroke is
    // inserted immediately (so the user sees their keystroke); if
    // the next keystroke is 1/2/3 we backspace the Y and re-insert
    // the fused Y1/Y2/Y3 token. Anything else just clears the flag
    // and the previously-inserted Y stays as a plain VarY token.
    // Cleared by cursor moves / CLEAR / ENTER so the fuse doesn't
    // fire across edits the user didn't intend to chain.
    property bool pendingY: false
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
        "÷":    "!",      // factorial — matches TI-83 PRB convention
        "(-)":  "Ans",
        // Phase C lists: `{`/`}` on 2ND+(/) and L1..L6 on 2ND+1..6,
        // both matching the TI-83 keytop layout.
        "(":    "{",
        ")":    "}",
        "1":    "L1",
        "2":    "L2",
        "3":    "L3",
        "4":    "L4",
        "5":    "L5",
        "6":    "L6"
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
        // K / L rehomed onto the π / e keys' free ALPHA slots after the
        // ( / ) ALPHA slots were repurposed for the [ / ] brackets.
        "π":     "K", "e":     "L",
        // ALPHA+( / ALPHA+) give the matrix-literal brackets [ / ] rather
        // than the letters K / L, grouping all three bracket pairs on the
        // two bracket keys (primary () · 2ND {} · ALPHA []). K/L stay
        // typeable via the physical keyboard.
        "(":     "[", ")":     "]", ",":    "J",
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

    // Arrow dispatch: in graph-mode + tracing, ←/→ moves the trace
    // cursor along the curve. Otherwise, it moves the expression
    // cursor (used during Inputting; no-op in Evaluated/Error). Both
    // the keyboard handler and the on-screen CURSOR keys go through
    // these so the behaviour stays consistent.
    function navLeft() {
        if (uiController.isGraphMode && uiController.isTracing)
            uiController.traceLeft()
        else
            uiController.moveCursorLeft()
    }
    function navRight() {
        if (uiController.isGraphMode && uiController.isTracing)
            uiController.traceRight()
        else
            uiController.moveCursorRight()
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
            // 2ND + + opens the MEM (memory management) menu.
            if (primary === "+") {
                memPopup.open()
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

            // Y lookahead — the previous keystroke inserted a visible
            // VarY and set pendingY. If THIS keystroke is a digit 0-9, we
            // backspace the Y and re-insert the fused Y1..Y9 / Y0 token.
            // Anything else just clears the flag and leaves the Y as
            // a plain VarY.
            if (root.pendingY) {
                root.pendingY = false
                const text = event.text
                if (text.length === 1 && text >= "0" && text <= "9") {
                    uiController.processInput("DEL")
                    root.handleKey("Y" + text)
                    event.accepted = true
                    return
                }
                // Fall through — the Y was already inserted on the
                // previous keystroke; continue processing this new
                // event normally.
            }

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
                root.pendingY = false
                root.handleKey("ENTER")
                event.accepted = true
                return
            case Qt.Key_Backspace:
                root.pendingY = false
                root.handleKey("DEL")
                event.accepted = true
                return
            case Qt.Key_Escape:
                root.pendingY = false
                root.clearModifiers()
                root.handleKey("CLEAR")
                event.accepted = true
                return
            case Qt.Key_Left:
                root.pendingY = false
                root.navLeft()
                event.accepted = true
                return
            case Qt.Key_Right:
                root.pendingY = false
                root.navRight()
                event.accepted = true
                return
            case Qt.Key_Up:
                // ↑ has two contextual roles:
                //   - graph + trace: cycle to previous function slot
                //   - table mode: scroll the visible window up by 1 step
                if (uiController.isTableMode) {
                    tableView.scrollOffsetSteps -= 1
                    event.accepted = true
                    return
                }
                if (uiController.isGraphMode && uiController.isTracing) {
                    const ai = uiController.activeFunctionIndex
                    uiController.setActiveFunction((ai + 2) % 3)  // up = previous
                    event.accepted = true
                }
                return
            case Qt.Key_Down:
                if (uiController.isTableMode) {
                    tableView.scrollOffsetSteps += 1
                    event.accepted = true
                    return
                }
                if (uiController.isGraphMode && uiController.isTracing) {
                    const ai = uiController.activeFunctionIndex
                    uiController.setActiveFunction((ai + 1) % 3)
                    event.accepted = true
                }
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
            if (event.text.length === 0) {
                // Modifier-only event (Shift, Ctrl release, etc.) —
                // don't let it disturb the pending-Y state.
                return
            }
            const ch = event.text.charAt(0)

            // First Y: insert it now (visible feedback) and arm the
            // lookahead. The next keystroke handler at the top will
            // either fuse (digit 1/2/3) or clear the flag.
            if (ch === "Y") {
                root.handleKey("Y")
                root.pendingY = true
                event.accepted = true
                return
            }

            const map = {
                "0": "0", "1": "1", "2": "2", "3": "3", "4": "4",
                "5": "5", "6": "6", "7": "7", "8": "8", "9": "9",
                ".": ".",
                "+": "+", "-": "−", "*": "×", "/": "÷",
                "^": "^", "(": "(", ")": ")",
                "[": "[", "]": "]",   // matrix-literal brackets [[1,2][3,4]]
                "{": "{", "}": "}",   // list-literal braces {1,2,3}
                "s": "sin(", "c": "cos(", "t": "tan(",
                "l": "log(", "n": "ln(", "r": "√(", "p": "π",
                "i": "i",  // imaginary unit
                "!": "!",
                "|": "→",  // STO assignment arrow — Shift-\\ on US layout.
                // Uppercase letters → single-letter variable tokens.
                // Lowercase is reserved for the function shortcuts
                // above (s/c/t/l/n/r/p), so users type SHIFT+letter
                // when they actually want a variable. Y1/Y2/Y3 still
                // need CATALOG (2ND+0) since they're multi-char tokens
                // and the keyboard handler is single-char.
                "A": "A", "B": "B", "C": "C", "D": "D", "E": "E",
                "F": "F", "G": "G", "H": "H", "I": "I", "J": "J",
                "K": "K", "L": "L", "M": "M", "N": "N", "O": "O",
                "P": "P", "Q": "Q", "R": "R", "S": "S", "T": "T",
                // "Y" is handled by the pendingY lookahead above so
                // a `Y` followed by `1`/`2`/`3` fuses into the
                // multi-char Y1/Y2/Y3 token.
                "U": "U", "V": "V", "W": "W", "X": "X", "Z": "Z"
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
                // Compact MODE indicator. Three segments, separated by
                // two spaces, all bound to live controller properties:
                //   - Notation:  NORMAL / SCI / ENG
                //   - Decimal:   "" if Float, otherwise "FIX N"
                //   - Angle:     RAD / DEG
                // Anything at its default just contributes its label; a
                // changed Decimal adds an extra "FIX N" segment so the
                // indicator widens to show non-default state.
                text: {
                    const n = uiController.notation
                    const fx = uiController.fixDecimals
                    const a = uiController.angleMode
                    const g = uiController.graphMode
                    const notation = (n === 1) ? "SCI" : (n === 2) ? "ENG" : "NORMAL"
                    const angle = (a === 1) ? "DEG" : "RAD"
                    const fixSeg = (fx >= 0) ? ("  FIX " + fx) : ""
                    // Surface a non-default graph mode; Func (default)
                    // stays implicit to avoid clutter.
                    const graphSeg = (g === 2) ? "  POL" : (g === 1) ? "  PAR"
                                    : (g === 3) ? "  SEQ" : ""
                    const cx = uiController.complexMode
                    const cxSeg = (cx === 1) ? "  a+bi" : (cx === 2) ? "  re^θi" : ""
                    return notation + fixSeg + "  " + angle + graphSeg + cxSeg
                }
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
                // 2ND + soft-key opens the TI-83 "blue" variants.
                // Currently wired: 2ND+WINDOW = TBLSET, 2ND+GRAPH = TABLE.
                // (2ND+ZOOM = MEMORY, 2ND+TRACE = CALC are still no-ops.)
                if (root.secondArmed) {
                    root.secondArmed = false
                    if (label === "WINDOW") {
                        tblSetPopup.open()
                        return
                    }
                    if (label === "GRAPH") {
                        if (!uiController.isTableMode)
                            uiController.toggleTableMode()
                        return
                    }
                    if (label === "Y=") {
                        // 2ND+Y= → STAT PLOT setup (TI-83 layout).
                        statPlotPopup.open()
                        return
                    }
                    if (label === "ZOOM") {
                        // 2ND+ZOOM → FORMAT menu (TI-83 layout).
                        formatPopup.open()
                        return
                    }
                    if (label === "TRACE") {
                        // 2ND+TRACE → DRAW menu (our binding; TI-83 uses
                        // 2ND+PRGM, which this keypad doesn't have).
                        drawPopup.open()
                        return
                    }
                    // 2ND+<unmapped> falls through to primary action.
                }

                if (label === "WINDOW") {
                    windowPopup.open()
                } else if (label === "ZOOM") {
                    zoomPopup.open()
                } else if (label === "TRACE") {
                    // TRACE only makes sense in graph mode; jump to it
                    // automatically if we're still on the keypad.
                    if (!uiController.isGraphMode)
                        uiController.toggleGraphMode()
                    uiController.toggleTrace()
                } else if (label === "GRAPH") {
                    if (!uiController.isGraphMode)
                        uiController.toggleGraphMode()
                } else if (label === "Y=") {
                    // Contextual: if we're in graph/table mode, Y= just
                    // returns to the keypad (so it also exits Trace)
                    // without popping the editor. Only when already on
                    // the keypad does Y= open the Y= editor.
                    if (uiController.isGraphMode || uiController.isTableMode) {
                        if (uiController.isGraphMode)
                            uiController.toggleGraphMode()
                        if (uiController.isTableMode)
                            uiController.toggleTableMode()
                    } else {
                        yEditorPopup.open()
                    }
                }
            }
        }

        // ── Mode switcher: keypad ↔ graph ↔ table ────────
        // The bottom half of the calculator column swaps between the
        // keypad sections (page 0), the graph canvas (page 1), and
        // the table view (page 2). Toggled by GRAPH / 2ND+GRAPH /
        // Y= in the soft-key row above.
        ColumnLayout {
            id: viewRegion
            Layout.fillWidth: true
            Layout.fillHeight: true
            readonly property bool viewActive: uiController.isGraphMode || uiController.isTableMode

            // ── View area: graph and/or table ──
            // MODE → Screen: Full shows the active view alone; G-T shows the
            // graph and table side by side; Horiz shows the view here with
            // the keypad below (so the graph renders on top). Placed above
            // the keypad on purpose.
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: viewRegion.viewActive
                spacing: 6

                GraphCanvas {
                    visible: viewRegion.viewActive
                             && (uiController.screenMode === 2 || uiController.isGraphMode)
                }
                TableView {
                    id: tableView
                    visible: viewRegion.viewActive
                             && (uiController.screenMode === 2 || uiController.isTableMode)
                }
            }

            // ── Keypad (CONTROL / SCIENTIFIC / NUMERIC) ──
            // Visible when not in a view (any screen mode) or in the Horiz
            // split. Fills the region only when it's the sole content; in
            // Horiz it takes its natural height with the graph filling above.
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: !viewRegion.viewActive
                visible: !viewRegion.viewActive || uiController.screenMode === 1
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
            CalcKey { label: "ALPHA"; keyType: "alpha"; armed: root.alphaArmed || root.alphaLocked; onPressed: root.armAlpha() }
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
            CalcKey { label: "←";    keyType: "function"; onPressed: root.navLeft() }
            CalcKey { label: "→";    keyType: "function"; onPressed: root.navRight() }
            CalcKey { label: "END";  keyType: "function"; onPressed: uiController.moveCursorEnd() }
            // STO▸ doesn't have a natural 2ND home on the existing
            // keypad, so it lives in the 5th CURSOR slot — out of the
            // way but always one click away (matches the spirit of the
            // dedicated key on real TI-83 hardware).
            CalcKey { label: "STO▸"; keyType: "function"; onPressed: uiController.processExpression("→") }
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

            // √( inserts the root; 2ND+√( opens the PRGM program manager
            // (this keypad has no dedicated PRGM key — see docs/TIBASIC.md).
            CalcKey { label: "√(";    keyType: "function"; secondLabel: "PRGM"; onPressed: {
                if (root.secondArmed) {
                    root.clearModifiers()
                    prgmPopup.open()
                } else {
                    root.handleKey("√(")
                }
            } }
            CalcKey { label: "ln(";   keyType: "function"; secondLabel: "eˣ"; alphaLabel: "S"; onPressed: root.handleKey("ln(")   }
            CalcKey { label: "log(";  keyType: "function"; alphaLabel: "N"; onPressed: root.handleKey("log(")  }
            CalcKey { label: "π";     keyType: "function"; alphaLabel: "K"; onPressed: root.handleKey("π")    }
            CalcKey { label: "e";     keyType: "function"; alphaLabel: "L"; onPressed: root.handleKey("e")    }
        }

        // ── 6. NUMERIC section ──────────────────────────
        SectionHeader { label: "NUMERIC" }
        GridLayout {
            Layout.fillWidth: true
            columns: 5
            rowSpacing: 6
            columnSpacing: 6

            // Row 1
            CalcKey { label: "(";  keyType: "function"; secondLabel: "{"; alphaLabel: "["; onPressed: root.handleKey("(") }
            CalcKey { label: ")";  keyType: "function"; secondLabel: "}"; alphaLabel: "]"; onPressed: root.handleKey(")") }
            CalcKey { label: ",";  keyType: "function"; alphaLabel: "J"; onPressed: root.handleKey(",") }
            // X inserts the graph variable; 2ND+X opens the Y-VARS picker
            // (the only on-screen way to enter a Y-function token — BUG-023).
            CalcKey { label: "X";  keyType: "function"; secondLabel: "Y-VARS"; onPressed: {
                if (root.secondArmed) {
                    root.clearModifiers()
                    yVarsPopup.open()
                } else {
                    root.handleKey("X")
                }
            } }
            CalcKey { label: "÷";  keyType: "operator"; secondLabel: "!"; alphaLabel: "M"; onPressed: root.handleKey("÷") }

            // Row 2
            CalcKey { label: "7";  keyType: "numeric"; alphaLabel: "O"; onPressed: root.handleKey("7") }
            CalcKey { label: "8";  keyType: "numeric"; alphaLabel: "P"; onPressed: root.handleKey("8") }
            CalcKey { label: "9";  keyType: "numeric"; alphaLabel: "Q"; onPressed: root.handleKey("9") }
            // MATRX: 2ND-armed → open the Stat list editor (STAT); we
            // have no dedicated STAT key, so the list editor shares this
            // key's 2ND slot with the matrix editor. ALPHA-armed → insert
            // letter B via handleKey; otherwise open the matrix popup.
            CalcKey { label: "MATRX"; keyType: "function"; secondLabel: "STAT"; alphaLabel: "B"; onPressed: {
                if (root.secondArmed) {
                    root.clearModifiers()
                    listPopup.open()
                } else if (root.alphaActive) {
                    root.handleKey("MATRX")
                } else {
                    root.clearModifiers()
                    matrixPopup.open()
                }
            } }
            CalcKey { label: "×";  keyType: "operator"; alphaLabel: "R"; onPressed: root.handleKey("×") }

            // Row 3
            CalcKey { label: "4";  keyType: "numeric"; secondLabel: "L4"; alphaLabel: "T"; onPressed: root.handleKey("4") }
            CalcKey { label: "5";  keyType: "numeric"; secondLabel: "L5"; alphaLabel: "U"; onPressed: root.handleKey("5") }
            CalcKey { label: "6";  keyType: "numeric"; secondLabel: "L6"; alphaLabel: "V"; onPressed: root.handleKey("6") }
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
            CalcKey { label: "1";  keyType: "numeric"; secondLabel: "L1"; alphaLabel: "Y"; onPressed: root.handleKey("1") }
            CalcKey { label: "2";  keyType: "numeric"; secondLabel: "L2"; alphaLabel: "Z"; onPressed: root.handleKey("2") }
            CalcKey { label: "3";  keyType: "numeric"; secondLabel: "L3"; onPressed: root.handleKey("3") }
            CalcKey { label: "Ans"; keyType: "function"; onPressed: root.handleKey("Ans") }
            CalcKey { label: "+";  keyType: "operator"; secondLabel: "MEM"; alphaLabel: "\""; onPressed: root.handleKey("+") }

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

    ListPopup {
        id: listPopup
        onStatsRequested: (listName) => {
            statResultsPopup.mode = "oneVar"
            statResultsPopup.sourceLabel = listName
            statResultsPopup.results = uiController.oneVarStats(listName)
            statResultsPopup.open()
        }
        onTwoVarRequested: () => {
            statResultsPopup.mode = "twoVar"
            statResultsPopup.sourceLabel = "L1,L2"
            statResultsPopup.results = uiController.twoVarStats("L1", "L2")
            statResultsPopup.open()
        }
        onRegMenuRequested: () => regMenuPopup.open()
    }

    StatResultsPopup {
        id: statResultsPopup
    }

    RegMenuPopup {
        id: regMenuPopup
        onPicked: (type, label) => {
            statResultsPopup.mode = "reg"
            statResultsPopup.sourceLabel = label
            statResultsPopup.results = uiController.regression(type, "L1", "L2")
            statResultsPopup.open()
        }
    }

    StatPlotPopup {
        id: statPlotPopup
    }

    MEMPopup {
        id: memPopup
    }

    FormatPopup {
        id: formatPopup
    }

    YEditorPopup {
        id: yEditorPopup
    }

    YVarsPopup {
        id: yVarsPopup
    }

    PRGMPopup {
        id: prgmPopup
    }

    PrgmRunPopup {
        id: prgmRunPopup
        // "◀ PRGM" — leave the run view and return to the program manager.
        onBackToPrograms: {
            prgmRunPopup.close()
            prgmPopup.open()
        }
    }

    // Open (or refresh) the run/output view whenever a program's run state
    // changes — on start, on each pause for input, and on completion.
    Connections {
        target: uiController
        function onProgramRunUpdated() { prgmRunPopup.open() }
    }

    DRAWPopup {
        id: drawPopup
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

    ZoomPopup {
        id: zoomPopup
    }

    TblSetPopup {
        id: tblSetPopup
    }
}

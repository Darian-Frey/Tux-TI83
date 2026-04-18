import QtQuick
import QtQuick.Layouts
import ".."

// LCD display panel for the calculator.
//
// Visual contract: a dark Rectangle (always #0b1120 — permanent, not themed)
// containing a small expression-history line (top-right) and a large main
// readout (bottom-right) with a blinking cursor when in INPUTTING state.
//
// The main readout is a read-only TextInput rather than a plain Text, which
// gives the user:
//   - horizontal scrolling when the result is wider than the panel
//     (arrow keys move the cursor; the view follows)
//   - Home / End to jump to either edge
//   - mouse-drag to select, Ctrl+C to copy
//   - default scroll-to-end on new output so the most recent characters
//     are visible
//
// Owners drive it via three properties: `currentState` (mirrors
// UIController::DisplayState — 0 Inputting / 1 Evaluated / 2 Error),
// `expressionText` (top line — typically empty in INPUTTING, "expression ="
// in EVALUATED/ERROR), and `mainText` (bottom line — the live expression
// in INPUTTING, the result in EVALUATED, "ERR:SYNTAX" in ERROR).
Rectangle {
    id: root

    // ── Public API ────────────────────────────────────────
    property string expressionText: ""
    property string mainText: ""
    // 0 = Inputting, 1 = Evaluated, 2 = Error
    property int currentState: 0
    // Character offset in mainText where the edit cursor sits. Only
    // meaningful in Inputting state — callers pass
    // `uiController.cursorOffset`; for Evaluated/Error states the
    // readout auto-snaps to the end of the text on change.
    property int cursorCharOffset: 0

    // ── Visual ────────────────────────────────────────────
    color: Style.bgDisplay
    radius: 4
    border.color: Style.bgSection
    border.width: 1
    implicitHeight: 96
    clip: true

    // Top: expression history line ("expression =" after evaluation).
    Text {
        id: exprLine
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 12
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        text: root.expressionText
        horizontalAlignment: Text.AlignRight
        color: root.currentState === 2 ? Style.textError : Style.textExpr
        font.family: Style.monoFamily
        font.pixelSize: Style.exprPixelSize
        elide: Text.ElideLeft
    }

    // Bottom: main readout as a read-only TextInput so the user can
    // scroll / select / copy long output (fixes the "matrix clipped at
    // the left" issue that plain Text had).
    //
    // `activeFocusOnPress` is gated by state: while INPUTTING, clicks
    // don't steal focus from the root keyboard handler — the user's
    // typing goes to the calculator engine as expected. After an
    // evaluation, clicking the display focuses it so arrow keys / mouse
    // / Ctrl+C work on the result.
    TextInput {
        id: readout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.bottomMargin: 12
        text: root.mainText
        readOnly: true
        selectByMouse: true
        horizontalAlignment: TextInput.AlignRight
        clip: true
        activeFocusOnPress: root.currentState !== 0
        color: root.currentState === 1 ? Style.textResult
             : root.currentState === 2 ? Style.textError
                                       : Style.textDisplay
        font.family: Style.monoFamily
        font.pixelSize: Style.displayPixelSize

        // Cursor shown in INPUTTING or whenever the user has clicked
        // the display to navigate/select. cursorDelegate replaces
        // TextInput's default cursor with our blinking Rectangle so
        // users see a consistent blink across all three states. The
        // cursor's colour tracks the text colour (white while typing,
        // green for results, red for errors) for visual consistency.
        cursorVisible: root.currentState === 0 || readout.activeFocus
        cursorDelegate: Rectangle {
            width: Style.cursorWidth
            height: Style.cursorHeight
            color: readout.color
            SequentialAnimation on opacity {
                running: true
                loops: Animation.Infinite
                PropertyAction { value: 1 }
                PauseAnimation { duration: Style.cursorBlinkMs }
                PropertyAction { value: 0 }
                PauseAnimation { duration: Style.cursorBlinkMs }
            }
        }

        // Cursor placement. In Inputting, it tracks the controller's
        // token-level cursor (translated to a char offset on the C++
        // side). In Evaluated/Error, snap to the end so long results
        // land scrolled to the right edge and Home lets the user walk
        // back through clipped leading characters.
        cursorPosition: root.currentState === 0
                        ? Math.min(root.cursorCharOffset, text.length)
                        : text.length
        onTextChanged: {
            if (root.currentState !== 0)
                cursorPosition = text.length
        }
    }
}

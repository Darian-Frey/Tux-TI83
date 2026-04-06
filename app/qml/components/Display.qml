import QtQuick
import QtQuick.Layouts
import ".."

// LCD display panel for the calculator.
//
// Visual contract: a dark Rectangle (always #0b1120 — permanent, not themed)
// containing a small expression-history line (top-right), a large main
// readout (bottom-right), and a blinking cursor positioned just to the
// right of the readout when the state machine is in INPUTTING.
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

    // Bottom: main readout + cursor as a right-anchored Row.
    // The row's right edge is fixed; as the readout text grows the row
    // extends leftward, keeping the cursor pinned just to the right of
    // the last character (TI-83 behaviour).
    Row {
        id: readoutRow
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 12
        anchors.bottomMargin: 12
        spacing: 3

        Text {
            id: readout
            anchors.verticalCenter: parent.verticalCenter
            text: root.mainText
            color: root.currentState === 1 ? Style.textResult
                 : root.currentState === 2 ? Style.textError
                                           : Style.textDisplay
            font.family: Style.monoFamily
            font.pixelSize: Style.displayPixelSize
        }

        Rectangle {
            id: cursor
            anchors.verticalCenter: parent.verticalCenter
            width: Style.cursorWidth
            height: Style.cursorHeight
            color: Style.textDisplay
            visible: root.currentState === 0  // INPUTTING only

            // Square-wave blink at Style.cursorBlinkMs intervals.
            // PropertyAction snaps opacity instantly so the cursor reads
            // as a hard on/off rather than a fade.
            SequentialAnimation on opacity {
                running: cursor.visible
                loops: Animation.Infinite
                PropertyAction { value: 1 }
                PauseAnimation { duration: Style.cursorBlinkMs }
                PropertyAction { value: 0 }
                PauseAnimation { duration: Style.cursorBlinkMs }
            }
        }
    }
}

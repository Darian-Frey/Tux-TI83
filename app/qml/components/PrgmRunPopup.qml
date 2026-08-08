import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Program run / output view (P2) — the program's "home screen". Shows the
// Disp / echo output on a dark LCD-style panel; opened automatically when a
// program finishes (uiController.programRunFinished). ClrHome clears the
// buffer during a run. Interactive I/O (Input/Prompt/Pause) arrives in P4.
Popup {
    id: root

    // Emitted by the "◀ PRGM" button — the owner returns to the program
    // manager (this run view is a sibling of the PRGM popup).
    signal backToPrograms()

    // Emitted by the "EDIT LINE" button after a runtime error — the owner
    // opens the editor for `program` positioned at `line` (0-based). P5b.
    signal editAtError(string program, int line)

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // When the run view opens on a running program, hand focus to the key
    // catcher so getKey sees physical keypresses (P5b).
    onOpened: if (uiController.programRunning) keyCatcher.forceActiveFocus()

    width: 360
    height: 460
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    background: Rectangle {
        color: Style.bgSurface
        radius: 8
        border.color: Style.opBorder
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 10

        // Captures physical keypresses while a program is running and forwards
        // them to getKey (P5b). Holds focus only during a run, so it doesn't
        // fight the Input TextField (which grabs focus when awaiting input).
        Item {
            id: keyCatcher
            Layout.preferredWidth: 0
            Layout.preferredHeight: 0
            focus: uiController.programRunning
            Keys.onPressed: function(event) {
                var code = root.tiKeyCode(event.key)
                if (code > 0) {
                    uiController.sendProgramKey(code)
                    event.accepted = true
                }
            }
            // Re-grab focus each time the program resumes running while the
            // view is already open (e.g. after an Input pause).
            Connections {
                target: uiController
                function onProgramRunStateChanged() {
                    if (uiController.programRunning && root.visible)
                        keyCatcher.forceActiveFocus()
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: "PROGRAM OUTPUT"
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

        // Output screen — dark LCD, monospace, scrollable.
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Style.bgDisplay
            radius: 4
            border.color: Style.keyBorderNeutral
            border.width: 1
            clip: true

            ListView {
                id: outView
                anchors.fill: parent
                anchors.margins: 8
                model: uiController.programOutput
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                delegate: Text {
                    width: ListView.view.width
                    text: modelData
                    color: modelData.indexOf("ERR:") === 0 ? Style.textError : Style.textDisplay
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                    wrapMode: Text.WrapAnywhere
                }
            }

            Text {
                anchors.centerIn: parent
                visible: uiController.programOutput.length === 0
                        && !uiController.programWaitingInput
                text: uiController.programRunning ? "running…" : "(no output)"
                color: Style.textMuted
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
            }
        }

        // ── Input row (Input / Prompt) ──
        RowLayout {
            Layout.fillWidth: true
            visible: uiController.programWaitingInput
            spacing: 6

            Text {
                text: uiController.programInputPrompt
                color: Style.textResult
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
            }
            TextField {
                id: inputField
                Layout.fillWidth: true
                color: Style.textDisplay
                selectByMouse: true
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
                background: Rectangle {
                    color: Style.bgDisplay
                    radius: 4
                    border.color: inputField.activeFocus ? Style.textExpr : Style.keyBorderNeutral
                    border.width: 1
                }
                onAccepted: root.submitInput()
                // Grab focus when the prompt appears.
                Connections {
                    target: uiController
                    function onProgramRunStateChanged() {
                        if (uiController.programWaitingInput)
                            inputField.forceActiveFocus()
                    }
                }
            }
            CalcKey {
                Layout.preferredWidth: 64
                Layout.fillWidth: false
                label: "ENTER"
                keyType: "enter"
                onPressed: root.submitInput()
            }
        }

        // ── Continue (Pause) ──
        CalcKey {
            Layout.fillWidth: true
            visible: uiController.programWaitingKey
            label: "▶ CONTINUE"
            keyType: "function"
            onPressed: uiController.resumeProgram()
        }

        // ── Stop (interrupt a running program) ──
        CalcKey {
            Layout.fillWidth: true
            visible: uiController.programRunning
            label: "■ STOP"
            keyType: "control"
            onPressed: uiController.stopProgram()
        }

        // ── Jump to the offending line in the editor (after an error) ──
        CalcKey {
            Layout.fillWidth: true
            visible: uiController.programErrorProgram.length > 0
                     && !uiController.programRunning
            label: uiController.programErrorLine >= 0
                   ? "✎ EDIT LINE " + (uiController.programErrorLine + 1)
                   : "✎ EDIT " + uiController.programErrorProgram
            keyType: "function"
            onPressed: root.editAtError(uiController.programErrorProgram,
                                        uiController.programErrorLine)
        }

        // Navigation is hidden while the program is actively running — only
        // STOP is offered so a tight loop can't be left mid-run.
        RowLayout {
            Layout.fillWidth: true
            visible: !uiController.programRunning
            spacing: 6
            CalcKey {
                label: "◀ PRGM"
                keyType: "function"
                onPressed: root.backToPrograms()
            }
            CalcKey {
                id: copyKey
                label: "COPY"
                keyType: "function"
                enabled: uiController.programOutput.length > 0
                onPressed: {
                    uiController.copyProgramOutput()
                    copyKey.label = "COPIED!"
                    copyResetTimer.restart()
                }
            }
            CalcKey {
                label: "DONE"
                keyType: "enter"
                onPressed: root.close()
            }
        }
    }

    // Revert the COPY button label after the brief confirmation.
    Timer {
        id: copyResetTimer
        interval: 1200
        onTriggered: copyKey.label = "COPY"
    }

    function submitInput() {
        uiController.provideProgramInput(inputField.text)
        inputField.text = ""
    }

    // Map a physical key to its TI-83 getKey code (0 = not mapped). Covers the
    // keys getKey programs actually use — arrows, ENTER, CLEAR, DEL, and the
    // number pad; more can be added later.
    function tiKeyCode(k) {
        switch (k) {
        case Qt.Key_Up:        return 25
        case Qt.Key_Down:      return 34
        case Qt.Key_Left:      return 24
        case Qt.Key_Right:     return 26
        case Qt.Key_Return:
        case Qt.Key_Enter:     return 105
        case Qt.Key_Escape:    return 45   // CLEAR
        case Qt.Key_Backspace: return 23   // DEL
        case Qt.Key_0:         return 102
        case Qt.Key_1:         return 92
        case Qt.Key_2:         return 93
        case Qt.Key_3:         return 94
        case Qt.Key_4:         return 82
        case Qt.Key_5:         return 83
        case Qt.Key_6:         return 84
        case Qt.Key_7:         return 72
        case Qt.Key_8:         return 73
        case Qt.Key_9:         return 74
        case Qt.Key_Period:    return 103
        }
        return 0
    }
}

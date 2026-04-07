import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Modal popup for matrix operations: select, insert, edit.
//
// Three tabs (matching the legacy popup's structure):
//   NAMES — list of available matrices ([A], [B], [C]); click inserts
//   MATH  — matrix-specific functions (det() for now); click inserts
//   EDIT  — 3×3 grid editor for matrix [A]; SAVE commits to the registry
//
// Behavioural contract: NAMES and MATH routes go through
// `uiController.processInput()` to insert tokens at the cursor position.
// EDIT calls `uiController.updateMatrix()` to write to the registry. The
// popup auto-closes after any insertion or save.
//
// Limitations carried over from the legacy popup (worth a future
// improvement entry): the EDIT tab always targets `[A]` (no matrix
// selector), is fixed at 3×3 (no variable dimensions), and starts with
// empty fields each time it opens (doesn't read existing values back).
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 320
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

        // ── Header ──
        Text {
            Layout.fillWidth: true
            text: "MATRIX"
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

        // ── Tab bar ──
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            currentIndex: stack.currentIndex
            onCurrentIndexChanged: stack.currentIndex = currentIndex
            background: Rectangle { color: "transparent" }

            TabButton {
                id: namesBtn
                text: "NAMES"
                contentItem: Text {
                    text: namesBtn.text
                    color: namesBtn.checked ? Style.textPrimary : Style.textMuted
                    font.family: Style.monoFamily
                    font.pixelSize: Style.funcKeyLabelPixelSize
                    font.weight: Style.keyLabelFontWeight
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: namesBtn.checked ? Style.bgSection : "transparent"
                    radius: 4
                }
            }

            TabButton {
                id: mathBtn
                text: "MATH"
                contentItem: Text {
                    text: mathBtn.text
                    color: mathBtn.checked ? Style.textPrimary : Style.textMuted
                    font.family: Style.monoFamily
                    font.pixelSize: Style.funcKeyLabelPixelSize
                    font.weight: Style.keyLabelFontWeight
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: mathBtn.checked ? Style.bgSection : "transparent"
                    radius: 4
                }
            }

            TabButton {
                id: editBtn
                text: "EDIT"
                contentItem: Text {
                    text: editBtn.text
                    color: editBtn.checked ? Style.textPrimary : Style.textMuted
                    font.family: Style.monoFamily
                    font.pixelSize: Style.funcKeyLabelPixelSize
                    font.weight: Style.keyLabelFontWeight
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: editBtn.checked ? Style.bgSection : "transparent"
                    radius: 4
                }
            }
        }

        // ── Tab content ──
        StackLayout {
            id: stack
            currentIndex: tabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true

            // NAMES tab — list of matrix references
            Item {
                ListView {
                    anchors.fill: parent
                    model: ["[A]", "[B]", "[C]"]
                    spacing: 4
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 36
                        color: nameArea.containsMouse
                               ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                               : Style.bgSection
                        radius: 4
                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData
                            color: Style.textDisplay
                            font.family: Style.monoFamily
                            font.pixelSize: Style.keyLabelPixelSize
                        }
                        MouseArea {
                            id: nameArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                uiController.processInput(modelData)
                                root.close()
                            }
                        }
                    }
                }
            }

            // MATH tab — matrix-specific functions
            Item {
                ListView {
                    anchors.fill: parent
                    model: [{ display: "1: det(", input: "det(" }]
                    spacing: 4
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 36
                        color: mathArea.containsMouse
                               ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                               : Style.bgSection
                        radius: 4
                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.display
                            color: Style.textDisplay
                            font.family: Style.monoFamily
                            font.pixelSize: Style.keyLabelPixelSize
                        }
                        MouseArea {
                            id: mathArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                uiController.processInput(modelData.input)
                                root.close()
                            }
                        }
                    }
                }
            }

            // EDIT tab — 3×3 matrix grid editor (target: [A])
            ColumnLayout {
                spacing: 8

                Text {
                    Layout.fillWidth: true
                    text: "Edit [A] (3×3)"
                    color: Style.textSecondary
                    font.family: Style.monoFamily
                    font.pixelSize: Style.exprPixelSize
                }

                GridLayout {
                    id: matrixGrid
                    columns: 3
                    rowSpacing: 6
                    columnSpacing: 6
                    Layout.fillWidth: true

                    Repeater {
                        model: 9
                        TextField {
                            placeholderText: "0"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32
                            color: Style.textDisplay
                            font.family: Style.monoFamily
                            font.pixelSize: Style.keyLabelPixelSize
                            horizontalAlignment: TextInput.AlignHCenter
                            selectByMouse: true
                            background: Rectangle {
                                color: Style.bgDisplay
                                radius: 4
                                border.color: parent.activeFocus ? Style.textExpr : Style.keyBorderNeutral
                                border.width: 1
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                CalcKey {
                    label: "SAVE TO [A]"
                    keyType: "enter"
                    onPressed: {
                        // Walk the GridLayout's children. Repeater inserts its
                        // instantiated items as children of the parent layout
                        // ahead of the Repeater itself, so children[0..8] are
                        // the nine TextFields. Non-finite parses fall back to 0
                        // (matches legacy popup behaviour after BUG-001-style
                        // guarding).
                        var vals = []
                        for (var i = 0; i < 9; i++) {
                            var raw = matrixGrid.children[i].text
                            var v = parseFloat(raw)
                            vals.push(Number.isFinite(v) ? v : 0)
                        }
                        uiController.updateMatrix("[A]", 3, 3, vals)
                        root.close()
                    }
                }
            }
        }
    }
}

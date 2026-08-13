import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Modal popup for matrix operations: select, insert, edit.
//
// Three tabs (matching the legacy popup's structure):
//   NAMES — list of available matrices ([A]–[J]); click inserts
//   MATH  — matrix-specific functions (det() for now); click inserts
//   EDIT  — 3×3 grid editor for matrix [A]; SAVE commits to the registry
//
// Behavioural contract: NAMES and MATH routes go through
// `uiController.processInput()` to insert tokens at the cursor position.
// EDIT calls `uiController.updateMatrix()` to write to the registry. The
// popup auto-closes after any insertion or save.
//
// Matrix editor v2 (IMP-007 + IMP-008): the EDIT tab now has a matrix
// selector ([A]–[J]), variable dimensions (1×1 up to 6×6), and reads any
// existing stored values back into the grid on open / tab-switch /
// selection so editing an existing matrix doesn't require retyping.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 320
    // Tall enough to show the full MATH-tab list (11 entries × 40px) plus
    // the header, tab bar, and padding without clipping the last rows.
    height: 640
    padding: 14

    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    // ── EDIT-tab state ──
    // `cells` is a flat row-major array of the field strings; `mRows`/
    // `mCols` are the live dimensions. The TextFields are not bound to
    // `cells` (binding would fight user edits) — instead each field
    // initialises from it and writes back via setCell, and reloads push
    // fresh values in through syncFields.
    property string selectedMatrix: "[A]"
    property int mRows: 3
    property int mCols: 3
    property var cells: []

    readonly property int maxDim: 6

    function cellText(i) { return (i >= 0 && i < cells.length) ? cells[i] : "" }
    function setCell(i, t) { if (i >= 0 && i < cells.length) cells[i] = t }

    // Load a stored matrix into the editor. Empty/undefined slots fall
    // back to a blank 3×3 grid.
    function loadMatrix(name) {
        selectedMatrix = name
        var m = uiController.getMatrix(name)
        var r = (m && m.rows > 0) ? m.rows : 3
        var c = (m && m.cols > 0) ? m.cols : 3
        var data = (m && m.data) ? m.data : []
        var arr = []
        for (var i = 0; i < r * c; i++)
            arr.push(i < data.length ? String(data[i]) : "")
        cells = arr
        mRows = r
        mCols = c
        Qt.callLater(syncFields)
    }

    // Grow/shrink the working array to mRows×mCols, preserving values by
    // flat index (fill from top-left, drop the tail).
    function resizeCells() {
        var arr = []
        for (var i = 0; i < mRows * mCols; i++)
            arr.push(i < cells.length ? cells[i] : "")
        cells = arr
        Qt.callLater(syncFields)
    }

    // Push `cells` back into whatever TextFields currently exist. Called
    // via Qt.callLater so it runs after the Repeater has (re)built its
    // delegates for the current dimensions.
    function syncFields() {
        for (var i = 0; i < matrixGrid.children.length; i++) {
            var ch = matrixGrid.children[i]
            if (ch && ch.hasOwnProperty("cellIndex") && ch.hasOwnProperty("text"))
                ch.text = cellText(ch.cellIndex)
        }
    }

    onOpened: loadMatrix(selectedMatrix)

    // Compact [−] N [+] dimension stepper used for both rows and cols.
    component DimStepper: Row {
        id: stp
        property string labelText: ""
        property int value: 0
        signal dec()
        signal inc()
        spacing: 4

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: stp.labelText
            color: Style.textMuted
            font.family: Style.monoFamily
            font.pixelSize: Style.funcKeyLabelPixelSize
        }
        Rectangle {
            width: 26; height: 26; radius: 4
            color: decArea.containsMouse
                   ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                   : Style.bgSection
            border.width: 1
            border.color: Style.keyBorderNeutral
            Text {
                anchors.centerIn: parent
                text: "−"
                color: Style.textPrimary
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
            }
            MouseArea {
                id: decArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: stp.dec()
            }
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: 18
            horizontalAlignment: Text.AlignHCenter
            text: stp.value
            color: Style.textPrimary
            font.family: Style.monoFamily
            font.pixelSize: Style.keyLabelPixelSize
        }
        Rectangle {
            width: 26; height: 26; radius: 4
            color: incArea.containsMouse
                   ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                   : Style.bgSection
            border.width: 1
            border.color: Style.keyBorderNeutral
            Text {
                anchors.centerIn: parent
                text: "+"
                color: Style.textPrimary
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
            }
            MouseArea {
                id: incArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: stp.inc()
            }
        }
    }

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

            // NAMES tab — matrix references plus the literal brackets
            // `[` / `]` so a matrix literal `[[1,2][3,4]]` can be built
            // on-screen (they also map to the physical [ and ] keys).
            Item {
                ListView {
                    anchors.fill: parent
                    model: ["[A]", "[B]", "[C]", "[D]", "[E]", "[F]", "[G]",
                            "[H]", "[I]", "[J]", "[", "]"]
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
                            color: Style.textPrimary
                            font.family: Style.monoFamily
                            font.pixelSize: Style.keyLabelPixelSize
                        }
                        MouseArea {
                            id: nameArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                // processExpression tokenises multi-char
                                // inputs (e.g. "^-1" → ^, -, 1) so the
                                // MATH tab can insert composite sequences.
                                uiController.processExpression(modelData)
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
                    model: [
                        { display: "1: det(",  input: "det("  },
                        { display: "2: T(",    input: "T("    },
                        { display: "3: rref(", input: "rref(" },
                        { display: "4: ref(",  input: "ref("  },
                        { display: "5: ^-1 (inverse)", input: "^-1" },
                        { display: "6: identity(", input: "identity(" },
                        { display: "7: dim(",     input: "dim("     },
                        { display: "8: augment(", input: "augment(" },
                        { display: "9: randM(",   input: "randM("   },
                        { display: "A: List▶Matr(", input: "List▶Matr(" },
                        { display: "B: Matr▶List(", input: "Matr▶List(" }
                    ]
                    spacing: 4
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    // Visible scrollbar so the lower entries (List▶Matr /
                    // Matr▶List) are discoverable even if the list overflows.
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

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
                            color: Style.textPrimary
                            font.family: Style.monoFamily
                            font.pixelSize: Style.keyLabelPixelSize
                        }
                        MouseArea {
                            id: mathArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                // processExpression tokenises multi-char
                                // inputs like "^-1" into [^, -, 1]. Plain
                                // single-token inputs (e.g. "det(") work
                                // identically — they just emit one token.
                                uiController.processExpression(modelData.input)
                                root.close()
                            }
                        }
                    }
                }
            }

            // EDIT tab — matrix selector + variable-dimension grid editor
            ColumnLayout {
                spacing: 8
                onVisibleChanged: if (visible) root.loadMatrix(root.selectedMatrix)

                // ── Matrix selector [A]–[J] (two rows of five) ──
                Grid {
                    Layout.fillWidth: true
                    columns: 5
                    spacing: 5
                    Repeater {
                        model: ["[A]", "[B]", "[C]", "[D]", "[E]",
                                "[F]", "[G]", "[H]", "[I]", "[J]"]
                        Rectangle {
                            width: (matrixGrid.width - 4 * 5) / 5
                            height: 30
                            radius: 4
                            property bool sel: root.selectedMatrix === modelData
                            color: selArea.containsMouse
                                   ? Qt.lighter(Style.bgSection, 1.0 + Style.keyHoverLighten)
                                   : Style.bgSection
                            border.width: 1
                            border.color: sel ? Style.textExpr : Style.keyBorderNeutral
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: parent.sel ? Style.textExpr : Style.textSecondary
                                font.family: Style.monoFamily
                                font.pixelSize: Style.funcKeyLabelPixelSize
                            }
                            MouseArea {
                                id: selArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: root.loadMatrix(modelData)
                            }
                        }
                    }
                }

                // ── Dimension steppers + live label ──
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        text: "Edit " + root.selectedMatrix
                        color: Style.textSecondary
                        font.family: Style.monoFamily
                        font.pixelSize: Style.exprPixelSize
                    }
                    Item { Layout.fillWidth: true }

                    // Rows stepper
                    DimStepper {
                        labelText: "R"
                        value: root.mRows
                        onDec: if (root.mRows > 1) { root.mRows--; root.resizeCells() }
                        onInc: if (root.mRows < root.maxDim) { root.mRows++; root.resizeCells() }
                    }
                    Text {
                        text: "×"
                        color: Style.textMuted
                        font.family: Style.monoFamily
                        font.pixelSize: Style.keyLabelPixelSize
                    }
                    // Cols stepper
                    DimStepper {
                        labelText: "C"
                        value: root.mCols
                        onDec: if (root.mCols > 1) { root.mCols--; root.resizeCells() }
                        onInc: if (root.mCols < root.maxDim) { root.mCols++; root.resizeCells() }
                    }
                }

                GridLayout {
                    id: matrixGrid
                    columns: root.mCols
                    rowSpacing: 6
                    columnSpacing: 6
                    Layout.fillWidth: true

                    Repeater {
                        id: gridRepeater
                        model: root.mRows * root.mCols
                        TextField {
                            property int cellIndex: index
                            placeholderText: "0"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 30
                            color: Style.textDisplay
                            font.family: Style.monoFamily
                            font.pixelSize: Style.keyLabelPixelSize
                            horizontalAlignment: TextInput.AlignHCenter
                            selectByMouse: true
                            // Initialise from the working array; new delegates
                            // (after a dimension change) pick up their value here.
                            Component.onCompleted: text = root.cellText(cellIndex)
                            onTextChanged: root.setCell(cellIndex, text)
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
                    label: "SAVE TO " + root.selectedMatrix
                    keyType: "enter"
                    onPressed: {
                        // Collect from the working array (kept in sync by each
                        // field's onTextChanged). Non-finite / empty parses
                        // fall back to 0.
                        var vals = []
                        for (var i = 0; i < root.mRows * root.mCols; i++) {
                            var v = parseFloat(root.cells[i])
                            vals.push(Number.isFinite(v) ? v : 0)
                        }
                        uiController.updateMatrix(root.selectedMatrix,
                                                  root.mRows, root.mCols, vals)
                        root.close()
                    }
                }
            }
        }
    }
}

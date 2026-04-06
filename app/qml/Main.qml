import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

ApplicationWindow {
    id: root
    visible: true
    width: 420
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
    // Layout
    // ─────────────────────────────────────────────────────
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
                // TODO: wire Y= / WINDOW / ZOOM / TRACE / GRAPH when graph
                // mode is reintroduced. The controller has no soft-key
                // vocabulary yet, so all labels are no-op for now.
            }
        }

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
            Item { Layout.fillWidth: true; Layout.preferredHeight: Style.keyHeight }  // filler
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
            Item { Layout.fillWidth: true; Layout.preferredHeight: Style.keyHeight }  // filler (^ moved to SCIENTIFIC)
            CalcKey { label: "×";  keyType: "operator"; onPressed: uiController.processInput("×") }

            // Row 3
            CalcKey { label: "4";  keyType: "numeric"; onPressed: uiController.processInput("4") }
            CalcKey { label: "5";  keyType: "numeric"; onPressed: uiController.processInput("5") }
            CalcKey { label: "6";  keyType: "numeric"; onPressed: uiController.processInput("6") }
            CalcKey { label: "x²"; keyType: "function"; onPressed: { /* TODO: x² insertion */ } }
            CalcKey { label: "−";  keyType: "operator"; onPressed: uiController.processInput("−") }

            // Row 4
            CalcKey { label: "1";  keyType: "numeric"; onPressed: uiController.processInput("1") }
            CalcKey { label: "2";  keyType: "numeric"; onPressed: uiController.processInput("2") }
            CalcKey { label: "3";  keyType: "numeric"; onPressed: uiController.processInput("3") }
            CalcKey { label: "Ans"; keyType: "function"; onPressed: { /* TODO: Ans recall */ } }
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
}

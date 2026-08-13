import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Modal popup for TBLSET — TBLstart and ΔTbl. Counterpart to
// WindowPopup but for the table-view sequencing. 2ND + WINDOW on a
// real TI-83 opens this.
//
// Behavioural contract: each TextField commits on `editingFinished`
// with a `Number.isFinite` guard (mirrors WindowPopup's BUG-001 fix
// — we don't want NaN sneaking into tblStart / tblStep). Step is
// additionally guarded against zero, since a zero step would produce
// identical rows forever.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 280
    height: 220
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

        Text {
            Layout.fillWidth: true
            text: "TBLSET"
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

        // Inline component for one label + numeric field row.
        component TblField: RowLayout {
            id: fieldRow
            property string label: ""
            property real value: 0
            property real defaultIfZero: 0  // if non-zero, reject 0 input
            signal commit(real v)

            Layout.fillWidth: true
            spacing: 8

            Text {
                text: fieldRow.label
                color: Style.textSecondary
                font.family: Style.monoFamily
                font.pixelSize: Style.keyLabelPixelSize
                Layout.preferredWidth: 80
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                radius: 4
                color: Style.bgDisplay
                border.color: tf.activeFocus ? Style.textExpr : Style.keyBorderNeutral
                border.width: 1
                TextField {
                    id: tf
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    text: fieldRow.value
                    color: Style.textDisplay
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                    background: Item {}
                    selectByMouse: true
                    onEditingFinished: {
                        const v = parseFloat(tf.text)
                        if (!Number.isFinite(v) ||
                            (fieldRow.defaultIfZero !== 0 && v === 0)) {
                            // Revert on bad input — same approach as
                            // WindowPopup (BUG-001).
                            tf.text = fieldRow.value
                            return
                        }
                        fieldRow.commit(v)
                    }
                }
            }
        }

        TblField {
            label: "TblStart"
            value: uiController.tblStart
            onCommit: (v) => uiController.tblStart = v
        }
        TblField {
            label: "ΔTbl"
            value: uiController.tblStep
            defaultIfZero: 1  // any non-zero marker; just enables the guard
            onCommit: (v) => uiController.tblStep = v
        }

        Item { Layout.fillHeight: true }

        CalcKey {
            label: "DONE"
            keyType: "enter"
            onPressed: root.close()
        }
    }
}

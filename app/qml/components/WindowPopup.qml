import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// Modal popup for editing the graph viewport bounds.
//
// Visual contract: a centred dialog with the four standard TI-83 viewport
// fields (Xmin, Xmax, Ymin, Ymax) and three action buttons (ZSTD, ZFIT,
// DONE). Styled to match the rest of the new UI via the Style singleton.
//
// Behavioural contract: bound to `uiController.xMin/xMax/yMin/yMax` and
// `uiController.resetViewport()` / `zoomFit()`. Each TextField guards
// against NaN / non-finite input by reverting on bad parses (this is the
// same fix the legacy popup got for BUG-001).
//
// Owner is responsible for opening this popup; close is handled
// internally via the DONE button or the standard Escape /
// click-outside policies.
Popup {
    id: root

    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    width: 280
    // Taller in Seq mode to fit the extra nMax + u/v/w(1) fields.
    height: uiController.graphMode === 3 ? 520 : 360
    padding: 14

    // Viewport fields, plus the sequence window fields when in Seq mode.
    readonly property var fields: {
        var base = [
            { label: "Xmin:", prop: "xMin" },
            { label: "Xmax:", prop: "xMax" },
            { label: "Ymin:", prop: "yMin" },
            { label: "Ymax:", prop: "yMax" }
        ]
        if (uiController.graphMode === 3) {
            base.push({ label: "nMax:", prop: "seqNMax" })
            base.push({ label: "u(1):", prop: "seqInitU" })
            base.push({ label: "v(1):", prop: "seqInitV" })
            base.push({ label: "w(1):", prop: "seqInitW" })
        }
        return base
    }

    // Centre on the parent (the application overlay).
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
            text: "WINDOW"
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

        // ── Viewport fields (+ sequence fields in Seq mode) ──
        Repeater {
            model: root.fields
            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    text: modelData.label
                    color: Style.textPrimary
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                    Layout.preferredWidth: 50
                }

                TextField {
                    id: field
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    text: uiController[modelData.prop].toFixed(2)
                    color: Style.textDisplay
                    selectByMouse: true
                    font.family: Style.monoFamily
                    font.pixelSize: Style.keyLabelPixelSize
                    horizontalAlignment: TextInput.AlignRight
                    background: Rectangle {
                        color: Style.bgDisplay
                        radius: 4
                        border.color: field.activeFocus ? Style.textExpr : Style.keyBorderNeutral
                        border.width: 1
                    }
                    onEditingFinished: {
                        // Same NaN guard as the legacy popup post-BUG-001 fix.
                        var v = parseFloat(text)
                        if (Number.isFinite(v)) {
                            uiController[modelData.prop] = v
                        } else {
                            text = uiController[modelData.prop].toFixed(2)
                        }
                    }
                }
            }
        }

        // ── Action buttons: ZSTD + ZFIT side by side ──
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 6
            spacing: 6

            CalcKey {
                label: "ZSTD"
                keyType: "function"
                onPressed: uiController.resetViewport()
            }

            CalcKey {
                label: "ZFIT"
                keyType: "function"
                onPressed: uiController.zoomFit()
            }
        }

        // ── DONE — dismiss the popup ──
        CalcKey {
            label: "DONE"
            keyType: "enter"
            onPressed: root.close()
        }

        Item { Layout.fillHeight: true }
    }
}

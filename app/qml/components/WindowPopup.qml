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
    // Taller when a mode adds extra window fields (Par/Pol T-window, Seq).
    height: (uiController.graphMode === 1 || uiController.graphMode === 2
             || uiController.graphMode === 3) ? 640 : 480
    padding: 14

    // Viewport fields, plus mode-specific window fields (Par/Pol T-window
    // or the Seq settings).
    readonly property var fields: {
        var base = []
        // Parametric (1) / polar (2) parameter window. θ labels in polar.
        if (uiController.graphMode === 1 || uiController.graphMode === 2) {
            var p = uiController.graphMode === 2 ? "θ" : "T"
            base.push({ label: p + "min:",  prop: "paramTMin" })
            base.push({ label: p + "max:",  prop: "paramTMax" })
            base.push({ label: p + "step:", prop: "paramTStep" })
        }
        base.push({ label: "Xmin:", prop: "xMin" })
        base.push({ label: "Xmax:", prop: "xMax" })
        base.push({ label: "Xscl:", prop: "xScl" })
        base.push({ label: "Ymin:", prop: "yMin" })
        base.push({ label: "Ymax:", prop: "yMax" })
        base.push({ label: "Yscl:", prop: "yScl" })
        base.push({ label: "Xres:", prop: "xres", integer: true, min: 1, max: 8 })
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
                    text: modelData.integer
                           ? uiController[modelData.prop].toString()
                           : uiController[modelData.prop].toFixed(2)
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
                            if (modelData.integer) {
                                // Integer field (Xres): round and clamp to
                                // the descriptor's [min, max] range.
                                v = Math.round(v)
                                if (modelData.min !== undefined) v = Math.max(modelData.min, v)
                                if (modelData.max !== undefined) v = Math.min(modelData.max, v)
                                uiController[modelData.prop] = v
                                text = v.toString()
                            } else {
                                uiController[modelData.prop] = v
                            }
                        } else {
                            text = modelData.integer
                                   ? uiController[modelData.prop].toString()
                                   : uiController[modelData.prop].toFixed(2)
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

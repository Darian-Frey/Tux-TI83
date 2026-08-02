import QtQuick
import QtQuick.Layouts
import ".."

// Reusable calculator key.
//
// Visual contract: a Rectangle sized by its parent layout (GridLayout cell),
// with a centred text label, semantic colour scheme driven by `keyType`,
// hover lightening, and a brief press-scale animation.
//
// Behavioural contract: emits `pressed()` on click. Owners decide what to do
// with it (typically forwarding to uiController.processInput).
Rectangle {
    id: root

    // ── Public API ────────────────────────────────────────
    property string label: ""
    // keyType ∈ "numeric" | "operator" | "function" | "control" | "enter" | "second" | "alpha"
    property string keyType: "function"
    // `armed` lights up a key when it's the active modifier (e.g. 2ND
    // after being pressed once, ALPHA similarly). The owner flips this
    // based on the root modifier state; CalcKey just reflects it.
    property bool armed: false
    // Corner sub-labels showing what the key does under the 2ND or
    // ALPHA modifier. Rendered as tiny amber/green text in the top
    // corners, mirroring TI-83 keytop markings. Leave empty to hide.
    property string secondLabel: ""
    property string alphaLabel:  ""
    signal pressed()

    // ── Layout defaults ───────────────────────────────────
    Layout.fillWidth: true
    Layout.preferredHeight: Style.keyHeight

    // ── Visual styling ────────────────────────────────────
    radius: Style.keyRadius
    border.width: root.armed ? Style.armedBorderWidth : Style.keyBorderWidth
    scale: pressArea.pressed ? Style.keyPressScale : 1.0
    Behavior on scale { NumberAnimation { duration: Style.keyPressDurationMs } }

    color: {
        const base =
            keyType === "numeric"  ? Style.numericBg :
            keyType === "operator" ? Style.opBg      :
            keyType === "enter"    ? Style.enterBg   :
            keyType === "second"   ? Style.secondBg  :
            keyType === "alpha"    ? Style.alphaBg   :
            keyType === "control"  ? Style.bgSurface :
                                     Style.funcBg
        const hovered = pressArea.containsMouse
            ? Qt.lighter(base, 1.0 + Style.keyHoverLighten)
            : base
        // When armed, lighten further to visually lock-in the state.
        return root.armed ? Qt.lighter(hovered, 1.0 + Style.keyArmedLighten) : hovered
    }

    border.color: {
        if (root.armed) return Style.armedBorder
        switch (keyType) {
        case "operator": return Style.opBorder
        case "enter":    return Style.enterBorder
        case "second":   return Style.secondBorder
        case "alpha":    return Style.alphaBorder
        default:         return Style.keyBorderNeutral
        }
    }

    Text {
        anchors.centerIn: parent
        text: root.label
        color: (root.keyType === "control" && root.label === "CLEAR")
               ? Style.textError
               : Style.textPrimary
        font.family: Style.monoFamily
        font.pixelSize: root.keyType === "function"
                        ? Style.funcKeyLabelPixelSize
                        : Style.keyLabelPixelSize
        font.weight: Style.keyLabelFontWeight
    }

    // 2ND sub-label — top-left, amber. Visible only when populated.
    Text {
        visible: root.secondLabel.length > 0
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 2
        anchors.leftMargin: 4
        text: root.secondLabel
        color: Style.armedBadge2nd
        font.family: Style.monoFamily
        font.pixelSize: Style.cornerLabelPixelSize
    }

    // ALPHA sub-label — top-right, green. Visible only when populated.
    Text {
        visible: root.alphaLabel.length > 0
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 2
        anchors.rightMargin: 4
        text: root.alphaLabel
        color: Style.armedBadgeAlpha
        font.family: Style.monoFamily
        font.pixelSize: Style.cornerLabelPixelSize
    }

    MouseArea {
        id: pressArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.pressed()
    }
}

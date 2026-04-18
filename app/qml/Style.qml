pragma Singleton
import QtQuick

QtObject {
    // ── Background layers ─────────────────────────────────
    readonly property color bgShell:   "#1e2030"  // calculator body
    readonly property color bgDisplay: "#0b1120"  // LCD panel (permanent, not themed)
    readonly property color bgSurface: "#252840"  // key surface / cards
    readonly property color bgSection: "#1a1c2e"  // section dividers

    // ── Text ──────────────────────────────────────────────
    readonly property color textPrimary:   "#e2e8f0"
    readonly property color textSecondary: "#a0aec0"
    readonly property color textMuted:     "#4a5568"
    readonly property color textDisplay:   "#e2e8f0"
    readonly property color textExpr:      "#3b82f6"
    readonly property color textResult:    "#4ade80"
    readonly property color textError:     "#f87171"

    // ── Semantic key colours ──────────────────────────────
    readonly property color opBorder:     "#1e3a5f"
    readonly property color opBg:         "#163354"
    readonly property color enterBorder:  "#14532d"
    readonly property color enterBg:      "#0f3d20"
    readonly property color secondBorder: "#78350f"
    readonly property color secondBg:     "#4c1d02"
    readonly property color funcBg:       bgSurface              // function keys
    readonly property color numericBg:    "#111827"              // numeric keys (display darkened)
    readonly property color keyBorderNeutral: "#2a2d44"

    // ── Key anatomy ───────────────────────────────────────
    readonly property int   keyHeight:          36
    readonly property int   keyRadius:          6
    readonly property real  keyBorderWidth:     0.5
    readonly property int   keyPressDurationMs: 60
    readonly property real  keyPressScale:      0.91
    readonly property real  keyHoverLighten:    0.08

    // ── Armed modifier (2ND / ALPHA) ──────────────────────
    // Used by CalcKey when its `armed` property is true. Border is a
    // warm amber that reads well against both the amber 2ND body and
    // the neutral ALPHA body, so a single colour works for both.
    readonly property real  armedBorderWidth:   1.5
    readonly property real  keyArmedLighten:    0.12
    readonly property color armedBorder:        "#f59e0b"
    readonly property color armedBadge2nd:      "#f59e0b"
    readonly property color armedBadgeAlpha:    "#4ade80"

    // ── Typography ────────────────────────────────────────
    readonly property string monoFamily: "Courier New"

    readonly property int displayPixelSize:      30
    readonly property int exprPixelSize:         11
    readonly property int sectionLabelPixelSize: 8
    readonly property int keyLabelPixelSize:     13
    readonly property int funcKeyLabelPixelSize: 12
    readonly property int headerBrandPixelSize:  10
    // Corner sub-labels showing each key's 2ND and ALPHA functions.
    // Deliberately small so they frame the primary label instead of
    // competing with it — mirrors the tiny printed markings on a real
    // TI-83 keytop.
    readonly property int cornerLabelPixelSize:  9

    readonly property real sectionLabelLetterSpacing: 0.12
    readonly property real headerBrandLetterSpacing:  0.14

    readonly property int keyLabelFontWeight: Font.Medium  // 500

    // ── Cursor ────────────────────────────────────────────
    readonly property int cursorWidth:   2
    readonly property int cursorHeight:  28
    readonly property int cursorBlinkMs: 500

    // ── Brand ─────────────────────────────────────────────
    readonly property string brandText: "TUX·TI83"
}

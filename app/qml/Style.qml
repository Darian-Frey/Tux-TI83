pragma Singleton
import QtQuick

QtObject {
    id: style

    // Theme index: 0 = Dark (default), 1 = Light, 2 = Amber (orange-on-black
    // terminal). Bound to uiController.theme in Main.qml. Every themed colour
    // reads from the active palette below, so switching restyles the whole
    // UI. The LCD *panel* background stays dark in all themes (an authentic
    // screen); its text is themed. Colours that also label the LCD (muted /
    // error) use tones that read on both the dark LCD and the body.
    property int theme: 0

    readonly property var _palettes: [
        ({ // 0 — Dark (Nord-ish, original)
            bgShell: "#1e2030", bgSurface: "#252840", bgSection: "#1a1c2e",
            textPrimary: "#e2e8f0", textSecondary: "#a0aec0", textMuted: "#4a5568",
            textError: "#f87171",
            textDisplay: "#e2e8f0", textExpr: "#3b82f6", textResult: "#4ade80",
            opBorder: "#1e3a5f", opBg: "#163354",
            enterBorder: "#14532d", enterBg: "#0f3d20",
            secondBorder: "#78350f", secondBg: "#4c1d02",
            alphaBorder: "#166534", alphaBg: "#052e16",
            numericBg: "#111827", keyBorderNeutral: "#2a2d44",
            badge2nd: "#f59e0b", badgeAlpha: "#4ade80",
            gridLine: "#1a1c2e", lcdOverlay: "#1e2030"
        }),
        ({ // 1 — Light (dark screen on a light body)
            bgShell: "#dfe3ec", bgSurface: "#ffffff", bgSection: "#ccd2df",
            textPrimary: "#1c2333", textSecondary: "#4b5568", textMuted: "#6b7280",
            textError: "#e53935",
            textDisplay: "#e2e8f0", textExpr: "#3b82f6", textResult: "#4ade80",
            opBorder: "#60a5fa", opBg: "#dbeafe",
            enterBorder: "#4ade80", enterBg: "#dcfce7",
            secondBorder: "#f59e0b", secondBg: "#fef3c7",
            alphaBorder: "#34d399", alphaBg: "#d1fae5",
            numericBg: "#edeff5", keyBorderNeutral: "#cad0dc",
            badge2nd: "#b45309", badgeAlpha: "#15803d",
            // Grid lines and trace-overlay strips sit on the (always-dark)
            // LCD, so these stay dark even in the light theme — otherwise
            // they'd glare / wash out the values.
            gridLine: "#232b3d", lcdOverlay: "#1c2536"
        }),
        ({ // 2 — Amber (orange-on-black terminal). Keys are outlined boxes:
           // near-black bodies with orange borders + orange text.
            bgShell: "#0d0a07", bgSurface: "#17110a", bgSection: "#2a1c0d",
            textPrimary: "#ffa04d", textSecondary: "#cc7a33", textMuted: "#b3722a",
            textError: "#ff5252",
            textDisplay: "#ff9a4d", textExpr: "#b3722a", textResult: "#ffb366",
            opBorder: "#ff7a1a", opBg: "#241503",
            enterBorder: "#ff9a4d", enterBg: "#4d2900",
            secondBorder: "#ff7a1a", secondBg: "#2e1a05",
            alphaBorder: "#ffc266", alphaBg: "#2a1d05",
            numericBg: "#120c06", keyBorderNeutral: "#7a4a1a",
            badge2nd: "#ff7a1a", badgeAlpha: "#ffc266",
            gridLine: "#33240f", lcdOverlay: "#1c1206"
        })
    ]
    readonly property var _p: _palettes[(theme >= 0 && theme < _palettes.length) ? theme : 0]

    // ── Background layers ─────────────────────────────────
    readonly property color bgShell:   _p.bgShell
    readonly property color bgDisplay: "#0b1120"   // LCD panel (fixed, always dark)
    readonly property color bgSurface: _p.bgSurface
    readonly property color bgSection: _p.bgSection
    // Graph grid-line colour. Drawn on the always-dark LCD, so it stays a
    // subtle dark tone in every theme (unlike bgSection, which is a body
    // colour that goes light in the light theme).
    readonly property color gridLine:  _p.gridLine
    // Background for translucent strips drawn on the LCD (trace readouts,
    // histogram bar separators). Dark in every theme so overlaid text and
    // values stay legible on the dark screen.
    readonly property color lcdOverlay: _p.lcdOverlay

    // ── Text ──────────────────────────────────────────────
    readonly property color textPrimary:   _p.textPrimary
    readonly property color textSecondary: _p.textSecondary
    readonly property color textMuted:     _p.textMuted
    readonly property color textError:     _p.textError
    readonly property color textDisplay:   _p.textDisplay   // LCD readout
    readonly property color textExpr:      _p.textExpr      // LCD expression line
    readonly property color textResult:    _p.textResult    // LCD result

    // Graph curve palette — 10 distinct colours for Y1..Y0 (Y-editor).
    // Kept across themes so curves stay distinguishable on the dark LCD.
    readonly property var graphColors: [
        "#3b82f6", "#f87171", "#4ade80", "#fbbf24", "#a78bfa",
        "#22d3ee", "#f472b6", "#fb923c", "#2dd4bf", "#a3e635"
    ]

    // ── Semantic key colours ──────────────────────────────
    readonly property color opBorder:     _p.opBorder
    readonly property color opBg:         _p.opBg
    readonly property color enterBorder:  _p.enterBorder
    readonly property color enterBg:      _p.enterBg
    readonly property color secondBorder: _p.secondBorder
    readonly property color secondBg:     _p.secondBg
    readonly property color alphaBorder:  _p.alphaBorder
    readonly property color alphaBg:      _p.alphaBg
    readonly property color funcBg:       bgSurface            // function keys
    readonly property color numericBg:    _p.numericBg
    readonly property color keyBorderNeutral: _p.keyBorderNeutral

    // ── Key anatomy ───────────────────────────────────────
    readonly property int   keyHeight:          36
    readonly property int   keyRadius:          6
    readonly property real  keyBorderWidth:     0.5
    readonly property int   keyPressDurationMs: 60
    readonly property real  keyPressScale:      0.91
    readonly property real  keyHoverLighten:    0.08

    // ── Armed modifier (2ND / ALPHA) ──────────────────────
    readonly property real  armedBorderWidth:   1.5
    readonly property real  keyArmedLighten:    0.12
    readonly property color armedBorder:        "#f59e0b"
    // Corner sub-labels (2ND / ALPHA), themed for contrast per palette.
    readonly property color armedBadge2nd:      _p.badge2nd
    readonly property color armedBadgeAlpha:    _p.badgeAlpha

    // ── Typography ────────────────────────────────────────
    readonly property string monoFamily: "Courier New"

    readonly property int displayPixelSize:      30
    readonly property int exprPixelSize:         11
    readonly property int sectionLabelPixelSize: 8
    // Popup header titles (FORMAT, MODE, Y= EDITOR, …). Larger than the tiny
    // keypad section-divider label so titles read clearly in every theme.
    readonly property int popupTitlePixelSize:   12
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

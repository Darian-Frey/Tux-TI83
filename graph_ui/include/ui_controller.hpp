#pragma once
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <deque>
#include <vector>
#include "capsules/capsule_math.hpp"

namespace tux_ti83 {

class UIController : public QObject {
    Q_OBJECT

public:
    enum DisplayState {
        Inputting = 0,
        Evaluated = 1,
        Error     = 2
    };
    Q_ENUM(DisplayState)

private:
    Q_PROPERTY(double xMin MEMBER m_xMin NOTIFY viewportChanged)
    Q_PROPERTY(double xMax MEMBER m_xMax NOTIFY viewportChanged)
    Q_PROPERTY(double yMin MEMBER m_yMin NOTIFY viewportChanged)
    Q_PROPERTY(double yMax MEMBER m_yMax NOTIFY viewportChanged)
    Q_PROPERTY(QString currentDisplay READ currentDisplay NOTIFY displayChanged)
    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)
    Q_PROPERTY(int activeFunctionIndex READ activeFunctionIndex NOTIFY activeFunctionIndexChanged)
    Q_PROPERTY(bool isGraphMode MEMBER m_isGraphMode NOTIFY graphModeChanged)
    // TABLE mode — 2ND+GRAPH on real TI-83. Replaces the keypad page
    // in the main StackLayout with a scrollable table of Y(X) values.
    // Mutually exclusive with isGraphMode.
    Q_PROPERTY(bool isTableMode MEMBER m_isTableMode NOTIFY tableModeChanged)
    // TblStart / ΔTbl from TBLSET. Defaults match TI-83: start 0, step 1.
    Q_PROPERTY(double tblStart MEMBER m_tblStart NOTIFY tableSettingsChanged)
    Q_PROPERTY(double tblStep  MEMBER m_tblStep  NOTIFY tableSettingsChanged)
    Q_PROPERTY(DisplayState displayState READ displayState NOTIFY displayStateChanged)
    Q_PROPERTY(QString displayExpression READ displayExpression NOTIFY displayStateChanged)
    // Angle mode: 0 = Radian, 1 = Degree. Mirrors
    // MathStateMachine::angleMode (process-global). Header indicator
    // and MODEPopup both bind to this.
    Q_PROPERTY(int angleMode READ angleMode WRITE setAngleMode NOTIFY angleModeChanged)
    // Number-format settings, both backed by the engine statics that
    // `formatScalar` consults. `notation`: 0 = Normal, 1 = Sci, 2 = Eng.
    // `fixDecimals`: -1 = Float, 0..9 = Fix N.
    Q_PROPERTY(int notation READ notation WRITE setNotation NOTIFY notationChanged)
    Q_PROPERTY(int fixDecimals READ fixDecimals WRITE setFixDecimals NOTIFY fixDecimalsChanged)
    // Cursor position within the current expression, expressed as a
    // character offset into the rendered display string. The backing
    // state is token-level (m_cursorPos, 0..buf.size()), but the
    // display TextInput needs char positions — this getter walks the
    // buffer up to the cursor and sums the displayStr lengths.
    Q_PROPERTY(int cursorOffset READ cursorOffset NOTIFY cursorMoved)
    // Insert vs. overwrite mode for mid-expression edits. Default true
    // (insert — splice the new token in, push everything right). When
    // false, typing a new token replaces the one currently at the
    // cursor (or appends if cursor is at the end). Toggled by
    // 2ND + DEL on a real TI-83.
    Q_PROPERTY(bool insertMode READ insertMode NOTIFY insertModeChanged)
    // Graph-canvas draw style: 0 = Connected (line segments between
    // adjacent samples — default), 1 = Dot (one filled circle per
    // sample, no connecting lines). Real TI-83 has this in the MODE
    // menu; behaviour purely affects rendering, not evaluation.
    Q_PROPERTY(int drawMode READ drawMode WRITE setDrawMode NOTIFY drawModeChanged)
    // TRACE soft-key state. When true, the graph canvas draws a
    // crosshair on the active function's curve at `traceX` and shows
    // an X / Y readout. Left/Right arrow input is routed to
    // traceLeft/traceRight while tracing — the QML dispatch layer
    // chooses between trace movement and expression-cursor movement
    // based on this flag + isGraphMode.
    Q_PROPERTY(bool isTracing READ isTracing NOTIFY traceChanged)
    Q_PROPERTY(double traceX READ traceX NOTIFY traceChanged)
    Q_PROPERTY(double traceY READ traceY NOTIFY traceChanged)

public:
    explicit UIController(QObject* parent = nullptr);

    QString currentDisplay() const;
    QStringList history() const { return m_history; }
    int activeFunctionIndex() const { return m_activeIdx; }
    DisplayState displayState() const { return m_displayState; }
    QString displayExpression() const { return m_displayExpression; }
    int angleMode() const {
        return static_cast<int>(MathStateMachine::angleMode);
    }
    void setAngleMode(int m);
    int notation() const {
        return static_cast<int>(MathStateMachine::notation);
    }
    void setNotation(int n);
    int fixDecimals() const { return MathStateMachine::fixDecimals; }
    void setFixDecimals(int n);
    int cursorOffset() const;
    bool insertMode() const { return m_insertMode; }
    Q_INVOKABLE void toggleInsertMode();
    int drawMode() const { return m_drawMode; }
    void setDrawMode(int m);
    bool isTracing() const { return m_isTracing; }
    double traceX() const { return m_traceX; }
    double traceY() const;
    Q_INVOKABLE void toggleTrace();
    Q_INVOKABLE void traceLeft();
    Q_INVOKABLE void traceRight();

    Q_INVOKABLE void processInput(const QString& input);
    // Format a scalar result for display. Uses enough precision (10
    // significant digits) that results like 10! (= 3,628,800) and larger
    // integers below the 170!-overflow cap display as plain decimal
    // numbers rather than scientific notation, while keeping trailing
    // zeros trimmed. Single source of truth for result formatting —
    // the evaluator, `▶Dec`, and the test suite all route through here.
    Q_INVOKABLE static QString formatScalar(double value);
    // Tokenise a free-form expression string ("2+sin(0.5)") into the
    // sequence of input strings the controller's processInput method
    // accepts. Longest-match against the kTokens table plus a small set
    // of control verbs (▶Frac, ▶Dec). Returns an empty list if any
    // character can't be tokenised. Whitespace is ignored.
    static QStringList tokenize(const QString& expr);
    // Tokenise `expr` and feed each token through processInput in order.
    // Returns true on full success, false if tokenisation failed.
    // Note: does not call ENTER — caller decides whether to evaluate.
    Q_INVOKABLE bool processExpression(const QString& expr);
    // CATALOG list: alphabetically-sorted display strings of every
    // insertable token in `kTokens`. Used by `CatalogPopup` (2ND + 0)
    // to populate its scrollable list. Each entry is the display
    // string itself — clicking feeds the same string back through
    // `processExpression` so insertion uses the unified token table.
    Q_INVOKABLE QStringList catalogEntries() const;
    Q_INVOKABLE void setActiveFunction(int index) { m_activeIdx = index; emit activeFunctionIndexChanged(); }
    Q_INVOKABLE void toggleGraphMode() {
        m_isGraphMode = !m_isGraphMode;
        // Mutually exclusive with TABLE mode.
        if (m_isGraphMode && m_isTableMode) {
            m_isTableMode = false;
            emit tableModeChanged();
        }
        emit graphModeChanged();
    }
    Q_INVOKABLE void toggleTableMode() {
        m_isTableMode = !m_isTableMode;
        if (m_isTableMode && m_isGraphMode) {
            m_isGraphMode = false;
            emit graphModeChanged();
        }
        emit tableModeChanged();
    }
    // Returns `count` table rows starting at X = xStart, stepping by
    // tblStep. Each row is a QVariantMap with keys "x" plus optional
    // "y1" / "y2" / "y3" (omitted when the function buffer is empty
    // or evaluates to a non-scalar / error). Called by TableView.qml
    // to populate the visible window — separate from getMultiGraphPoints
    // because the table needs explicit X stepping (not viewport-derived).
    Q_INVOKABLE QVariantList getTableRows(int count, double xStart);
    Q_INVOKABLE void resetViewport() { m_xMin = -10; m_xMax = 10; m_yMin = -10; m_yMax = 10; emit viewportChanged(); }
    // Last-entry recall (2ND+ENTER on a real TI-83). Each successful or
    // failed ENTER with a non-empty buffer pushes the token stream into
    // a 10-deep ring buffer; successive calls walk back through it.
    // Any non-recall processInput resets the cycle.
    Q_INVOKABLE void recallLastEntry();
    // Persist + restore everything that should outlive the process —
    // scalar variables A..Z, matrices [A]/[B]/[C], function buffers
    // Y1..Y3, viewport, MODE settings (angle/notation/decimal/draw),
    // and the active function slot. Session-scoped state (history,
    // entry-recall ring, insertMode, tracing) deliberately omitted.
    // State file lives at $XDG_STATE_HOME/tux-ti83/state.json
    // (~/.local/state/tux-ti83/state.json on most Linux desktops).
    Q_INVOKABLE void saveState() const;
    Q_INVOKABLE void loadState();
    // Token-level cursor movement. Left/Right step by one token
    // (matches TI-83 behaviour: `sin(` is one visual step, not four);
    // Home/End jump to the extremes. All four are no-ops outside
    // Inputting state.
    Q_INVOKABLE void moveCursorLeft();
    Q_INVOKABLE void moveCursorRight();
    Q_INVOKABLE void moveCursorHome();
    Q_INVOKABLE void moveCursorEnd();
    Q_INVOKABLE void zoomFit();
    // Convenience zooms for the ZOOM popup. Both keep the viewport
    // centre fixed and scale x and y by the same factor — `zoomIn`
    // halves each axis (curve appears bigger), `zoomOut` doubles it.
    // The existing free-form `zoom(...)` is more flexible but takes
    // pixel coordinates; these wrap it for the menu's preset use.
    Q_INVOKABLE void zoomIn();
    Q_INVOKABLE void zoomOut();
    Q_INVOKABLE void updateMatrix(const QString& name, int rows, int cols, const QVariantList& values);
    Q_INVOKABLE QVariantList getMultiGraphPoints(int resolution);
    Q_INVOKABLE void pan(double dx, double dy, double vw, double vh);
    Q_INVOKABLE void zoom(double f, double mx, double my, double vw, double vh);

signals:
    void displayChanged();
    void historyChanged();
    void activeFunctionIndexChanged();
    void viewportChanged();
    void graphModeChanged();
    void tableModeChanged();
    void tableSettingsChanged();
    void displayStateChanged();
    void angleModeChanged();
    void notationChanged();
    void fixDecimalsChanged();
    void cursorMoved();
    void insertModeChanged();
    void drawModeChanged();
    void traceChanged();

private:
    // processInput dispatches to these. Each handles one concern; the
    // dispatcher itself stays a thin switch over the input string.
    void clearAll();
    void backspace();
    void evaluate();
    void insertToken(const QString& input);
    // Post-hoc display conversions applied to the last result. No-op
    // unless we're in Evaluated state with a scalar result.
    void convertDisplayToFraction();
    void convertDisplayToDecimal();

    std::vector<std::vector<Token>> m_functionBuffers;
    std::vector<QString> m_displayStrings;
    QStringList m_history;
    int m_activeIdx;
    bool m_isGraphMode = false;
    bool m_isTableMode = false;
    double m_tblStart = 0.0;
    double m_tblStep  = 1.0;
    double m_xMin = -10, m_xMax = 10, m_yMin = -10, m_yMax = 10;
    DisplayState m_displayState = Inputting;
    QString m_displayExpression;
    // Entry-recall ring buffer. Newest at back; oldest evicted when
    // size exceeds kEntryHistoryCap. m_recallCycleIdx tracks how far
    // back the current cycle has walked: -1 = not cycling, 0 = last
    // entry, 1 = second-to-last, etc.
    static constexpr int kEntryHistoryCap = 10;
    std::deque<std::vector<Token>> m_entryHistory;
    int m_recallCycleIdx = -1;
    // Token-level cursor position within m_functionBuffers[m_activeIdx].
    // 0 = before first token; buf.size() = past last (append). Reset to
    // 0 on clear and on state transitions back into Inputting; set to
    // end on recall. Edit operations keep it consistent with the
    // buffer — inserting pushes it forward, backspace pulls it back.
    int m_cursorPos = 0;
    // True = splice new tokens in at the cursor (default, TI-83 INS).
    // False = replace the token at the cursor (TI-83 OVR / overwrite).
    bool m_insertMode = true;
    // 0 = Connected (default), 1 = Dot. See drawMode property above.
    int m_drawMode = 0;
    // TRACE state. When `m_isTracing` is true the graph canvas paints
    // a crosshair at (m_traceX, evaluated Y) on the active function.
    // m_traceX is reset to viewport centre on every toggleTrace(true).
    bool m_isTracing = false;
    double m_traceX = 0.0;
};

} // namespace tux_ti83

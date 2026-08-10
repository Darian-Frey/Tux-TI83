#pragma once
#include <QObject>
#include <QMap>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QJsonObject>
#include <deque>
#include <vector>
#include "capsules/capsule_math.hpp"
#include "interpreter.hpp"

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
    // Axis tick spacing (Xscl/Yscl) and graph resolution (Xres). Xscl/Yscl
    // set the interval between grid lines / axis tick marks; Xres is the
    // Func-mode sample stride (1 = finest, higher = coarser/faster).
    Q_PROPERTY(double xScl MEMBER m_xScl NOTIFY viewportChanged)
    Q_PROPERTY(double yScl MEMBER m_yScl NOTIFY viewportChanged)
    Q_PROPERTY(int xres MEMBER m_xres NOTIFY viewportChanged)
    Q_PROPERTY(QString currentDisplay READ currentDisplay NOTIFY displayChanged)
    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)
    // TI-BASIC program run output (P2) — one entry per Disp/echo line.
    Q_PROPERTY(QStringList programOutput READ programOutput NOTIFY programOutputChanged)
    // Resumable run state (P4): the run view shows an input field while
    // waiting for Input/Prompt, or a "continue" button while waiting on Pause.
    Q_PROPERTY(bool programWaitingInput READ programWaitingInput NOTIFY programRunStateChanged)
    Q_PROPERTY(QString programInputPrompt READ programInputPrompt NOTIFY programRunStateChanged)
    Q_PROPERTY(bool programWaitingKey READ programWaitingKey NOTIFY programRunStateChanged)
    Q_PROPERTY(bool programRunning READ programRunning NOTIFY programRunStateChanged)
    Q_PROPERTY(bool programMenuActive READ programMenuActive NOTIFY programRunStateChanged)
    Q_PROPERTY(QString programMenuTitle READ programMenuTitle NOTIFY programRunStateChanged)
    Q_PROPERTY(QStringList programMenuOptions READ programMenuOptions NOTIFY programRunStateChanged)
    Q_PROPERTY(int programErrorLine READ programErrorLine NOTIFY programRunStateChanged)
    Q_PROPERTY(QString programErrorProgram READ programErrorProgram NOTIFY programRunStateChanged)
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
    // Integer display base: 0 = Dec, 1 = Hex, 2 = Oct, 3 = Bin. Mirrors
    // MathStateMachine::numberBase. Non-Dec modes only affect scalars
    // that are exact integers in int64 range — everything else (floats,
    // out-of-range values, NaN, ±inf) falls back through to the
    // existing Notation/Decimal formatter.
    Q_PROPERTY(int numberBase READ numberBase WRITE setNumberBase NOTIFY numberBaseChanged)

    // MODE → Complex: 0 Real, 1 a+bi (Rect), 2 re^θi (Polar). Governs
    // whether √ of a negative etc. produce a complex result or error.
    Q_PROPERTY(int complexMode READ complexMode WRITE setComplexMode NOTIFY complexModeChanged)
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
    // Plot order (MODE → Plot row). 0 = Sequential (each curve drawn fully
    // before the next), 1 = Simul (all curves advance together). Only the
    // GraphCanvas draw animation observes it; the final image is identical.
    Q_PROPERTY(int plotMode MEMBER m_plotMode NOTIFY plotModeChanged)
    // Screen layout (MODE → Screen row). 0 = Full (one view at a time),
    // 1 = Horiz (graph over the keypad), 2 = G-T (graph beside the table).
    // The QML layout observes it; no evaluator effect.
    Q_PROPERTY(int screenMode MEMBER m_screenMode NOTIFY screenModeChanged)
    // UI theme. 0 = Dark (default), 1 = Light, 2 = Amber (orange-on-black).
    // The Style singleton binds to it; the LCD panel stays dark in all
    // themes. No evaluator effect.
    Q_PROPERTY(int theme MEMBER m_theme NOTIFY themeChanged)

    // Graph type (MODE → Graph row). 0 = Func (Cartesian y=f(x)),
    // 2 = Pol (polar r=f(θ)). Values match the row's option order
    // [Func,Par,Pol,Seq]; Par(1)/Seq(3) are unimplemented and rejected
    // by setGraphMode. In Pol mode the function buffers are read as
    // r1/r2/r3 and the sweep variable X stands in for θ.
    Q_PROPERTY(int graphMode READ graphMode WRITE setGraphMode NOTIFY graphModeSettingChanged)

    // Stat plot (Phase C Wave 5b). A single Plot1: on/off, type
    // (0 scatter, 1 xyLine, 2 histogram, 3 box), and the source list
    // names. Rendered on the graph canvas when on + in graph mode.
    Q_PROPERTY(bool statPlotOn MEMBER m_statPlotOn NOTIFY statPlotChanged)
    Q_PROPERTY(int statPlotType MEMBER m_statPlotType NOTIFY statPlotChanged)
    Q_PROPERTY(QString statPlotXList MEMBER m_statPlotXList NOTIFY statPlotChanged)
    Q_PROPERTY(QString statPlotYList MEMBER m_statPlotYList NOTIFY statPlotChanged)

    // Graph FORMAT flags (2ND+ZOOM). Toggle grid lines, axes, the trace
    // coordinate readout, and the tick-number labels. All default on to
    // preserve the pre-Format-menu appearance.
    // ZBox arm flag — while true, a canvas drag defines the zoom box.
    Q_PROPERTY(bool zoomBoxArm MEMBER m_zoomBoxArm NOTIFY zoomBoxArmChanged)
    // Sequence mode (graphMode==3) window/seed settings: nMax and the
    // initial term for u/v/w (used when the recurrence references Ans).
    // Parametric/polar parameter window (shared — the two modes are
    // mutually exclusive). Sweep runs Tmin..Tmax; the point count comes
    // from Tstep. In Pol mode these read as θmin/θmax/θstep.
    Q_PROPERTY(double paramTMin MEMBER m_paramTMin NOTIFY paramWindowChanged)
    Q_PROPERTY(double paramTMax MEMBER m_paramTMax NOTIFY paramWindowChanged)
    Q_PROPERTY(double paramTStep MEMBER m_paramTStep NOTIFY paramWindowChanged)
    Q_PROPERTY(double seqNMax MEMBER m_seqNMax NOTIFY seqSettingsChanged)
    Q_PROPERTY(double seqInitU MEMBER m_seqInitU NOTIFY seqSettingsChanged)
    Q_PROPERTY(double seqInitV MEMBER m_seqInitV NOTIFY seqSettingsChanged)
    Q_PROPERTY(double seqInitW MEMBER m_seqInitW NOTIFY seqSettingsChanged)
    Q_PROPERTY(bool gridOn MEMBER m_gridOn NOTIFY formatChanged)
    Q_PROPERTY(bool axesOn MEMBER m_axesOn NOTIFY formatChanged)
    Q_PROPERTY(bool coordOn MEMBER m_coordOn NOTIFY formatChanged)
    Q_PROPERTY(bool labelOn MEMBER m_labelOn NOTIFY formatChanged)
    // Trace cursor coordinate mode: 0 = RectGC (X/Y), 1 = PolarGC (R/θ).
    Q_PROPERTY(int coordMode MEMBER m_coordMode NOTIFY formatChanged)
    // ExprOn/ExprOff: show the traced function's equation while tracing.
    Q_PROPERTY(bool exprOn MEMBER m_exprOn NOTIFY formatChanged)
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
    QStringList programOutput() const { return m_programOutput; }
    bool programWaitingInput() const { return m_progWaitingInput; }
    QString programInputPrompt() const { return m_progInputPrompt; }
    bool programWaitingKey() const { return m_progWaitingKey; }
    bool programRunning() const { return m_progRunning; }
    bool programMenuActive() const { return m_progMenuActive; }
    QString programMenuTitle() const { return m_progMenuTitle; }
    QStringList programMenuOptions() const { return m_progMenuOptions; }
    // 0-based editor source line of the last runtime error (-1 if none), and
    // the program it occurred in — used by the run view's jump-to-line (P5b).
    int programErrorLine() const { return m_progErrorLine; }
    QString programErrorProgram() const { return m_progErrorProgram; }
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
    int numberBase() const {
        return static_cast<int>(MathStateMachine::numberBase);
    }
    void setNumberBase(int b);
    int complexMode() const {
        return static_cast<int>(MathStateMachine::complexMode);
    }
    void setComplexMode(int m);
    int cursorOffset() const;
    bool insertMode() const { return m_insertMode; }
    Q_INVOKABLE void toggleInsertMode();
    int drawMode() const { return m_drawMode; }
    void setDrawMode(int m);
    int graphMode() const { return m_graphMode; }
    void setGraphMode(int m);
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
    // Format a complex value a+bi (Phase F); real when im==0.
    Q_INVOKABLE QString formatComplex(double re, double im) const;
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

    // Y-editor (Phase D): 10 function slots Y1..Y9, Y0. Each has an
    // enabled (on/off) flag and a line style (0 thin, 1 thick, 2 dotted).
    Q_INVOKABLE int functionCount() const { return kFunctionCount; }
    Q_INVOKABLE bool functionEnabled(int i) const {
        return (i >= 0 && i < static_cast<int>(m_functionEnabled.size()))
                   ? m_functionEnabled[i] : false;
    }
    Q_INVOKABLE void toggleFunctionEnabled(int i) {
        if (i >= 0 && i < static_cast<int>(m_functionEnabled.size())) {
            m_functionEnabled[i] = !m_functionEnabled[i];
            emit functionsChanged();
        }
    }
    Q_INVOKABLE int functionStyle(int i) const {
        return (i >= 0 && i < static_cast<int>(m_functionStyle.size()))
                   ? m_functionStyle[i] : 0;
    }
    Q_INVOKABLE void cycleFunctionStyle(int i) {
        if (i >= 0 && i < static_cast<int>(m_functionStyle.size())) {
            m_functionStyle[i] = (m_functionStyle[i] + 1) % 3;
            emit functionsChanged();
        }
    }
    // The display string (expression preview) for slot i.
    Q_INVOKABLE QString functionExpr(int i) const {
        return (i >= 0 && i < static_cast<int>(m_displayStrings.size()))
                   ? m_displayStrings[i] : QString();
    }
    // The plotted expression for slot i, rebuilt from the token BUFFER
    // rather than the live edit string (which the home screen clobbers
    // with the result after ENTER). Used by the trace ExprOn overlay so
    // it always matches the drawn curve. Defined in the .cpp because it
    // needs the file-local token→display map.
    Q_INVOKABLE QString functionBufferText(int i) const;
    // Slot label for the current graph mode: Y1..Y0 (Func), r1..r0 (Pol),
    // or the parametric pairs X1T/Y1T/X2T/Y2T/... (Par — buffers are
    // read two at a time, X then Y).
    Q_INVOKABLE QString functionLabel(int i) const {
        if (m_graphMode == 1) {  // parametric
            const int pair = i / 2 + 1;
            return QStringLiteral("%1%2T")
                .arg(i % 2 == 0 ? "X" : "Y").arg(pair);
        }
        if (m_graphMode == 3) {  // sequence — only u/v/w (slots 0/1/2)
            if (i == 0) return QStringLiteral("u(n)");
            if (i == 1) return QStringLiteral("v(n)");
            if (i == 2) return QStringLiteral("w(n)");
            return QString();
        }
        return functionPrefix() +
               (i == 9 ? QStringLiteral("0") : QString::number(i + 1));
    }
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
    Q_INVOKABLE void resetViewport() { m_xMin = -10; m_xMax = 10; m_yMin = -10; m_yMax = 10; m_xScl = 1.0; m_yScl = 1.0; emit viewportChanged(); }
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
    // Absolute path of the on-disk state file. Exposed so main() can place
    // a single-instance lock file alongside it (BUG-024).
    static QString stateFilePath();
    // Save/load export (Phase F #34): named snapshots under a `saves/`
    // dir, sharing the auto-state JSON via buildStateJson/applyStateJson.
    Q_INVOKABLE bool exportState(const QString& name);
    Q_INVOKABLE bool importState(const QString& name);
    Q_INVOKABLE QStringList listSaves() const;
    Q_INVOKABLE void deleteSave(const QString& name);
    // Factory reset — clears every piece of session and persisted
    // state (scalars A..Z, matrices [A]/[B]/[C], function buffers
    // Y1/Y2/Y3, history, entry-recall ring, viewport, MODE settings,
    // TBLSET) and removes state.json so the next launch starts truly
    // clean. Lasting state goes away; ephemeral state (current display,
    // arming flags) likewise resets. No confirmation prompt — the
    // trigger is a deliberate click in the MODE popup.
    Q_INVOKABLE void resetAll();

    // ── TI-BASIC programs (P1) ──────────────────────────────
    // Programs are stored as source text (one entry per line) in a
    // ProgramStore; the interpreter re-tokenises each line at run time.
    // Names are normalised to 1–8 chars of A–Z/0–9 (uppercase), matching
    // the TI-83. See docs/TIBASIC.md.
    Q_INVOKABLE QStringList programNames() const;
    Q_INVOKABLE bool programExists(const QString& name) const;
    // Program body as one newline-joined string (for the editor).
    Q_INVOKABLE QString programText(const QString& name) const;
    // Create or replace a program from newline-separated source text.
    // Returns the normalised name actually stored (empty if the name was
    // invalid).
    Q_INVOKABLE QString saveProgram(const QString& name, const QString& text);
    Q_INVOKABLE void deleteProgram(const QString& name);
    // Normalise a candidate program name (uppercase, A–Z/0–9, ≤8 chars).
    Q_INVOKABLE QString normalizeProgramName(const QString& name) const;
    // Run a program: loads it into the interpreter and steps until it
    // finishes, errors, or pauses for interaction (P4). Output goes to the
    // run view.
    Q_INVOKABLE void runProgram(const QString& name);
    // Supply the value a paused Input/Prompt is waiting for, then continue.
    Q_INVOKABLE void provideProgramInput(const QString& value);
    // Continue a program paused on Pause.
    Q_INVOKABLE void resumeProgram();
    // Pick option `index` (0-based) from a Menu( — jumps to its Lbl (P4).
    Q_INVOKABLE void provideProgramMenuChoice(int index);
    // Request that a running program stop now (the STOP key / ON-break).
    // Sets a flag the slice loop checks; the run ends with ERR:BREAK (P5b).
    Q_INVOKABLE void stopProgram();
    // Report a keypress to a running program for getKey (P5b). `keyCode` is
    // the TI-83 getKey code (see the run view's key map); the next getKey
    // poll returns it once, then 0. Delivered mid-run via processEvents.
    Q_INVOKABLE void sendProgramKey(int keyCode);
    // Copy the current program output (all lines, newline-joined) to the
    // system clipboard, so a run's results can be pasted elsewhere.
    Q_INVOKABLE void copyProgramOutput() const;

    // MEM menu (2ND++): targeted clearing + a memory summary. Each
    // clears one category; memInfo() reports how much is currently in
    // use ({vars, matrices, lists, functions, entries}).
    Q_INVOKABLE void clearAllLists();
    Q_INVOKABLE void clearAllMatrices();
    Q_INVOKABLE void clearAllVars();
    Q_INVOKABLE void clearEntries();
    Q_INVOKABLE QVariantMap memInfo() const;
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
    // ZSquare: equalise the y-range to the x-range so 1 unit X looks
    // 1 unit Y on screen. Centred on the current viewport centre.
    // Approximate — assumes a square canvas; close enough for most
    // window aspect ratios.
    Q_INVOKABLE void zoomSquare();
    // ZTrig: a window suited for trig functions — x ∈ [-2.3π, 2.3π],
    // y ∈ [-4, 4]. Matches TI-83 conventions.
    Q_INVOKABLE void zoomTrig();
    // ZDecimal: TI-83's "nice decimal coords" window —
    // [-4.7, 4.7] × [-3.1, 3.1].
    Q_INVOKABLE void zoomDecimal();
    // ZInteger: snap the current viewport edges to the nearest
    // integers. Useful when stepping through integer X values.
    Q_INVOKABLE void zoomInteger();

    // --- Zoom menu completion (Phase D) ---
    // Snapshot the current viewport as the "previous" one. Called by the
    // ZOOM popup before each menu zoom so ZoomPrevious can undo it (pan
    // and scroll-zoom deliberately don't snapshot).
    Q_INVOKABLE void savePrevViewport() {
        m_prevXMin = m_xMin; m_prevXMax = m_xMax;
        m_prevYMin = m_yMin; m_prevYMax = m_yMax;
    }
    // ZoomPrevious — swap current ↔ previous (so it's reversible).
    Q_INVOKABLE void zoomPrevious() {
        std::swap(m_xMin, m_prevXMin); std::swap(m_xMax, m_prevXMax);
        std::swap(m_yMin, m_prevYMin); std::swap(m_yMax, m_prevYMax);
        emit viewportChanged();
    }
    // ZoomMemory: store the current window / recall the stored one.
    Q_INVOKABLE void zoomStore() {
        m_savedXMin = m_xMin; m_savedXMax = m_xMax;
        m_savedYMin = m_yMin; m_savedYMax = m_yMax;
    }
    Q_INVOKABLE void zoomRecall() {
        savePrevViewport();
        m_xMin = m_savedXMin; m_xMax = m_savedXMax;
        m_yMin = m_savedYMin; m_yMax = m_savedYMax;
        emit viewportChanged();
    }
    // ZBox: arm box-select; the next click-drag on the canvas defines the
    // zoom rectangle, which is committed via zoomBox() (data coords).
    Q_INVOKABLE void armZoomBox() {
        m_isGraphMode = true;
        m_zoomBoxArm = true;
        emit graphModeChanged();
        emit zoomBoxArmChanged();
    }
    Q_INVOKABLE void zoomBox(double x1, double y1, double x2, double y2);
    // ZoomStat: fit the viewport to the stat-plot lists.
    Q_INVOKABLE void zoomStat();

    // --- DRAW menu (Phase D) ---
    // Persistent graph overlays. Each is stored as a QVariantMap
    // {type, a, b, c, d, text} and rendered by the canvas over the
    // curves. getDrawObjects() feeds the canvas; the draw* methods add
    // one; clrDraw() removes all.
    Q_INVOKABLE QVariantList getDrawObjects() const { return m_drawObjects; }
    // Reactive count of overlays, so QML can show/hide a clear affordance.
    Q_PROPERTY(int drawObjectCount READ drawObjectCount NOTIFY drawObjectsChanged)
    int drawObjectCount() const { return static_cast<int>(m_drawObjects.size()); }
    Q_INVOKABLE void drawLine(double x1, double y1, double x2, double y2);
    Q_INVOKABLE void drawCircle(double x, double y, double r);
    Q_INVOKABLE void drawHorizontal(double y);
    Q_INVOKABLE void drawVertical(double x);
    Q_INVOKABLE void drawPoint(double x, double y);
    Q_INVOKABLE void drawText(double x, double y, const QString& text);
    Q_INVOKABLE void clrDraw();
    // Delete a single overlay by index (from the DRAW popup's list).
    Q_INVOKABLE void deleteDrawObject(int index);
    Q_INVOKABLE void updateMatrix(const QString& name, int rows, int cols, const QVariantList& values);
    // IMP-007: read a stored matrix back for the EDIT tab. Returns
    // {"rows": int, "cols": int, "data": [doubles]}. Undefined/unknown
    // names return rows=cols=0 with an empty data list.
    Q_INVOKABLE QVariantMap getMatrix(const QString& name) const;
    // Phase C Wave 2: list editor bridge. getList returns the stored
    // values for "L1".."L6" (empty list if unset/unknown); updateList
    // writes them into the registry.
    Q_INVOKABLE QVariantList getList(const QString& name) const;
    Q_INVOKABLE void updateList(const QString& name, const QVariantList& values);
    // Phase C Wave 4a: 1-Var Stats over a list. Returns a map with keys
    // n, mean, sumX, sumX2, Sx (sample sd), sigmaX (pop sd), minX, Q1,
    // median, Q3, maxX — or {"error": "UNDEFINED"} for an unset/empty
    // list. Quartiles use the TI-83 median-of-halves rule.
    Q_INVOKABLE QVariantMap oneVarStats(const QString& name) const;
    // Phase C Wave 4b: 2-Var Stats + linear regression y = ax + b over a
    // pair of equal-length lists. Returns n, meanX/meanY, sumX/sumY/sumXY
    // /sumX2/sumY2, Sx/Sy, and (when the data isn't degenerate) the
    // regression a, b, r, r2. {"error": ...} on undefined/mismatched
    // lists.
    Q_INVOKABLE QVariantMap twoVarStats(const QString& xName,
                                        const QString& yName) const;
    // Phase C Wave 4c: regression models over a list pair. `type` is one
    // of "quad", "cubic", "exp", "ln", "pwr". Returns n, the coefficients
    // (a,b[,c,d] highest-degree first), and r/r2 where defined, or
    // {"error": ...} on undefined/mismatched/out-of-domain data.
    Q_INVOKABLE QVariantMap regression(const QString& type,
                                       const QString& xName,
                                       const QString& yName) const;
    // Render-ready stat-plot data for the current Plot1 config. Returns
    // {on, type, error, ...} where the payload depends on type: `points`
    // (scatter/xyLine), `bins`+`maxCount` (histogram), or `box` (box
    // plot). Empty/undefined/mismatched lists set `error`.
    Q_INVOKABLE QVariantMap getStatPlotData() const;
    Q_INVOKABLE QVariantList getMultiGraphPoints(int resolution);
    Q_INVOKABLE void pan(double dx, double dy, double vw, double vh);
    Q_INVOKABLE void zoom(double f, double mx, double my, double vw, double vh);

signals:
    void displayChanged();
    void historyChanged();
    void programsChanged();
    void programOutputChanged();
    void programRunStateChanged();  // waiting-input / waiting-key changed
    void programRunUpdated();       // run state changed — open/refresh the view
    void showGraphFromProgram();    // a program did DispGraph — show the graph (P6)
    void activeFunctionIndexChanged();
    void functionsChanged();
    void drawObjectsChanged();
    void viewportChanged();
    void graphModeChanged();
    void tableModeChanged();
    void tableSettingsChanged();
    void displayStateChanged();
    void angleModeChanged();
    void notationChanged();
    void fixDecimalsChanged();
    void numberBaseChanged();
    void complexModeChanged();
    void cursorMoved();
    void insertModeChanged();
    void drawModeChanged();
    void plotModeChanged();
    void screenModeChanged();
    void themeChanged();
    void graphModeSettingChanged();
    void statPlotChanged();
    void formatChanged();
    void zoomBoxArmChanged();
    void seqSettingsChanged();
    void paramWindowChanged();
    void traceChanged();

private:
    // TI-BASIC program storage (P1). Name → source lines.
    ProgramStore m_programs;
    // Program run output (P2) — filled by runProgram, shown in the run view.
    QStringList m_programOutput;
    // The live interpreter for the current run (P4: held so Input/Prompt/
    // Pause can suspend and resume).
    tux_ti83::Interpreter m_interp;
    bool m_progWaitingInput = false;
    QString m_progInputPrompt;
    bool m_progWaitingKey = false;
    bool m_progRunning = false;        // actively stepping (STOP button live)
    bool m_progBreakRequested = false; // STOP pressed → break at next slice
    bool m_inProgramRun = false;       // re-entrancy guard (processEvents)
    int m_progKey = 0;                 // pending getKey code (0 = none)
    int m_progErrorLine = -1;          // 0-based source line of last error
    QString m_progErrorProgram;        // program that raised the last error
    bool m_progMenuActive = false;     // paused on Menu(
    QString m_progMenuTitle;
    QStringList m_progMenuOptions;
    bool m_progGraphShown = false;     // program did DispGraph → don't reopen run view
    // Step the interpreter until it pauses / finishes, then publish state.
    void stepProgramToPause();
    void publishProgramState();
    // Copy the interpreter's output buffer into m_programOutput (no status /
    // waiting-flag handling). Used for live mid-run refresh and by publish.
    void syncProgramOutput();
    // Evaluate a program source expression: tokenise → MathStateMachine →
    // format. Injected into the Interpreter as its Evaluator (P2). Side
    // effects (Sto) go through the shared registries, like the home screen.
    tux_ti83::EvalResult evalProgramSource(const std::string &src);
    // Tokenise a source string into an engine token buffer (Sub→Neg aware);
    // shared by evalProgramSource and setFunctionFromSource (P6).
    std::vector<Token> sourceToTokens(const QString &src);
    // ── List/matrix element access in programs (P7-A1) ──
    // Substitute innermost `Ln(idx)` / `[X](r,c)` element reads in `expr` with
    // their values (the engine would otherwise read them as implicit multiply).
    // Returns false + `err` on a bad index / undefined list-matrix.
    bool resolveElementReads(QString &expr, std::string &err);
    // Evaluate `expr` to a scalar (element reads resolved first).
    bool evalScalarValue(const QString &expr, double &val, std::string &err);
    // If `src` is an element assignment (`<rhs>→Ln(idx)` / `→[X](r,c)`), carry
    // it out and set `out`; returns true if it was one (handled).
    bool tryElementStore(const QString &src, tux_ti83::EvalResult &out);
    // ── User functions (P7-B3) ──
    struct UserFunc {
      QStringList params;
      std::vector<std::string> body;  // statements
    };
    QMap<QString, UserFunc> m_userFuncs;  // name → definition (per run)
    int m_funcDepth = 0;                  // call-recursion guard
    // Substitute innermost `name(args)` calls to user functions with their
    // return values; returns false + `err` on failure.
    bool resolveUserFunctions(QString &expr, std::string &err);
    // Run a user function with `args` (params bound as locals) and return its
    // value; `err` set on failure.
    double callUserFunction(const QString &name, const QVector<double> &args,
                            std::string &err);
    // Wire an interpreter's evaluator / program-loader / graph-sink /
    // define-sink to this controller (shared by runProgram and function calls).
    void configureInterpreter(tux_ti83::Interpreter &it);
    // Store a function expression into a Y= slot from a program (P6); returns
    // false if the slot or expression is invalid.
    bool setFunctionFromSource(int slot, const QString &expr);
    // Format a CalculationResult to its display string (scalar / matrix /
    // list / complex), matching the home-screen formatting.
    QString formatCalcResult(const CalculationResult &r) const;
    // Bind this controller's Y-VARS buffer source onto a MathStateMachine
    // instance (IMP-045). Call on every engine the controller constructs.
    void bindEngine(MathStateMachine &m) const;
    // Serialise all persisted state to JSON / apply a JSON snapshot.
    // Shared by saveState/loadState and exportState/importState.
    QJsonObject buildStateJson() const;
    void applyStateJson(const QJsonObject& root);
    // Home-screen slot label prefix: "r" in polar graph mode, else "Y".
    QString functionPrefix() const {
        return (m_graphMode == 2) ? QStringLiteral("r") : QStringLiteral("Y");
    }
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

    static constexpr int kFunctionCount = 10;  // Y1..Y9, Y0
    std::vector<std::vector<Token>> m_functionBuffers;
    std::vector<QString> m_displayStrings;
    // Per-slot on/off (default on) and line style (0 thin/1 thick/2 dot).
    std::vector<bool> m_functionEnabled;
    std::vector<int> m_functionStyle;
    // DRAW-menu overlays (each a QVariantMap {type, a, b, c, d, text}).
    QVariantList m_drawObjects;
    QStringList m_history;
    int m_activeIdx;
    bool m_isGraphMode = false;
    bool m_isTableMode = false;
    double m_tblStart = 0.0;
    double m_tblStep  = 1.0;
    double m_xMin = -10, m_xMax = 10, m_yMin = -10, m_yMax = 10;
    double m_xScl = 1.0, m_yScl = 1.0;  // axis tick spacing (Xscl/Yscl)
    int m_xres = 1;                      // Func-mode sample stride (1..8)
    // Zoom-menu state: previous viewport (ZoomPrevious), stored viewport
    // (ZoomMemory), and the ZBox arm flag.
    double m_prevXMin = -10, m_prevXMax = 10, m_prevYMin = -10, m_prevYMax = 10;
    double m_savedXMin = -10, m_savedXMax = 10, m_savedYMin = -10, m_savedYMax = 10;
    bool m_zoomBoxArm = false;
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
    // 0 = Sequential (default), 1 = Simul. See plotMode property above.
    int m_plotMode = 0;
    // 0 = Full (default), 1 = Horiz, 2 = G-T. See screenMode property above.
    int m_screenMode = 0;
    // 0 = Dark (default), 1 = Light, 2 = Amber. See theme property above.
    int m_theme = 0;
    // Graph type: 0 = Func, 2 = Pol (option-index encoding). See the
    // graphMode property above.
    int m_graphMode = 0;
    // Stat plot (Plot1) config — see the statPlot* properties above.
    bool m_statPlotOn = false;
    int m_statPlotType = 0;
    QString m_statPlotXList = QStringLiteral("L1");
    QString m_statPlotYList = QStringLiteral("L2");
    // FORMAT flags — see the gridOn/axesOn/coordOn/labelOn properties.
    bool m_gridOn = true;
    bool m_axesOn = true;
    bool m_coordOn = true;
    bool m_labelOn = true;
    int  m_coordMode = 0;    // 0 = RectGC (X/Y), 1 = PolarGC (R/θ)
    bool m_exprOn = true;    // show equation while tracing (ExprOn)
    // Parametric/polar parameter window (defaults: a smooth full turn
    // in radians — ~314 points). Reset to the angle-appropriate defaults
    // when the angle mode changes.
    double m_paramTMin = 0.0;
    double m_paramTMax = 6.283185307179586;  // 2π
    double m_paramTStep = 0.02;
    // Sequence-mode settings (see the seq* properties).
    double m_seqNMax = 10.0;
    double m_seqInitU = 1.0;
    double m_seqInitV = 1.0;
    double m_seqInitW = 1.0;
    // TRACE state. When `m_isTracing` is true the graph canvas paints
    // a crosshair at (m_traceX, evaluated Y) on the active function.
    // m_traceX is reset to viewport centre on every toggleTrace(true).
    bool m_isTracing = false;
    double m_traceX = 0.0;
};

} // namespace tux_ti83

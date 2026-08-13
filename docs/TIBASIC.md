# TI-BASIC Programming — Design & Roadmap

Status legend (matches [ROADMAP.md](../ROADMAP.md)): ✅ done · 🚧 in
progress · 🔜 next · 📅 planned · 💭 idea / maybe.

TI-BASIC is the last major unbuilt Tux-TI83 feature and the capability the
project is named after. It is large enough to warrant its own design doc.
This file is the single source of truth for the plan; the
[ROADMAP.md](../ROADMAP.md) "Programming" section links here.

**Scope decision (2026-08-01): _pragmatic subset first._** We target the
common command set that makes real programs runnable, ship it in small
milestones, then expand. Exotic commands, in-program graphics, `.8xp`
import, and `getKey` game loops are explicitly deferred to later phases.

---

## 1. Goals & non-goals

**Goals**

- Write, name, save, edit, and run programs in the GUI.
- A useful language subset: sequential statements, `Sto`, `Disp`/`Output`,
  the full control-flow set, `Input`/`Prompt`, `Lbl`/`Goto`, sub-program
  calls — enough to write real utilities (quadratic solver, unit
  converters, guessing games, loops).
- Reuse the existing expression evaluator for every value/condition, so all
  current math (scalars, lists, matrices, complex, functions) works inside
  programs for free.
- Run the same interpreter **headless** in the CLI/REPL (batch execution of
  a program from a file / stdin).

**Non-goals (at least initially)**

- Byte-for-byte `.8xp` compatibility (deferred to P6).
- Assembly programs (`Asm(`), archived/flash apps.
- Exact TI-83 timing / cycle behaviour.
- Full string library on day one (strings arrive in P4, after control flow).

---

## 2. Architecture

### 2.1 The interpreter layer

A new **`Interpreter`** component sits **above** `MathStateMachine`, never
inside it. The math engine stays a pure single-expression evaluator; the
interpreter orchestrates statements and calls
`MathStateMachine::evaluate()` for the expression and condition parts.

```
          ┌─────────────────────────────────────────┐
          │  UIController (Qt bridge)                │
          │   • PRGM store, editor state             │
          │   • drives the run-loop, routes I/O      │
          └───────────────┬─────────────────────────┘
                          │ steps / resumes
          ┌───────────────▼─────────────────────────┐
          │  Interpreter                             │
          │   • program counter, block/loop stack    │
          │   • label table, call stack              │
          │   • output buffer, run status            │
          └───────────────┬─────────────────────────┘
                          │ evaluate(expr) for values/conditions
          ┌───────────────▼─────────────────────────┐
          │  MathStateMachine (unchanged core)       │
          │   • tokeniser, shunting-yard, registries │
          └─────────────────────────────────────────┘
```

The interpreter shares the engine's global registries (`varRegistry`,
`listRegistry`, `matrixRegistry`, `lastResult`/`Ans`), so `5→A` inside a
program and `A` on the home screen refer to the same `A`.

### 2.2 Resumable execution (the foundational commitment)

GUI code cannot block waiting for `Input`. The interpreter therefore runs
in **steps** and reports a status after each pause point:

```cpp
enum class RunStatus {
  Running,     // more to do; call step() again
  Output,      // emitted text to the output buffer; UI should render
  NeedInput,   // Input/Prompt — waiting for a value/string
  NeedKey,     // Pause/getKey/Menu — waiting for a keypress/choice
  Done,        // program finished
  Error        // ERR:… (with line number)
};
```

Execution state — program counter, the block/loop stack, the label table,
the call stack, and the pending Input target — lives in the `Interpreter`
object, not on the C++ call stack. So a program can suspend at any I/O
point and resume when the UI supplies input. The UIController drives a
loop: `while (step() == Running) {}`, then acts on the pause status.

The CLI/REPL uses the **same** interpreter but resolves `NeedInput`/
`NeedKey` synchronously from stdin, so programs run headless too.

### 2.3 Program storage

A program is a **name + ordered list of source lines**. Statements are
separated by newlines or `:` (colon). Stored as source text (re-tokenised
per line at run time via the existing tokeniser), which keeps editing
simple and sidesteps a binary format.

- `std::map<QString, QStringList>` (name → lines) in the controller.
- Persisted in the state JSON and named saves (extends
  `buildStateJson`/`applyStateJson`).
- Program names: 1–8 chars, `A`–`Z`/`0`–`9`, matching the TI-83.

### 2.4 Statement model

Each line is one statement, classified by its leading token:

- **Bare expression** — evaluate, store to `Ans`, and (unless suppressed)
  echo to output, like the home screen.
- **Store** — `expr→VAR` (already handled by `evaluate()`).
- **Keyword statement** — `Disp`, `If`, `For(`, `Goto`, … dispatched by the
  interpreter, with the remainder of the line parsed as the keyword's
  arguments (often themselves expressions evaluated via the engine).

Control-flow structures use a **block stack**: `If…Then`/`For(`/`While`/
`Repeat` push a frame; `End` pops the matching one. `Lbl`/`Goto` use a
one-pass **label table** built when the program is first run.

### 2.5 New value type: strings

Real I/O needs text. `CalculationResult`/`Operand` gain a **string** kind
(alongside scalar/list/matrix/complex). Introduced in **P4**, not before —
control flow (P3) is numeric-only and doesn't need it.

- `Str1`–`Str9` registry + `"…"` string literals.
- Ops: concatenation (`+`), `sub(`, `length(`, `inString(`, `expr(` (parse a
  string as an expression), and number→string for `Disp`.

---

## 3. Phases (pragmatic subset first)

Each phase is independently shippable and GUI-verifiable. Suggested first
milestone: **P0 + P1 + P2** — write, save, and run a program that computes
and `Disp`s results.

### P0 — Scaffolding 🚧 (core landed 2026-08-01)
- ✅ New pure-C++ `interpreter` library (`interpreter/`), linked by
  `graph_ui` + the test binary — reusable headless (CLI) and testable
  without Qt.
- ✅ `Interpreter`: `RunStatus` enum (`Running`/`Output`/`NeedInput`/
  `NeedKey`/`Done`/`Error`), statement splitter (lines → statements on
  top-level `:`), and the resumable `step()` / `run()` loop. P0 executes no
  statements yet — a program advances its program counter to `Done`.
- ✅ `ProgramStore` (name → source lines): put / has / get / names / remove.
- ✅ 21 unit tests (splitter, load/flatten, run-to-Done, reset, store).
- 📅 Program persistence in state JSON / named saves — deferred to P1 (no
  way to create a program until the editor exists).
- Milestone reached: store a program and run it to completion.

### P1 — Program editor (UI) ✅ (landed 2026-08-01)
- ✅ **PRGM popup** opened via **2ND + `√(`** (resolves open decision #1 —
  this keypad has no dedicated PRGM key). List mode (RUN / EDIT / ✕ per
  program + NEW) and edit mode (name field + multi-line source editor +
  SAVE / CANCEL).
- ✅ **Editor model:** chose a **freeform multi-line text editor** (source
  text) over a keypad-driven line-list — pragmatic and robust, since
  programs are stored as source and re-tokenised per line at run time.
  Typed with the physical keyboard (`->` for →, `Disp`, `sin(`, … as text).
- ✅ `ProgramStore` wired into `UIController`: `programNames` /
  `programText` / `saveProgram` / `deleteProgram` / `normalizeProgramName`
  / `runProgram`. Names normalised to A–Z/0–9, ≤8 chars.
- ✅ **Persistence** — programs serialise into the state JSON (and named
  saves) via `buildStateJson`/`applyStateJson`; cleared by RESET.
- ✅ P1 RUN loads the program into an `Interpreter` and runs to `Done`,
  recording completion in history. (Real `Disp` output + a run view = P2.)
- ✅ 16 controller tests (CRUD, run, persistence round-trip).
- Milestone reached: author, save, edit, delete, and persist a program.

### P2 — Sequential interpreter core ✅ (landed 2026-08-01)
- ✅ Statement dispatch in `Interpreter::step()` (was a no-op): **bare
  expressions**, **`Sto` (`→`)**, **`Disp`** (numeric, multi-arg, and quoted
  string literals printed verbatim), **`ClrHome`**, **`Stop`**. Bare
  expressions and stores echo their result (TI-authentic). Runtime errors
  stop the run and record the line + `ERR:…` label.
- ✅ **Injected evaluator** — the pure-C++ interpreter takes an
  `Evaluator` callback (`EvalResult(std::string)`); the controller wires it
  to its tokeniser + `MathStateMachine` + result formatter
  (`evalProgramSource`/`formatCalcResult`). Programs thus reuse all existing
  math and share the registries with the home screen (a program's `5→A` is
  visible as `A` afterwards). Headless-friendly (no Qt in the interpreter).
- ✅ **Run/output view** (`PrgmRunPopup`) — a dark LCD screen showing the
  Disp/echo output, opened automatically when a program finishes
  (`programRunFinished`). `ClrHome` clears it; errors render in red.
- ✅ 8 execution tests incl. the milestone `5→A : A²→B : Disp B` → `25`.
- 📅 `Pause` (numeric) moved to the interaction phase (P4) — it shares the
  resumable input/keypress UI with `Input`/`Prompt`/`getKey`.
- Milestone reached: programs compute and display results.

### P3 — Control flow ✅ (landed 2026-08-01)
- ✅ `If` — single-statement (`If cond` guards the next statement) and block
  form (`If/Then/Else/End`).
- ✅ `For(var,start,end[,step])…End` (custom + negative steps; end/step
  captured once at entry, TI-style), `While…End`, `Repeat…End` (body runs
  once, loops until true).
- ✅ `Lbl` / `Goto` (missing label → `ERR:LABEL`).
- ✅ **Design:** a structural pre-pass (`buildControlTables`) matches block
  openers (`Then`/`For(`/`While`/`Repeat`) to their `Else`/`End` and collects
  `Lbl` targets; `execStatement` now owns the program counter so control
  statements jump. A small For-frame stack holds per-loop state. Conditions
  reuse the engine's relational/boolean operators (non-zero = true).
- ✅ Unary-minus fix: `evalProgramSource` promotes `Sub`→`Neg` in unary
  contexts (mirrors `insertToken`), so `For(I,3,1,-1)` and `Disp -5` work.
- ✅ A 5M-step guard stops runaway loops from hanging the caller (a real
  user break lands in P5).
- ✅ 13 control-flow tests (If forms, For asc/step/desc, While, Repeat,
  Goto, nested loops, missing-label error).
- Milestone reached: loops and branches run.

### P4 — Strings + real interaction 🚧
Split into two increments; interaction landed first (it validates the
resumable model and hits the milestone with numeric input + string-literal
`Disp`).

**P4a — Interaction ✅ (landed 2026-08-01)**
- ✅ `Input VAR` / `Input "prompt",VAR` / `Prompt VAR` — pause the run
  (`NeedInput`), show an input field in the run view, resume with the typed
  value (stored into VAR). A bad value re-prompts.
- ✅ `Pause` (optional displayed arg) — pause (`NeedKey`); the run view
  shows a "▶ CONTINUE" button.
- ✅ **Resumable execution proven out:** the controller now holds the
  `Interpreter` as a member; `runProgram` steps until Done/Error/NeedInput/
  NeedKey; `provideProgramInput` / `resumeProgram` feed input and continue.
  Works across loops (repeated suspend/resume). `execStatement` returns the
  I/O statuses without advancing the PC; `provideInput`/`resumeFromPause`
  advance it.
- ✅ Run view: an input row (prompt + field + ENTER) and a CONTINUE button,
  plus a **◀ PRGM** button to return to the program manager.
- ✅ Fix: `evalProgramSource` now distinguishes a blank statement from an
  unparseable one, so bad Input values re-prompt (`ERR:SYNTAX`).
- ✅ 12 tests (Prompt→branch→result, prompt text, re-prompt, Pause, Input in
  a loop). Milestone reached: interactive prompt → branch → labelled result.

**P4b — String type ✅ (core landed 2026-08-01)**
- ✅ **Design decision:** strings live at the **interpreter level**, not in
  the engine. The engine's token stream (enum `Token`s) can't carry
  arbitrary string content, and strings are a program feature — so `core_math`
  stays untouched. The interpreter resolves `Str1`–`Str9` and `"…"` literals
  itself.
- ✅ `Str1`–`Str9` string variables (registry in the interpreter; persist
  across runs in a session), `"…"` literals, concatenation with `+`
  (`evalStringExpr` / `splitPlus`).
- ✅ String store `<strexpr>→StrN` (`stringStoreTarget`), `Disp` of string
  expressions (with mixed string/number args), text `Input` into `StrN`
  (stores the raw typed line). Type mismatches → `ERR:DATA TYPE`.
- ✅ Quote-aware `splitStatements` / `splitArgs` — a `:` or `,` inside a
  `"…"` string is no longer a separator.
- ✅ 10 tests (store/Disp, concat, literal+var, mixed args, quote-aware
  split, type errors, cross-run persistence, string Input).

**P4c — String functions ✅ (2026-08-08)**
- ✅ `length(` / `sub(` / `inString(` / `expr(`, resolved at the interpreter
  level (innermost-first text substitution via `resolveStrFuncs`, like
  `getKey` but with argument parsing): number-returning funcs become numbers,
  `sub(` becomes a `"…"` literal (so it stays a string), `expr(` splices its
  string as a sub-expression. All numeric evaluation now routes through
  `mEval` (= `resolveStrFuncs` + engine); `evalStringExpr` = resolve +
  `evalStringChain`. Compose + nest (`sub(Str1,1,length(Str1))`,
  `If length(Str1)>3`, `"X"+sub(…)`, `expr(Str1)→A`); `sub(` out of range →
  `ERR:DOMAIN`, type mismatch → `ERR:DATA TYPE`. +13 tests.
- ✅ **Str-var disk-persistence** (2026-08-08): `Str1`–`Str9` serialise into
  the state JSON (`"strings"` object) alongside scalars/lists/programs, so
  they survive a restart. Interpreter exposes `stringVars()` /
  `setStringVar()` / `clearStringVars()`; the controller reads/writes them in
  `buildStateJson` / `applyStateJson`. +1 test (save→load round-trip).
- ✅ **`Output(row,col,value)`** (2026-08-08): positioned text on the
  home-screen grid (rows 1–8, cols 1–16; `ERR:DOMAIN` out of range).
  Implemented purely at the interpreter level (`placeOutput` writes into the
  line buffer with space-padding for the column) — no UI change, since the
  run view already renders lines in a monospace font. Coexists with `Disp`
  and `ClrHome`. +10 tests.
- ✅ **`Menu("title","opt",Lbl,…)`** (2026-08-08): pause-and-branch menu.
  New `NeedMenu` pause state; the run view shows the title + numbered option
  buttons, and picking one jumps to that option's `Lbl` (a `Goto`, via the
  existing label table). Bad arg count → `ERR:ARGUMENT`; missing target label
  → `ERR:LABEL` on selection. +7 tests. **P4 complete.**

### P5a — Program control (sub-calls) ✅ (2026-08-06)
- `prgmNAME` **sub-program calls** — a call stack saves/restores each
  caller's execution frame (statements, PC, control tables, For stack);
  globals (vars/lists/`Str`) are shared TI-style. A program loader is
  injected into the interpreter (the controller looks names up in its
  `ProgramStore`). Nesting/recursion depth cap (128) → `ERR:MEMORY`.
- `Return` — pops one frame back to the caller; in the main program it
  ends the run. A missing sub-program → **`ERR:UNDEFINED`** (loud, not a
  silent no-op), matching the TI-83.
- `DelVar` — resets a scalar to 0 / clears a `Str` variable.
- +11 tests (sub-calls, nested unwind, `Return`, undefined program,
  recursion cap, `DelVar`). Also added a **COPY** button to the program
  run/output view (`copyProgramOutput()` → system clipboard) so a run's
  output can be pasted out.

### P5b-1 — Break / interrupt ✅ (2026-08-07)
- **User-triggered break** — a running program is now interruptible instead
  of blocking the UI. The interpreter runs in bounded slices
  (`runSlice(maxSteps)`); the controller (`stepProgramToPause`) pumps the
  event queue between slices so a **■ STOP** button in the run view stays
  live, and `interrupt()` ends the run with `ERR:BREAK` (TI-83 ON-key
  behaviour). The 5M-step lifetime guard remains as a headless backstop
  (moved into `runSlice`). Normal programs finish in the first slice → the
  CLI/tests stay fully synchronous; a re-entrancy guard blocks a stray
  event from re-entering the interpreter. +9 tests.

### P5b-2 — getKey ✅ (2026-08-08)
- **`getKey`** — a non-blocking key poll: evaluates to the code of the key
  pressed since the last poll, or `0` if none. Handled at the controller
  level (live keyboard state, not math): `evalProgramSource` substitutes the
  current code for `getKey` before tokenising, then consumes it (read-once).
  The run view captures physical keys → TI-83 codes (arrows 24/25/26/34,
  ENTER 105, CLEAR 45, DEL 23, digits, `.`) and reports them via
  `sendProgramKey()`.
- The run loop is now **time-bounded** (~8 ms slices) instead of fixed-step,
  so key/STOP response stays snappy regardless of per-statement cost; the
  runaway guard is headless-only (`run()`), so an interactive getKey loop
  isn't cut off (STOP is the GUI control). Disp output now refreshes **live**
  mid-run.
- Fixed **BUG-025** on the way: assignments (`→var`) echoed their value,
  which flooded a getKey loop with `0`s. Stores are now silent (TI-style);
  only a bare expression echoes. +6 tests.

### P5b-3 — Error jump-to-line ✅ (2026-08-08)
- The interpreter maps each flattened statement back to its **editor source
  line** (`m_statementSrcLine`) and tracks the **current program name**
  (`m_currentProgram`, saved/restored across `prgm` calls). `errorSourceLine()`
  + `currentProgram()` expose where an error occurred — correct even when a
  line holds a `:`-chain, and pointing at the sub-program when the error is
  inside one.
- The run view reports the true source line and shows an **✎ EDIT LINE n**
  button; pressing it opens the editor for that program with the offending
  line highlighted (`PRGMPopup.openAtLine`). +6 tests.

### P5b-4 — In-editor command-paste menu ✅ (2026-08-09)
- A **⌨ COMMANDS** palette in the PRGM editor: category tabs (CTL / I/O /
  STR / FN) each showing a wrapping grid of keyword buttons that insert at
  the cursor (`PRGMPopup.insertCmd` → `bodyArea.insert`). Statement words
  insert with a trailing space, function forms leave `(` open, `""` drops the
  cursor between the quotes, and `→` / `√(` are one tap. QML-only.
- Milestone: keywords are insertable without hand-typing. **P5 complete.**

### P6 — Advanced / optional 💭
**P6-1 — Program-driven graphs ✅ (2026-08-09)**
- Programs drive the graph engine via an injected **graph sink**
  (`Interpreter::setGraphSink` → `GraphCmd`; controller carries it out — the
  engine `core_math` stays untouched): `"X²"→Y1` / `X²→Y1` (function store,
  quoted or bare, shared `setFunctionFromSource`), window vars (`Xmin`/`Xmax`/
  `Ymin`/`Ymax`/`Xscl`/`Yscl`), `FnOn`/`FnOff` (one/all), `ZStandard`/
  `ZoomFit`, and `DispGraph` (closes the run view, shows the plot). Store
  targets detected via `storeTargetName`; bad function → `ERR:SYNTAX`. +10
  tests.
**P6-2 — Draw overlay ✅ (2026-08-09)**
- Programs draw on the graph via the graph sink → the **existing** DRAW
  layer (`drawLine`/`drawCircle`/`drawHorizontal`/`drawVertical`/`drawPoint`/
  `drawText`/`clrDraw`, rendered on `GraphCanvas`; graph coordinates):
  `Line(`, `Circle(`, `Horizontal`, `Vertical`, `Pt-On(`, `Text(`, `ClrDraw`.
  Like the TI-83, any graphics command shows the graph. Wrong arg count →
  `ERR:ARGUMENT`. +9 tests.
- Added a **✕ CLR** button in the top-right of `GraphCanvas` (shown only when
  overlays exist, via a reactive `drawObjectCount` property) → one tap clears
  all drawings; the DRAW menu (2ND+TRACE) still offers per-item delete.
  **P6 graphics complete.**
- ✅ `.8xp` import (2026-08-13) — parse real TI-83/84 program files. Pure-C++
  `decode8xp` (interpreter lib) validates the container + detokenises the body
  using the TI token table (verified against TI-Toolkit `8X.xml`), decoding to
  our source forms so it re-tokenises + runs. Controller `importProgram8xp`;
  PRGM popup **Import .8xp** path field. Samples in `docs/programs/`. Unknown
  tokens → `?` (counted); Asm + TI-84+ CE `0xEF` page out of scope.
- 💭 `Repeat`/`While` performance, `rand`-seeded games, live per-frame graph
  animation from a `getKey` loop.

### P7 — Modern language enhancements ✅ (complete 2026-08-11)

Improvements the TI-83 community always wanted, added on our modern base (a
fast interpreter, a real editor, disk persistence). Faithful gaps come first,
then extensions *beyond* strict TI-BASIC. Element access etc. are done at the
**interpreter level** (like the Y-store / string funcs) so `core_math` stays
untouched.

Done:
- ✅ **Comments (2026-08-10):** `#` to end of line is ignored (quote-aware,
  stripped per line in `loadStatements`).
- ✅ **`break` / `continue` (2026-08-10):** exit / skip the innermost loop.
  `buildControlTables` records each statement's enclosing loop
  (`m_enclosingLoop`); `break` jumps past its `End` (popping a `For` frame),
  `continue` jumps to the `End`. Outside a loop → `ERR:SYNTAX`. +7 tests.
- ✅ **Editor syntax highlighting (2026-08-10):** a C++ `QSyntaxHighlighter`
  (`ProgramHighlighter`, QML `Tux/ProgramHighlighter`, attached to the
  editor's `textDocument`) colours keywords, variables, strings, numbers, and
  `#` comments live. (Fixed BUG-026 alongside — light-theme surface labels.)
- ✅ **List/matrix element access + assignment (2026-08-10):** `L1(3)` /
  `[A](r,c)` reads and `5→L1(3)` / `9→[A](r,c)` writes, done in the controller
  (`resolveElementReads` substitutes innermost accesses before tokenising, so
  the engine no longer reads `L1(3)` as `L1×3`; `tryElementStore` intercepts an
  element store; `evalScalarValue` evaluates indices/RHS). Computed indices
  (`L1(K+1)`), append at `dim+1`, out-of-range → `ERR:INVALID DIM`, undefined →
  `ERR:UNDEFINED`. Program-only (home screen keeps implicit-multiply; core_math
  untouched). +10 tests.

- ✅ **Local variables (2026-08-10):** `Local A,B,…` saves each scalar's
  value, resets it to 0, and restores it when the program / sub-program frame
  exits — so a sub-program can't clobber the caller's globals (fixes the
  everything-is-global complaint). Per-frame `m_locals` (saved/restored in the
  `CallFrame`, written back in `returnFromCall` and at program end via
  `restoreLocals`). +4 tests. Foundation for user functions (params = locals).

- ✅ **User functions (2026-08-10):** multi-statement `Define name(params) …
  Return expr … End`, callable as `f(3,4)` in any expression (incl. nesting
  and recursion). `Define` is an interpreter opener skipped at runtime; its
  body is registered via a define-sink into the controller's `m_userFuncs`.
  `resolveUserFunctions` substitutes `f(args)` before tokenising (like element
  access); `callUserFunction` binds params (as saved/restored globals), runs
  the body in a nested interpreter (shared registries), and reads the value
  set by `Return expr`. Params must be `A`–`Z`; names lowercase. Wrong arg
  count → `ERR:ARGUMENT`, runaway recursion (depth 64) → `ERR:MEMORY`. +6 tests.

- ✅ **`SortA(` / `SortD(` (2026-08-10):** sort a list in place (ascending /
  descending); `SortA(L1,L2,…)` reorders parallel lists by the first's
  permutation (mismatched lengths → `ERR:INVALID DIM`). Silent command routed
  to the controller (`sortLists`). +4 tests. **A1 complete.**
- ✅ **`toString(`** **(2026-08-10):** number → string, resolved in the
  string-function pass (like `sub(`): `toString(number)` → its display text as
  a `"…"` literal, so it concatenates (`"A="+toString(A)`) and composes
  (`length(toString(123))`). +5 tests.

- ✅ **Error trapping (2026-08-10):** `Try … Else … End` (TI-84 style) — an
  error inside the `Try` block jumps to the `Else` handler (or past `End` if
  none) instead of halting; execution resumes after `End`. `Try` is a
  non-loop opener; a per-block `TryFrame` records the For/local/call-stack
  depths at entry, and the catch in `step()` unwinds those (popping sub-program
  frames, restoring locals) before running the handler. Doesn't catch a user
  STOP / the runaway guard. +5 tests.

- ✅ **Pixel graphics (2026-08-10):** `Pxl-On(`/`Pxl-Off(` set/clear pixels on
  a 63×95 screen grid (controller `m_pixels` set, rendered on `GraphCanvas`);
  `Pxl-Test(row,col)` reads 0/1 in expressions (resolved like element access);
  `Pt-Off(`/`Pt-Change(` erase/toggle the existing vector points. `ClrDraw`
  clears pixels; ✕ CLR accounts for them. New GFX palette tab collects the
  graphics keywords. +8 tests.
- ✅ **Graph-drawing extras (2026-08-11):** `StorePic n`/`RecallPic n`
  snapshot/overlay the current drawing (overlays + pixels; survives
  `ClrDraw`, kept in `m_pics`); `DrawF <expr>` plots `f(X)` as a sampled
  polyline ("curve" draw object); `Tangent(<expr>,x)` draws the tangent line
  at `x` (numeric slope, reuses `drawLine`); `Shade(<lower>,<upper>)` fills
  between two sampled curves ("shade" object). Expressions are sampled over
  the x-window by the controller (`sampleCurve`) since the engine needs a
  concrete X per point. +5 tests. **This completes P7.**

---

## 4. Command coverage (initial subset)

| Group | In the subset (P2–P5) | Deferred (P6 / later) |
|---|---|---|
| Sequential | expression, `Sto` (`→`), `:` separator | — |
| Output | `Disp`, `Output(`, `ClrHome`, `Pause` | `Disp` of graphs |
| Control | `If`/`Then`/`Else`/`End`, `For(`, `While`, `Repeat`, `End` | — |
| Jumps | `Lbl`, `Goto` | — |
| Input | `Input`, `Prompt`, `Menu(`, `getKey` | `getKey` game loops |
| Program | `Stop`, `Return`, `prgmNAME`, `DelVar` | `Archive`/`Unarchive` |
| Strings | `Str1`–`Str9`, `"…"`, `sub(`, `length(`, `inString(`, `expr(` | full string library |
| Graphics | — | `Line(`, `Circle(`, `Text(`, `Pt-On(` in programs |
| Interop | `.8xp` import | link-cable transmit |

---

## 5. Testing strategy

- **Interpreter unit tests** run headless (no Qt UI) — feed a program as
  source, run to completion with scripted `Input`/`getKey` responses, and
  assert on the output buffer and final registry state. This mirrors how
  the CLI runs programs, so one harness covers both.
- Golden programs per phase: P2 arithmetic, P3 loops/branches, P4
  interactive I/O with scripted input, P5 sub-calls + error/line reporting.
- Keep the existing 629-test math suite green throughout — the engine core
  must not regress.

---

## 6. Open design decisions (resolve as we reach them)

1. **PRGM menu home** — which key/soft-key opens PRGM (the TI-83 uses a
   dedicated PRGM key we don't have). Decide in P1.
2. **Output view vs. history** — does program output render in the existing
   history pane, a dedicated run view, or reuse the LCD? Decide in P2.
3. **Editor model** — line-list editor (each line edited with the keypad)
   vs. a freeform text area. Line-list is closer to the TI-83 and reuses
   the tokeniser; leaning that way.
4. **Break key** — what interrupts a running program in the GUI (ESC? a
   dedicated on-screen STOP?). Decide in P5.
5. **String storage in state JSON** — schema for `Str1`–`Str9` + programs.

---

## 7. Build order recommendation

Ship **P0 → P1 → P2** as the first milestone (author + run + `Disp`), then
**P3** (control flow) as the second — those two together already make the
feature genuinely useful. **P4** (strings + interaction) is the third and
largest single step. **P5** hardens it; **P6** is optional polish.

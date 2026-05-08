# ROADMAP.md — Tux-TI83

The full feature ledger for Tux-TI83. This is the canonical list of what
the project does, what's in flight, and what a real TI-83 / TI-83 Plus
has that we haven't built yet.

## Tracking — three files, three jobs

| File | Tracks |
|---|---|
| **ROADMAP.md** *(this file)* | Features and capabilities — what to build, organized by area |
| [BUGS.md](BUGS.md) | Things that are broken |
| [IMPROVEMENTS.md](IMPROVEMENTS.md) | Things that work but could be better (refactors, code quality) |

> **Note:** A ✅ on this roadmap means the feature is implemented, but it
> may still have known bugs. Always cross-reference [BUGS.md](BUGS.md)
> before assuming a ✅ feature works in every edge case.

## Status legend

| Symbol | Meaning |
|---|---|
| ✅ | Done |
| 🚧 | In progress |
| 🔜 | Up next (immediate priority) |
| 📅 | Planned (committed, scheduled) |
| 💭 | Considering (long-term, may or may not happen) |

---

## Current focus

**Phase A complete (2026-04-07).** All legacy features are now ported
into the new component-based UI: history pane, WINDOW popup, matrix
editor (NAMES/MATH/EDIT tabs), graph mode (canvas + Y1/Y2/Y3 selector +
mode toggle), x² CalcKey wired. The legacy `graph_ui/qml/Main.qml` has
been audited and deleted. Group A engine cleanup also landed in this
phase, taking the open-bugs count from 6 to 0.

Next-up work is the no-op CalcKeys that depend on infrastructure not
yet built: 2ND modifier system, ALPHA modifier system (see
[IMP-003](IMPROVEMENTS.md)), MATH menu, MODE menu, Ans recall (needs a
`core_math/` change to add `Token::Ans`).

---

## Tooling & testing

- ✅ `tux_ti83_cli` — headless one-shot binary. `tux_ti83_cli "2+2"`
  prints `4`, exits. Usage message + exit code 2 when run without args.
  Added 2026-04-08; split into its own binary on 2026-04-08 after the
  REPL moved to `tux_ti83_repl`.
- ✅ `tux_ti83_repl` — interactive REPL binary. Prompt-per-line with
  `Ans` recall between lines, `:quit` / `:q` / Ctrl+D to exit. Reads
  stdin, so it bypasses bash's history expansion entirely — you can
  type `5!+3!` at the prompt without any shell-escape gotcha.
  Added 2026-04-08.
- ✅ Shared CLI helpers live in [cli/cli_common.hpp](cli/cli_common.hpp)
  (inline header) so both binaries route through the same result
  formatting, error reporting, and ANSI colour logic. No duplicated
  code between the two entry points.
- ✅ `tux_ti83_tests` — regression test binary with plain C++ assertions
  (no external test framework). Drives the same `UIController` the GUI
  uses; covers basic arithmetic, trig, log/ln, sqrt, constants, unary
  negation (BUG-014), number functions (Phase B Wave 1), `▶Frac`/`▶Dec`
  (BUG-015), Ans recall, all engine bug fixes (BUG-005/006/007/009/010/
  011/013), matrix subtraction (BUG-008), matrix dimension errors. 56
  tests at first cut. Wired into CTest (`enable_testing()` +
  `add_test(NAME math_regression …)`). Added 2026-04-08.
- ✅ `Q_INVOKABLE bool UIController::processExpression(const QString&)`
  — full-string tokeniser + dispatcher. Used by both the CLI and the
  test binary; available to QML for future paste-an-expression / batch
  features.
- ✅ ASCII operator aliases (`-`, `*`, `/`) added to the `kTokens` table
  so keyboard typists and the CLI can use plain ASCII without any
  pre-conversion layer. Display still shows the Unicode forms (`−`,
  `×`, `÷`) for consistency.
- 📅 Wire CTest into the build script (`./build.sh --test`?) so tests
  run automatically alongside builds.
- 📅 Continuous integration (GitHub Actions runner that builds + runs
  `tux_ti83_tests` on every push).
- 📅 CLI commands beyond bare expressions: `:vars`, `:matrix [A]`,
  `:graph X^2 -10 10`, etc. Currently REPL only handles expressions.
- 🚧 [`USER_MANUAL.md`](USER_MANUAL.md) — end-user documentation covering
  the GUI keypad layout, MATH / MATRX / WINDOW menus, graph mode,
  keyboard shortcuts, CLI / REPL usage, error messages, and worked
  examples. **Skeleton landed 2026-04-08** with all 17 sections stubbed;
  sections marked *"planned"* will be fleshed out over time (screenshots,
  worked examples, the function-reference appendix, troubleshooting
  expansion).
- 💭 Test coverage measurement (gcov / lcov) once the test suite grows.
- 💭 Property-based / fuzz testing for the parser (FuzzTest, libFuzzer).

---

## Architecture

- ✅ Custom recursive-descent parser + shunting-yard evaluator (`core_math/`)
- ✅ SCHEMA_V5 state machine + capsule-based memory
- ✅ Modular QML component architecture (`Style`, `CalcKey`, `Display`, `SoftKeyRow`)
- ✅ Display state machine (INPUTTING / EVALUATED / ERROR) in `UIController`
- ✅ Unified token table (single source of truth for input ↔ display)
- ✅ Bug catalogue (BUGS.md) and improvements catalogue (IMPROVEMENTS.md)
- ✅ Legacy `graph_ui/qml/Main.qml` audited and deleted 2026-04-07 (Phase A wrap-up)
- ✅ Session crash logger — added 2026-04-29 (every UIController entry point appends a millisecond-timestamped event to `~/.local/state/tux-ti83/session.log` with fsync; SIGSEGV/SIGABRT/SIGFPE/SIGILL/SIGBUS handlers append a CRASH marker + libc backtrace via async-signal-safe APIs; `std::terminate` handler captures the exception's `what()`)
- 💭 Unit tests for `core_math` (parser, evaluator, matrix ops)

## Numeric core

### Basic arithmetic
- ✅ `+ − × ÷` (matrix `−` added 2026-04-07 in the Group A engine cleanup)
- ✅ Power `^` (right-associative — `2^3^2 = 512`; fixed 2026-04-07)
- ✅ Square root `√(` (returns `ERR:NONREAL ANS` for negatives; fixed 2026-04-07)
- ✅ Parentheses, decimal point, π
- ✅ Order of operations
- ✅ Unary negation `(-)` — `Token::Neg` with precedence 2 and `is_function` semantics; the on-screen `(-)` CalcKey sends `"neg"` while the `−` key and keyboard `-` still send binary Sub (fixed 2026-04-07, closes BUG-014)
- 📅 `x²` shortcut key
- 📅 `nthroot(`
- 📅 Implicit multiplication by juxtaposition (`2(3)`, `2π`) — see [IMP-005](IMPROVEMENTS.md) for the dead `Token::ImplicitMul`

### Transcendental
- ✅ `sin`, `cos`, `tan` (engine + UI keys in SCIENTIFIC section)
- ✅ `asin`, `acos`, `atan` (engine + `ERR:DOMAIN` for inputs outside `[-1, 1]`; fixed 2026-04-07) — UI exposure pending: best route is via 2ND modifier on the sin/cos/tan keys when the modifier system lands
- ✅ `log` (base 10), `ln` (both return `ERR:NONREAL ANS` for non-positive inputs; fixed 2026-04-07)
- ✅ `e` constant — engine + UI (SCIENTIFIC row 2 col 5, next to π; added 2026-04-07)
- 📅 `e^(` exponential function
- ✅ Hyperbolic: `sinh`, `cosh`, `tanh`, `asinh`, `acosh`, `atanh` — engine + UI via MATH menu (added 2026-04-08). Domain checks: `acosh` requires x ≥ 1, `atanh` requires |x| < 1 (both return `ERR:DOMAIN` otherwise); the other four accept all reals.
- 💭 `logBASE(` for arbitrary base

### Number functions
- ✅ `abs(` — engine + UI via MATH menu (added 2026-04-07, Phase B Wave 1)
- ✅ `int(` — floor; engine + UI via MATH menu (added 2026-04-07)
- ✅ `iPart(` — truncation toward zero; engine + UI via MATH menu (added 2026-04-07)
- ✅ `fPart(` — fractional part; engine + UI via MATH menu (added 2026-04-07)
- ✅ `▶Frac` — MATH menu entry; post-hoc conversion of the last scalar result to its fraction form (fixed 2026-04-07, closes BUG-015)
- ✅ `▶Dec` — MATH menu entry; reverse of `▶Frac`, restores the raw decimal display of the last result
- ✅ `round(x, n)` — binary, rounds x to n decimal places (engine + UI via MATH menu; added 2026-04-08, Phase B Wave 2)
- ✅ `min(a, b)` — engine + UI via MATH menu (Wave 2)
- ✅ `max(a, b)` — engine + UI via MATH menu (Wave 2)
- ✅ `mod(a, b)` — engine + UI via MATH menu (returns `ERR:DIVIDE BY 0` on zero divisor; Wave 2)
- ✅ `Comma` token + binary-function infrastructure: shunting-yard pushes a synthetic `LeftParen` for functions whose input string ends in `(`, so the matching `)` and inner commas have a clear scope marker. Unlocks future n-ary functions like `nCr(n, r)` (added 2026-04-08)
- 📅 Sign function

### Combinatorics
- ✅ `nCr(n, r)` — combinations, n choose r (engine + UI via MATH menu; added 2026-04-08). Requires 0 ≤ r ≤ n with both non-negative integers; `ERR:DOMAIN` otherwise.
- ✅ `nPr(n, r)` — permutations, n permute r (engine + UI via MATH menu; added 2026-04-08). Same domain rules as nCr.
- ✅ `!` (factorial) — unary postfix, engine + UI via MATH menu, keyboard shortcut `!` (added 2026-04-08). Accepts non-negative integers ≤ 170; returns `ERR:DOMAIN` otherwise. Binds tighter than `^` (so `2^3! = 2^(3!) = 64`).
- ✅ `UIController::formatScalar(double)` helper — centralised display formatting at 10-significant-digit precision, used by `evaluate`, `▶Dec`, and the test suite. Fixes an implicit bug where results ≥ 10⁶ displayed as scientific notation by default (`10! = 3628800` now renders as the integer, not `3.6288e+06`).

### Calculus
- 📅 Numeric integration `fnInt(`
- 📅 Numeric derivative `nDeriv(`
- 📅 `sum(`, `prod(`, `seq(` over a range
- 💭 Equation solver (Solver app)
- 💭 Symbolic operations (well beyond original TI-83 scope)

### Number systems
- 💭 Complex numbers (a+bi mode, polar form, conjugate, real/imag parts)
- 💭 Base conversion (DEC/HEX/OCT/BIN — TI-83 Plus's BASE app)

## Comparators & boolean

Engine implements more than the UI currently exposes — listed below.

- ✅ `=`, `≠`, `<`, `>` (engine + UI via `LogicMenuPopup` → TEST section, 2026-04-18)
- ✅ `≤`, `≥` (engine + UI via `LogicMenuPopup` → TEST section; ASCII aliases `<=`/`>=` also work from the keyboard, 2026-04-18)
- ✅ `and`, `or`, `not` (engine + UI via `LogicMenuPopup` → LOGIC section, 2026-04-18)
- ✅ `xor` (engine + UI via `LogicMenuPopup` → LOGIC section, 2026-04-18)
- ✅ Logic operator menu in the new UI — `LogicMenuPopup` landed 2026-04-18 (opened via 2ND + MATH; two sections: TEST with `=`, `≠`, `<`, `≤`, `>`, `≥`, and LOGIC with `and`, `or`, `xor`, `not`; kTokens now has the missing Unicode entries `≤`/`≥` and `xor` plus ASCII aliases `<=`/`>=`; MATH CalcKey shows a `TEST` sub-label)

## Variables & storage

- ✅ 26 single-letter scalar variables `A`–`Z` — added 2026-04-18 (contiguous `Token::VarA..VarZ`, backed by `MathStateMachine::varRegistry`; unset reads as 0; ALPHA + letter key inserts via the unified dispatcher; `X` doubles as the graph sweep variable in graph mode and as a scalar in calc mode)
- ✅ `STO→` store-to-variable — added 2026-04-18 (`Token::Sto`, lowest precedence, preprocessing consumes the target `VarA..VarZ` and records its index; accessible via the MATH menu `→ (STO)` entry, keyboard `|`, or ASCII `->`; error paths don't mutate the registry)
- ✅ `Ans` (last answer) — auto-populated after every successful ENTER via `MathStateMachine::lastResult`; `Token::Ans` pushes the stored scalar or matrix onto the operand stack; NUMERIC row 4 col 4 CalcKey inserts it into the expression (added 2026-04-07)
- ✅ Last-entry recall (2nd + ENTER cycles backwards through history) — added 2026-04-18 (10-deep ring buffer in UIController; each non-empty ENTER pushes the raw token stream; recallLastEntry walks back one step per call and clamps at the oldest entry; any non-recall input resets the cycle; errors get stored too so typos can be fixed)
- 📅 `Y-VARS` store/recall (functions, window vars, statistics, etc.)
- 📅 Memory management menu (`MEM`): list/delete/clear by category
- 💭 Persistent storage across runs (save calculator state to disk)
- 💭 `DelVar` for explicit variable deletion

## Matrices

- ✅ Add, scalar multiply, matrix multiply
- ✅ Determinant `det(` — engine fully implemented; reachable from the new UI via the MATRIX popup's MATH tab (no dedicated keypad button yet)
- ✅ Registry `[A]`, `[B]`, `[C]` (UI exposure — engine actually supports `[A]`–`[J]`, just needs more entries in the controller's token table)
- ✅ 3×3 matrix editor reintegrated in the new UI as `MatrixPopup` (NAMES / MATH / EDIT tabs, opened from the new MATRX CalcKey in the SCIENTIFIC section; reintegrated 2026-04-07; current limitations tracked as [IMP-007](IMPROVEMENTS.md) and [IMP-008](IMPROVEMENTS.md))
- ✅ Matrix subtraction `[A] − [B]` (fixed 2026-04-07; returns `ERR:INVALID DIM` on mismatched dimensions)
- ✅ Transpose `T(` — unary matrix function; engine + UI via the MatrixPopup's MATH tab (added 2026-04-08). Swaps rows and columns; returns `ERR:DATA TYPE` for scalar input.
- ✅ Inverse `^-1` — TI-83 syntax; engine via Gauss-Jordan on the augmented `[A | I]` form; UI via MatrixPopup MATH tab entry "4: ^-1 (inverse)" which inserts the multi-token `^-1` sequence (added 2026-04-08). Non-square input returns `ERR:INVALID DIM`; singular matrices return `ERR:SINGULAR MAT`.
- ✅ Reduced row-echelon form `rref(` — shared row-reduction engine with inverse; UI via MatrixPopup MATH tab entry "3: rref(" (added 2026-04-08). Cells with magnitude < 1e-12 are clamped to zero so results don't display with floating-point noise.
- 📅 Row-echelon form `ref(`
- 📅 `dim(`, `identity(`, `randM(`
- 📅 `augment(`
- 📅 Matrix ↔ List conversion (`Matr→List`, `List→Matr`)
- 📅 Variable matrix dimensions (currently editor is fixed at 3×3)
- 📅 Extend UI registry exposure to `[A]`–`[E]` (matches TI-83 hardware default)
- 💭 Extend UI registry exposure to all 10 (`[A]`–`[J]`, TI-83 Plus / TI-84 range; engine is already there)

## Lists

- 📅 Lists `L1`–`L6`
- 📅 List entry / editing UI (Stat editor)
- 📅 List arithmetic (vectorised ops)
- 📅 List functions: `sum(`, `mean(`, `min(`, `max(`, `stdDev(`, `variance(`
- 📅 `seq(` for generating lists from formulas
- 📅 List ↔ Matrix conversion
- 📅 Custom named lists (`L1`–`L6` plus `αLIST`)

## Statistics & probability

- 📅 1-variable stats (mean, median, mode, σ, σ², quartiles)
- 📅 2-variable stats (correlation, regression coefficients)
- 📅 `rand`, `randInt(`, `randNorm(`, `randBin(`
- 📅 `nCr`, `nPr`, factorial `!`
- 📅 Statistical regressions: `LinReg`, `QuadReg`, `CubicReg`, `QuartReg`, `LnReg`, `ExpReg`, `PwrReg`, `SinReg`, `Logistic`
- 📅 Distributions: `normalpdf(`, `normalcdf(`, `invNorm(`, `tpdf(`, `tcdf(`, `χ²pdf(`, `χ²cdf(`, `Fpdf(`, `Fcdf(`, `binompdf(`, `binomcdf(`, `poissonpdf(`, `poissoncdf(`, `geometpdf(`, `geometcdf(`
- 📅 Stat plots (scatter, xy-line, histogram, box plot)

## Graphing

### Function mode (current)
- ✅ Multi-function plot (Y1, Y2, Y3) — `GraphCanvas` component, ported from legacy 2026-04-07
- ✅ Pan (click-drag) and zoom (scroll wheel)
- ✅ ZStandard, Zoom Fit (via the WINDOW popup's ZSTD/ZFIT buttons)
- ✅ Active-function selector (`FunctionSelector` Y1/Y2/Y3 row, bound to `uiController.activeFunctionIndex`)
- ✅ Mode toggle: `GRAPH` soft key enters graph view, `Y=` soft key returns to keypad
- ✅ ZOOM soft-key — `ZoomPopup` landed 2026-05-08 (ZStandard / Zoom In / Zoom Out / ZFit, see Window settings entry below)
- ✅ TRACE soft-key — landed 2026-05-08 (movable crosshair on the active curve, X/Y readout that respects the user's Notation/Decimal MODE settings; ←/→ steps along the curve in 1/100-of-viewport increments; ↑/↓ cycles through Y1/Y2/Y3 while tracing)
- ✅ Tag function curves with their Y index in the canvas legend — fixed via [BUG-012](BUGS.md) on 2026-05-08; `getMultiGraphPoints()` now emits one entry per slot so colour-by-index is stable
- 📅 Y-editor screen (visual list of Y1–Y9, Y0 with on/off toggles, styles)
- 📅 Function on/off toggling
- 📅 Function styles (thin, thick, dotted, shaded above/below, animate)
- 📅 Extend Y-editor to Y1–Y9 + Y0 (10 functions, TI-83 standard)

### Window settings
- ✅ Xmin/Xmax/Ymin/Ymax editable in the new UI's `WindowPopup` (reintegrated 2026-04-07; opened from the WINDOW soft-key)
- ✅ ZSTANDARD and ZOOM FIT actions wired to `uiController.resetViewport()` and `zoomFit()`
- ✅ ZOOM soft-key opens a `ZoomPopup` with ZStandard / Zoom In / Zoom Out / ZFit presets (2026-05-08)
- 📅 Xscl/Yscl (axis tick spacing — controller doesn't track these yet)
- 📅 Xres (graph resolution / x-step — controller doesn't track this yet)

### Tables
- 📅 `TABLE` view — function values for a sequence of x's
- 📅 `TBLSET` (start, step, auto/ask)

### Zoom menu (full TI-83 set)
- ✅ ZStandard, ZoomFit
- 📅 ZBox (zoom to a user-drawn rectangle)
- 📅 Zoom In, Zoom Out
- 📅 ZDecimal (nice round decimal coords)
- 📅 ZSquare (equal x/y scaling)
- 📅 ZTrig (trig-friendly window: -2π..2π)
- 📅 ZInteger (integer x coords)
- 📅 ZoomStat (fit current stat plot data)
- 📅 ZoomPrevious
- 📅 ZoomMemory (save/recall window settings)

### DRAW menu
- 📅 `Pt-On(`, `Pt-Off(`, `Pt-Change(`
- 📅 `Line(`, `Vertical`, `Horizontal`
- 📅 `Circle(`
- 📅 `Tangent(`
- 📅 `Pen` (freehand draw)
- 📅 `Text(` (overlay text on graph)
- 📅 `Shade(`
- 📅 `DrawF` (draw a function expression)
- 📅 `DrawInv` (draw inverse of a function)
- 📅 `ClrDraw`

### Format menu
- 📅 `RectGC` / `PolarGC` (rectangular vs polar coords for cursor)
- 📅 `CoordOn` / `CoordOff`
- 📅 `GridOn` / `GridOff`
- 📅 `AxesOn` / `AxesOff`
- 📅 `LabelOn` / `LabelOff`
- 📅 `ExprOn` / `ExprOff`

### Other graphing modes
- 💭 Parametric mode (X1T, Y1T)
- 💭 Polar mode (r1, r2)
- 💭 Sequence mode (u(n), v(n))
- 💭 Inequality shading (Inequalz app on TI-83 Plus)

## UI / UX

### Done
- ✅ Nord palette extended with semantic roles (operator/enter/second/numeric/function categories)
- ✅ `Style` singleton — single source of truth for colours, sizes, fonts
- ✅ `CalcKey`, `Display`, `SoftKeyRow`, `HistoryPane`, `FunctionSelector`, `GraphCanvas` reusable components
- ✅ Display state machine with three states + cursor blink
- ✅ Keyboard shortcuts (literal CLAUDE.md keymap)
- ✅ Section dividers (CONTROL / SCIENTIFIC / NUMERIC) with hairline rules
- ✅ History side panel — right-side column bound to `uiController.history` (reintegrated 2026-04-07)
- ✅ `x²` CalcKey wiring (sends `^` then `2`)

### Up next
- ✅ `MATH` CalcKey wired — opens `MathMenuPopup` (see Numeric core › Number functions)
- ✅ Wire `MODE` CalcKey — `MODEPopup` landed 2026-04-18 (8 TI-83-authentic rows; Angle row wired to `MathStateMachine::angleMode`, which feeds trig/inverse-trig evaluation; header indicator binds to the same property; remaining rows rendered as TI-83-authentic greyed placeholders)
- ✅ Wire `2ND` CalcKey — modifier system landed 2026-04-18 (toggle arm, mutually exclusive with ALPHA, one-shot, amber header badge; wired 2ND variants: sin(/cos(/tan(→asin(/acos(/atan(, x²→√(, ln(→e^(, (-)→Ans)
- ✅ Wire `ALPHA` CalcKey — infrastructure landed 2026-04-18 (toggle arm, mutually exclusive with 2ND, one-shot, green α header badge); no letter variants wired yet (blocked on variable registry)

### Planned

- ✅ ALPHA letter bindings — added 2026-04-18 (handleKey's alphaMap routes ALPHA + primary-key to the matching VarA..VarZ token, mirroring the on-key alphaLabel annotations)
- 📅 Remaining 2ND variants — TEST menu, insert-mode toggle, CATALOG (2ND+0), and nth-root (2ND+^) all now done. Outstanding: 2ND+`(` / `)` for `{` / `}` lists, EE / scientific exponent entry, and a few catalog-only variants.
- ✅ Cursor movement within an expression (left/right arrow editing) — added 2026-04-18 (token-level cursor in UIController; insertToken/backspace are cursor-aware; Left/Right/Home/End keyboard shortcuts; Display's TextInput binds to cursorOffset so the visual cursor tracks edits mid-expression; unary-negation disambiguation now looks at the token immediately left of the cursor rather than the tail)
- ✅ Insert mode toggle (2nd + DEL) — added 2026-04-29 (`m_insertMode` flag on UIController; default INS splices, OVR replaces the token at the cursor and falls back to append past the end; header `OVR` badge; DEL key has an `INS` 2ND corner label; on-screen `CURSOR` section also added with HOME / ← / → / END to complement the existing keyboard shortcuts)
- ✅ ALPHA-lock mode — added 2026-04-18 (2ND + ALPHA toggles a persistent `alphaLocked` flag in addition to the one-shot `alphaArmed`; header shows "A-LOCK" when locked; any letter keypress fires its ALPHA variant without clearing the lock; ALPHA alone or CLEAR releases it; 2ND during lock preserves the lock so 2ND+letter combos stay usable mid-typing)
- 📅 `MODE` menu follow-ups — Notation (Normal/Sci/Eng), Decimal (Float/Fix N), and Draw (Connected/Dot) all wired now. Remaining placeholder rows: Graph: Func/Par/Pol/Seq, Plot: Sequential/Simul (needs frame-by-frame animation framework), Complex: Real/a+bi/re^θi, Screen: Full/Horiz/G-T. Each needs a backing property + evaluator/renderer support.
- 📅 `CATALOG` browser (alphabetical list of every command)
- 📅 Mode indicator in the header (currently hardcoded "NORMAL  DEG")
- ✅ WINDOW popup ported to the new UI (see Graphing › Window settings)
- ✅ Matrix editor popup ported to the new UI (see Matrices)
- ✅ Logic operator menu ported to the new UI — `LogicMenuPopup` landed 2026-04-18 (see Comparators & boolean section above for details)
- ✅ `:` statement separator — added 2026-04-29 (`Token::Colon`; `evaluate()` splits on Colon, evaluates each segment in order, returns the last non-empty segment; errors short-circuit but earlier Sto mutations commit, matching TI-83 per-statement semantics; ALPHA + `.` inserts `:`, wired in alphaMap)

### Considering
- 💭 Resizable window (currently fixed 720×760)
- 💭 Themes beyond Nord (light mode, high contrast, monochrome retro LCD)
- 💭 UI scale / zoom setting — global multiplier on key sizes, font sizes, and display pixel sizes so the whole calculator can be scaled up for accessibility or larger monitors. Likely implemented as a `Style.uiScale` property feeding into every pixel-size/size constant, with a persisted setting and a control in a future Settings panel. Pairs naturally with the theming work since both live in `Style.qml`.
- 💭 Touch input refinement
- ✅ On-screen 2nd/Alpha sub-labels on each key — added 2026-04-18 (tiny amber 2ND function in top-left corner, green ALPHA letter in top-right; 2ND labels shown only for wired variants, ALPHA letters follow TI-83 layout)

## Programming (TI-BASIC subset)

All long-term — no commitment, but worth listing because it's a major
TI-83 capability the project is named after.

- 💭 Program editor (in-app text editor)
- 💭 Control flow: `If`/`Then`/`Else`/`End`, `For`, `While`, `Repeat`
- 💭 `Goto`/`Lbl`
- 💭 I/O: `Input`, `Prompt`, `Disp`, `Output(`, `ClrHome`, `Pause`
- 💭 `Menu(`, `getKey`, `DelVar`, `Stop`, `Return`
- 💭 Subprogram calls
- 💭 Run/edit/transmit menus

## Connectivity & data exchange

- 💭 Save/load full calculator state to disk
- 💭 Export/import variables, lists, matrices, programs as files
- 💭 `.8xp` (TI program) import — would let users run real TI-83 programs
- 💭 Link cable simulation between two running instances

---

## Phasing suggestion

A natural ordering, smallest meaningful chunk first:

### Phase A — finish the UI redesign integration
1. Wire `2ND`, `MODE`, `ALPHA`, `MATH`, `x²`, `Ans` CalcKeys
2. Reintegrate history pane in the new UI
3. Reintegrate WINDOW popup
4. Reintegrate graph mode (canvas + soft-key row wiring)
5. Reintegrate matrix editor
6. Audit and delete legacy `graph_ui/qml/Main.qml`

### Phase B — fill in the math vocabulary users actually expect
7. Variables `A`–`Z` and `STO→`
8. `Ans` recall
9. Last-entry recall
10. `abs`, `int`, `round`, `min`, `max`, `mod`
11. Hyperbolic functions
12. `e`, `e^(`
13. Determinant, transpose, inverse, RREF for matrices
14. `nCr`, `nPr`, `!`

### Phase C — lists and stats
15. Lists `L1`–`L6` with editor
16. List arithmetic and functions
17. 1-variable and 2-variable statistics
18. Statistical regressions (Lin, Quad, Exp, Ln)
19. Stat plots
20. Random functions

### Phase D — graphing maturity
21. Trace mode
22. Tables (`TABLE`, `TBLSET`)
23. Y-editor with 10 functions, on/off, styles
24. Full Zoom menu
25. DRAW menu
26. Format menu

### Phase E — modes and meta
27. ALPHA + 2ND modifier systems (IMP-003 lands here)
28. MODE menu (angle, display, etc.)
29. CATALOG
30. Memory management

### Phase F — long term
31. Calculus (`fnInt`, `nDeriv`, `sum`, `seq`)
32. Distributions
33. Programming (TI-BASIC subset)
34. Save/load
35. Parametric / polar / sequence modes
36. Complex numbers

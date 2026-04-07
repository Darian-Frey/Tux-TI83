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

## Architecture

- ✅ Custom recursive-descent parser + shunting-yard evaluator (`core_math/`)
- ✅ SCHEMA_V5 state machine + capsule-based memory
- ✅ Modular QML component architecture (`Style`, `CalcKey`, `Display`, `SoftKeyRow`)
- ✅ Display state machine (INPUTTING / EVALUATED / ERROR) in `UIController`
- ✅ Unified token table (single source of truth for input ↔ display)
- ✅ Bug catalogue (BUGS.md) and improvements catalogue (IMPROVEMENTS.md)
- ✅ Legacy `graph_ui/qml/Main.qml` audited and deleted 2026-04-07 (Phase A wrap-up)
- 💭 Unit tests for `core_math` (parser, evaluator, matrix ops)

## Numeric core

### Basic arithmetic
- ✅ `+ − × ÷` (matrix `−` added 2026-04-07 in the Group A engine cleanup)
- ✅ Power `^` (right-associative — `2^3^2 = 512`; fixed 2026-04-07)
- ✅ Square root `√(` (returns `ERR:NONREAL ANS` for negatives; fixed 2026-04-07)
- ✅ Parentheses, decimal point, π
- ✅ Order of operations
- 📅 Unary negation `(-)` *(currently inert — no token in controller's tokenMap)*
- 📅 `x²` shortcut key
- 📅 `nthroot(`
- 📅 Implicit multiplication by juxtaposition (`2(3)`, `2π`) — see [IMP-005](IMPROVEMENTS.md) for the dead `Token::ImplicitMul`

### Transcendental
- ✅ `sin`, `cos`, `tan` (engine + UI keys in SCIENTIFIC section)
- ✅ `asin`, `acos`, `atan` (engine + `ERR:DOMAIN` for inputs outside `[-1, 1]`; fixed 2026-04-07) — UI exposure pending: best route is via 2ND modifier on the sin/cos/tan keys when the modifier system lands
- ✅ `log` (base 10), `ln` (both return `ERR:NONREAL ANS` for non-positive inputs; fixed 2026-04-07)
- ✅ `e` constant (engine supports `Token::E` and `M_E`; needs UI exposure — no input string in the controller's token table yet)
- 📅 `e^(` exponential function
- 📅 Hyperbolic: `sinh cosh tanh` and inverses
- 💭 `logBASE(` for arbitrary base

### Number functions
- 📅 `abs(`
- 📅 `int(`, `iPart(`, `fPart(`
- 📅 `round(`
- 📅 `min(`, `max(`
- 📅 `mod(`, integer division
- 📅 Sign function
- 📅 `▶Frac` — convert last result to fraction (engine already alternates as ENTER via `processInput("▶Frac")`; UI access deleted with the legacy LOGIC popup, needs the future MATH menu)

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

- ✅ `=`, `≠`, `<`, `>` (engine; UI access pending — was in the legacy LOGIC popup, deleted 2026-04-07 with the rest of the legacy file)
- ✅ `≤`, `≥` (engine implemented; needs entries in the controller's token table to be reachable from the UI)
- ✅ `and`, `or`, `not` (engine; UI access pending — same situation as the comparators above)
- ✅ `xor` (engine implemented; same — needs UI exposure)
- 🔜 Logic operator menu in the new UI — replacement for the deleted legacy LOGIC popup

## Variables & storage

- 📅 26 single-letter scalar variables `A`–`Z`
- 📅 `STO→` store-to-variable key
- 📅 `Ans` (last answer) — auto-populated after every ENTER
- 📅 Last-entry recall (2nd + ENTER cycles backwards through history)
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
- 📅 Transpose `T`
- 📅 Inverse `^-1`
- 📅 Reduced row-echelon form `rref(`, row-echelon form `ref(`
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
- 🔜 Wire remaining soft-key actions: `ZOOM` and `TRACE` (still no-op)
- 📅 Trace mode (cursor walks along the function, shows coordinates)
- 📅 Tag function curves with their Y index in the canvas legend (currently inferred from result-list order — see [BUG-012](BUGS.md))
- 📅 Y-editor screen (visual list of Y1–Y9, Y0 with on/off toggles, styles)
- 📅 Function on/off toggling
- 📅 Function styles (thin, thick, dotted, shaded above/below, animate)
- 📅 Extend Y-editor to Y1–Y9 + Y0 (10 functions, TI-83 standard)

### Window settings
- ✅ Xmin/Xmax/Ymin/Ymax editable in the new UI's `WindowPopup` (reintegrated 2026-04-07; opened from the WINDOW soft-key)
- ✅ ZSTANDARD and ZOOM FIT actions wired to `uiController.resetViewport()` and `zoomFit()`
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
- 🔜 Wire `Ans` CalcKey — needs `Token::Ans` in `core_math/`
- 🔜 Wire `MATH` CalcKey — needs a MATH menu popup
- 🔜 Wire `MODE` CalcKey — needs a MODE menu popup
- 🔜 Wire `2ND` CalcKey — needs the modifier system
- 🔜 Wire `ALPHA` CalcKey — needs the modifier system (see [IMP-003](IMPROVEMENTS.md))

### Planned
- 📅 ALPHA modifier system — see [IMP-003](IMPROVEMENTS.md#imp-003-add-an-alpha-modifier-gate-to-single-letter-keyboard-shortcuts)
- 📅 2ND modifier system (yellow secondary functions on every key)
- 📅 Cursor movement within an expression (left/right arrow editing)
- 📅 Insert mode toggle (2nd + DEL)
- 📅 ALPHA-lock mode
- 📅 `MODE` menu (angle: Deg/Rad; display: Float/Sci/Eng/Fix; func/par/pol/seq; etc.)
- 📅 `CATALOG` browser (alphabetical list of every command)
- 📅 Mode indicator in the header (currently hardcoded "NORMAL  DEG")
- ✅ WINDOW popup ported to the new UI (see Graphing › Window settings)
- ✅ Matrix editor popup ported to the new UI (see Matrices)
- 📅 Logic operator menu ported to the new UI

### Considering
- 💭 Resizable window (currently fixed 720×760)
- 💭 Themes beyond Nord (light mode, high contrast, monochrome retro LCD)
- 💭 Touch input refinement
- 💭 On-screen 2nd/Alpha indicator badges over the keys

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

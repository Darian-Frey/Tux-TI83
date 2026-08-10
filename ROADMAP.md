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

**Phase C complete (2026-07-25).** Lists, statistics, and probability
are done end to end: lists `L1`–`L6` with a Stat editor; element-wise
arithmetic; list functions (`sum`/`prod`/`mean`/`median`/`min`/`max`/
`stdDev`/`variance`/`seq`); 1-Var and 2-Var stats; six regression models
(Lin/Quad/Cubic/Exp/Ln/Pwr); random functions (`rand`/`randInt`/
`randNorm`/`randBin`); and stat plots (scatter/xyLine/histogram/box).
**Phase D complete (2026-07-26)** too — graphing maturity: Trace,
Tables, the full Zoom menu, the FORMAT menu, the 10-function Y-editor
(Y1–Y0 with on/off + line styles), and the DRAW menu (Line/Circle/
Horizontal/Vertical/Pt-On/Text/ClrDraw with per-element delete).

Phases A, B, C, D are complete; E is largely done; F is partially done
(calculus + polar landed).

At this point the test suite stands at **419 passing / 0 open bugs**.

Candidate next areas (no commitment):
- **Phase D graphing maturity** — a full Y-editor (Y1–Y0 with on/off +
  styles), the rest of the Zoom menu, DRAW menu, Format menu.
- **Distributions** (`normalpdf(`, `normalcdf(`, `invNorm(`, …) — the
  natural follow-on to the stats work.
- **CATALOG** completeness, MEM menu, remaining MODE rows (each fronts a
  larger feature — parametric/sequence graphing, complex numbers,
  split-screen).

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
  examples. **Skeleton landed 2026-04-08**; **brought current 2026-08-01**
  — now documents all four graph modes, the full Y= editor, the complete
  MODE menu (incl. Plot/Complex/Screen), UI themes, complex numbers,
  probability distributions, typed matrix literals + the matrix/list
  toolkit, Y-VARS store, bracket entry, and named save snapshots. A few
  sections still flag *"planned"* content (screenshots, the
  function-reference appendix, more worked examples).
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
- ✅ `x²` shortcut key — wired in NUMERIC row 3 as "^ then 2" (Phase A); also `√` via 2ND+x²
- ✅ `nthroot(` — landed 2026-04-29 as binary infix `ˣ√` (2ND+^), see [IMP-025](IMPROVEMENTS.md)
- ✅ Implicit multiplication by juxtaposition (`2(3)`, `2π`) — landed 2026-05-09 as a preprocessing pass that injects `Token::ImplicitMul` between value-like token pairs, see [IMP-031](IMPROVEMENTS.md) (closes IMP-005)

### Transcendental
- ✅ `sin`, `cos`, `tan` (engine + UI keys in SCIENTIFIC section)
- ✅ `asin`, `acos`, `atan` (engine + `ERR:DOMAIN` for inputs outside `[-1, 1]`; fixed 2026-04-07) — UI exposure pending: best route is via 2ND modifier on the sin/cos/tan keys when the modifier system lands
- ✅ `log` (base 10), `ln` (both return `ERR:NONREAL ANS` for non-positive inputs; fixed 2026-04-07)
- ✅ `e` constant — engine + UI (SCIENTIFIC row 2 col 5, next to π; added 2026-04-07)
- ✅ `e^(` exponential function — dedicated `Exp` token (single first-class entry, matches the TI-83 keytop), engine + UI; complex-aware. See [IMP-041](IMPROVEMENTS.md).
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
- ✅ `sgn(` sign function — returns -1, 0, or +1; engine + UI. See [IMP-041](IMPROVEMENTS.md).

### Combinatorics
- ✅ `nCr(n, r)` — combinations, n choose r (engine + UI via MATH menu; added 2026-04-08). Requires 0 ≤ r ≤ n with both non-negative integers; `ERR:DOMAIN` otherwise.
- ✅ `nPr(n, r)` — permutations, n permute r (engine + UI via MATH menu; added 2026-04-08). Same domain rules as nCr.
- ✅ `!` (factorial) — unary postfix, engine + UI via MATH menu, keyboard shortcut `!` (added 2026-04-08). Accepts non-negative integers ≤ 170; returns `ERR:DOMAIN` otherwise. Binds tighter than `^` (so `2^3! = 2^(3!) = 64`).
- ✅ `UIController::formatScalar(double)` helper — centralised display formatting at 10-significant-digit precision, used by `evaluate`, `▶Dec`, and the test suite. Fixes an implicit bug where results ≥ 10⁶ displayed as scientific notation by default (`10! = 3628800` now renders as the integer, not `3.6288e+06`).

### Calculus
- ✅ Numeric integration `fnInt(expr, var, a, b)` — composite Simpson's rule, N=100 subintervals; nested calls supported via thread-local deferred side-table (IMP-044, 2026-05-25)
- ✅ Numeric derivative `nDeriv(expr, var, x [, h])` — symmetric finite difference, default h=0.001 (IMP-044, 2026-05-25)
- ✅ `sum(expr, var, start, end)` / `prod(expr, var, start, end)` — 4-arg form; integer iteration with 100k-call cap. Deviates from TI-83's list-based `sum(seq(...))` syntax pending Phase C lists (IMP-044, 2026-05-25)
- ✅ `seq(expr, var, start, end[, step])` — landed 2026-07-22 (Phase C Wave 3b) once list infrastructure existed. Returns a list; see Lists section. Enables the TI-83 `sum(seq(...))` summation form.
- 💭 Equation solver (Solver app)
- 💭 Symbolic operations (well beyond original TI-83 scope)

### Number systems
- ✅ Complex numbers (a+bi) — landed 2026-07-26 (Phase F #36). The operand/result value model gained an `imag` field (threaded through the stack, `Ans`, and results); the complex code path only activates when an imaginary part is present, so all real arithmetic is untouched. `i` unit + complex `+ − × ÷ ^` (exact integer powers via repeated multiplication) and negation; `conj(`/`real(`/`imag(`/`angle(` and complex `abs(` (magnitude); `√` of a negative → complex in a+bi mode (`ERR:NONREAL ANS` in Real mode). MODE → Complex row live (Real / a+bi / re^θi — the latter shows magnitude∠angle); `a+bi`/`re^θi` header indicator; MATH-menu + keyboard `i` entry; persisted. Complex trig/exp/log added 2026-07-26 (`sin`/`cos`/`tan`/inverse-trig/`e^(`/`ln`/`log`/hyperbolics on complex via `std::complex`, radian; Euler `e^(iπ) = -1` via a display-noise snap; `ln`/`log` of a negative real → complex in a+bi mode). Deferred: complex in vars/lists/matrices, negative-real fractional powers.
- ✅ Base conversion (DEC/HEX/OCT/BIN) — MODE-popup row; integer results display in the selected base, non-integers fall back to decimal (IMP-043, 2026-05-24)

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
- ✅ `Y-VARS` recall — `Y1`–`Y9` / `Y0` (all ten slots) reference the corresponding function buffer, in both bare (`Y5`) and explicit-argument (`Y5(3)`) forms; recursively evaluated at the current X with cycle detection (`ERR:RECURSION` on self-ref or mutual cycles), see [IMP-036](IMPROVEMENTS.md). Contiguous `Y1..Y0` / `Y1Call..Y0Call` token ranges map slot index via `t - Token::Y1`.
- ✅ `Y-VARS` store — `<expr>→Yn` from the home screen stores the LHS expression **tokens** into the Yn function buffer (so it plots and shows in the Y= editor), rather than evaluating to a number; reports `Done` when the target isn't the active slot, else echoes the stored expression. Handled in `UIController::evaluate()` ahead of the engine (the engine's Sto pass rejects a Y target). Added 2026-08-01. 📅 still: window/stat store variants (RegEq, statistical variables).
- ✅ Memory management — factory RESET button in the MODE popup clears every persisted + in-memory piece of state and removes state.json (see [IMP-038](IMPROVEMENTS.md)), **plus** a full MEM menu (`MEMPopup`, 2ND++) landed 2026-07-26: a live in-use summary (`memInfo()` → vars / matrices / lists / Y= funcs / history counts) and targeted clears — Clear All Lists / All Matrices / Vars A–Z / Entries — alongside the full RESET.
- ✅ Persistent storage across runs — session state (scalars, matrices, Y= buffers, viewport, MODE, TBLSET) is saved to `~/.local/state/tux-ti83/state.json` on clean exit and reloaded on launch; see [IMP-033](IMPROVEMENTS.md). Save-on-clean-exit only for now — periodic timer save would protect against crashes (follow-up).
- 💭 `DelVar` for explicit variable deletion

## Matrices

- ✅ Add, scalar multiply, matrix multiply
- ✅ Typed matrix literals + matrix store (`[[1,2][3,4]]`, `…→[A]`) — engine parses `[[…][…]]` literals (rows reuse the list-literal `MakeList` machinery, emitting `MakeMatrix`; ragged rows → `ERR:INVALID DIM`; elements may be expressions) and stores a matrix into `[A]`–`[J]` via `→` (scalar→matrix → `ERR:DATA TYPE`). Closes [IMP-011](IMPROVEMENTS.md): CLI/REPL can now enter matrices (`tux_ti83_cli 'det([[1,2][3,4]])'` → `-2`). Added 2026-08-01. GUI entry (2026-08-01): physical `[`/`]` (and `{`/`}`) keys are mapped; on the keypad `[`/`]` are **ALPHA+`(`/`)`** (grouping all three bracket pairs on the two bracket keys — primary `()` · 2ND `{}` · ALPHA `[]`; this replaced the on-screen ALPHA K/L, which stay typeable via the keyboard); and the MatrixPopup NAMES tab has `[`/`]` insert entries. Literals are now typeable in the GUI, not just CLI/REPL.
- ✅ Determinant `det(` — engine fully implemented; reachable from the new UI via the MATRIX popup's MATH tab (no dedicated keypad button yet)
- ✅ Registry `[A]`, `[B]`, `[C]` (UI exposure — engine actually supports `[A]`–`[J]`, just needs more entries in the controller's token table)
- ✅ 3×3 matrix editor reintegrated in the new UI as `MatrixPopup` (NAMES / MATH / EDIT tabs, opened from the new MATRX CalcKey in the SCIENTIFIC section; reintegrated 2026-04-07; current limitations tracked as [IMP-007](IMPROVEMENTS.md) and [IMP-008](IMPROVEMENTS.md))
- ✅ Matrix subtraction `[A] − [B]` (fixed 2026-04-07; returns `ERR:INVALID DIM` on mismatched dimensions)
- ✅ Transpose `T(` — unary matrix function; engine + UI via the MatrixPopup's MATH tab (added 2026-04-08). Swaps rows and columns; returns `ERR:DATA TYPE` for scalar input.
- ✅ Inverse `^-1` — TI-83 syntax; engine via Gauss-Jordan on the augmented `[A | I]` form; UI via MatrixPopup MATH tab entry "4: ^-1 (inverse)" which inserts the multi-token `^-1` sequence (added 2026-04-08). Non-square input returns `ERR:INVALID DIM`; singular matrices return `ERR:SINGULAR MAT`.
- ✅ Reduced row-echelon form `rref(` — shared row-reduction engine with inverse; UI via MatrixPopup MATH tab entry "3: rref(" (added 2026-04-08). Cells with magnitude < 1e-12 are clamped to zero so results don't display with floating-point noise.
- ✅ Row-echelon form `ref(` — unary matrix function; forward Gaussian elimination with partial pivoting and leading-1 pivots (upper-triangular echelon, no back-elimination — distinct from `rref(`). Engine + UI via MatrixPopup MATH tab; `ERR:DATA TYPE` for scalar input. Added 2026-07-31.
- ✅ `dim(`, `identity(`, `randM(` — engine + UI via MatrixPopup MATH tab (added 2026-07-31). `identity(n)` → n×n identity (integer 1–99, else `ERR:DOMAIN`); `dim(` → `{rows,cols}` list for a matrix or length scalar for a list (`ERR:DATA TYPE` on a scalar); `randM(r,c)` → r×c matrix of random ints in [-9,9] (shared seedable RNG; integer 1–99 dims else `ERR:DOMAIN`).
- ✅ `augment(` — binary; matrix‖matrix horizontal concat (equal rows, else `ERR:INVALID DIM`) or list‖list concatenation. Mixed matrix/list → `ERR:DATA TYPE`. Engine + UI via MatrixPopup MATH tab (added 2026-07-31).
- ✅ Matrix ↔ List conversion (`List▶Matr`, `Matr▶List`) — value-producing forms matching the engine's value+STO idiom. `List▶Matr(L1,…,Ln)` → m×n matrix with each list a column (**now variadic** — any number of lists; equal lengths else `ERR:INVALID DIM`; non-list arg → `ERR:DATA TYPE`); the arg count rides in the shunting-yard's variadic-paren counter (new `is_variadic_function`, mirroring `{…}`/`MakeList`). `Matr▶List([A],col)` → 1-based column as a list (out-of-range col → `ERR:INVALID DIM`). Store results with `→[C]` / `→Ln`. Engine + UI via MatrixPopup MATH tab (binary added 2026-08-01, variadic `List▶Matr` 2026-08-01). 📅 still: `Matr▶List` splitting **all** columns into several lists at once — needs multiple baked-in store targets, which doesn't fit the value-producing model (the `([A],col)` form covers single-column extraction).
- ✅ Variable matrix dimensions — matrix editor v2 supports 1×1 up to 6×6 via R/C steppers (was fixed 3×3); landed 2026-07-22 (IMP-007 + IMP-008). Grid cap is 6 (QML/popup-height pragmatism, not the TI-83 99×99 max).
- ✅ Extend UI registry exposure to `[A]`–`[E]` (matches TI-83 hardware default) — matrix editor v2 selector + NAMES tab + persistence now cover `[A]`–`[E]`; landed 2026-07-22 (IMP-007 + IMP-008)
- 💭 Extend UI registry exposure to all 10 (`[A]`–`[J]`, TI-83 Plus / TI-84 range; engine is already there — `matrixTokenForName` already maps A–J, just needs more selector/NAMES entries)

**Phase C in progress — Wave 1 (engine foundation) landed 2026-07-22.**
The list value type, `L1`–`L6` registry, `{…}` literals, element-wise
arithmetic, and `STO→` to a list are done in the engine and covered by
30 regression tests. UI exposure (`{`/`}` keys + Stat editor) is Wave 2.

- ✅ Lists `L1`–`L6` — engine registry + `{1,2,3}` literals + display; leaf resolution with `ERR:UNDEFINED` for unset slots. UI keys/editor pending (Wave 2). Landed 2026-07-22.
- ✅ List entry / editing UI (Stat editor) — Wave 2, landed 2026-07-22. `{`/`}` on 2ND+`(`/`)`, `L1`–`L6` on 2ND+`1`–`6`, and a `ListPopup` Stat editor (L1–L6 selector, length stepper 1–10, editable column, value read-back) opened via 2ND+`MATRX`. `L1`–`L6` persist in `state.json`. Editor length caps at 10 (UI pragmatism; the engine itself is unbounded).
- ✅ List arithmetic (vectorised ops) — element-wise `+ − × ÷ ^` with equal-length lists (`ERR:INVALID DIM` otherwise) and scalar broadcasting; implicit-mul (`2L1`, `2{1,2}`) works. Unary/binary math functions reject lists (`ERR:DATA TYPE`) pending Wave 3 element-wise mapping. Landed 2026-07-22.
- ✅ List functions: `sum(`, `prod(`, `mean(`, `min(`, `max(`, `stdDev(`, `variance(` — Wave 3a, landed 2026-07-22. `mean`/`stdDev`/`variance` are list-only (sample n−1 for stdDev/variance; `ERR:DOMAIN` for n<2). `sum(`/`prod(` overload the calculus 4-arg forms by arity (1 list arg → reduction); `min(`/`max(` overload the 2-scalar forms by operand type. In the MATH menu. Limitation: 2-arg `min(`/`max(` with a list operand (element-wise) not yet supported.
- ✅ `seq(expr, var, start, end[, step])` — Wave 3b, landed 2026-07-22. Reuses the deferred-eval framework (IMP-044) to sample the unevaluated first arg over the stepped range and collect a list. Default step 1; negative steps allowed; backwards range → `ERR:INVALID DIM`, zero step → `ERR:DOMAIN`. The authentic TI-83 `sum(seq(...))` summation form now works. Also added `median(` (Wave 3b).
- 📅 `median(` alongside 1-var stats — ✅ done early as a list reduction in Wave 3b (odd → middle, even → mean of the two middle values).
- ✅ List ↔ Matrix conversion — `List▶Matr(L1,…,Ln)` (variadic, n columns) / `Matr▶List([A],col)` value-producing forms landed 2026-08-01 (see the Matrices section). 📅 still: `Matr▶List` all-column split (needs multiple store targets).
- 📅 Custom named lists (`L1`–`L6` plus `αLIST`)

## Statistics & probability

- ✅ 1-variable stats — Wave 4a, landed 2026-07-22. `UIController::oneVarStats(list)` computes n, x̄, Σx, Σx², Sx (sample sd), σx (pop sd), minX, Q1, Med, Q3, maxX (TI-83 median-of-halves quartiles); shown in a `StatResultsPopup` opened from the Stat editor's **1-VAR STATS** button. Blank editor cells are skipped, so an untouched list reports `ERR:UNDEFINED`. Not yet: mode, and stat-variable recall (`x̄`, `Sx`, … as usable variables).
- ✅ 2-variable stats (correlation, regression coefficients) — Wave 4b, landed 2026-07-22. `UIController::twoVarStats(xList, yList)` computes n, x̄, ȳ, Σx, Σy, Σxy, Σx², Σy², Sx, Sy plus least-squares linear regression a (slope), b (intercept), r, r² (degenerate X → regression omitted, stats kept; unequal lengths → `ERR:INVALID DIM`). Shown in the shared `StatResultsPopup` (now a scrollable `mode`-driven list) via the Stat editor's **2-VAR L1,L2** button (L1=Xlist, L2=Ylist, TI-83 defaults).
- ✅ `rand`, `randInt(`, `randNorm(`, `randBin(` — Wave 5, landed 2026-07-25. `rand` is a bare value in [0,1); the others take 2 args (scalar) or 3 (a `count`, giving a list — resolved by an arg-counting rewrite pass). Backed by a shared `std::mt19937` (seedable via `MathStateMachine::seedRandom` for deterministic tests). Domain-checked (`randInt` lo≤hi, `randNorm` sd>0, `randBin` n≥0 & 0≤p≤1, list count 1–100000 → `ERR:DOMAIN`). In the MATH menu; composes with list functions (`mean(randInt(1,100,50))`).
- ✅ `nCr`, `nPr`, factorial `!` — see the Combinatorics section above (all implemented Phase B, 2026-04-08; engine + UI + keyboard `!`).
- ✅ Statistical regressions (core set) — `LinReg` (Wave 4b), plus `QuadReg`, `CubicReg`, `ExpReg`, `LnReg`, `PwrReg` (Wave 4c, landed 2026-07-22). `UIController::regression(type, xList, yList)`: Quad/Cubic solve the least-squares normal equations by Gaussian elimination (with R²); Exp/Ln/Pwr are linear fits on transformed data with domain checks (positive X/Y as required) and r/r². Picked from the **RegMenuPopup** (REGRESSIONS ▸ in the Stat editor), shown in `StatResultsPopup`'s "reg" mode. `polyReg` already supports degree 4, so `QuartReg` is a one-line menu add when wanted. Still 📅: `QuartReg` (unexposed), `SinReg`, `Logistic`.
- ✅ Distributions — **all three families done** (2026-07-26).
  - Normal (`normalpdf(`, `normalcdf(`, `invNorm(`). Optional μ/σ (default 0/1) padded to fixed arity by `rewriteDistCalls` (recurses for nested calls); `normalcdf` via `std::erf`, `invNorm` via Acklam's inverse-normal approximation.
  - Discrete (`binompdf(`, `binomcdf(`, `poissonpdf(`, `poissoncdf(`, `geometpdf(`, `geometcdf(`). Closed-form via a stable multiplicative binomial-coefficient helper; `binompdf`/`binomcdf` also have the 2-arg whole-distribution list form (`binompdf(n,p)` → x=0..n), selected by `rewriteBinomCalls`. Sums/lists capped at 100000.
  - Continuous (`tpdf(`, `tcdf(`, `χ²pdf(`, `χ²cdf(`, `Fpdf(`, `Fcdf(`). CDFs use the regularized incomplete gamma (χ², NR series/continued-fraction) and incomplete beta (t, F, Lentz continued fraction). `χ²` also accepts the ASCII alias `chi2`.
  - All in the MATH menu, domain-checked (σ/df params > 0, probabilities in range → `ERR:DOMAIN`).
- ✅ Stat plots (scatter, xy-line, histogram, box plot) — Wave 5b, landed 2026-07-25. A single Plot1 (`UIController::getStatPlotData()` → render-ready data), configured in the `StatPlotPopup` (2ND+Y=): on/off, type, Xlist/Ylist. Rendered on the graph canvas over any function curves — scatter/xyLine as points (xyLine x-sorted), histogram as auto-binned frequency bars, box plot as a five-number box-and-whisker. Persisted in `state.json`. **This completes Phase C.**

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
- ✅ Y-editor screen — `YEditorPopup` on the Y= soft-key (Phase D, 2026-07-26): a scrollable list of all 10 slots (`Yn = expr`, per-slot on/off toggle, line-style cycle), each `Yn` shown in its curve colour; tapping a row's expression makes that slot active for keypad editing.
- ✅ Function on/off toggling — per-slot `enabled` flag; `getMultiGraphPoints` skips disabled slots; persisted.
- ✅ Function styles — thin / thick / dotted, per slot, applied on the canvas (line width + dash); persisted. (Shaded above/below and animate not implemented.)
- ✅ Extend Y-editor to Y1–Y9 + Y0 (10 functions) — buffers/display-strings/enabled/style all sized to 10; a shared 10-colour palette (`Style.graphColors`) covers the canvas, trace, and editor. Cross-*referencing* Y4–Y0 inside another expression now works too (engine tokens `Y4..Y0` / `Y4Call..Y0Call`; keyboard `Y`+digit fuse extended to 0–9).

### Window settings
- ✅ Xmin/Xmax/Ymin/Ymax editable in the new UI's `WindowPopup` (reintegrated 2026-04-07; opened from the WINDOW soft-key)
- ✅ ZSTANDARD and ZOOM FIT actions wired to `uiController.resetViewport()` and `zoomFit()`
- ✅ ZOOM soft-key opens a `ZoomPopup` with ZStandard / Zoom In / Zoom Out / ZFit presets (2026-05-08)
- ✅ Xscl/Yscl (axis tick spacing) — `xScl`/`yScl` controller properties (default 1) drive the grid-line / axis tick-mark interval in GraphCanvas, with a fallback to a zoom heuristic for zero/negative/absurdly-dense values; editable via WINDOW; persisted; reset to 1 by ZSTD. Added 2026-08-01.
- ✅ Xres (graph resolution / x-step) — `xres` controller property (default 1, clamped 1–8) coarsens the Func-mode sample stride (`funcRes = resolution / xres`); editable via WINDOW (integer field); persisted. Applies to Func mode only (polar/parametric use their own T-window step). Added 2026-08-01.

### Tables

- ✅ `TABLE` view — function values for a sequence of x's, opened via 2ND+GRAPH; scrollable X | Y1 | Y2 | Y3 with ↑/↓ stepping; see [IMP-035](IMPROVEMENTS.md)
- ✅ `TBLSET` (start, step) — popup opened via 2ND+WINDOW; auto/ask not yet implemented (currently auto-only)

### Zoom menu (full TI-83 set) — **complete (2026-07-26)**
All 13 entries in the `ZoomPopup` (scrollable):
- ✅ ZStandard, ZoomFit
- ✅ Zoom In, Zoom Out, ZSquare, ZTrig, ZDecimal, ZInteger (IMP-039)
- ✅ ZBox (drag a rubber-band rectangle on the canvas → `armZoomBox`/`zoomBox`) — Phase D
- ✅ ZoomStat (fit the stat-plot Xlist/Ylist with 10% padding) — Phase D
- ✅ ZoomPrevious (swap with the pre-zoom window; the popup snapshots before each menu zoom) — Phase D
- ✅ ZoomMemory (ZoomSto / ZoomRcl — store & recall a window, persisted in state.json) — Phase D

### DRAW menu — **core set landed 2026-07-26** (Phase D)

`DRAWPopup` on 2ND+TRACE (TI-83 uses 2ND+PRGM, which this keypad lacks):
pick a command, fill in the (data-coordinate) arg fields, DRAW adds a
persistent overlay. Overlays are stored as `QVariantMap`s, rendered by
the canvas over the curves (circles as a 60-point polygon so they're
true circles in data coords), and persisted in `state.json`. The popup
lists the current drawings with a per-item ✕ delete; CLRDRAW clears all.

- ✅ `Pt-On(` — point marker. (Pt-Off/Pt-Change deferred.)
- ✅ `Line(`, `Horizontal`, `Vertical`
- ✅ `Circle(`
- ✅ `Text(` (overlay text on graph)
- ✅ `ClrDraw` + per-element delete (delete one / clear all)
- 📅 `Tangent(`, `Pen` (freehand), `Shade(`, `DrawF`, `DrawInv` — deferred

### Format menu

**FORMAT menu landed 2026-07-26** (Phase D slice 1) — `FormatPopup` on
2ND+ZOOM, four persisted flags (`gridOn`/`axesOn`/`coordOn`/`labelOn`,
default on) gating the graph canvas. RESET restores them.

- ✅ `CoordOn` / `CoordOff` — toggles the trace coordinate readout (crosshair still shown).
- ✅ `GridOn` / `GridOff` — toggles the grid lines.
- ✅ `AxesOn` / `AxesOff` — toggles the x/y = 0 axis lines.
- ✅ `LabelOn` / `LabelOff` — toggles the tick-number labels.
- ✅ `RectGC` / `PolarGC` (rectangular vs polar coords for the cursor) — `coordMode` FORMAT flag (0 Rect / 1 Polar). The trace readout shows `X=/Y=` in RectGC and `R=/θ=` in PolarGC (θ in the current angle unit); toggled via the FORMAT menu "GC" row; persisted. Added 2026-08-01.
- ✅ `ExprOn` / `ExprOff` (show the expression while tracing) — `exprOn` FORMAT flag (default on) draws the traced function's `label=body` (e.g. `Y1=X²`) top-left while tracing; toggled via the FORMAT menu "Expr" row; persisted. Added 2026-08-01.

### Other graphing modes
- ✅ Parametric mode (X1T, Y1T) — landed 2026-07-26 (Phase F). MODE → Graph → Par; the 10 function buffers are read as 5 X/Y pairs (X1T,Y1T,X2T,Y2T,…) and each pair plots `(X_nT(t), Y_nT(t))` over a full-turn sweep. Reuses the graph pipeline and the Y-editor; `X` stands in for the parameter `t` (like polar's θ — no core_math change). Slot labels adapt everywhere; `PAR` header indicator; persisted. **Tmin/Tmax/Tstep window** added 2026-07-26 (shared `param*` settings, in the WINDOW popup; reset to the angle-appropriate full turn on angle-mode change). Caveat: X-as-t rather than a dedicated `T` token.
- ✅ Polar mode (r1, r2, r3) — landed 2026-07-22. MODE → Graph → Pol; each function buffer is read as `r = f(θ)` and rendered on the shared canvas via `(r,θ)→(x,y)`. Angle-unit-aware (radian/degree). Reuses the Func-mode render/pan/zoom pipeline; `r1/r2/r3` selector + `POL` header indicator + persisted `graphMode`. **θmin/θmax/θstep window** added 2026-07-26 (shares the parametric `param*` settings, shown as θ in the WINDOW popup). Caveats: the angle variable is entered as `X` (no dedicated `θ` token — avoids a core_math change), and trace isn't polar-aware yet.
- ✅ Sequence mode (u(n), v(n), w(n)) — landed 2026-07-26 (Phase F). MODE → Graph → Seq; slots 0/1/2 are u/v/w, plotted as (n, value). `X` stands in for n and `Ans` for the previous term u(n−1); the engine auto-detects recursion (buffer contains `Ans`) — explicit sequences evaluate directly, recursive ones seed u(nMin) from an initial value and iterate. nMax + u/v/w(nMin) initial values live in the WINDOW popup (Seq mode only); nMin fixed at 1; `SEQ` header indicator; persisted. Caveats: one previous term only (`Ans` = u(n−1), so two-term recurrences like Fibonacci aren't directly expressible), nMin fixed at 1, three sequences.

  **All four graph modes (Func / Par / Pol / Seq) are now implemented.**
- 💭 Inequality shading (Inequalz app on TI-83 Plus)

## UI / UX

### Done
- ✅ Nord palette extended with semantic roles (operator/enter/second/numeric/function categories)
- ✅ `Style` singleton — single source of truth for colours, sizes, fonts
- ✅ UI themes — Dark / Light / Amber (orange-on-black terminal), MODE → Theme, persisted (added 2026-08-01). `Style` is palette-indexed by `theme` (bound to `uiController.theme`); every body colour reads the active palette. The LCD *panel* stays dark in all themes (authentic screen) but its text is themed. LCD-drawn elements use theme-independent dark tones (`gridLine`, `lcdOverlay`) so they don't glare on the dark screen in the light theme. Graph curve palette is shared across themes for distinctness.
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
- 🚧 `MODE` menu follow-ups — Angle, Notation (Normal/Sci/Eng), Decimal (Float/Fix N), Base (Dec/Hex/Oct/Bin), Draw (Connected/Dot), **Graph: Func/Par/Pol/Seq (all four wired)**, **Complex: Real/a+bi/re^θi (wired)**, and **Plot: Sequential/Simul (wired 2026-08-01)** are done — Graph rows back `setGraphMode`, Complex backs `setComplexMode`, Plot backs `plotMode` (drives the GraphCanvas draw animation: Sequential reveals each curve fully then the next, Simul advances all curves in lockstep; ~0.4s sweep on entering the graph / editing functions / switching Plot; persisted). Remaining placeholder, still deliberately greyed: Screen (see below).
- ✅ `MODE` → **Screen: Full / Horiz / G-T** (split-screen) — `screenMode` property (0 Full / 1 Horiz / 2 G-T), wired + persisted (added 2026-08-01). The main view region was restructured from a `StackLayout` into a `ColumnLayout` with a graph+table `RowLayout` above the keypad: Full shows the active view alone; **G-T** shows graph beside table; **Horiz** shows the graph on top with the keypad below (our "home" is the keypad, so Horiz keeps the keys usable while graphing rather than a TI-style home-entry strip). Keypad fills only when it's the sole content; in Horiz it takes its natural height with the graph filling above.
- ✅ `CATALOG` browser (alphabetical list of every command) — `CatalogPopup` (2ND+0), auto-generated from `UIController::catalogEntries()` (deduped + sorted from the token table), with an incremental search field. Because it's token-table-driven it stays complete automatically — every function added since (lists, stats, regressions, random, distributions, …) is already listed.
- ✅ Mode indicator in the header — dynamic, binds to `notation` / `fixDecimals` / `angleMode`. Renders e.g. `NORMAL  RAD` (defaults) or `SCI  FIX 2  DEG`; see [IMP-034](IMPROVEMENTS.md).
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

The last major unbuilt feature and the capability the project is named
after. It's a whole statement-level **interpreter** layered above the
single-expression evaluator. Full design + phased plan lives in
**[docs/TIBASIC.md](docs/TIBASIC.md)**; scope decision (2026-08-01):
_pragmatic subset first_. Phases (each independently shippable):

- 🚧 **P0 — Scaffolding** — core landed 2026-08-01: new pure-C++
  `interpreter` library with the `Interpreter` (RunStatus step loop,
  statement splitter, run-to-Done) and `ProgramStore`; 21 tests. Persistence
  wiring deferred to P1 (nothing to persist until the editor exists).
- ✅ **P1 — Program editor** — landed 2026-08-01: PRGM popup (2ND+`√(`) with
  list (RUN/EDIT/✕/NEW) + a freeform multi-line source editor; `ProgramStore`
  in the controller (CRUD + `runProgram`); programs persist in state JSON;
  16 tests.
- ✅ **P2 — Sequential core** — landed 2026-08-01: statement dispatch (bare
  expr, `Sto`, `Disp` incl. multi-arg + string literals, `ClrHome`, `Stop`)
  via an injected evaluator (reuses the tokeniser/`MathStateMachine`, shares
  registries with the home screen); a `PrgmRunPopup` output view; errors
  report the line. 8 tests. *(First milestone P0–P2 reached: author + run +
  `Disp`.)* `Pause` deferred to P4 (shares the resumable-input UI).
- ✅ **P3 — Control flow** — landed 2026-08-01: `If` (single + `Then`/`Else`/
  `End`), `For(` (asc/step/desc), `While`, `Repeat`, `Lbl`/`Goto`. Structural
  pre-pass jump table + For-frame stack; `execStatement` owns the PC; a 5M-step
  runaway guard. Unary-minus fix in the program evaluator. 13 tests.
- 🚧 **P4 — Strings + I/O** — split in two. **Interaction ✅ (2026-08-01):**
  `Input`/`Prompt`/`Pause` via the resumable step model (`NeedInput`/`NeedKey`);
  the controller holds the interpreter and drives `runProgram`/
  `provideProgramInput`/`resumeProgram`; run view gets an input field, a
  CONTINUE button, and a ◀ PRGM back button. Works across loops. 12 tests.
  **String type ✅ (2026-08-01):** `Str1`–`Str9` vars, `"…"` literals, concat
  (`+`), string store, `Disp`/text `Input` of strings — interpreter-level
  (engine untouched); quote-aware statement/arg splitting; 10 tests.
  **String functions ✅ (2026-08-08):** `length(`/`sub(`/`inString(`/`expr(`,
  resolved interpreter-level (innermost-first substitution via
  `resolveStrFuncs`; numeric eval routes through `mEval`); compose + nest;
  `sub(` out of range → `ERR:DOMAIN`. 13 tests. **Str-var disk-persistence ✅
  (2026-08-08):** `Str1`–`Str9` serialise into state JSON (`"strings"`) so they
  survive a restart. 1 test. **`Output(row,col,value)` ✅ (2026-08-08):**
  positioned text on the home-screen grid (rows 1–8, cols 1–16; `ERR:DOMAIN`
  out of range), interpreter-level (`placeOutput` space-pads into the line
  buffer — no UI change; the run view is already monospace). 10 tests.
  **`Menu(` ✅ (2026-08-08):** pause-and-branch menu (`NeedMenu` state; run
  view shows title + numbered option buttons; picking one jumps to its
  `Lbl`). 7 tests. **P4 complete.**
- **P5a — Program control (sub-calls) ✅ (2026-08-06)** — `prgmNAME`
  sub-program calls (call stack, depth cap → `ERR:MEMORY`), `Return`,
  `DelVar`; a missing sub-program → `ERR:UNDEFINED`. Plus a COPY-output
  button on the program run view. 11 tests.
- **P5b-1 — Break / interrupt ✅ (2026-08-07)** — a running program is
  interruptible instead of blocking the UI: the interpreter runs in bounded
  slices (`runSlice`), the controller pumps events between them so a ■ STOP
  button stays live, and `interrupt()` ends the run with `ERR:BREAK`; 5M
  guard kept as a headless backstop. 9 tests.
- **P5b-2 — getKey ✅ (2026-08-08)** — non-blocking key poll; the run loop is
  now time-bounded (~8 ms slices) with live Disp refresh, and the runaway
  guard is headless-only so interactive loops aren't cut off. Physical keys →
  TI-83 codes via `sendProgramKey()`. Fixed BUG-025 (stores echoed → flooded
  getKey loops; now silent, TI-style). 6 tests.
- **P5b-3 — Error jump-to-line ✅ (2026-08-08)** — the interpreter maps each
  flattened statement to its editor source line and tracks the current
  program name (across `prgm` calls), so a runtime error reports the true
  line (even through `:`-chains) and the run view's ✎ EDIT LINE button opens
  the editor there with the line highlighted. 6 tests.
- **P5b-4 — In-editor command-paste menu ✅ (2026-08-09)** — a ⌨ COMMANDS
  palette in the PRGM editor (CTL / I/O / STR / FN tabs) inserts keywords at
  the cursor (`→`, `√(`, `Disp`, `For(`, `Menu(`, `""` with cursor inside, …)
  so they don't have to be hand-typed. QML-only. **P5 complete — the TI-BASIC
  subset is feature-complete.**
- **P6-1 — Program-driven graphs ✅ (2026-08-09)** — programs drive the graph
  engine via an injected graph sink: `"X²"→Y1` / `X²→Y1` (function store),
  window vars (`Xmax`/`Ymin`/…), `FnOn`/`FnOff`, `ZStandard`/`ZoomFit`, and
  `DispGraph` (closes the run view → shows the plot). 10 tests.
- **P6-2 — Draw overlay ✅ (2026-08-09)** — programs draw on the graph via the
  existing DRAW layer: `Line(`/`Circle(`/`Horizontal`/`Vertical`/`Pt-On(`/
  `Text(`/`ClrDraw` (graphics commands auto-show the graph). Added a ✕ CLR
  button on `GraphCanvas` (shown when overlays exist) to clear all drawings.
  9 tests. **P6 graphics complete.** 💭 `.8xp` import remains (optional).
- **P7 — Modern language enhancements** (make TI-BASIC genuinely better than
  the original — see [docs/TIBASIC.md](docs/TIBASIC.md)):
  - ✅ **Comments** (`#` to EOL) — 2026-08-10
  - ✅ **`break` / `continue`** (exit / skip the innermost loop) — 2026-08-10
  - ✅ **Editor syntax highlighting** (`ProgramHighlighter`: keywords / vars /
    strings / numbers / comments) — 2026-08-10
  - ✅ **List/matrix element access + assignment** (2026-08-10) — `L1(3)` /
    `[A](r,c)` reads, `5→L1(3)` / `9→[A](r,c)` writes; computed indices, append
    at `dim+1`. Controller-level (`resolveElementReads`/`tryElementStore`),
    core_math untouched. 10 tests.
  - 📅 **`SortA(` / `SortD(`** — sort a list in place (remaining bit of A1)
  - ✅ **Local variables** (2026-08-10) — `Local A,B,…` save/zero/restore per
    frame, so a sub-program can't clobber the caller's globals. 4 tests.
  - ✅ **User functions** (2026-08-10) — multi-statement `Define f(A,B) …
    Return expr … End`, called as `f(3,4)` in any expression (nesting +
    recursion). Body registered via a define-sink into `m_userFuncs`;
    `resolveUserFunctions` substitutes calls; `callUserFunction` binds params
    and runs the body in a nested interpreter. 6 tests.
  - 📅 **Pixel graphics** — `Pxl-On(`/`Pxl-Off(`/`Pxl-Test(`, `Pt-Off(`,
    `Pt-Change(`
  - 📅 **`StorePic`/`RecallPic`, `Shade(`, `Tangent(`, `DrawF`**
  - 📅 **`toString(`** (number → string)
  - 📅 **Local variables + user functions** (params/return — the big one)
  - 📅 **Error trapping** (`try`-style recovery)

## Connectivity & data exchange

- ✅ Save/load full calculator state to disk — landed 2026-07-26 (Phase F #34). Named snapshots (`~/.local/state/tux-ti83/saves/<name>.t83`, JSON) via `UIController::exportState`/`importState`/`listSaves`/`deleteSave`, sharing the auto-state serialization through refactored `buildStateJson`/`applyStateJson`. Managed from a SAVE/LOAD section in the MEM menu (name field + Export; per-save Load / ✕ delete). This is the automatic `state.json` plus explicit user-named saves.
- 💭 Export/import individual variables/lists/matrices as files (whole-state snapshots done above; per-object export is the remaining bit)
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
23. Y-editor with 10 functions, on/off, styles ✅
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
34. Save/load ✅
35. Parametric / polar / sequence modes ✅ (all four graph modes done)
36. Complex numbers ✅

# BUGS.md — Tux-TI83

Catalogue of bugs discovered during development. Per the project workflow, bugs
are **logged here when found, not silently fixed**. The user decides whether to
fix immediately, defer, or leave alone.

## Format

Each entry uses this template:

```
### BUG-NNN: <short title>
- **Status:** open | fixed | wontfix | deferred
- **Found:** YYYY-MM-DD (session/commit context)
- **Location:** path/to/file.ext:line
- **Severity:** low | medium | high
- **Description:** what's wrong
- **Reproduction:** how to trigger it (if known)
- **Notes:** related context, suggested fix, links
```

---

## Open

### BUG-012: Graph curve colours can shift when some Y slots are empty
- **Status:** open
- **Found:** 2026-04-07 (Phase A — graph mode reintegration)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) `getMultiGraphPoints()` (the `continue` on empty buffers) and [app/qml/components/GraphCanvas.qml](app/qml/components/GraphCanvas.qml) (the `fnColors[f % length]` indexing)
- **Severity:** low (cosmetic / confusing, not incorrect math)
- **Description:** `getMultiGraphPoints()` skips empty function buffers
  with a `continue`, so the returned list is compacted. The QML canvas
  iterates the result and uses the result index for colour selection.
  As a result, if Y1 is empty and Y3 is defined, Y3's curve renders in
  Y1's colour (blue), making it impossible to tell which function is
  which when slots are non-contiguous.
- **Reproduction:** Set Y1 empty, set Y3 to `X^2`, switch to graph mode.
  The parabola renders in Y1's blue colour even though it's stored in
  Y3.
- **Notes:** Fix has two reasonable shapes:
  1. Make `getMultiGraphPoints()` return a fixed-length list with empty
     entries for empty buffers, preserving function index. (Small
     controller change.)
  2. Have `getMultiGraphPoints()` return tagged points
     (`{index, points}` pairs) so QML knows which Y is which.
  Option 1 is the smaller change. Pre-existing in the legacy UI; not a
  regression introduced by the reintegration.

---

## Fixed

### BUG-001: Window popup TextField writes NaN to viewport on bad input
- **Status:** fixed (2026-04-06, Step 4)
- **Location:** [graph_ui/qml/Main.qml:213-228](graph_ui/qml/Main.qml#L213-L228)
- **Severity:** medium
- **Description:** The WINDOW popup `TextField` for `xMin/xMax/yMin/yMax`
  committed with `uiController[modelData.p] = parseFloat(text)`. If the
  field was empty or contained non-numeric input, `parseFloat` returned
  `NaN`, which was then written into the viewport bound. Downstream graph
  rendering and zoom math would silently break (blank canvas, frozen pan).
- **Fix:** `onEditingFinished` now guards with `Number.isFinite(v)` before
  assigning. On invalid input the field reverts to the current viewport
  value rather than committing NaN. Note: the file is no longer loaded by
  the new UI (Step 2 switched to `qrc:/App/Main.qml`), so this fix is
  hygiene for the legacy QML file in case it's reused or referenced.

### BUG-002: `√(` key shows only `√` on the display string
- **Status:** fixed (2026-04-06, Step 4)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) (token-input formatting branch in `processInput`)
- **Severity:** low
- **Description:** `processInput("√")` correctly pushed `Token::Sqrt` into
  the buffer, but the display-string branch only added `"("` for inputs
  with `length() > 1`. `"√"` is a single QChar (U+221A), so it fell through
  and rendered as `√4)` instead of `√(4)`. Token-level evaluation was
  unaffected.
- **Fix:** Added an explicit `else if (input == "√") currentStr += "√(";`
  branch ahead of the length check. The structural cleanup (a single
  data-driven token table that would prevent this class of bug) is logged
  as IMP-001 in IMPROVEMENTS.md.

### BUG-003: After ENTER, controller leaves buffer/state dirty for the next keypress
- **Status:** fixed (2026-04-06, Step 4)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) (`processInput`, state machine block)
- **Severity:** medium
- **Description:** Before the fix, ENTER overwrote `currentStr` with the
  result but left `currentBuf` populated with the original expression
  tokens. The next key press appended to *both* — display became
  `"<result><key>"` while the token buffer became
  `[<old tokens>, <new token>]`. Display and evaluation state diverged
  silently.
- **Fix:** Closed as a side effect of implementing the CLAUDE.md display
  state machine. The controller now exposes a `DisplayState` enum
  (Inputting / Evaluated / Error) and a `displayExpression` property.
  When a token input arrives while state is not Inputting, the controller
  clears `currentBuf` + `currentStr`, transitions back to Inputting, and
  *then* appends the new token. This implements the spec's "next
  digit/function keypress: clears expr, returns to INPUTTING" rule and
  closes the divergence.
- **Notes:** The spec also says "next operator keypress: appends to result
  value, returns to INPUTTING" — that variant is **not** implemented yet.
  Currently any keypress after eval clears, regardless of operator vs digit.
  Reintroducing the operator-appends behaviour would require encoding the
  result back into the token buffer (fragile for fractions, matrices), so
  it's deferred until we have a need or a clean approach.

### BUG-004: `asin` / `acos` / `atan` are no-ops in the evaluator
- **Status:** fixed (2026-04-07, Group A engine cleanup)
- **Location:** [core_math/src/core_math.cpp](core_math/src/core_math.cpp) (function-dispatch chain in `MathStateMachine::evaluate`)
- **Severity:** medium
- **Description:** `is_function` recognised ASin/ACos/ATan but the
  unary-function dispatch chain had no else-if branches for them. The
  function fired, popped its argument, and pushed it back unchanged.
  `asin(0.5)` returned `0.5` instead of `≈0.524`.
- **Fix:** Added explicit branches for `Token::ASin`, `Token::ACos`,
  `Token::ATan` calling `std::asin`, `std::acos`, `std::atan`. ASin
  and ACos return `DOMAIN` error for inputs outside `[-1, 1]`. ATan
  accepts all reals. The error string is propagated to the UI as
  `ERR:DOMAIN` via [IMP-006](IMPROVEMENTS.md).

### BUG-005: Division by zero silently returns 0
- **Status:** fixed (2026-04-07, Group A engine cleanup)
- **Location:** [core_math/src/core_math.cpp](core_math/src/core_math.cpp) (`Token::Div` branch in `MathStateMachine::evaluate`)
- **Severity:** medium
- **Description:** The Div branch had `b.val == 0 ? 0.0 : a.val / b.val`,
  silently masking division-by-zero with a zero result.
- **Fix:** Now returns `CalculationResult{success=false,
  error_message="DIVIDE BY 0"}` when the divisor is zero. Surfaced in
  the UI as `ERR:DIVIDE BY 0` via [IMP-006](IMPROVEMENTS.md).

### BUG-006: `√` of a negative silently returns 0
- **Status:** fixed (2026-04-07, Group A engine cleanup)
- **Location:** [core_math/src/core_math.cpp](core_math/src/core_math.cpp) (`Token::Sqrt` branch in the function-dispatch chain)
- **Severity:** low
- **Description:** `(v >= 0) ? std::sqrt(v) : 0.0` silently returned 0
  for negative inputs.
- **Fix:** Now returns `error_message="NONREAL ANS"` when `v < 0`.
  Surfaced as `ERR:NONREAL ANS` via [IMP-006](IMPROVEMENTS.md).

### BUG-007: `log` and `ln` of non-positive silently return `-HUGE_VAL`
- **Status:** fixed (2026-04-07, Group A engine cleanup)
- **Location:** [core_math/src/core_math.cpp](core_math/src/core_math.cpp) (`Token::Log` and `Token::Ln` branches)
- **Severity:** low
- **Description:** `(v > 0) ? std::log10(v) : -HUGE_VAL` (and the same
  for Ln) silently returned a giant negative number for non-positive
  inputs, which then formatted as scientific notation in the display.
- **Fix:** Both branches now return `error_message="NONREAL ANS"` when
  `v <= 0`. Surfaced as `ERR:NONREAL ANS` via [IMP-006](IMPROVEMENTS.md).

### BUG-008: Matrix subtraction not supported
- **Status:** fixed (2026-04-07, Group A engine cleanup)
- **Location:** [core_math/src/core_math.cpp](core_math/src/core_math.cpp) (`Token::Sub` branch + new `matrixSub` helper)
- **Severity:** low (feature gap)
- **Description:** The `Sub` branch only handled scalar-scalar
  subtraction. For two matrices, it fell through to `Type Error`.
- **Fix:** Added a `matrixSub` helper mirroring `matrixAdd`, plus an
  `a.isMat && b.isMat` branch in the Sub case that checks dimensions
  (returning `ERR:INVALID DIM` on mismatch via
  [IMP-006](IMPROVEMENTS.md)) and pushes the result.

### BUG-010: Matrix addition with mismatched dimensions silently returns an empty matrix
- **Status:** fixed (2026-04-07, immediately after Group A engine cleanup)
- **Location:** [core_math/src/core_math.cpp](core_math/src/core_math.cpp) (`Token::Add` branch in `MathStateMachine::evaluate`; `matrixAdd` helper)
- **Severity:** medium
- **Description:** When two matrices with mismatched dimensions were
  added, `matrixAdd` returned an empty `Matrix{}` (rows = cols = 0)
  rather than signalling an error. The empty matrix was then pushed to
  the operand stack as a successful "matrix result" and the user saw
  `[[]]` displayed with no indication that anything had gone wrong.
- **Reproduction:** Define `[A]` as 2×2 and `[B]` as 3×3 (via the matrix
  editor), then evaluate `[A]+[B]` → display showed `[[]]`.
- **Fix:** The `Token::Add` branch now checks
  `a.mat.rows != b.mat.rows || a.mat.cols != b.mat.cols` before calling
  `matrixAdd` and returns `error_message="Dim Mismatch"`, which IMP-006
  surfaces as `ERR:INVALID DIM`. The defensive guard in `matrixAdd`
  itself is left in place — it's now unreachable but still cheap.
- **Notes:** Found while reviewing the BUG-008 fix in the same session.
  Same fix shape, same root pattern: helper returns `{}` on bad input,
  caller doesn't check, empty result silently propagates.

### BUG-011: Matrix multiplication with non-conformable dimensions silently returns an empty matrix
- **Status:** fixed (2026-04-07, immediately after Group A engine cleanup)
- **Location:** [core_math/src/core_math.cpp](core_math/src/core_math.cpp) (`Token::Mul` branch in `MathStateMachine::evaluate`; `matrixMul` helper)
- **Severity:** medium
- **Description:** Same shape as BUG-010 but for matrix multiplication.
  When `a.cols != b.rows` (matrices not conformable for multiplication),
  `matrixMul` returned an empty `Matrix{}` and the empty result was
  pushed silently as a success.
- **Reproduction:** Define `[A]` as 2×3 and `[B]` as 2×3 (not
  conformable, since `3 != 2`), evaluate `[A]×[B]` → display showed
  `[[]]`.
- **Fix:** The `Token::Mul` branch now checks `a.mat.cols != b.mat.rows`
  before calling `matrixMul` and returns `error_message="Dim Mismatch"`,
  surfaced as `ERR:INVALID DIM` via IMP-006. The defensive guard in
  `matrixMul` is left in place.
- **Notes:** Found alongside BUG-010 in the same review pass. Both
  fixed together.

### BUG-014: Unary negation is inert — `(-)` key does nothing, `abs(-5)` errors
- **Status:** fixed (2026-04-07, user-reported during Phase B Wave 1 testing)
- **Location:** [core_math/src/core_math.cpp](core_math/src/core_math.cpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/Main.qml](app/qml/Main.qml)
- **Severity:** high (blocked basic use — any expression with a unary
  minus errored, including `abs(-5)`, `int(-3.7)`, `iPart(-3.7)`,
  `fPart(-3.7)`, `−3²`, etc.)
- **Description:** Two related problems:
  1. The on-screen `(-)` CalcKey sent `"-"` (plain hyphen) through
     `processInput`, but the controller's token table had no entry for
     `"-"` — the keypress was silently dropped and nothing appeared on
     the display.
  2. The keyboard `-` key correctly mapped to `−` (binary Sub). Typing
     something like `abs(-5)` via keyboard produced a token sequence
     `[Abs, (, Sub, 5, )]`, which parsed as "abs of (subtract 5 from
     nothing)" — a syntax error with no left operand for Sub.
  A TI-83's solution is two physically different keys (`−` and `(-)`)
  with two internally distinct tokens. We had the physical distinction
  on the UI but not the token-level distinction.
- **Reproduction:** `abs(-5)` ENTER → `ERR:SYNTAX`. `int(-3.7)` ENTER →
  `ERR:SYNTAX`. See history in the user's screenshot from Phase B Wave 1
  testing.
- **Fix:** landed in two passes within the same session:
  - Pass 1: added `Token::Neg` to the enum in `capsule_math.hpp`;
    registered `Neg` in `is_function()` (so the shunting-yard pushes
    it like a unary prefix function) with precedence 2 — same family
    as Mul/Div, but `is_function` keeps it from popping prior operators
    of higher precedence. Gives the correct TI-83 groupings:
    `−3^2 = −9` (Pow binds tighter), `−3*4 = −12`, `3*−4 = −12`.
    Added a unary dispatch case `else if (t == Token::Neg) v = -v;`.
    Added `{"neg", Token::Neg, "−"}` to `kTokens`. Rewired the `(-)`
    CalcKey to send `"neg"`.
  - Pass 2: added context-aware disambiguation in
    `UIController::insertToken` — when a `Token::Sub` is about to be
    inserted and there's no left operand available (empty buffer,
    previous token is `LeftParen`, an operator, or a function),
    promote it to `Token::Neg` instead. This makes both the keyboard
    `-` key and the on-screen `−` key do the right thing in unary
    contexts (e.g. `abs(-5)` via keyboard now works). User reported
    Pass 1 alone wasn't enough during Phase B Wave 1 verification.
- **Notes:** The `(-)` CalcKey bypasses disambiguation by sending
  `"neg"` directly, so it's always an explicit unary. Keyboard `-` and
  the `−` CalcKey become context-aware: unary at the start of an
  expression or after an operator/function/paren, binary otherwise.
  Matches the informal convention most calculator apps and typed
  math use.

### BUG-015: `▶Frac` is not really implemented — acts as ENTER alias, results auto-convert to fractions with no way to "exit"
- **Status:** fixed (2026-04-07, user-reported during Phase B Wave 1 testing)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) `evaluate()` and `processInput()`, [app/qml/components/MathMenuPopup.qml](app/qml/components/MathMenuPopup.qml)
- **Severity:** medium
- **Description:** Previously, `UIController::evaluate()` ran every
  scalar result through `MathStateMachine::toFraction()` and displayed
  the fraction form whenever the conversion succeeded. This made
  `fPart(3.7)` display as `7/10` (exact rational) instead of `0.7` — a
  design mismatch with what users expect. Worse, `▶Frac` itself was
  wired as a literal ENTER alias (`if (input == "ENTER" || input ==
  "▶Frac")`), doing nothing meaningful beyond forcing an evaluation.
  User couldn't distinguish "I want a fraction" from "I want a decimal"
  because the calculator always tried fractions on its own.
- **Reproduction:** `fPart(3.7)` ENTER → displayed `7/10` instead of
  `0.7`. No way to toggle display format. User's mental model assumed
  some sort of "frac mode", but there was none.
- **Fix:** Option A — the TI-83-faithful design:
  - Removed the auto-`toFraction` call from `evaluate()`'s success
    branch. Scalar results now display as decimal by default.
  - `▶Frac` is now a post-hoc display conversion: if the current state
    is `Evaluated` and the result was scalar, it looks up the stored
    `MathStateMachine::lastResult.value`, runs `toFraction` on it, and
    replaces the display with the fraction form (if one exists within
    tolerance).
  - Added `▶Dec` as the reverse — redisplays the stored result in its
    raw decimal form.
  - Both conversions prepend an `Ans▶Frac = …` / `Ans▶Dec = …` history
    entry so the transformation is visible.
  - Added `▶Dec` as a new entry in `MathMenuPopup` alongside `▶Frac`.
- **Notes:** Means rationals like `1÷3` now show as `.333333...` by
  default; the user presses `MATH` → `▶Frac` to see `1/3`. Irrational
  results (`e`, `π`, `√(2)`) silently leave the decimal alone when
  `▶Frac` is pressed, since `toFraction` returns empty for those after
  the BUG-013 fix. Could add an error signal for the "can't convert"
  case in the future but silent no-op is the TI-83 behaviour.

### BUG-013: `toFraction` returns misleading fractions for irrational results
- **Status:** fixed (2026-04-07, same session it was reported)
- **Location:** [core_math/src/core_math.cpp](core_math/src/core_math.cpp) `MathStateMachine::toFraction`
- **Severity:** medium
- **Description:** `toFraction` uses a continued-fraction algorithm to
  render scalar results as fractions (`1/3` instead of `0.333...`). The
  loop ran up to 10 iterations checking for convergence within
  `tolerance` (1e-9). For exact rationals the check passed and the loop
  broke early. **But for irrationals, the loop just hit its iteration
  limit without ever converging — and the function still returned
  whatever the last convergent was**, treating it as a valid fraction.
  `e` → `1457/536` (off by ~1.76e-6), `π` and `√(2)` similar.
- **Reproduction:** Press `e` ENTER. Display showed `1457/536` instead
  of `2.71828182846`. User reported via screenshot.
- **Fix:** Added a `converged` flag inside the loop — set to `true`
  only when the tolerance check passes or the continued-fraction
  expansion terminates exactly. If the loop exits without convergence,
  return an empty string so the caller falls back to
  `QString::number(result.value)` (decimal).
- **Notes:** Pre-existing since the initial core_math commit but
  dormant — no key produced an irrational result until the `e`
  constant landed today. Also retroactively fixes the same
  misrepresentation for `π`, `√(2)`, and any other irrational scalar
  output.

### BUG-009: Chained `^` evaluates left-associatively (`2^3^2 = 64`, should be `512`)
- **Status:** fixed (2026-04-07, Group A engine cleanup)
- **Location:** [core_math/src/core_math.cpp](core_math/src/core_math.cpp) (the shunting-yard precedence loop)
- **Severity:** medium
- **Description:** `is_left_associative` was declared and correctly
  returned `false` for `Pow`, but the shunting-yard loop never called
  it — the loop used `>=` unconditionally, treating `Pow` as
  left-associative. `2^3^2` evaluated to `64` instead of `512`.
- **Fix:** The precedence loop now consults `is_left_associative` for
  the top-of-stack operator and uses `>=` for left-associative ops or
  `>` for right-associative ones. The dead `is_left_associative`
  function is now actually wired in.

## Won't Fix / Deferred

_(none yet)_

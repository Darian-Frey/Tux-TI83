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

### BUG-004: `asin` / `acos` / `atan` are no-ops in the evaluator
- **Status:** open
- **Found:** 2026-04-06 (post-Step 6 spot-check of `core_math/`)
- **Location:** [core_math/src/core_math.cpp:217-235](core_math/src/core_math.cpp#L217-L235) (the unary-function eval chain in `MathStateMachine::evaluate`)
- **Severity:** medium
- **Description:** `EOSPrecedence::is_function` recognises `Token::ASin`,
  `Token::ACos`, `Token::ATan` as functions, and the controller's token
  map already accepts the input strings `"asin"`, `"acos"`, `"atan"`. But
  the unary-function evaluation chain only branches on Sin, Cos, Tan,
  Sqrt, Log, Ln, Not (and Det for the matrix path). For ASin/ACos/ATan,
  the function fires (popping its argument from the operand stack), no
  transformation matches, and the unmodified value is pushed back.
- **Reproduction:** `asin(0.5)` returns `0.5` instead of `≈0.524`. Hard
  to trigger from the new UI directly because there are no inverse-trig
  CalcKeys yet, but reachable via the controller's existing token map.
- **Notes:** Trivial fix: add three else-if branches mirroring sin/cos/tan
  with `std::asin`, `std::acos`, `std::atan`. Domain checks (asin/acos
  require [-1, 1]) need to set `error_message` so the UI can show a
  meaningful error — see [IMP-006](IMPROVEMENTS.md) for the related
  error-propagation work.

### BUG-005: Division by zero silently returns 0
- **Status:** open
- **Found:** 2026-04-06 (post-Step 6 spot-check)
- **Location:** [core_math/src/core_math.cpp:272-274](core_math/src/core_math.cpp#L272-L274)
- **Severity:** medium
- **Description:** The `Token::Div` branch evaluates
  `b.val == 0 ? 0.0 : a.val / b.val`. When the divisor is zero the
  expression silently produces `0` with `success = true`. A real TI-83
  shows `ERR:DIVIDE BY 0`; ours shows nothing — the user can't
  distinguish `5÷0` from `0÷5`.
- **Reproduction:** `5÷0` ENTER → display shows `0` with no error.
- **Notes:** Should return `CalculationResult{success=false,
  error_message="DIVIDE BY 0"}`. Pairs with [IMP-006](IMPROVEMENTS.md)
  to actually surface the message in the UI.

### BUG-006: `√` of a negative silently returns 0
- **Status:** open
- **Found:** 2026-04-06 (post-Step 6 spot-check)
- **Location:** [core_math/src/core_math.cpp:227-228](core_math/src/core_math.cpp#L227-L228)
- **Severity:** low
- **Description:** `(v >= 0) ? std::sqrt(v) : 0.0`. For negative inputs,
  returns 0 with `success = true` instead of an error. Real TI-83 shows
  `ERR:NONREAL ANS` (without complex mode enabled).
- **Reproduction:** `√(-4)` ENTER → display shows `0`.
- **Notes:** Same fix shape as BUG-005: return `success=false` with a
  domain error. Pairs with [IMP-006](IMPROVEMENTS.md).

### BUG-007: `log` and `ln` of non-positive silently return `-HUGE_VAL`
- **Status:** open
- **Found:** 2026-04-06 (post-Step 6 spot-check)
- **Location:** [core_math/src/core_math.cpp:229-232](core_math/src/core_math.cpp#L229-L232)
- **Severity:** low
- **Description:** `(v > 0) ? std::log10(v) : -HUGE_VAL` (and similarly
  for `Ln`). For inputs ≤ 0, returns the C macro `-HUGE_VAL` with
  `success = true`. The huge negative number then formats as scientific
  notation, totally confusing the user.
- **Reproduction:** `log(-5)` ENTER → display shows a giant negative
  number.
- **Notes:** Same fix shape as BUG-005/006. Pairs with
  [IMP-006](IMPROVEMENTS.md).

### BUG-008: Matrix subtraction not supported
- **Status:** open
- **Found:** 2026-04-06 (post-Step 6 spot-check)
- **Location:** [core_math/src/core_math.cpp:252-256](core_math/src/core_math.cpp#L252-L256)
- **Severity:** low (feature gap, not a regression)
- **Description:** The `Token::Sub` branch only handles scalar-scalar
  subtraction. For two matrices, it falls through to `Type Error`.
  Real TI-83 supports element-wise matrix subtraction.
- **Reproduction:** Define `[A]` and `[B]` as same-dimension matrices,
  evaluate `[A]−[B]` ENTER → ERR.
- **Notes:** Trivial — mirror the existing `matrixAdd` helper into a
  `matrixSub`, then add an `a.isMat && b.isMat` branch in the Sub case.
  Belongs in the same matrix-vocabulary work as transpose/inverse
  exposure.

### BUG-009: Chained `^` evaluates left-associatively (`2^3^2 = 64`, should be `512`)
- **Status:** open
- **Found:** 2026-04-06 (post-Step 6 spot-check)
- **Location:** [core_math/src/core_math.cpp:51](core_math/src/core_math.cpp#L51) (`is_left_associative`) and [core_math/src/core_math.cpp:166-174](core_math/src/core_math.cpp#L166-L174) (the shunting-yard precedence loop)
- **Severity:** medium
- **Description:** `EOSPrecedence::is_left_associative` is declared and
  defined to return `false` for `Pow` and `true` for everything else,
  but it's **never called anywhere in the codebase**. The shunting-yard
  precedence loop uses `precedence(opStack.top()) >= precedence(t)`
  unconditionally, which is correct for left-associative operators but
  wrong for `Pow` (which is right-associative). Result: `2^3^2`
  evaluates as `(2^3)^2 = 64` instead of `2^(3^2) = 512`.
- **Reproduction:** `2^3^2` ENTER → returns `64`. Real TI-83 returns `512`.
- **Notes:** Wire `is_left_associative` into the precedence loop: use
  `>` instead of `>=` when the top-of-stack operator is right-
  associative. This was originally going to be logged as a dead-code
  improvement (since `is_left_associative` is unused), but the
  underlying issue is a real evaluation bug, so it's logged here.

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

## Won't Fix / Deferred

_(none yet)_

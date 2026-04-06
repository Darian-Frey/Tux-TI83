# IMPROVEMENTS.md — Tux-TI83

Catalogue of code-quality improvements, refactors, and architectural changes
proposed during development. Per the project workflow, improvements are
**logged here when noticed, not silently applied**. The user decides whether
to apply, defer, or decline.

This is the dual of [BUGS.md](BUGS.md): bugs are things that are *broken*,
improvements are things that *work but could be better* (clarity, reuse,
maintainability, performance, future flexibility).

## Format

Each entry uses this template:

```
### IMP-NNN: <short title>
- **Status:** suggested | applied | declined | deferred
- **Found:** YYYY-MM-DD (session/commit context)
- **Location:** path/to/file.ext:line (or "cross-cutting")
- **Effort:** trivial | small | medium | large
- **Description:** what could be improved and why
- **Proposal:** how to do it
- **Trade-offs:** what we'd give up or risk
- **Notes:** related context, dependencies on other work
```

---

## Suggested

### IMP-004: `Token::Num0` doubles as the "numeric literal" sentinel
- **Status:** suggested
- **Found:** 2026-04-06 (post-Step 6 spot-check of `core_math/`)
- **Location:** [core_math/src/core_math.cpp:123-129](core_math/src/core_math.cpp#L123-L129) and [core_math/src/core_math.cpp:147-150](core_math/src/core_math.cpp#L147-L150)
- **Effort:** small
- **Description:** The first pre-pass in `MathStateMachine::evaluate`
  collects digit tokens into a string, parses to a double, stores it in
  a parallel `numericValues` vector, and pushes `Token::Num0` as a
  placeholder in `processedTokens`. The shunting-yard loop later sees
  `Num0` and pulls the next value out of `numericValues` via
  `numIdx++`. This works but conflates two meanings of `Num0`: "the
  literal digit 0" and "this is a numeric literal, look up the value
  in the parallel array." Anyone reading `processedTokens` has to know
  the trick.
- **Proposal:** Add a `Token::NumLiteral` enum value used only as the
  sentinel. Or refactor `processedTokens` to a `vector<RpnNode>` with
  explicit literal/op/var variants and eliminate the parallel
  `numericValues` array entirely.
- **Trade-offs:** The minimal rename is essentially zero-risk. The
  fuller refactor touches the parser hot path and would benefit from
  having tests in place first.
- **Notes:** Worth doing before adding negative literals or scientific
  notation, both of which would extend this code path.

### IMP-005: `Token::ImplicitMul` declared but never generated or handled
- **Status:** suggested
- **Found:** 2026-04-06 (post-Step 6 spot-check)
- **Location:** [core_math/include/capsules/capsule_math.hpp:27](core_math/include/capsules/capsule_math.hpp#L27) (declared); never referenced in `core_math.cpp`
- **Effort:** medium
- **Description:** `ImplicitMul` is in the `Token` enum but the input
  pre-pass never synthesises it (e.g., between a number and a variable
  in `2x`), and the evaluator has no case for it. It's currently a
  dead enum value.
- **Proposal:** Either delete it (if implicit multiplication isn't
  planned), or implement it: post-process the token stream after
  parsing, walking adjacent pairs and inserting `ImplicitMul` between
  juxtaposed pairs like `(Num, VarX)`, `(RightParen, LeftParen)`,
  `(VarX, LeftParen)`, `(Pi, Num)`, etc. Then add a case in the
  operator-handling branch that treats it as a high-precedence
  multiplication.
- **Trade-offs:** Implementing it makes the calculator feel more
  natural (`2π` works without an explicit `*`) but adds complexity to
  the token-stream rewrite pass. Real TI-83 supports it for specific
  pairings.
- **Notes:** If we keep it as a planned feature, log it on
  [ROADMAP.md](ROADMAP.md) as a numeric-core item. If we don't, delete
  the enum value.

### IMP-006: `CalculationResult.error_message` is never propagated to the display
- **Status:** suggested
- **Found:** 2026-04-06 (post-Step 6 spot-check)
- **Location:** [core_math/src/core_math.cpp](core_math/src/core_math.cpp) sets specific error messages ("Empty", "Type Error", "Dim Mismatch", "Undefined Matrix"); [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) `evaluate()` ignores `result.error_message` and unconditionally sets `currentStr = "ERR:SYNTAX"` on failure.
- **Effort:** small
- **Description:** The math engine already classifies several failure
  modes by string, but the UI flattens every failure to `ERR:SYNTAX`.
  Real TI-83 shows specific errors: `ERR:SYNTAX`, `ERR:DOMAIN`,
  `ERR:DIVIDE BY 0`, `ERR:INVALID DIM`, `ERR:UNDEFINED`,
  `ERR:DATA TYPE`, etc.
- **Proposal:** In `UIController::evaluate()`, switch on
  `result.error_message` and pick a display string accordingly:
  `"Type Error"` → `"ERR:DATA TYPE"`, `"Dim Mismatch"` →
  `"ERR:INVALID DIM"`, `"Undefined Matrix"` → `"ERR:UNDEFINED"`,
  empty/`"Empty"`/`"Error"` → `"ERR:SYNTAX"` (fallback).
- **Trade-offs:** None worth noting. Adds ~10 lines.
- **Notes:** Pairs naturally with [BUG-005, BUG-006, BUG-007](BUGS.md),
  since those need to set `error_message` in the first place. Doing
  this IMP alone isn't useful until at least one of those bugs sets
  the field.

### IMP-003: Add an ALPHA-modifier gate to single-letter keyboard shortcuts
- **Status:** suggested
- **Found:** 2026-04-06 (UI redesign session, before Step 6)
- **Location:** [app/qml/Main.qml](app/qml/Main.qml) (Keys.onPressed handler) and the eventual ALPHA CalcKey hook
- **Effort:** small-to-medium
- **Description:** Step 6 wires the CLAUDE.md keyboard shortcut map
  literally — bare letters `s`, `c`, `t`, `l`, `n`, `r`, `p` immediately
  produce `sin(`, `cos(`, `tan(`, `log(`, `ln(`, `√(`, `π`. This is fast,
  matches the spec exactly, and gets us a working keyboard now. The
  trade-off is that single letters can never be typed as literal text.
  The moment we add any feature that needs literal letters (variable
  assignment, matrix labels, MATH menu search, ANS naming, etc.), bare
  letter shortcuts become ambiguous.
- **Proposal:** Mirror the TI-83's hardware ALPHA modifier. Add an
  `alphaArmed` boolean (root-level in Main.qml, or as a controller
  property). Pressing the ALPHA `CalcKey` — or a keyboard mapping like
  Tab or backslash — sets it true and lights up a visual indicator on
  the ALPHA key. The next single-letter keypress checks the flag: if
  armed, route to the function shortcut and clear the flag; if not
  armed, either do nothing or route to a future text-input path.
  Optionally add an "ALPHA-lock" mode (double-press or 2ND+ALPHA) that
  keeps the flag set across multiple keystrokes, matching real TI-83
  behaviour.
- **Trade-offs:** Adds modal state to the keyboard handler, which is more
  complex than the literal spec. But it's the standard TI behaviour and
  anyone familiar with a real TI-83 will expect it. Without the gate
  we're locked out of any future feature that needs literal letters.
- **Notes:** Defer until we actually have a feature that needs literal
  letters. Good time to revisit: when wiring the MATH menu, variable
  assignment, or matrix labels. Also pairs naturally with implementing
  the 2ND modifier for the same set of CalcKeys.

---

## Applied

### IMP-001: Unify the forward and reverse token maps in UIController
- **Status:** applied (2026-04-06, post-Step 4)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) (anonymous namespace at file scope)
- **Effort:** small
- **Description:** Before this change, `processInput` defined a forward
  `tokenMap` (display string → `Token`) and the `DEL` branch defined a
  separate `revMap` (`Token` → display string). The two were hand-mirrored
  and had already drifted in subtle ways: `√` was a single-character
  function name that fell through the forward path's `length() > 1`
  auto-paren heuristic but was correctly stored as `"√("` in the reverse
  map (which is what made BUG-002 visible only in INPUTTING and not in
  the post-DEL rebuild).
- **Change:** Introduced a single `TokenSpec { input, token, displayStr }`
  table at file scope (`kTokens`) as the source of truth for the entire
  calculator vocabulary. The forward and reverse lookup maps are now
  built from it once via static lazy init. Adding a new token = adding
  one row. The display-string column already includes the opening paren
  for function tokens, so the entire `length() > 1` heuristic and the
  per-character special cases (`√`, `[A]`/`[B]`/`[C]`) are gone — display
  formatting is now data-driven.
- **Side effects:** BUG-002 is closed *by construction*: a future
  single-character function name added to the table just works, no code
  path needs touching.
- **Notes:** This was a prerequisite for cleanly adding 2ND / ALPHA / MATH
  menu tokens in upcoming roadmap work.

### IMP-002: Split `processInput` into smaller dispatch methods
- **Status:** applied (2026-04-06, post-Step 4)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) (UIController::processInput + private helpers)
- **Effort:** small
- **Description:** After the Step 4 state machine landed, `processInput`
  was ~160 lines and handled five concerns: CLEAR, DEL, ENTER, the
  state-transition reset, and token insertion.
- **Change:** Extracted four private helpers (`clearAll`, `backspace`,
  `evaluate`, `insertToken`) and reduced `processInput` to a 13-line
  dispatcher. Each helper owns one concern. Behavioural change: none —
  every state transition and signal emission was preserved verbatim.
- **Notes:** New input categories should now be added by extending the
  dispatcher's switch, not by growing the helpers. Combined with IMP-001,
  this means future token features (2ND, ALPHA, MATH menus, ANS recall)
  touch one row of `kTokens` and at most one new helper rather than
  threading more conditionals through a single megafunction.

## Declined

_(none yet)_

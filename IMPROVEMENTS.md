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

### IMP-007: MatrixPopup EDIT tab doesn't read existing values back
- **Status:** suggested
- **Found:** 2026-04-07 (Phase A — matrix editor reintegration)
- **Location:** [app/qml/components/MatrixPopup.qml](app/qml/components/MatrixPopup.qml) (EDIT tab GridLayout)
- **Effort:** medium
- **Description:** When the user opens the matrix editor and switches to
  the EDIT tab, the nine TextFields are always empty (placeholder `0`).
  If `[A]` already has values stored in the registry, they're not shown.
  Users editing an existing matrix have to retype every value, and any
  field they leave blank gets saved as `0` — silently overwriting
  previous data. This matches the legacy popup behaviour but it's a
  pretty rough UX.
- **Proposal:** Two-step:
  1. Add a `Q_INVOKABLE QVariantList getMatrix(QString name)` getter on
     `UIController` that returns the current matrix data for the named
     registry entry (or empty if undefined).
  2. In `MatrixPopup`, populate the TextFields from that getter when the
     EDIT tab becomes visible (or when the popup opens).
- **Trade-offs:** Requires a small controller addition. No risk to the
  math engine — the registry is already exposed, just needs a typed
  getter.
- **Notes:** Pairs naturally with [IMP-008](IMPROVEMENTS.md) (matrix
  selector) and the planned variable-dimensions work for the matrix
  editor.

### IMP-008: MatrixPopup EDIT tab is hardcoded to `[A]` and 3×3
- **Status:** suggested
- **Found:** 2026-04-07 (Phase A — matrix editor reintegration)
- **Location:** [app/qml/components/MatrixPopup.qml](app/qml/components/MatrixPopup.qml) (EDIT tab; the SAVE button hardcodes `"[A]"` and `3, 3`)
- **Effort:** medium
- **Description:** The EDIT tab can only edit `[A]`, and only at 3×3.
  This was true of the legacy popup too. Real TI-83 lets you edit any of
  the matrices `[A]`–`[J]` at any dimensions up to 99×99 (memory
  permitting). The current UI offers no way to edit `[B]`, `[C]`, or
  larger matrices.
- **Proposal:** Add two new controls to the top of the EDIT tab:
  1. A matrix selector (dropdown or row of small CalcKeys: `[A] [B] [C]`)
  2. Two SpinBox / numeric fields for rows and columns
  When the selector or dimensions change, regenerate the GridLayout's
  Repeater model. The SAVE button uses the currently selected matrix
  name and dimensions instead of hardcoded values.
- **Trade-offs:** Adds a chunk of UI logic. Worth doing once the new
  matrix vocabulary (transpose, inverse, rref) starts landing on
  ROADMAP, since users will want to manipulate more than just `[A]`.
- **Notes:** Strongly pairs with [IMP-007](IMPROVEMENTS.md). Both should
  ship together as a single "matrix editor v2" pass.

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

### IMP-009: Remove unused `<algorithm>` include from core_math.cpp
- **Status:** applied (2026-04-07)
- **Location:** [core_math/src/core_math.cpp:2](core_math/src/core_math.cpp#L2)
- **Effort:** trivial
- **Description:** clangd flagged `#include <algorithm>` as unused. A
  grep for any `std::` algorithm function (min, max, sort, find, copy,
  fill, count, any_of, all_of, none_of, reverse, swap, transform,
  accumulate, for_each) confirmed nothing in the file uses it.
- **Change:** Removed the line. Build still passes — the include was
  pure dead weight, possibly left over from an earlier version.
- **Notes:** Found by an IDE diagnostic during the BUG-010/011 fix
  pass. Pre-existing dead code, not introduced by any recent edit.

### IMP-006: Propagate `CalculationResult.error_message` to the display
- **Status:** applied (2026-04-07, Group A engine cleanup)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) (`evaluate()` error branch)
- **Effort:** small
- **Description:** Before this change the UI flattened every failure to
  `ERR:SYNTAX`, even though the engine already classified failures by
  string in `CalculationResult.error_message`. Users couldn't tell
  whether their input was syntactically wrong or semantically out of
  domain.
- **Change:** Added a switch in `UIController::evaluate()` mapping the
  engine's classification strings to TI-83-style display labels:
  - `"DIVIDE BY 0"` → `ERR:DIVIDE BY 0`
  - `"NONREAL ANS"` → `ERR:NONREAL ANS`
  - `"DOMAIN"` → `ERR:DOMAIN`
  - `"Type Error"` → `ERR:DATA TYPE`
  - `"Dim Mismatch"` → `ERR:INVALID DIM`
  - `"Undefined Matrix"` → `ERR:UNDEFINED`
  - anything else → `ERR:SYNTAX` (fallback)
- **Notes:** Landed alongside the BUG-004/5/6/7/8/9 fixes in the same
  Group A engine cleanup pass. The bug fixes set the new error strings
  on the engine side; this improvement plumbs them through to the UI.

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

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

_(none)_

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

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

_(none)_

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

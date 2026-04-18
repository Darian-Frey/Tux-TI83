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

### IMP-011: CLI / REPL can't populate matrices
- **Status:** suggested
- **Found:** 2026-04-08 (user-reported after matrix-inverse landed)
- **Location:** architectural — spans [cli/cli_main.cpp](cli/cli_main.cpp), [cli/repl_main.cpp](cli/repl_main.cpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) (`MathStateMachine::matrixRegistry`)
- **Effort:** medium
- **Description:** Matrix values live in `MathStateMachine::matrixRegistry`
  — a `static std::map<Token, Matrix>` inside the `tux_ti83` library.
  Each binary is its own process, so the registry is fresh in every
  `tux_ti83_cli` invocation and every `tux_ti83_repl` session. Worse,
  the CLI binaries have no way to *set* a matrix — only the GUI's
  MatrixPopup EDIT tab populates the registry. Any CLI expression
  involving `[A]`, `[B]`, or `[C]` returns `ERR:UNDEFINED` unless the
  user happens to reference a matrix in the same GUI instance. From a
  user's perspective, matrix operations are GUI-only right now.
- **Proposal:** Two shapes, picking one:
  1. **REPL command.** Add a `:matrix NAME ROWS COLS VALUES…` command
     so users can `:matrix [A] 2 2 1 2 3 4` and then evaluate
     `[A]^-1` or `det([A])` in the same session. One-liner for one-shot
     mode via stdin pipe: `echo ':matrix [A] 2 2 1 2 3 4\n[A]^-1' | tux_ti83_repl`.
  2. **On-disk persistence.** Serialise `matrixRegistry` to a dotfile
     (`~/.config/tux-ti83/matrices.json` or similar) whenever a matrix
     is updated, load on startup. GUI and CLI share the same file, so
     matrices set in one session survive to the next — and across
     binaries.
  Option 1 is smaller but keeps CLI and GUI as separate worlds. Option 2
  makes them share state but introduces file-I/O concerns (locking,
  corruption recovery, schema versioning).
- **Trade-offs:** Option 1 is a few dozen lines of parsing + dispatch
  in `repl_main.cpp`. Option 2 is more like 100+ lines but benefits
  the GUI too (matrices survive across app launches).
- **Notes:** User's actual test case
  (`./build/tux_ti83_cli '[A]^-1'` → `ERR:UNDEFINED`) is exactly this
  gap. The current behaviour is technically correct but uselessly so —
  no way for a CLI user to populate the matrix first.

### IMP-010: Route matrix element formatting through `formatScalar`
- **Status:** suggested
- **Found:** 2026-04-08 (during matrix-transpose work)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) `evaluate()` matrix-result formatting branch
- **Effort:** trivial (one-line change)
- **Description:** The scalar-result branch of `evaluate()` routes
  through `formatScalar(double)` — a centralised helper that uses
  10-significant-digit precision to avoid scientific-notation surprises
  (fixed during the factorial work). But the matrix-result branch
  still uses bare `QString::number(matrixValue.at(i, j))` with the
  default 6-digit precision. A matrix with a large integer element
  (anything ≥ 10⁶) would render that cell as scientific, inconsistent
  with scalar display.
- **Proposal:** Replace `QString::number(result.matrixValue.at(i, j))`
  with `formatScalar(result.matrixValue.at(i, j))` in the matrix-to-
  string loop. One-line change.
- **Trade-offs:** None — strictly more consistent. The two display
  paths become one semantic (10-sig-digit 'g' format).
- **Notes:** Not user-visible yet because no test matrix has elements
  large enough to hit the 6-digit precision cliff, but the
  inconsistency would surface the moment someone computes e.g.
  `[A]*[B]` where `[A]` and `[B]` have large values. Trivial to fix;
  flagging because it's the kind of small consistency gap that grows
  into real bugs if left to accumulate.

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

---

## Applied

### IMP-020: MODE menu — Notation (Normal/Sci/Eng) + Decimal (Float/Fix N) rows

- **Status:** applied (2026-04-18)
- **Location:** [core_math/include/capsules/capsule_math.hpp](core_math/include/capsules/capsule_math.hpp), [core_math/src/core_math.cpp](core_math/src/core_math.cpp), [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/components/MODEPopup.qml](app/qml/components/MODEPopup.qml), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** medium
- **Description:** Two of the seven greyed-out MODE-popup rows were the most immediately useful: Notation controls whether results render as `12345`, `1.234E4`, or `12.345E3` (engineering), and Decimal fixes the number of displayed decimals. Without them, there was no way to get consistent precision across a session or to see very large / very small numbers in a readable form.
- **Change:**
  - Engine: added `NumberNotation { Normal, Sci, Eng }` enum and two static fields on `MathStateMachine` — `notation` (default `Normal`) and `fixDecimals` (int, -1 = Float, 0..9 = Fix N). Defaults preserve historical behaviour.
  - `UIController::formatScalar` grew a four-branch dispatch:
    - Normal + Float: unchanged ('g' precision 10, trims zeros).
    - Normal + Fix N: `%.Nf` via Qt's `'f'` formatter.
    - Sci + Float/Fix: Qt's `'E'` formatter at precision 9 or N.
    - Eng + Float/Fix: exponent normalised to `3 * floor(log10|v| / 3)` so the mantissa sits in `[1, 1000)`; zero special-cased as `0E0`.
  - Controller: two new Q_PROPERTYs (`notation` as int 0/1/2; `fixDecimals` as int -1..9) with WRITE + NOTIFY. Setters clamp to the valid ranges so downstream consumers never see a corrupt state.
  - MODEPopup: flipped the Notation and Decimal rows from `active: false` to live bindings against the new properties. Decimal's 11 segments (`Float`, `0`..`9`) made the row geometry fragile — added `Layout.minimumWidth: segLabel.implicitWidth + 12` to the segment delegate so short digits stay tight and longer labels expand (the user caught a "Float" clip on first pass; this fix also helps the rest of the placeholder rows — `Connected`, `Sequential`, `re^θi` — when they get wired).
  - Bumped the MODE popup width from 340 → 420 so the Decimal row has room to breathe.
  - Tests: 10 new assertions covering Normal Float / Fix 0 / Fix 2, Sci Float / Fix 2, Eng Fix 3 (including the `0 → 0.000E0` special case and a negative-exponent case). Tests restore the Normal + Float defaults at exit so downstream assertions see the environment they were written against. 187/187 passing.
- **Trade-offs:** The engine-static approach (same pattern as `angleMode` from IMP-015) means settings are process-global — two UIController instances in the same process would share them. Fine for our shape (the app only runs one). Alternative would be per-controller state, which is more plumbing with no current benefit.
- **Notes:** Closes out the user-facing queue for this session (cursor movement → ALPHA-lock → Logic menu → MODE Notation/Decimal). Remaining MODE placeholders (Graph mode variants, Connected/Dot, Sequential/Simul, Complex, Screen) each depend on feature work that isn't on the immediate roadmap.

### IMP-019: Logic operator menu (2ND + MATH → TEST / LOGIC)

- **Status:** applied (2026-04-18)
- **Location:** [app/qml/components/LogicMenuPopup.qml](app/qml/components/LogicMenuPopup.qml), [app/qml/Main.qml](app/qml/Main.qml), [app/qml/qmldir](app/qml/qmldir), [CMakeLists.txt](CMakeLists.txt), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** small
- **Description:** Six operators were engine-implemented but had no UI path after the legacy LOGIC popup was deleted during Phase A: `≤`, `≥`, and `xor` weren't even in `kTokens`, and while `=`, `≠`, `<`, `>`, `and`, `or`, `not` were reachable via keyboard, there was no menu to discover them. 2ND + MATH on a real TI-83 opens the TEST menu — the natural home.
- **Change:**
  - Added `≤`, `≥`, `xor` entries to `kTokens` alongside ASCII aliases `<=` / `>=` so keyboard and CLI users have both Unicode and plain-text input paths.
  - Built `LogicMenuPopup.qml` with two sections — TEST (6 comparators) and LOGIC (4 boolean ops). Single-column scrollable list mirrors the MATH menu's pattern; entries insert via `processInput` and close the popup. Factored the delegate into an inline `EntryRow` component and the section separators into a `SectionLabel` component so the structure stays readable even as the list grows.
  - Wired 2ND + MATH in `handleKey` as a dedicated path (like 2ND + ENTER for recall) — popup triggers don't fit the string-in-string-out shape of `secondMap`. MATH CalcKey's `onPressed` now routes through `handleKey` whenever 2ND is armed or ALPHA is active, so the three modifier modes (none → MATH menu, 2ND → LOGIC menu, ALPHA → insert `A`) all converge on the dispatcher.
  - Added `secondLabel: "TEST"` to the MATH CalcKey so the keytop annotation matches the new behaviour.
  - Registered the popup in `qmldir` and the CMake resource list.
  - Tests: 8 new assertions covering `≤`/`≥` in both Unicode and ASCII forms, and `xor` at each of its three truth-table corners. 177/177 passing.
- **Trade-offs:** Fixed popup height was sized for the exact row count; if more operators land here it'll need to be bumped (user caught a 70px overflow on first pass — `not` was getting clipped, fixed by bumping 420→520). Alternative: size-to-content, which is a QML-y pattern but pulls in extra layout work.
- **Notes:** Closes the Phase A logic-UI gap. Follow-up: the TEST menu on real TI-83 also has sub-tabs for specific test types (equality, inequality) — we're a single flat list, which is simpler but diverges slightly from the authentic layout. Acceptable for the operator count we have.

### IMP-018: ALPHA-lock mode (2ND+ALPHA)

- **Status:** applied (2026-04-18)
- **Location:** [app/qml/Main.qml](app/qml/Main.qml)
- **Effort:** small
- **Description:** With ALPHA letter bindings wired (IMP-014), typing any run of letters — a variable sequence, a TI-BASIC identifier in future work — meant pressing ALPHA before *every* keystroke. Real TI-83 has a lock mode (2ND+ALPHA) that keeps ALPHA armed across presses; without it, our workflow was a strictly worse version of the real device.
- **Change:**
  - Added `alphaLocked` boolean root property alongside the existing one-shot `alphaArmed`, plus a derived `alphaActive = alphaArmed || alphaLocked` convenience for call sites that need the combined state.
  - `armAlpha()` now branches three ways: 2ND + ALPHA toggles the lock, ALPHA-while-locked releases it, otherwise a single tap toggles `alphaArmed` as before. `armSecond()` no longer clears `alphaLocked` (so 2ND + letter combos mid-typing keep the lock alive). `clearModifiers()` clears all three flags.
  - `handleKey` fires the ALPHA variant whenever `alphaArmed || alphaLocked`, and only clears the one-shot flag — the lock persists.
  - ALPHA CalcKey's `armed` visual binds to `alphaActive` so it stays highlighted across the lock. Header badge reads `α` for a one-shot arm and `A-LOCK` when locked (matches TI-83's convention).
  - Follow-up fix after user testing: four special-cased CalcKeys (MATH, MATRX, x², (-)) used to check only `alphaArmed` before deciding whether to route through `handleKey` or run their default action. Once locked, those checks fell through to the popup/default — making MATH open the menu instead of inserting A. Switched the four to read `alphaActive` (or `secondArmed || alphaActive` for x²).
- **Trade-offs:** No engine or controller work — this is purely a QML modifier-state change. The `:`/`?`/`"` corner labels on `.`/`(-)`/`+` remain layout-accurate but unwired (they need statement-separator / string-literal support that's out of scope for this session); falls through to a silent arm-clear when pressed.
- **Notes:** Completes the TI-83 modifier model. Natural follow-ups: wire `:` as a statement separator (enables `5→A:A+1→A` chained expressions), and eventually `?`/`"` once Input/Disp commands land.

### IMP-017: Cursor movement within an expression

- **Status:** applied (2026-04-18)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/components/Display.qml](app/qml/components/Display.qml), [app/qml/Main.qml](app/qml/Main.qml), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** medium
- **Description:** The expression cursor was pinned to the end of the
  buffer — every backspace had to chew through trailing tokens before
  reaching a mid-expression typo, and every insertion appended. With
  last-entry recall (IMP-016) landing, this gap became acute: recalling
  a long expression only to retype it was just a slower way to
  re-enter it.
- **Change:**
  - Controller: added a token-level `m_cursorPos` (0..buf.size()). `insertToken` splices at the cursor and advances by one; `backspace` erases at `cursor-1` and retreats; `clearAll` resets to 0; `recallLastEntry` sets the cursor to the end of the restored buffer. Four new Q_INVOKABLE methods (`moveCursorLeft` / `Right` / `Home` / `End`) move the cursor — each a no-op outside Inputting state and clamped at the buffer extremes. The unary-negation heuristic now inspects `buf[cursorPos-1]` rather than `buf.back()` so mid-expression `-` correctly promotes to `Neg` in unary contexts.
  - Q_PROPERTY: `cursorOffset` (int, NOTIFY `cursorMoved`) translates the token-level cursor to a character offset by summing `displayStr` lengths for the tokens before the cursor — the Display's TextInput needs char positions for its internal cursor.
  - Display: binds `TextInput.cursorPosition` to `root.cursorCharOffset` in Inputting state. Evaluated/Error still auto-snap to the end on text change so long results land scrolled right. Dropped the unconditional end-snap in `onTextChanged` (it was fighting the new binding).
  - Main.qml: wired `Qt.Key_Left/Right/Home/End` to the four controller methods in the root keyboard handler.
  - Tests: 7 new assertions covering mid-expression insert, mid-expression backspace, over-clamp behaviour at the ends, Home/End positioning, and cursor-move no-op outside Inputting. 169/169 passing.
- **Trade-offs:** On-screen arrow CalcKeys were deliberately deferred — the current 5-column layout has no free slot, and wedging in a D-pad would require a layout shuffle that's out of scope. Keyboard arrow keys cover most of the value; an on-screen nav pad can land later alongside a MODE menu redesign or similar layout work.
- **Notes:** Closes a long-standing editability gap and makes last-entry recall genuinely useful for editing — the original impetus for this change. Pairs with a future on-screen D-pad for touch/mouse-only users.

### IMP-016: Last-entry recall (2ND+ENTER)

- **Status:** applied (2026-04-18)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/Main.qml](app/qml/Main.qml), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** small
- **Description:** Last-entry recall was the final Phase B item. Real
  TI-83 users rely on 2ND+ENTER to bring a prior expression back to
  the edit line — either to re-run it or to tweak one term without
  retyping the whole thing. Without it, the only way to repeat work
  was the keyboard history scroll in the REPL, which doesn't exist in
  the GUI at all.
- **Change:**
  - Controller: added a 10-deep `std::deque<std::vector<Token>>`
    ring buffer (`m_entryHistory`) + a cycle counter
    (`m_recallCycleIdx`) on UIController. Each non-empty ENTER
    pushes the raw token stream into the deque before the
    evaluate/display branching; oldest entries evict once the cap is
    reached. A new `Q_INVOKABLE recallLastEntry()` advances the
    counter (clamping at the oldest entry), restores the token
    buffer verbatim, rebuilds the display string via the unified
    `tokenToSpec` table, and flips back to Inputting state. The
    cycle resets whenever `processInput` fires with anything else —
    including CLEAR, a fresh keypress, or an edit.
  - UI: special-cased 2ND+ENTER in `handleKey` to call
    `recallLastEntry()` directly (the existing `secondMap` is
    string-in-string-out and ENTER is a control sentinel, not a
    kTokens entry). Added `secondLabel: "ENTRY"` to the ENTER
    CalcKey so the keytop annotation matches the new behaviour.
  - Tests: 8 new assertions covering recall-on-empty-history (no-op),
    walking back through three entries, clamp-at-oldest, cycle
    reset after a non-recall input, edit-and-reeval, and the
    empty-ENTER-doesn't-pollute-history guard. 161/161 passing.
- **Trade-offs:** Chose a 10-entry cap to match real TI-83 depth —
  small enough to keep memory negligible, big enough for any
  realistic workflow. Storing failed entries (not just successful
  ones) was deliberate: if a user types a typo and hits ENTER,
  2ND+ENTER lets them fix it rather than starting over. Considered
  exposing the deque as a Q_PROPERTY for an external history viewer
  but that's out of scope — the existing HistoryPane already shows
  "expr = result" strings.
- **Notes:** Closes Phase B. Next natural follow-ups: cursor
  movement within an expression (so `2ND+ENTER` + edit becomes truly
  useful for long expressions), and an ALPHA-lock mode now that the
  modifier infrastructure has matured.

### IMP-015: MODE menu + angle-mode (Radian/Degree) wiring

- **Status:** applied (2026-04-18)
- **Location:** [core_math/include/capsules/capsule_math.hpp](core_math/include/capsules/capsule_math.hpp), [core_math/src/core_math.cpp](core_math/src/core_math.cpp), [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/Main.qml](app/qml/Main.qml), [app/qml/components/MODEPopup.qml](app/qml/components/MODEPopup.qml), [app/qml/qmldir](app/qml/qmldir), [CMakeLists.txt](CMakeLists.txt), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** medium
- **Description:** The MODE CalcKey was a no-op — the one remaining
  unwired control-row key. The header read "NORMAL DEG" regardless of
  any actual setting, and trig functions always used radians with no
  way to switch.
- **Change:**
  - Engine: added `AngleMode` enum (`Radian` | `Degree`) and the
    static `MathStateMachine::angleMode` field, defaulting to Radian.
    Extended the trig evaluator — sin/cos/tan scale inputs by π/180
    when degree mode is active, and asin/acos/atan scale outputs by
    180/π. Hyperbolic functions stay mode-agnostic (their argument
    isn't an angle).
  - Controller: exposed `angleMode` as a Q_PROPERTY (int, 0=Radian,
    1=Degree) with WRITE and NOTIFY so QML can bind bidirectionally.
    Setter clamps to the two valid values.
  - UI: built `MODEPopup.qml` with a reusable `ModeRow` inline
    component (label + segmented options + active/inactive styling),
    rendering 8 TI-83-authentic rows. The Angle row is wired to
    `uiController.angleMode`; the other seven (Notation, Decimal,
    Graph, Draw, Plot, Complex, Screen) render as greyed placeholders
    so the shape of the MODE screen is right today — each one will
    get a backing property as the corresponding feature lands.
    Registered the component in `qmldir` and CMakeLists's resource
    list, then wired the MODE CalcKey to open it and the header
    string to bind to the live property.
  - Tests: 12 new assertions covering sin/cos/tan at standard
    degree reference points (0°, 30°, 45°, 60°, 90°), inverse trig
    returning degrees in Degree mode, and a round-trip flip back to
    Radian to confirm the mutation is symmetric. 153/153 passing.
- **Trade-offs:** The placeholder rows risk implying more than is
  true — users might expect clicking them to do something. Greying
  them out at 0.4 opacity and disabling their MouseAreas makes it
  clear they're not interactive; keeping them visible is still
  valuable as a roadmap-in-the-UI. Alternative considered: render
  only the Angle row and add others as they're wired. Rejected
  because a one-row MODE screen felt thinner than the TI-83
  reference and hid upcoming work.
- **Notes:** Unblocks any future trig-heavy work (graphing
  `sin(x)` where users expect degrees, etc.) and establishes the
  pattern for the remaining MODE rows. The controller's `setAngleMode`
  is a reusable pattern — Notation/Decimal/Graph will follow the
  same shape.

### IMP-014: Scalar variable registry A–Z + STO assignment

- **Status:** applied (2026-04-18)
- **Location:** [core_math/include/capsules/capsule_math.hpp](core_math/include/capsules/capsule_math.hpp), [core_math/src/core_math.cpp](core_math/src/core_math.cpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/Main.qml](app/qml/Main.qml), [app/qml/components/MathMenuPopup.qml](app/qml/components/MathMenuPopup.qml), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** medium
- **Description:** The two remaining Phase B engine features were
  scalar variables A–Z and the STO assignment arrow. Without them, the
  ALPHA modifier could arm and flash labels but pressing a letter
  inserted nothing, and there was no way to bind a value to a name —
  blocking any TI-BASIC-like expression (`5→A`, `A+3→A`).
- **Change:**
  - Engine: replaced the standalone `Token::VarX` with a contiguous
    `VarA..VarZ` block (26 tokens), added `Token::Sto` at precedence
    -10, added `std::array<double, 26> MathStateMachine::varRegistry`
    (zero-initialised, mutated only on successful stores), extended
    the preprocessing pass to consume the target VarA..VarZ that
    follows each Sto (syntax error if missing), and added evaluator
    cases for both. `VarX` stayed dual-purpose: graph-mode eval
    passes the sweep x, calc-mode eval now passes
    `varRegistry[X_idx]`.
  - Controller: added A–Z entries and `→` / `->` aliases to
    `kTokens`. Renamed the CLEAR sentinel from `"C"` to `"CLEAR"`
    to resolve the collision with `VarC` (CalcKey, keyboard
    handler, CLI helper, and tests all updated).
  - UI: added `alphaMap` mirroring the on-key `alphaLabel` layout
    and extended `handleKey` to route ALPHA-armed presses through
    it. Special-cased CalcKeys (MATH, MATRX, x², (-)) that had
    bespoke `onPressed` bodies now also branch on `alphaArmed`
    before running their default action. Added a `→ (STO)` entry
    to the MATH menu and a `|` keyboard shortcut.
  - Tests: 16 new regression assertions covering defaults, basic
    stores, chained read-modify-writes, letter independence,
    error-don't-mutate, malformed Sto syntax, matrix-to-scalar
    type error, ASCII alias, and the graph-X dual role. 141/141
    passing.
- **Trade-offs:** The CLEAR sentinel rename touched a handful of
  files but removed an ambiguous one-character input. Letters that
  real TI-83 maps to punctuation (`.`→`:`, `(-)`→`?`, `+`→`"`) kept
  their layout-accurate labels but the `alphaMap` doesn't wire them
  — they'd need to land in `kTokens` first, and none of those
  characters have evaluator semantics yet. ALPHA-armed press on
  those keys disarms silently.
- **Notes:** Closes Phase B proper. Unblocks the programming roadmap
  (TI-BASIC `:` separator, `Disp`, `Input`, `For(`, etc.) which all
  depend on readable/writable scalars. Follow-up: `:` as a
  statement separator, ALPHA-lock mode, last-entry recall
  (2ND+ENTER).

### IMP-013: On-key corner labels for 2ND and ALPHA functions

- **Status:** applied (2026-04-18)
- **Location:** [app/qml/components/CalcKey.qml](app/qml/components/CalcKey.qml), [app/qml/Main.qml](app/qml/Main.qml), [app/qml/Style.qml](app/qml/Style.qml)
- **Effort:** small
- **Description:** With the 2ND/ALPHA modifier infrastructure landed
  (IMP-003), keys had no visible indication of what their modifier
  functions were — users had to guess or read the codebase. Real
  TI-83 hardware prints tiny coloured markings above each key to
  teach the layout; we were missing that affordance.
- **Change:**
  - Added `secondLabel` and `alphaLabel` optional string properties
    to `CalcKey`, rendered as 7px text in the top-left (amber, 2ND
    colour) and top-right (green, ALPHA colour) corners respectively.
    Hidden when empty, so keys without modifier functions stay clean.
  - Added `Style.cornerLabelPixelSize` constant so the sub-label
    typography is centralised.
  - Populated `secondLabel` on every CalcKey whose 2ND variant is
    wired in `secondMap` — `sin⁻¹`/`cos⁻¹`/`tan⁻¹` on the trig keys,
    `eˣ` on LN, `√` on x², and `ANS` on `(-)`. 2ND labels are
    deliberately withheld on unwired keys (MATH→TEST, ^→ˣ√, LOG→10ˣ,
    etc.) to avoid advertising behaviour that doesn't work yet.
  - Populated `alphaLabel` on every CalcKey whose real-TI-83
    counterpart has an ALPHA letter, adapted to our key layout.
    Letters are shown even though ALPHA variants aren't wired yet —
    this teaches the layout now and is harmless since pressing ALPHA
    then any key currently just clears the flag.
- **Trade-offs:** Keys are visibly busier, but the sub-labels are
  small enough (7px in the corners) that they frame the primary
  label rather than compete with it. The ALPHA-letter asymmetry
  (visible but non-functional) is a deliberate trade — we agreed
  showing the layout early outweighs the "does nothing" surprise.
- **Notes:** Promoted the old "💭 On-screen 2nd/Alpha indicator
  badges over the keys" ROADMAP entry to ✅. Blocked follow-ups:
  wiring the remaining 2ND actions (CATALOG, TEST menu, nth-root,
  etc.) and ALPHA letter inputs (requires variable registry).

### IMP-003: 2ND / ALPHA modifier infrastructure (supersedes original "ALPHA gate" proposal)

- **Status:** applied (2026-04-18)
- **Location:** [app/qml/Main.qml](app/qml/Main.qml), [app/qml/components/CalcKey.qml](app/qml/components/CalcKey.qml), [app/qml/Style.qml](app/qml/Style.qml)
- **Effort:** medium
- **Description:** The original suggestion was narrowly scoped to
  "gate the bare-letter keyboard shortcuts behind ALPHA". In practice
  the 2ND and ALPHA modifiers form one shared mechanism, so this
  landed as a unified infrastructure pass rather than just the ALPHA
  half.
- **Change:**
  - Added root-level `secondArmed` / `alphaArmed` booleans in
    `Main.qml` with `armSecond()` / `armAlpha()` / `clearModifiers()`
    helpers. Arming one disarms the other (mutual exclusion), and
    pressing an already-armed modifier disarms it (toggle).
  - Added a central `handleKey(primary)` dispatcher. Every CalcKey
    and physical-key shortcut routes through it, so the modifier
    state applies uniformly to both input paths. When `secondArmed`,
    the dispatcher consults a `secondMap` lookup
    (`sin(→asin(`, `cos(→acos(`, `tan(→atan(`, `x²→√(`, `ln(→e^(`,
    `(-)→Ans`), then clears the flag (one-shot).
  - Added an `armed` property to `CalcKey` that thickens the border
    and lightens the fill when active; the armed border uses a warm
    amber that reads well against both the amber 2ND body and the
    neutral ALPHA body.
  - Added header badges (`2ND` in amber, `α` in green) that appear
    only when the corresponding flag is armed.
  - Physical-key modifier arming: `\` toggles 2ND, `Tab` toggles
    ALPHA. Chosen to avoid conflict with existing literal keymap
    entries.
  - ALPHA has no letter variants wired yet — it arms and clears
    silently on the next keypress. Wiring A–Z is blocked on the
    variable-registry work (ROADMAP: ALPHA letter bindings).
- **Trade-offs:** Doubled the state managed in the root QML, but the
  dispatcher pattern kept CalcKey.qml mostly untouched — only the
  visual-feedback plumbing changed. All existing keypresses route
  through the same path, so regressions would have been caught by
  any key working incorrectly in a no-modifier state.
- **Notes:** Supersedes the narrower ALPHA-only proposal. Unlocks
  the engine's existing `asin/acos/atan` tokens that had no UI
  exposure before. Next step in this area: MODE menu wiring,
  ALPHA-lock mode (double-tap or 2ND+ALPHA), and the variable
  registry that enables ALPHA letter inputs.

### IMP-012: Right-click copy on history entries

- **Status:** applied (2026-04-18)
- **Location:** [app/qml/components/HistoryPane.qml](app/qml/components/HistoryPane.qml)
- **Effort:** small
- **Description:** The history pane previously had no way to extract
  past results — no selection, no copy. Long matrix answers were
  unreachable as plain text, and users couldn't re-use a prior result in
  another application.
- **Change:** Added a right-click context menu on each history entry
  with a single `Copy` action, scoped to that one entry (copies the full
  `expression = result` string, not the whole history). Uses a hidden
  `TextEdit` as a clipboard proxy since Qt6 QML doesn't expose the
  system clipboard directly. Also added a subtle hover highlight on
  entries so it's discoverable that they're interactive.
- **Notes:** Left-click is currently a no-op — a future improvement
  could re-load the expression into the display for editing (see the
  delegate comment). Triggered by user request after BUG-018 verification
  when they noticed long answers in history needed to be copyable.

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

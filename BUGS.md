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

### BUG-025: Assignments echo their value in programs (TI-83 stores are silent) — floods getKey loops

- **Status:** fix implemented (2026-08-08), pending GUI verification
- **Found:** 2026-08-08 (P5b getKey testing — a `getKey→K` poll loop printed `0` every iteration)
- **Location:** [interpreter/src/interpreter.cpp](interpreter/src/interpreter.cpp) (execStatement bare-eval + string-store paths)
- **Severity:** medium (wrong/cluttered program output; makes getKey loops unusable)
- **Description:** `execStatement` echoed the result of *every* bare statement, including assignments (`5→A`, `getKey→K`, `"HI"→Str1`). On the TI-83 an assignment in a program runs **silently** — only a bare expression with no store echoes (like the home screen). The bug flooded any poll loop (`While 1:getKey→K:…:End` prints `0` thousands of times) and cluttered ordinary output (the earlier `TEST` run showed a stream of intermediate `T`/`P` values from `T+1→T` echoes).
- **Reproduction (was):** Run `getKey→K:Disp K` in a loop → screen fills with `0`. Or `5→A` on its own program line → prints `5`.
- **Fix:** Echo only when the statement is **not** an assignment — added `isAssignment()` (detects a top-level `→`/`->` outside strings); numeric store and string store (`→StrN`) are now silent. Bare expressions still echo. All 735 tests stay green (they assert `Disp`/`last` output or bare-expr echo, not store echoes).
- **Notes:** Surfaced the same clutter in the P5 `TEST` self-check (now clean). Fixed alongside P5b getKey since it blocked verifying it.

### BUG-024: Saved programs (and all state) vanish — the test suite overwrites the real state file, plus non-atomic writes + multi-instance last-write-wins

- **Status:** fix implemented (2026-08-06), pending GUI verification
- **Found:** 2026-08-06 (TI-BASIC P5 session; user reported programs missing after rebuild/restart)
- **Location:** [tests/test_math.cpp:75](tests/test_math.cpp#L75) (**primary** — tests wrote the real state path), [graph_ui/src/ui_controller.cpp:492](graph_ui/src/ui_controller.cpp#L492) (`writeJsonFile` truncate-then-write), [app/main.cpp](app/main.cpp) (30s autosave / exit save / no single-instance guard), [graph_ui/src/ui_controller.cpp:509](graph_ui/src/ui_controller.cpp#L509) (`loadState` skips on parse error)
- **Severity:** high (silent, permanent loss of user-created programs and all persisted state)
- **Description:** State (`~/.local/state/tux-ti83/state.json`) is written by immediate `saveState()` on `saveProgram`/`deleteProgram`, a 30s autosave timer, and a save-on-exit — each dumping the **entire in-memory state**. Four compounding defects made this lossy:
  0. **(PRIMARY) The test suite was not isolated from the real state file.** `tux_ti83_tests` constructs `UIController`s and calls `saveProgram`/`deleteProgram`, each of which calls `saveState()` → the real `~/.local/state/tux-ti83/state.json` (resolved from `XDG_STATE_HOME`). The interpreter tests create temp programs and delete them all in cleanup, so the **final write leaves `programs: {}`**. Every `./tux_ti83_tests` run — i.e. after every rebuild during development — silently wiped the user's saved programs. Confirmed by watching the real file's mtime change across a test run. This is almost certainly the main cause of the user's report.
  1. **Non-atomic write.** `writeJsonFile` opened the real file with `Truncate` then wrote. A kill mid-write (IDE stop during a rebuild) left `state.json` truncated/corrupt.
  2. **Corrupt/failed load → silent empty → overwrite.** `loadState` bailed on a bad file leaving `m_programs` **empty**; the next autosave/exit-save wrote that emptiness back, making a transient bad read permanent.
  3. **Multi-instance last-write-wins.** No single-instance guard. Leaving the pre-rebuild process running while launching the new build gave two instances one file; the stale instance's autosave/exit-save clobbered the new instance's programs — the "when we rebuild" correlation.
- **Reproduction (primary):** Note saved programs exist; run `./tux_ti83_tests`; relaunch app → programs gone. (Confirmed: real `state.json` mtime updates on every test run, ending with `programs: {}`.)
- **Fix (implemented, pending GUI verify):**
  0. **Test isolation** — `tests/test_math.cpp` `main()` now `qputenv("XDG_STATE_HOME", <tempdir>)` before constructing any `UIController`, so the suite writes a throwaway dir and never touches `~/.local/state`. Verified: real file mtime unchanged across a run; 720 tests still green.
  a. **Atomic write** — `writeJsonFile` uses `QSaveFile` (temp + `commit()` rename); a kill mid-write can't corrupt the live file.
  b. **Preserve corrupt data** — `loadState` distinguishes missing file (first run) from an existing-but-unparseable one, and renames the latter to `state.json.corrupt` instead of leaving it to be silently overwritten.
  c. **Single-instance guard** — `main()` takes a `QLockFile` beside `state.json`; a second instance exits with a warning (stale locks from a dead PID auto-reclaimed). Verified with two headless instances.
- **Notes:** The user's DBL/RET/TEST programs (wiped by the test-suite bug during this session) were restored to `state.json` from the confirmed-passing source. Move to Fixed once the user confirms programs survive a rebuild+test+restart cycle in the GUI.

---

## Fixed

### BUG-023: On-screen keypad can't enter Y-function tokens — "Y1" becomes variable-Y × 1

- **Status:** fixed (2026-08-01, same session as IMP-045)
- **Location:** [app/qml/components/YVarsPopup.qml](app/qml/components/YVarsPopup.qml) (new), [app/qml/Main.qml](app/qml/Main.qml) (2ND+X trigger)
- **Severity:** medium (silent wrong answer for home-screen Y-VARS recall via mouse)
- **Description:** The on-screen keypad had no way to insert a Y-function token (`Y1`..`Y0`). Entering "Y1" via ALPHA+`1` inserts the **letter-Y scalar variable** (`VarY`) followed by `Num1`, so `Y1(3)` parsed as `Y·1·(3)` and evaluated to `0` (Y unset) instead of recalling the `Y1` function → `9`. The physical-keyboard `pendingY` fuse worked, but there was no mouse-only equivalent.
- **Reproduction (was):** Set `Y1=X^2` (plots). On the home screen via the on-screen keys, type `Y1(3)` using ALPHA+1 for the Y → showed `0`, not `9`.
- **Fix:** Added `YVarsPopup` — a Y-VARS picker (buttons `Y1`..`Y0`) opened via **2ND+X**. Each button calls `processExpression("Yn")`, which tokenises to the fused `Yn` function token (longest match), so the on-screen path now inserts the real function reference. Chose option (b) from the original notes (proper entry point, no parser ambiguity) over a `[VarY,digit]` rewrite. Regression test ("BUG-023") mirrors the popup's insertion path (recall → 9) and contrasts it with the broken `VarY`+digit form (→ 0).
- **Notes:** Not an IMP-045 regression; pre-existing since Y-VARS recall landed. Self-reference while the target slot is the active home slot is still the separate home-screen/Y-slot-overlap quirk (BUG-022 family), unaffected by this fix.

### BUG-022: Y-editor list shows the live edit string, not the plotted function, after a home-screen eval

- **Status:** fixed (2026-08-01, RectGC/ExprOn session)
- **Location:** [app/qml/components/YEditorPopup.qml](app/qml/components/YEditorPopup.qml):101
- **Severity:** low (display-only; the plotted curve and evaluation are unaffected)
- **Description:** `functionExpr(i)` returns `m_displayStrings[i]`, the live per-slot edit/display buffer. When a slot is the active selector and the user evaluates on the home screen, that buffer is overwritten with the **result** (e.g. `0`) while the plotted token buffer `m_functionBuffers[i]` keeps the real function (`X^2`). So the Y-editor row for that slot rendered `0` instead of `X²`, even though the graph still plots `X²`.
- **Reproduction (was):** Set Y1 to `X^2` (it plots). With Y1 the active slot, type `X^2` on the home screen and press ENTER (shows `0`). Open the Y= editor → Y1 showed `0`, not `X^2`.
- **Fix:** Use `uiController.functionBufferText(index)` in place of `functionExpr(index)` in YEditorPopup. `functionBufferText(i)` (added this session) detokenises `m_functionBuffers[i]`, the plotted buffer, so the list always matches the drawn curve. Same helper fixed the trace ExprOn overlay's original `Y1=0` display.
- **Notes:** Surfaced while implementing FORMAT → ExprOn (the overlay had the identical `Y1=0` symptom). Both call sites now go through the buffer detokeniser.

### BUG-021: Loaded state leaves display in Inputting, so first keystroke appends to the prior buffer

- **Status:** fixed (2026-05-23, same session as IMP-039)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) `loadState()`
- **Severity:** medium (user-visible — silently appends to the loaded expression instead of starting a fresh one)
- **Description:** After `loadState()` restored the persisted Y= buffers via `processExpression`, the display state stayed at `Inputting` with the cursor positioned at the end of the active buffer. The next keystroke went through the normal Inputting path, which appends instead of clearing. So a user whose saved Y1 was `0`, typing `52` on launch, saw `052` rather than `52`.
- **Reproduction (was):** With any saved Y1 content, close the GUI cleanly to persist state, relaunch, and type a digit on the active slot — the digit landed after the saved content rather than replacing it.
- **Fix:** After loadState's restore loop, set `m_displayState = Evaluated` and emit `displayStateChanged`. This applies the existing state-machine rule "next digit/function keypress in Evaluated state: clears expr, returns to Inputting" — the loaded content is treated as a stale previous result; the first keystroke replaces it. Users who want to preserve a slot's saved content can switch to a different slot via FunctionSelector before typing.
- **Notes:** Surfaced and fixed during IMP-039 testing. Latent in the original IMP-033 persistence design but only obvious once a user actually persisted then resumed.

### BUG-020: Cross-slot cursor position causes vector out-of-bounds insertion

- **Status:** fixed (2026-05-23, same session as IMP-033)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) `insertToken()`
- **Severity:** high (SIGSEGV; user-reproducible)
- **Description:** `m_cursorPos` is a single value shared across all three function slots (Y1/Y2/Y3). Any time the active slot switched — via `setActiveFunction`, `recallLastEntry`, or (newly) `loadState` — the cursor was left pointing into the previous slot's range. If the new slot's buffer was shorter (or empty), the next `insertToken` call did `currentBuf.insert(currentBuf.begin() + m_cursorPos, ...)` where `m_cursorPos > buf.size()` — past-end iterator, undefined behaviour. Manifested as a segfault on the first relaunch with persistent state (loaded Y1="0" → cursor at 1 → switched to empty Y2 → `begin()+1` on a zero-element vector → SIGSEGV).
- **Reproduction (was):** With persisted state where slot N had non-empty content and slot N+1 was empty, restoring Y= buffers during `loadState` crashed mid-replay. Also potentially triggerable in normal use by switching from a long Y1 to an empty Y2 then typing — though the `atEnd` branch usually shielded the explicit-typing case since it was the first thing the user did after switching.
- **Fix:** Defensive clamp at the top of `insertToken`: `if (m_cursorPos > buf.size()) m_cursorPos = buf.size();`. User-facing effect is "switching slots puts the cursor at the end of the new buffer", matching what real TI-83 hardware does.
- **Notes:** Surfaced by IMP-033 (persistent state). Latent in earlier code but unlikely to trigger in interactive use because the `atEnd` shortcut handled most switch-then-type flows correctly; only a switch with `m_cursorPos > 0` AND a strictly-shorter-than-cursor new buffer exposes the UB.

### BUG-012: Graph curve colours can shift when some Y slots are empty

- **Status:** fixed (2026-05-08)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) `getMultiGraphPoints()`
- **Severity:** low (cosmetic / confusing, not incorrect math)
- **Description:** `getMultiGraphPoints()` skipped empty function buffers with a `continue`, so the returned list was compacted. The QML canvas iterates the result and uses the result index for colour selection — meaning Y3 with Y1 empty would land at result index 0 and render in Y1's blue.
- **Reproduction (was):** Y1 empty, Y3 = `X^2`, switch to graph mode → parabola in Y1's blue.
- **Fix:** Removed the `continue` — `getMultiGraphPoints()` now always emits one inner list per Y slot, using an empty list for slots with no expression. The QML canvas already skipped empty inner lists with `if (!pts || pts.length === 0) continue`, so the visual change is just that `f` is now the slot index (0=Y1, 1=Y2, 2=Y3) and the colour-by-index mapping is stable.
- **Notes:** Pre-existing from the legacy UI. Verified end-to-end — Y1 empty + Y2=`X^2` + Y3=`X^3` now renders Y2 in red and Y3 in green (Y1's blue stays unused).

### BUG-019: Bare `.` in the buffer crashes the engine via uncaught `std::stod` exception

- **Status:** fixed (2026-04-18, same session as IMP-021)
- **Location:** [core_math/src/core_math.cpp](core_math/src/core_math.cpp) `evaluate()` digit-flush lambda
- **Severity:** high (process crash, not just an error)
- **Description:** Pre-existing latent crash. The digit-coalescing pass collected `Token::Decimal` characters into `currentNumStr` and called `std::stod(currentNumStr)` on flush. For a bare `.` (or any malformed numeric run), `std::stod` throws `std::invalid_argument`, which propagated up uncaught and aborted the process via `terminate()`. Discovered when adding `Token::Colon` (IMP-021) — the Colon split surfaced the same path more readily, but the underlying bug is older.
- **Reproduction:** `./build/tux_ti83_cli '.'` aborts with `std::invalid_argument: stod`. In the GUI, pressing `.` then ENTER produced the same crash.
- **Fix:** Wrapped the `std::stod` call in `try/catch`, set a `parseFailed` flag, and return `ERR:SYNTAX` from `evaluate()` if any parse failed during the pass. Bare `.` now produces `ERR:SYNTAX` (matching TI-83 behaviour) instead of crashing.
- **Notes:** Discovered while implementing IMP-021 (`:` statement separator). Fixed transparently as part of the same change since it was blocking GUI verification of the new feature.

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

### BUG-018: MatrixPopup MATH-tab `^-1` click silently does nothing
- **Status:** fixed (2026-04-08, user-reported minutes after the inverse/rref feature shipped)
- **Location:** [app/qml/components/MatrixPopup.qml](app/qml/components/MatrixPopup.qml) MATH-tab delegate `onClicked`
- **Severity:** medium (a shipping menu entry was inert)
- **Description:** The Phase B inverse feature added a `4: ^-1 (inverse)`
  entry to MatrixPopup's MATH tab with `input: "^-1"`. The expectation
  was that clicking it would insert the three tokens `^`, `-`, `1` into
  the expression buffer. In the same turn I updated the NAMES tab's
  `onClicked` to route through `processExpression` (which tokenises
  multi-char inputs), but the MATH-tab `onClicked` still called
  `processInput(modelData.input)`. `processInput` does an exact-match
  lookup against `kTokens`, and `"^-1"` isn't a kTokens entry —
  insertToken returned silently and the click did nothing. The
  tokenisation required to split `^-1` lives one layer up in
  `processExpression`.
- **Reproduction:** From the GUI: MATRX → MATH tab → click
  `4: ^-1 (inverse)` → nothing inserted into the display.
- **Fix:** Changed the MATH-tab `onClicked` to call
  `processExpression(modelData.input)` instead, matching the NAMES-tab
  update from the previous turn. Now `^-1` tokenises cleanly into
  `^ / - / 1`, the unary-negation disambiguation promotes the `-` to
  `Neg` (since it follows `Pow`), and the evaluator's extended `Token::Pow`
  branch (`matrix ^ -1`) applies the inverse.
- **Notes:** Missed this in the previous turn's `replace_all` because
  the NAMES tab used `modelData` (bare string) while the MATH tab used
  `modelData.input` — different source patterns, so only one got
  replaced. A consistency sweep of the two tabs would've caught it.

### BUG-017: Long display output is clipped with no way to see, select, or copy
- **Status:** fixed (2026-04-08, user-reported after the matrix-transpose demo)
- **Location:** [app/qml/components/Display.qml](app/qml/components/Display.qml) — the bottom-line main-readout region
- **Severity:** medium (UX — correctness unaffected but the result is literally unreachable for wide output)
- **Description:** The LCD's main readout was a plain `Text` element
  inside a right-anchored `Row`. When the displayed result was wider
  than the panel (e.g., a 3×3 matrix: `[[1,4,7][2,5,8][3,6,9]]`), the
  Row extended past the left edge of the parent Rectangle and got
  clipped. Users saw `1,4,7][2,5,8][3,6,9]]` with the leading `[[`
  missing entirely, no way to scroll, no way to select, no way to
  copy to clipboard.
- **Reproduction:** Set matrix [A] to any 3×3, press MATRX → MATH tab
  → `2: T(`, press MATRX again → NAMES → `[A]`, close paren, ENTER.
  The resulting 21-char string was wider than the ~400px-wide display
  panel. Leading characters were cut off.
- **Fix:** Replaced the `Text` with a read-only `TextInput` in
  `Display.qml`. This gives us:
  - native horizontal scrolling (cursor movement via arrow keys
    scrolls the visible window — Home/End jump to edges)
  - mouse-drag selection + Ctrl+C copy to clipboard (built-in)
  - `onTextChanged: cursorPosition = text.length` so new output
    always defaults to "scrolled to the right edge" (the last
    characters of a wide result are visible)
  - `activeFocusOnPress: root.currentState !== 0` — the display is
    only focusable after an ENTER, so typing keys during input still
    reaches the main keyboard handler
  - `cursorDelegate` preserves the existing blinking-cursor visual,
    with `color: readout.color` so the cursor automatically matches
    the text (white while typing, green when a result is shown, red
    on error)
- **Notes:** TextInput's native key handling consumes arrow keys,
  Home/End, Ctrl+A/C/X for selection/navigation but lets other keys
  (Enter, digits, operators, function keys) bubble to the parent
  RowLayout's `Keys.onPressed`, so the main keyboard handler still
  receives input naturally when the TextInput has focus. User
  verified all seven behaviours (scroll via arrows, select, copy,
  resume typing a new expression, blink persists, focus-gate, Ctrl+C
  roundtrip) after the fix.

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

### BUG-016: `max(sin(0), cos(0))` errors with ERR:SYNTAX from the GUI
- **Status:** fixed (2026-04-08, user-reported after Phase B Wave 2 shipped)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) `kTokens` + [app/qml/Main.qml](app/qml/Main.qml) (SCIENTIFIC CalcKey handlers and keyboard shortcut map) + [core_math/src/core_math.cpp](core_math/src/core_math.cpp) `has_built_in_paren`
- **Severity:** medium
- **Description:** Phase B Wave 2's binary-function infrastructure pushed
  a synthetic `LeftParen` onto the shunting-yard stack for functions
  with input strings ending in `(` (abs, int, round, min, max, mod,
  det). But trig/log functions (sin, cos, tan, asin, acos, atan, log,
  ln, √) had bare input strings (`"sin"` not `"sin("`) and did NOT get
  the synth. The two conventions conflicted when nested: in
  `max(sin(0), cos(0))`, the inner sin's `)` closed max's synthetic
  LeftParen, popping Max prematurely and leaving the evaluator with
  only one operand when Max actually fired. Result: ERR:SYNTAX.
  Interestingly the test suite caught no failure because the CLI
  tokeniser emitted an explicit `LeftParen` for inputs matching
  `"("` after `sin` (longest-match didn't produce `"sin("` since that
  wasn't in the map), giving a different buffer shape than the GUI
  produced. So GUI and CLI disagreed — user found the GUI case, tests
  happened to take the CLI path.
- **Reproduction:** From the GUI: click MATH → `max(` → `sin(` →
  `0` → `)` → `,` → `cos(` → `0` → `)` → `)` → ENTER. Was
  `ERR:SYNTAX`, should be `1`.
- **Fix:** Unified the function-token convention. All functions with
  displayStr ending in `(` now also have kTokens input strings ending
  in `(` — `"sin("`, `"cos("`, `"tan("`, `"asin("`, `"acos("`,
  `"atan("`, `"log("`, `"ln("`, `"√("`. The CalcKey handlers in
  SCIENTIFIC send the new input strings; the keyboard shortcut map
  updates `s`, `c`, `t`, `l`, `n`, `r` to send the `(`-suffixed forms.
  `has_built_in_paren` now returns true for all of these, giving each
  one a synthetic LeftParen in the shunting-yard. Buffers produced by
  the GUI and the CLI tokeniser now match token-for-token.
- **Notes:** Caught by the user within minutes of the Wave 2 release —
  good case for the regression suite being load-bearing, and a
  reminder that visible-in-GUI-but-not-in-tests bugs exist when the
  two code paths diverge. Added an implicit future task: keep
  CalcKey/input-string convention consistent across all tokens.

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

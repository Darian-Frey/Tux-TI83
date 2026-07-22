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

---

## Applied

### IMP-007 + IMP-008: Matrix editor v2 — selector, variable dimensions, read-back

- **Status:** applied (2026-07-22)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/components/MatrixPopup.qml](app/qml/components/MatrixPopup.qml)
- **Effort:** medium
- **Description:** The EDIT tab was hardcoded to `[A]` at a fixed 3×3 and always opened with blank fields — editing an existing matrix meant retyping every value, and any left-blank cell silently overwrote stored data with 0. IMP-007 (read existing values back) and IMP-008 (matrix selector + variable dimensions) shipped together as a single "matrix editor v2" pass, as their entries anticipated.
- **Change:**
  - Controller: new static `matrixTokenForName()` helper maps `[A]`–`[J]` (or bare `A`–`J`) to the registry `Token`, replacing the hardcoded `[A]/[B]/[C]` if-chain in `updateMatrix`. New `Q_INVOKABLE QVariantMap getMatrix(const QString&) const` returns `{rows, cols, data}` for the named slot (rows=cols=0 for unset/unknown). Persistence (`persistMatrix`/`restoreMatrix`) and the `kTokens` NAMES table extended from `[A]`–`[C]` to `[A]`–`[E]` (engine already backs `[A]`–`[J]`; UI now exposes five, matching TI-83 hardware defaults).
  - QML: EDIT tab rewritten. A `[A]`–`[E]` selector row (selected slot highlighted with the expr-blue border), `R`/`C` `[−] N [+]` steppers (1–6, via a new inline `DimStepper` component), a live-reflowing `GridLayout` (`columns: mCols`, `model: mRows*mCols`), and a dynamic header + `SAVE TO [x]` label. The working values live in a flat `cells` array that fields initialise from (`Component.onCompleted`) and write back to (`onTextChanged`); `loadMatrix()` pulls stored values via `getMatrix` and pushes them into the fields through `Qt.callLater(syncFields)`. Load fires on popup open, on EDIT-tab visibility, and on every selector click.
- **Trade-offs:** Dimension cap is 1×6 (not TI-83's 99×99) — a QML grid of TextFields is impractical past a handful of rows, and the popup has finite height. Resize preserves cells by flat index (fill top-left, drop the tail), so growing the column count shifts existing values; users set dimensions before filling. Both acceptable for a desktop editor; the flat-index behaviour is documented in the component header.
- **Notes:** Verified end-to-end in the GUI — read-back of a stored `[A]` (incl. the 2000000 element from IMP-010 testing), live dimension changes, per-slot editing of `[B]`, save/recall round-trip through NAMES, and re-open read-back. 279/279 regression tests unchanged (the new paths are GUI-only — matrices remain non-CLI-populatable, tracked as the still-open [IMP-011](IMPROVEMENTS.md)).

### IMP-004: `Token::Num0` doubles as the "numeric literal" sentinel

- **Status:** applied (2026-07-22)
- **Location:** [core_math/include/capsules/capsule_math.hpp](core_math/include/capsules/capsule_math.hpp) (Token enum), [core_math/src/core_math.cpp](core_math/src/core_math.cpp) (`MathStateMachine::evaluate`)
- **Effort:** small
- **Description:** The digit-flush prepass in `evaluate()` coalesced runs of `Num0..Num9`/`Decimal` into a single parsed double and pushed `Token::Num0` as a placeholder, with the value stored in the parallel `numericValues` array. Downstream passes then read `Token::Num0` as "a numeric literal, look up its value" — conflating that meaning with "the literal digit 0". Anyone reading the post-flush stream had to know the trick.
- **Change:** Took the minimal-rename option (not the fuller `RpnNode` refactor). Added a dedicated `Token::NumLiteral` at the **end** of the enum — placed there so the digit-detection `(int)t in [0,9]` check and the contiguous `VarA..VarZ`/`MatA..MatJ` ranges renumber nothing. Renamed the five post-flush sentinel sites (the flush push, both implicit-mul value-like classifiers, the RPN builder's value lookup, and the evaluator's literal-push) to `NumLiteral`. The four pre-flush raw-digit emitters (`emitDigits`' base offset and the nDeriv default-`h` `0.001` emit) correctly stay as raw `Num0..Num9` tokens — they feed *into* the flush pass. Pure mechanical rename, zero behaviour change: 279/279 regression tests pass unchanged.
- **Trade-offs:** None for the minimal rename. The parallel-array `numericValues` design is untouched — the fuller `vector<RpnNode>` refactor that would eliminate it remains available as a future step, now unblocked by the clearer sentinel. Worth having done before negative literals / scientific-notation input extend this path.
- **Notes:** Applied alongside IMP-010 as a "clear the trivial improvements backlog" pass. The 279-test suite made the hot-path rename safe to verify mechanically.

### IMP-010: Route matrix element formatting through `formatScalar`

- **Status:** applied (2026-07-22)
- **Location:** [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp) `evaluate()` matrix-result formatting branch
- **Effort:** trivial (one-line change)
- **Description:** The scalar-result branch of `evaluate()` routed through `formatScalar(double)` (10-significant-digit precision, base/Notation/Decimal aware), but the matrix-result branch used bare `QString::number(matrixValue.at(i, j))` at default 6-digit precision. A matrix element ≥ 10⁶ would render that cell in scientific notation, inconsistent with scalar display, and matrix cells ignored the active MODE display settings entirely.
- **Change:** Replaced `QString::number(result.matrixValue.at(i, j))` with `formatScalar(result.matrixValue.at(i, j))` in the matrix-to-string loop. One line. Matrix cells now share the exact same display semantic as scalars — 10-sig-digit `g` format, and (as a bonus, more TI-83-faithful) they now also honour the active Notation/Decimal/Base MODE settings.
- **Trade-offs:** None for the precision fix. Matrix cells now also reflect Sci/Fix/Base mode; this matches real TI-83 behaviour (matrix elements respect display mode) so it's an improvement rather than a surprise.
- **Notes:** Applied alongside IMP-004. Not previously user-visible because no regression matrix had elements large enough to hit the 6-digit cliff; the fix pre-empts it.

### IMP-044: Calculus framework — `fnInt(`, `nDeriv(`, `sum(`, `prod(`

- **Status:** applied (2026-05-25)
- **Location:** [core_math/include/capsules/capsule_math.hpp](core_math/include/capsules/capsule_math.hpp), [core_math/src/core_math.cpp](core_math/src/core_math.cpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/components/MathMenuPopup.qml](app/qml/components/MathMenuPopup.qml), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** large
- **Description:** Four calculus functions whose first argument must stay unevaluated so the engine can sample it at many bound points. The shunting-yard is strictly eager, so this required a dedicated preprocessing-and-side-channel framework — the framework lift is paid once and reused by all four functions.
- **Change:**
  - Engine: eight new tokens — surface (`FnInt`, `NDeriv`, `Sum`, `Prod`) and synthetic-after-rewrite (`FnIntCall`, `NDerivCall`, `SumCall`, `ProdCall`). The surface tokens never reach the shunting-yard.
  - Rewrite pass (new, runs as the first operation in `evaluate()` before digit-flush): walks the source tokens, finds each surface deferred-call function, locates the matching `)` (depth-aware on built-in-paren functions via the new `opensParenScope` helper — the missing-built-in-paren bug caused `nDeriv(sin(X), X, 0)` and all nested calls to fail with `ERR:SYNTAX` until this was found), splits the parenthesised contents by top-level commas, recursively rewrites every argument so inner deferred calls resolve first, captures the unevaluated first argument plus the bound variable letter into a `thread_local std::vector<DeferredCall>`, and emits a synthetic *Call token followed by the eager argument subexpressions (lower, upper / point, h / start, end) and the side-table index `K` encoded as raw digit tokens. The synthetic call has `has_built_in_paren=true` so the shunting-yard adds a synthetic LeftParen and treats it as a normal n-ary function.
  - RAII depth guard (`EvalGuard`): only the outermost `evaluate()` call clears the deferred table; nested calls (Y-VARS recursion, deferred-call handlers recursing on a captured expression) inherit, which is what lets a captured sub-stream containing already-rewritten synthetic call tokens resolve against the parent's table.
  - Evaluator branches: `FnIntCall` runs composite Simpson's rule with N=100 even subintervals (handles `a > b` by flipping and negating, `a == b` short-circuits to 0). `NDerivCall` symmetric finite difference; default h = 0.001 if the user omits the 4th arg, `h == 0` returns `ERR:DOMAIN`. `SumCall` / `ProdCall` floor the bounds, iterate inclusive, return the identity element (0 / 1) for an empty range, and cap iteration at 100,000 to keep `sum(X, X, 1, 10^12)` from wedging the engine. All four share a `sample` lambda that saves/restores the bound variable's registry slot and passes the sample as `xValue` when the bound variable is X (so graph-mode X resolution sees the loop value). Sub-eval errors propagate verbatim — the initial implementation flattened them to a generic `"Error"`, which masked recursion / divide-by-zero / domain errors as `ERR:SYNTAX`.
  - MATH menu: four new entries route through the existing `processExpression` insertion path.
  - 19 new regression tests cover closed-form integrals (X², X³, sin over [0, π]), derivatives, exact sums / products, empty ranges, the iteration cap, nested calls (`fnInt(fnInt(1, Y, 0, X), X, 0, 1)` = 0.5), the X-vs-other-var sampler split, and the syntax-error gates (non-Var var arg, wrong arity). 279/279 passing.
- **Trade-offs:**
  - `sum(`/`prod(` use a 4-arg textbook form (`expr, var, start, end`) rather than TI-83's list-based `sum(seq(...))` shape. List-based forms can be added later as overloads once Phase C lists land; the 4-arg form will stay because it's clearer and unambiguous.
  - `seq(` is deliberately not in this commit — it returns a list, so there's nothing useful to do without list infrastructure. ROADMAP entry stays 📅, pending Phase C.
  - The call-index encoding via raw digit tokens means each rewritten call adds 1–3 extra digit tokens to the stream (one per decimal digit of `K`). At realistic expression sizes (<100 deferred calls per expression) this is invisible; the alternative — extending `Token` with a payload field — would have rippled through every site that compares or copies tokens.
- **Notes:** The framework is reusable: any future function that needs an unevaluated expression argument (e.g. an equation solver `solve(eqn, var, guess)`) can follow the same surface-token + *Call-token pattern and the existing rewriter handles arg capture, nesting, and variable binding for free.

### IMP-043: DEC/HEX/OCT/BIN base conversion

- **Status:** applied (2026-05-24) — closes the "Base conversion" entry in ROADMAP Number systems.
- **Location:** [core_math/include/capsules/capsule_math.hpp](core_math/include/capsules/capsule_math.hpp), [core_math/src/core_math.cpp](core_math/src/core_math.cpp), [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/components/MODEPopup.qml](app/qml/components/MODEPopup.qml), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** trivial
- **Description:** Adds a Base row to the MODE popup with Dec / Hex / Oct / Bin segments. The selection drives a new formatter branch that renders integer-valued scalars in the chosen base with sign + prefix (`0xFF`, `0o77`, `0b1010`, `-0xFF`). Non-integer values, NaN, ±inf, and magnitudes outside int64 range fall back to the existing Notation/Decimal formatter so the rest of the calculator continues to work in non-Dec modes.
- **Change:**
  - Engine: `enum class NumberBase { Dec, Hex, Oct, Bin }` + `MathStateMachine::numberBase` static (initial value `Dec`). The engine itself is format-agnostic — only `UIController::formatScalar` consults the static.
  - Formatter: new branch ahead of the Notation/Decimal path. Guards on `std::isfinite(value)`, `value == std::floor(value)`, and int64 range; if any guard fails the branch is skipped and the existing Normal/Sci/Eng path runs unchanged. Magnitude is taken via the standard `-(iv+1)+1` trick so `INT64_MIN` doesn't overflow during negation. Uppercase hex per common convention.
  - QML: new `numberBase` Q_PROPERTY with getter/setter/signal and a one-line Base row in `MODEPopup`. Pattern mirrors the existing Angle/Notation/Decimal rows.
  - Persistence: `mode["numberBase"]` field in `state.json`. `loadState`, `resetAll`, and the QML binding all kept in sync.
  - 12 new regression tests covering Dec passthrough, positive/zero/larger values in each base, negatives with sign prefix, non-integer fallback, and an end-to-end `2^8 → 0x100` check. 255/255 passing.
- **Trade-offs:** Sign-prefix style (`-0xFF`) is chosen over two's-complement padding for readability on a calculator — a 16-digit `0xFFFFFFFFFFFFFFFF` representation of `-1` would be technically more "bitwise" but harder to read at a glance. Bitwise programming use-cases that want the padded form aren't a TI-83 use case anyway.
- **Notes:** Engine has no Hex/Oct/Bin *input* literal yet — this is display-side only. Adding `0xFF` parsing as a literal would be an independent follow-up if a future user wanted to type hex.

### IMP-042: `Y1(arg)` explicit-argument form for Y-VARS

- **Status:** applied (2026-05-24) — closes the v1 limitation noted in IMP-036.
- **Location:** [core_math/include/capsules/capsule_math.hpp](core_math/include/capsules/capsule_math.hpp), [core_math/src/core_math.cpp](core_math/src/core_math.cpp), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** medium
- **Description:** IMP-036 only supported the bare form (`Y1` uses the current X). `Y1(3)` parsed as `Y1 * 3` via implicit-mul rather than evaluating Y1 with X=3. Closing this limitation requires Y_n to behave both as a leaf token (bare) and a function-with-arg, which the shunting-yard parser doesn't natively support.
- **Change:**
  - Engine: three internal tokens `Y1Call` / `Y2Call` / `Y3Call`. These have function semantics with built-in paren (in `is_function` and `has_built_in_paren`). They're never typed directly — synthesised by a new preprocessing pass slotted between Sto-rewriting and implicit-mul injection. The pass walks the post-Sto token stream and collapses any `[Y_n, LeftParen]` adjacency into a single `Y_nCall` token, dropping the LeftParen (the synthetic LeftParen pushed by the function-with-built-in-paren machinery takes its place).
  - The implicit-mul pass operates on the post-Y-call token stream — by the time it runs, `Y_n` is no longer followed by `LeftParen` and so doesn't get wedged with an `ImplicitMul`.
  - Hoisted `activeYn` (the cycle-detection `static thread_local std::set<int>`) from the bare-Y branch's block scope to function scope so the bare and call forms share one guard. Mixed cycles (bare Y1 referencing Y2 which references Y1(0)) now trip the same recursion check.
  - New evaluator branch for `Y_nCall`: pops the argument off the operand stack and evaluates the referenced buffer with the popped value as `xValue` (instead of the outer call's xValue). Same cycle / empty-buffer / type-error handling as the bare form.
  - 8 new regression tests: `Y1(3)`, `Y1(3+1)`, `Y1(-2)` (unary-minus arg), `Y1(Y3(3))` (nested call), empty-target call form, self-reference detection, and a mixed cycle across the bare and call forms. 243/243 passing.
- **Trade-offs:** Rewrite happens at evaluation time, not in the live buffer — `m_displayStrings` and any post-edit re-render still show the original `Y1(...)` form (correct). Could surface the rewrite into the buffer too, but the display side already does the right thing and the eval rewrite is per-call, so this is fine.
- **Notes:** Pairs well with the TABLE view (IMP-035) — `Y2 = Y1(X)` shows Y2 column matching Y1's at every X. The previously-documented "Y1(3) parses as Y1*3" limitation is now closed.

### IMP-041: `e^(` and `sgn(` as proper unary functions

- **Status:** applied (2026-05-24)
- **Location:** [core_math/include/capsules/capsule_math.hpp](core_math/include/capsules/capsule_math.hpp), [core_math/src/core_math.cpp](core_math/src/core_math.cpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/components/MathMenuPopup.qml](app/qml/components/MathMenuPopup.qml), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** trivial
- **Description:** Two small math primitives that were missing or awkward. `e^(` was reachable via the 2ND+LN macro but produced three tokens (`E`, `Pow`, `LeftParen`) — semantically identical to `std::exp` but not first-class. `sgn(` (sign function) wasn't there at all.
- **Change:**
  - Engine: added `Token::Exp` and `Token::Sgn`. Standard unary plumbing — `precedence`, `is_function`, `has_built_in_paren` all extended. Evaluator branches: `Exp` calls `std::exp`; `Sgn` returns `-1`/`0`/`+1`.
  - Controller: kTokens entries `e^(` → `Token::Exp` and `sgn(` → `Token::Sgn`. The 2ND+LN macro string `"e^("` now tokenises as a single `Exp` token (longest-match in `tokenize`).
  - UI: MathMenuPopup gained `e^(` and `sgn(` entries so they're discoverable without keyboard shortcuts.
  - Tests: 8 new assertions covering inverse round-trip (`e^(ln(5)) = 5`), all three Sgn branches, non-integer negative, and juxtaposition (`2sgn(7) = 2`).
- **Trade-offs:** Sgn's sign convention at zero matches TI-83 (`sgn(0) = 0`) rather than mathematical `sgn(0) = undefined` or some libraries' `sgn(0) = NaN`. Returning 0 keeps the function total over the reals.
- **Notes:** Closes two small bullets from the Number-functions backlog. Outstanding small math primitives: `fnInt(` and `nDeriv(` (need delayed-evaluation infrastructure to capture an "expression argument") and `sum(` / `prod(` / `seq(` (same).

### IMP-040: Periodic save protects against crash-loss

- **Status:** applied (2026-05-24)
- **Location:** [app/main.cpp](app/main.cpp)
- **Effort:** trivial
- **Description:** IMP-033 persistence saved only on clean exit, so a crash (BUG-019, BUG-020, or any future regression) lost the entire session. The follow-up note in the IMP-033 entry called this out explicitly.
- **Change:** New `QTimer` in `main.cpp` set to fire every 30 seconds, wired to `uiController.saveState()`. Owned by the function-local stack so it stops cleanly on `app.exec()` return. Tests / CLI / REPL don't run a Qt event loop and don't get a timer.
- **Trade-offs:** Naïve interval-based — saves whether or not state actually changed. The JSON is ~1 KB and the disk write is fsynced inside saveState's downstream path; the cost is invisible in practice. A "dirty flag + debounce" version would save fewer no-op writes but adds tracking complexity. Acceptable to revisit if the state file grows much larger.
- **Notes:** Verified by leaving the GUI idle for >2 min and grepping `session.log` — three `saveState ok` events fired at exactly 30000-ms intervals, plus the existing on-exit save. Worst-case loss on a crash is now ~30s of unrecorded input instead of the full session.

### IMP-039: Four more ZOOM presets (ZSquare / ZTrig / ZDecimal / ZInteger)

- **Status:** applied (2026-05-23)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/components/ZoomPopup.qml](app/qml/components/ZoomPopup.qml)
- **Effort:** small
- **Description:** ZoomPopup had only the four presets that shipped with IMP-028 (ZStandard, Zoom In, Zoom Out, ZFit). Real TI-83 has ~10 entries; the most useful absent ones were ZSquare (equal X/Y scaling), ZTrig (trig-friendly window), ZDecimal (the canonical "clean decimals" preset), and ZInteger (snap edges to integers).
- **Change:**
  - Four new Q_INVOKABLE methods on UIController:
    - `zoomSquare()` keeps the current centre and snaps y-range to match x-range so 1 X = 1 Y on screen
    - `zoomTrig()` sets `[-2.3π, 2.3π] × [-4, 4]` (TI-83 conventions)
    - `zoomDecimal()` sets `[-4.7, 4.7] × [-3.1, 3.1]`
    - `zoomInteger()` snaps all four viewport edges to their nearest integer with a `< 1.0` degenerate-window guard
  - ZoomPopup grew to 8 entries; popup height bumped 260→380 to fit them.
- **Trade-offs:** ZSquare here ignores the actual canvas aspect ratio — assumes square pixels. Close enough for the default window size; a real-aspect implementation would need the canvas to report its render size to the controller.
- **Notes:** Surfaced BUG-021 during verification (saved-state appending to typed input). Brings ZOOM coverage from 40% to 80% of the real TI-83 menu — outstanding entries (ZBox draggable rect, ZoomStat, ZoomPrevious, ZoomMemory) are deferred.

### IMP-038: Factory RESET button + MODE popup height fix

- **Status:** applied (2026-05-23)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/components/MODEPopup.qml](app/qml/components/MODEPopup.qml)
- **Effort:** small
- **Description:** No way to reset the calculator. State accumulated across sessions could put the user in a confused configuration with no recourse short of deleting `state.json` manually. Real TI-83 has a MEM menu with a reset option; surfaced when the user's session-spanning state showed unexpected results.
- **Change:**
  - New `Q_INVOKABLE UIController::resetAll()`. Clears scalars A..Z, matrix registry, function buffers Y1/Y2/Y3, display strings, history, entry-recall ring, cursor position, display state, viewport (back to `-10..10`), MODE settings (Radian / Normal / Float / Connected), insert mode, trace state, and TBLSET (start=0, step=1). Also removes the `state.json` file so a subsequent restart starts truly clean. Emits every change signal in one pass so QML bindings refresh in lockstep.
  - Added a `RESET` button (control-style colouring) to MODEPopup, sitting above DONE. Bumped the popup's height from 420→500 to fit the new row — the original DONE button had been clipped off the bottom.
- **Trade-offs:** No confirmation prompt. Adding one would be a few lines but the deliberate-click-in-MODE-popup gesture is intentional enough; a one-stray-tap risk is low. CLEAR doesn't auto-reset because that would be very disruptive (CLEAR is high-frequency).
- **Notes:** Pairs with IMP-037 (keyboard letters) — together they unblock the typical "I want to test something with a clean slate" workflow.

### IMP-037: Uppercase keyboard letters + `Y1`/`Y2`/`Y3` keyboard fuse

- **Status:** applied (2026-05-23)
- **Location:** [app/qml/Main.qml](app/qml/Main.qml) (keyboard handler)
- **Effort:** small
- **Description:** Two related keyboard gaps. (1) Uppercase letters fell through the keymap unmapped, so typing `A`, `B`, `X`, `Y` did nothing — variables were menu-only. (2) After fixing (1), typing `Y1` on the keyboard inserted `VarY` then `Num1`, which with implicit-mul evaluated as `Y * 1 = 0` rather than the Y-VAR token introduced in IMP-036.
- **Change:**
  - Added uppercase `A`–`Z` to the keyboard keymap (each → the single-letter token input). Lowercase remains reserved for the function shortcuts (`s` → `sin(`, etc.) so users press SHIFT+letter for a variable, lowercase for a function.
  - One-keystroke lookahead for `Y`: the first `Y` keystroke is inserted immediately (visible feedback — the display shows `Y` and the cursor advances) and arms a `pendingY` flag at the root. If the very next keystroke is `1`/`2`/`3`, the handler backspaces the just-inserted `Y` and re-inserts the fused `Y1`/`Y2`/`Y3` token in its place. Any other keystroke just clears the flag — the `Y` stays as a plain `VarY`. ENTER, CLEAR, backspace, and cursor moves also clear `pendingY` so the fuse only fires on the immediate next keystroke.
- **Trade-offs:** The earlier deferred-insert version had no visual feedback after typing `Y` — user couldn't tell if their keystroke registered. Switched to optimistic-insert + rewrite for the visible-feedback win. Edge case: if the user types `Y`, moves the cursor away, then types `1` somewhere else, no fuse fires (the cursor-move clears the flag) — that's the right call.
- **Notes:** Y1/Y2/Y3 via keyboard now matches the on-screen CATALOG path. Other multi-char tokens (sin(, cos(, etc.) already had single-char keyboard shortcuts so they don't need this treatment.

### IMP-036: Y-VARS recall — Y1/Y2/Y3 referenced from another expression

- **Status:** applied (2026-05-23)
- **Location:** [core_math/include/capsules/capsule_math.hpp](core_math/include/capsules/capsule_math.hpp), [core_math/src/core_math.cpp](core_math/src/core_math.cpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** medium
- **Description:** TI-83 lets you compose functions: define `Y1 = X^2`, then write `Y2 = Y1+10` and Y2 is automatically `X² + 10`. Our function slots existed as buffers but couldn't reference each other — the parser didn't know `Y1` was a thing.
- **Change:**
  - Engine: three new leaf tokens `Token::Y1` / `Y2` / `Y3`. When the evaluator hits one, it recursively evaluates the referenced buffer at the current `xValue`. The lookup goes through a `static std::function<std::vector<Token>(int)> MathStateMachine::yLookup` that UIController populates on construction with a lambda returning `m_functionBuffers[idx]`.
  - Cycle detection via a `static thread_local std::set<int>` keyed by Y-index. Inserts on recursion entry, erases on return. Self-reference (`Y1 = Y1+1`) or mutual cycles (`Y1=Y2`, `Y2=Y1`) return the `"Recursion"` error string, which UIController maps to the TI-83-style `ERR:RECURSION` display label.
  - Empty referenced buffer evaluates to `0` (matches TI-83 behaviour for an empty Y slot). Non-scalar (matrix) result from a Y-VAR returns a Type Error.
  - kTokens entries for `Y1`/`Y2`/`Y3`. Implicit-mul preprocessor's value-like classifier extended so `2Y1`, `Y1(3)` (= Y1*3 in v1) tokenise correctly.
  - Tests: 6 new assertions covering cross-slot reference (`Y1+1` from Y2 with Y1=5), X-threading through Y-VAR lookups, self-reference detection, mutual-cycle detection, empty-target → 0. 228/228 passing.
- **Trade-offs:** v1 only supports the **bare** form. `Y1(3)` parses as `Y1 * 3` via implicit-mul, not as an X override. Real TI-83 supports both interpretations; ours doesn't yet. The fix would be to promote Y_n to function-like tokens with optional argument syntax — bigger change, deferred.
- **Notes:** Pairs naturally with TABLE view (IMP-035) — you can now define Y1 and Y2 = `Y1+10` and watch them side by side. In our model, the active slot's buffer is the one being evaluated, so referencing Y1 from inside Y1's own buffer is genuine self-recursion — to query Y1's value from REPL, switch to Y2 first or use a different slot.

### IMP-035: TABLE view (2ND + GRAPH) + TBLSET popup (2ND + WINDOW)

- **Status:** applied (2026-05-23)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/components/TableView.qml](app/qml/components/TableView.qml), [app/qml/components/TblSetPopup.qml](app/qml/components/TblSetPopup.qml), [app/qml/Main.qml](app/qml/Main.qml), [app/qml/qmldir](app/qml/qmldir), [CMakeLists.txt](CMakeLists.txt)
- **Effort:** medium
- **Description:** TABLE mode (the X / Y1 / Y2 / Y3 tabular function view) is a TI-83 fixture that we had no equivalent for. With three function slots and a usable graph mode in place, a stepwise numeric view of the same data was the obvious next gap.
- **Change:**
  - Controller: three Q_PROPERTYs — `isTableMode` (mirrors `isGraphMode`, mutually exclusive), `tblStart`, `tblStep`. `Q_INVOKABLE toggleTableMode()` flips between table and keypad and clears `isGraphMode` so the two pages never coexist. `Q_INVOKABLE QVariantList getTableRows(int count, double xStart)` evaluates Y1/Y2/Y3 at each X for the requested window — empty / non-scalar / errored cells just get omitted from the row map so the QML can render them as `—`.
  - New `TableView.qml`: header row (`X | Y1 | Y2 | Y3`) + 14 data rows refreshed whenever the table settings, any function buffer, the active slot, or a MODE setting changes. Numbers route through `uiController.formatScalar` so Sci/Fix2/Eng settings still apply. Footer hint shows the current TblStart/ΔTbl and how to scroll / open TBLSET.
  - New `TblSetPopup.qml`: two numeric fields (TblStart, ΔTbl) with the same `Number.isFinite` guard pattern as WindowPopup, plus a zero-rejection on ΔTbl since a zero step would loop forever.
  - Main.qml: StackLayout grew a third page (table). SoftKeyRow handler now branches on `root.secondArmed` — 2ND+GRAPH → toggleTableMode, 2ND+WINDOW → TBLSET popup. Up/Down keyboard handler gained a table-mode branch that nudges the TableView's `scrollOffsetSteps` by ±1 (taking precedence over the existing trace/cursor roles when in table mode).
  - Persistence: state.json schema gained a `"table"` object with `tblStart` and `tblStep`. Restore guards against zero step (falls back to 1) so a corrupt state file can't strand the table renderer.
  - Y= soft-key now also leaves TABLE mode (previously only left graph mode).
- **Trade-offs:** Fixed visible-row count (14) — could grow with the window height, but the simple constant keeps the renderer trivial. Header always shows X/Y1/Y2/Y3 columns even if some slots are empty, which trades a slightly busier header for column-alignment stability when slots toggle on/off mid-session.
- **Notes:** Closes the biggest remaining "calculator feature gap" — every primary soft-key + its 2ND companion now does something. Outstanding 2ND-soft-key combos: 2ND+ZOOM (MEMORY), 2ND+TRACE (CALC).

### IMP-034: Header MODE indicator becomes dynamic

- **Status:** applied (2026-05-23)
- **Location:** [app/qml/Main.qml](app/qml/Main.qml) (header strip Text element)
- **Effort:** trivial
- **Description:** The header text was `"NORMAL  " + (angleMode === 1 ? "DEG" : "RAD")` — Notation/Decimal MODE settings (IMP-020) were live in the engine and the popup but never reflected in the header. Set Notation to Sci, evaluate something, see `5.00E+00` in the history but `NORMAL` still in the header — confusing.
- **Change:** Replaced the static prefix with a computed expression bound to `uiController.notation` / `fixDecimals` / `angleMode`. Format: `<NOTATION>[  FIX N]  <ANGLE>` — Notation always present, Fix segment only when non-Float, Angle always present. Examples: `NORMAL  RAD` (defaults), `SCI  FIX 2  DEG` (a Sci+Fix2+Degree session), `ENG  FIX 4  RAD`.
- **Trade-offs:** Header widens when Fix is on — gives the indicator visual variability that matches the state. Could have packed the indicator into a fixed-width fielded layout but the simple text concat is cleaner and the variability is itself informative (a wider indicator = non-default mode active).
- **Notes:** Surfaced while verifying IMP-033 — saved Sci+Fix2 came back correctly but the header lied. Closes a small but real "you can't trust what you see" gap.

### IMP-033: Persistent state across runs

- **Status:** applied (2026-05-23)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/main.cpp](app/main.cpp)
- **Effort:** medium
- **Description:** Quitting the GUI erased every variable, matrix, Y= buffer, MODE setting, and viewport. Real TI-83 keeps everything across power-cycles — this was the biggest "wait, where did my work go?" gap.
- **Change:**
  - Two new `Q_INVOKABLE` methods on `UIController` — `saveState() const` and `loadState()` — wired by `main.cpp` post-construction (load) and just before `app.exec()` returns (save). Both use a single JSON file at `$XDG_STATE_HOME/tux-ti83/state.json` (or `~/.local/state/tux-ti83/state.json` fallback), sharing the same dir as `session.log` so admin tools find everything in one place.
  - Schema (version 1): `scalars` (26 doubles A..Z), `matrices` (only persisted slots A/B/C), `functions` (3 display strings for Y1/Y2/Y3), `activeFunction` (int), `viewport` (xMin/xMax/yMin/yMax), `mode` (angle/notation/fixDecimals/drawMode).
  - Session-scoped state — history, entry-recall ring buffer, insertMode, tracing — deliberately omitted; users don't expect those to survive a restart.
  - Loading function buffers replays each display string through `processExpression`, so tokenisation goes through the same path as live typing. MODE settings restored first so any side effects use the right format. Signals (`angleModeChanged`, `notationChanged`, `viewportChanged`, etc.) fired at the end so QML bindings refresh.
  - Auto-load deliberately **not** wired into the UIController constructor — the CLI / REPL / test binaries instantiate a controller too, and we don't want the GUI's persistent state polluting their default-zero starting point. Explicit `uiController.loadState()` in `main.cpp` is GUI-only.
- **Trade-offs:** Save fires on clean exit only — a crash loses state since the last clean close. Acceptable for first pass; an obvious follow-up is periodic save-on-change. Schema versioning means future migrations are straightforward; v1 files will be rejected gracefully (skip, default state) if/when the schema bumps.
- **Notes:** Surfaced and closed [BUG-020](BUGS.md) (cross-slot cursor SIGSEGV) during testing. Also motivated IMP-034 (dynamic header) once the round-trip exposed the static "NORMAL" lie.

### IMP-032: Dedicated `!` and `STO▸` keys on the GUI

- **Status:** applied (2026-05-09)
- **Location:** [app/qml/Main.qml](app/qml/Main.qml)
- **Effort:** trivial
- **Description:** Factorial and the STO assignment arrow were both menu-only — `!` lived under MATH, `→` under MATH → STO. Users testing implicit multiplication couldn't find either without scrolling. Real TI-83 has a dedicated STO▸ key and `!` accessible as 2ND+÷ (the PRB menu) — both more discoverable than our menus.
- **Change:**
  - Added `"÷": "!"` to `secondMap` and a `secondLabel: "!"` corner label on the ÷ CalcKey. 2ND+÷ now inserts `!` directly. Matches the TI-83 PRB-menu convention.
  - The empty 5th slot of the CURSOR section now holds a `STO▸` CalcKey wired to `processExpression("→")`. STO doesn't have a natural 2ND home on the existing layout, so it gets its own key — one click away, out of the main flow.
- **Trade-offs:** STO living in the CURSOR section is layout pragmatism, not semantics. The alternative (rearrange the keypad to free a numeric-grid slot) would have shuffled keys users have already learned. Acceptable for now; a future Y-editor or keypad redesign could move it.
- **Notes:** Closes the UX gap surfaced when verifying IMP-031 (implicit multiplication).

### IMP-031: Implicit multiplication by juxtaposition

- **Status:** applied (2026-05-09) — closes IMP-005 (originally suggested 2026-04-06).
- **Location:** [core_math/include/capsules/capsule_math.hpp](core_math/include/capsules/capsule_math.hpp), [core_math/src/core_math.cpp](core_math/src/core_math.cpp), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** small
- **Description:** `Token::ImplicitMul` has been in the enum since the early days but never generated or handled — `2π`, `2(3+4)`, `(3)(4)`, `2sin(x)`, `5X` all returned ERR:SYNTAX. Real TI-83 supports implicit multiplication for these juxtapositions, and not having it was a genuine annoyance.
- **Change:**
  - Added `Token::ImplicitMul` to the precedence table (same as Mul/Div — 2) and to the binary-op evaluator branch (`t == Token::Mul || t == Token::ImplicitMul`). Behaviourally identical to Mul on the eval side; separate token so the source structure stays inspectable.
  - New third preprocessing pass in `evaluate()` walks the token stream after digit-flush and Sto-target consumption, injecting `ImplicitMul` whenever a "value-like-end" token (Num0, Pi, E, Ans, RightParen, Fact, VarA..Z, MatA..J) is immediately followed by a "value-like-start" (the same set plus LeftParen and any function token). Deliberately skips injection when the next token is `Neg` so `2-3` stays as subtraction rather than `2 * (-3)` — the Sub-vs-Neg disambiguation happens earlier in `UIController::insertToken` and is authoritative.
  - 11 new regression tests: `2π`, `2(3+4)`, `(3)(4)`, `(1+2)(3+4)`, `2sin(0)`, `3!2`, chained `2π3`, `π^2`, variable juxtaposition `5A` and `2A+3A`, plus a `2-3 → -1` regression guard. 221/221 passing.
- **Trade-offs:** Chose Mul-equal precedence rather than the TI-83's slightly-higher-than-Mul precedence for implicit (which would make `1/2X` → `1/(2X)`). Equal precedence gives `1/2X` → `X/2`, which matches the explicit equivalent `1/2*X` and is less surprising for new users. Parens cover the other reading.
- **Notes:** Closes IMP-005, which has sat in Suggested since 2026-04-06. Surfaced a discoverability gap (`!` and `STO▸` were menu-only) that IMP-032 closed in the same session.

### IMP-030: TRACE polish — ↑/↓ cycles function, readout respects MODE

- **Status:** applied (2026-05-08)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [app/qml/Main.qml](app/qml/Main.qml), [app/qml/components/GraphCanvas.qml](app/qml/components/GraphCanvas.qml)
- **Effort:** trivial
- **Description:** Two follow-up gaps from IMP-029 (TRACE). Real TI-83 uses ↑/↓ in trace mode to walk between function slots, and its coordinate readout respects the active number-display mode. We had neither.
- **Change:**
  - Made `UIController::formatScalar` `Q_INVOKABLE` so QML can call it directly. Updated `GraphCanvas` trace readout to format `traceX` / `traceY` through it — now respects Notation (Normal/Sci/Eng) and Decimal (Float/Fix N) live.
  - Added Up/Down branches to Main.qml's keyboard handler. In graph mode + tracing, ↑ is "previous slot" and ↓ is "next slot" (modular over Y1/Y2/Y3 via `setActiveFunction`). Outside that mode, both keys stay unbound — no semantics elsewhere yet.
- **Trade-offs:** ↑/↓ skip empty buffers? No — cycling through all three is simpler and matches real TI-83 (which lets you trace an empty function slot and just shows nothing). Empty slot in trace mode renders the marker invisible (NaN guard already in place) and the readout shows `Y=—`.
- **Notes:** Closes the residual "TRACE works but feels incomplete" feedback. With this in, TRACE matches the TI-83's discoverability and flexibility.

### IMP-029: TRACE soft-key — graph cursor with X/Y readout

- **Status:** applied (2026-05-08)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/Main.qml](app/qml/Main.qml), [app/qml/components/GraphCanvas.qml](app/qml/components/GraphCanvas.qml)
- **Effort:** medium
- **Description:** TRACE was the last no-op soft-key. Real TI-83 binds it to a movable graph cursor that displays the active function's `(x, y)` at the cursor position — useful for reading off curve values without zooming or eyeballing the grid.
- **Change:**
  - Controller: three new Q_PROPERTYs (`isTracing`, `traceX`, `traceY` — the latter computed live by evaluating the active function buffer at `traceX`, returning NaN if empty / non-scalar). `Q_INVOKABLE toggleTrace()` flips the flag and snaps `traceX` to viewport centre on entry. `Q_INVOKABLE traceLeft()` / `traceRight()` step by 1/100 of the viewport width — matches TI-83's sample density.
  - Main.qml: introduced `navLeft()` / `navRight()` dispatch helpers at the root. In graph mode + tracing, ←/→ goes to the trace cursor; otherwise to the expression cursor. Both the keyboard arrow handlers and the on-screen `CURSOR` section's ←/→ CalcKeys route through these so behaviour stays consistent. TRACE soft-key wired in `SoftKeyRow` — auto-engages graph mode if pressed from the keypad, then toggles trace.
  - GraphCanvas: drawn after the curves so the marker sits on top. Crosshair (8px arms) + 2.5px filled dot in the active function's colour. Bottom-left readout strip on a translucent shell-coloured background reads `Y<n>  X=...  Y=...` (NaN renders as `—`). Added `onTraceChanged` to `Connections` so trace movement triggers immediate repaints.
- **Trade-offs:** Trace cursor stays on the active function until the user switches via the FunctionSelector. Real TI-83 uses ↑/↓ to walk between functions in trace mode — small follow-up that needs an on-screen ↑/↓ pair (we deliberately skipped those in IMP-023 because there were no semantics for them; trace gives them one). Coordinate readout currently uses `toFixed(4)` — a follow-up could route it through `formatScalar` so it respects the MODE Notation/Decimal settings.
- **Notes:** Closes the soft-key reintegration. With this in, every key on the keypad does something useful.

### IMP-028: ZOOM soft-key popup (ZStandard / In / Out / ZFit)

- **Status:** applied (2026-05-08)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/components/ZoomPopup.qml](app/qml/components/ZoomPopup.qml), [app/qml/Main.qml](app/qml/Main.qml), [app/qml/qmldir](app/qml/qmldir), [CMakeLists.txt](CMakeLists.txt)
- **Effort:** small
- **Description:** ZOOM soft-key was the second-to-last no-op (after TRACE). Pan + scroll-wheel zoom worked, but discrete presets — go back to the default viewport, double / halve, autoscale Y — needed a menu.
- **Change:**
  - Controller: `Q_INVOKABLE zoomIn()` / `zoomOut()` — factor 0.5 / 2.0 around the current viewport centre, mirroring real TI-83 ZIn / ZOut. `resetViewport()` and `zoomFit()` already existed and serve as ZStandard / ZFit.
  - New `ZoomPopup.qml` — four entries dispatching to the controller methods. Same modal/escape-to-close shape as MathMenuPopup / LogicMenuPopup.
  - Wired the ZOOM soft-key in `SoftKeyRow`'s `onPressed` switch. Registered the popup in `qmldir` + the CMake resource list.
- **Trade-offs:** Just the four most-used presets — TI-83's full ZOOM menu has 10+ entries (ZBox, ZSquare, ZTrig, ZInteger, ZoomStat, etc.). Adding more is mechanical; leaving them off keeps the popup focused.
- **Notes:** TRACE is now the only no-op soft-key remaining. Closes another Phase-A reintegration gap.

### IMP-027: Graph mode — Connected / Dot drawing

- **Status:** applied (2026-05-08)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/components/MODEPopup.qml](app/qml/components/MODEPopup.qml), [app/qml/components/GraphCanvas.qml](app/qml/components/GraphCanvas.qml)
- **Effort:** small
- **Description:** Another greyed MODE row activated. Connected (default) draws line segments between adjacent samples; Dot draws a single filled circle per sample with no connecting strokes — useful for noisy / discontinuous functions where the connected interpolation lies.
- **Change:**
  - Controller: `drawMode` Q_PROPERTY (int, 0=Connected default / 1=Dot) with WRITE+NOTIFY. Setter clamps to {0,1} so a stray value can't strand the user with no rendering.
  - MODEPopup: flipped the `Draw` row from greyed placeholder to live binding against `uiController.drawMode`.
  - GraphCanvas: branches at the per-curve render — Connected unchanged (existing path); Dot uses `ctx.arc()` at radius 1.5px per sample. Added `onDrawModeChanged` to the controller `Connections` block so toggling the mode immediately repaints.
- **Trade-offs:** Dot radius (1.5px) chosen to be visible without dominating the grid; could grow it for high-DPI later. No engine impact — purely a rendering split.
- **Notes:** Closes another MODE placeholder. Plot row (Sequential/Simul) is the next obvious one but only meaningful with frame-by-frame animation, which we don't have — deferred until that lands.

### IMP-026: Crash logger session-log rotation

- **Status:** applied (2026-05-08)
- **Location:** [graph_ui/src/crash_logger.cpp](graph_ui/src/crash_logger.cpp)
- **Effort:** trivial
- **Description:** `session.log` was append-only — a long-running setup or a crash-loop scenario could grow it without bound. Want a generous cap with one prior session retained.
- **Change:** On `init()`, `stat()` the existing `session.log`; if it exceeds 1 MiB, atomically `rename()` it to `session.log.prev` (overwriting any older prev) before opening a fresh `session.log`. The new session header is followed by a `(rotated: prior log exceeded 1 MiB cap, moved to session.log.prev)` notice when rotation fired. Below the cap, behaviour is unchanged.
- **Trade-offs:** Keeps only one prior session — if the user wants a longer trail, they'd need to copy `session.log.prev` aside between sessions. A larger ring (`.prev`, `.prev2`, ...) would be the obvious extension; deferred until anyone actually wants more than the immediate predecessor.
- **Notes:** Verified end-to-end — manufactured a 1.5 MB log file, launched the GUI, confirmed `session.log.prev` carried the old contents and `session.log` opened fresh with the rotation notice on its first line.

### IMP-025: Nth-root operator (2ND + ^)

- **Status:** applied (2026-04-29)
- **Location:** [core_math/include/capsules/capsule_math.hpp](core_math/include/capsules/capsule_math.hpp), [core_math/src/core_math.cpp](core_math/src/core_math.cpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/Main.qml](app/qml/Main.qml), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** small
- **Description:** Real TI-83 binds 2ND+^ to a binary nth-root operator (`n ˣ√ x`). We had `√(` for square roots only — anything else needed `x^(1/n)`. Awkward to type and easy to mis-parenthesise.
- **Change:**
  - Engine: added `Token::NthRoot`, sharing precedence (3) and right-associativity with `Pow`. Evaluator pops two scalars, returns `b^(1/a)`. Domain checks: `n=0 → DOMAIN`; even integer root of negative `→ NONREAL ANS`. Odd integer root of negative is real and computed via `-|b|^(1/n)` to avoid `pow`'s NaN-on-fractional-negative path.
  - kTokens: `ˣ√` (Unicode) plus `xroot` ASCII alias.
  - Main.qml: `secondMap["^"] = "ˣ√"` and the `^` CalcKey grew a `ˣ√` 2ND corner label.
  - Unary-context heuristic in `insertToken` now recognises `NthRoot` as a position where `-` means unary negation, so `3ˣ√-8` parses as `3 NthRoot (-8)` rather than `3 NthRoot - 8` (stray binary minus). Caught and fixed during test runs.
  - Tests: 8 new assertions covering happy-path roots, ASCII alias, n=0 DOMAIN, even-root-of-negative NONREAL, odd-root-of-negative real, and right-associative chaining.
- **Trade-offs:** Could have made it a function (`xroot(n, x)`) rather than infix, which would have skipped the precedence + unary-heuristic plumbing. Rejected because TI-83 users expect the infix form and it's also more readable for the canonical `n ˣ√ x` shape.
- **Notes:** Closes another 2ND-variant gap. Outstanding on the same backlog: EE (scientific exponent entry), `{`/`}` for lists (needs list type), and a few catalog-only variants.

### IMP-024: CATALOG browser (2ND + 0)

- **Status:** applied (2026-04-29)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/components/CatalogPopup.qml](app/qml/components/CatalogPopup.qml), [app/qml/Main.qml](app/qml/Main.qml), [app/qml/qmldir](app/qml/qmldir), [CMakeLists.txt](CMakeLists.txt)
- **Effort:** small
- **Description:** Discoverability gap. We have ~80 insertable tokens spread across MATH, MATRX, TEST/LOGIC, and the keypad — users couldn't see the full vocabulary anywhere. Real TI-83 has CATALOG (2ND + 0) as the canonical "show me everything alphabetically" browser.
- **Change:**
  - New `Q_INVOKABLE QStringList catalogEntries() const` on UIController. Walks `kTokens`, dedupes display strings (ASCII aliases like `<=` / `->` share displayStrs with their Unicode siblings), and returns the result sorted case-insensitively.
  - New `CatalogPopup.qml`: scrollable ListView fed by `catalogEntries()`, plus a search field that incrementally filters by case-insensitive substring. Click any row to insert via `processExpression` and close. Empty-state shows "no matches" when the filter excludes everything.
  - Wired 2ND + 0 in `handleKey` (dedicated branch alongside 2ND+ENTER / 2ND+MATH / 2ND+DEL — popup triggers don't fit `secondMap`).
  - `0` CalcKey gets a `CATALOG` 2ND corner label so the binding is visible without reading the manual.
  - Registered the popup in `qmldir` and the CMake resource list.
- **Trade-offs:** Cached the entries on first open (kTokens is compile-time static, so a one-shot fetch is fine and skips the cost on every keystroke). Filter recomputes on every `onTextChanged` — acceptable for ~80 entries; would memoise if the list grew an order of magnitude.
- **Notes:** Closes another item from the "remaining 2ND variants" backlog. The popup is also a useful reference even when the user knows what they want — typing `co` and seeing `cos(`, `cosh(`, `acos(`, `acosh(` is faster than navigating multiple menus.

### IMP-023: On-screen D-pad + insert-mode toggle (2ND+DEL)

- **Status:** applied (2026-04-29)
- **Location:** [graph_ui/include/ui_controller.hpp](graph_ui/include/ui_controller.hpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/Main.qml](app/qml/Main.qml), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** small
- **Description:** Cursor movement (IMP-017) was keyboard-only — mouse / touch users had no way to navigate mid-expression. And there was no overwrite mode: typing always inserted, even when the user wanted to fix a single token without shifting everything right.
- **Change:**
  - New `CURSOR` section in Main.qml between CONTROL and SCIENTIFIC: `HOME · ← · → · END` mapped to the existing `moveCursor*` Q_INVOKABLEs. ↑/↓ omitted — no multi-line semantics, and 2ND+ENTER already covers history recall.
  - Insert-mode state on UIController: `m_insertMode` (default true) + Q_PROPERTY (NOTIFY) + `Q_INVOKABLE toggleInsertMode()`. `insertToken` checks the flag — true (or cursor at end) splices, false replaces the token at the cursor.
  - 2ND + DEL toggles insert/overwrite via a dedicated branch in `handleKey` (next to the existing 2ND+ENTER and 2ND+MATH special cases — controller-method calls don't fit `secondMap`).
  - DEL CalcKey gets a `INS` 2ND corner label so the toggle is discoverable.
  - Header `OVR` badge (amber, same style as the `2ND` / `α` badges) appears when insertMode is false.
  - Tests: 8 new assertions covering default-insert, mid-expression overwrite, continued overwrite, overwrite-past-end fallback to append, toggle-back-to-insert prepend, and the Q_PROPERTY round-trip. 202/202 passing.
- **Trade-offs:** Cursor section adds ~50px of vertical layout; the `fillHeight` spacer in NUMERIC absorbed it without the window needing to grow. Could have shoehorned arrows into the existing CONTROL row but it would have meant rearranging keys users have already learned.
- **Notes:** Closes the keyboard-only gap from IMP-017. No on-screen ↑/↓ yet — natural follow-up if/when multi-line editing or a forward-history feature lands.

### IMP-022: Crash logger with always-on session trail

- **Status:** applied (2026-04-29)
- **Location:** [graph_ui/include/crash_logger.hpp](graph_ui/include/crash_logger.hpp), [graph_ui/src/crash_logger.cpp](graph_ui/src/crash_logger.cpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/main.cpp](app/main.cpp), [CMakeLists.txt](CMakeLists.txt)
- **Effort:** medium
- **Description:** When the GUI crashed (BUG-019, any future regression) we had no record of what the user was doing. Reproducing was guesswork. Wanted: an always-on, on-disk trail of every action plus a crash trap that captures the signal + backtrace.
- **Change:**
  - New `CrashLogger` class in `graph_ui` with three public methods: `init()` (idempotent — opens the log fd, installs handlers), `logEvent(QString)` (appends a millisecond-timestamped line and `fsync`s before returning so the trail survives any subsequent crash), `shutdown()` (writes a clean-exit marker).
  - Log location: `$XDG_STATE_HOME/tux-ti83/session.log` with a fallback to `~/.local/state/tux-ti83/session.log`. Sessions append; delimited by `=== Session start: <iso8601> ===` headers and either `=== Session end (clean) ===` or `=== CRASH: signal N ===` / `=== TERMINATE: <what> ===` markers.
  - Signal handlers (SIGSEGV/SIGABRT/SIGFPE/SIGILL/SIGBUS) are async-signal-safe: only `write` and `backtrace_symbols_fd` are called inside them. The handler appends a CRASH marker plus a libc backtrace, then re-raises with `SIG_DFL` so a core dump still drops (subject to ulimit) and the OS exits with the conventional status.
  - `std::set_terminate` handler — used for the BUG-019 path and any other uncaught C++ exception — has more freedom (not in a signal context) and captures the active exception's `what()` string before aborting.
  - UIController hooks: every external entry point logs (`processInput`, `processExpression`, `evaluate`, `recallLastEntry`, `moveCursor*`, `setAngleMode`, `setNotation`, `setFixDecimals`, `updateMatrix`). Internal helpers don't log to keep the trail readable.
  - `main.cpp` calls `CrashLogger::init()` immediately after constructing `QGuiApplication` (so we capture even early QML load errors) and `shutdown()` just before returning from `app.exec()`. The CLI/REPL binaries are unchanged — they're short-lived and a session log there would mostly be noise.
- **Trade-offs:** `fsync` on every event slows the per-keystroke path by milliseconds at most — invisible to a human but real. Acceptable: durability is the whole point of this feature. Alternative considered: ring-buffer-in-memory + dump-on-crash. Rejected because the dump path inside a signal handler is exactly the place where you can't trust dynamic state.
- **Notes:** Verified end-to-end: clean session writes the expected event sequence + exit marker; the BUG-019 crash repro (`./build/tux_ti83_cli '.'` before its own fix) would have terminated through the `std::terminate` handler with a captured `what(): stod` line. Future improvement: rotate or cap the log file size — currently it grows without bound across sessions.

### IMP-021: `:` as a statement separator

- **Status:** applied (2026-04-29)
- **Location:** [core_math/include/capsules/capsule_math.hpp](core_math/include/capsules/capsule_math.hpp), [core_math/src/core_math.cpp](core_math/src/core_math.cpp), [graph_ui/src/ui_controller.cpp](graph_ui/src/ui_controller.cpp), [app/qml/Main.qml](app/qml/Main.qml), [tests/test_math.cpp](tests/test_math.cpp)
- **Effort:** small
- **Description:** With Variables A–Z + STO landed (IMP-014), the natural next step was chained statements: `5→A:A+1→A`. The `.` CalcKey already had a `:` ALPHA corner label (placeholder); wiring it kept the layout honest.
- **Change:**
  - Engine: added `Token::Colon`. `evaluate()` short-circuits when the input contains a Colon — splits the token stream into segments, recurses per segment, and returns the last non-empty result. Errors abort the chain immediately, but earlier Sto mutations commit (matching TI-83 per-statement semantics).
  - Controller: registered `:` → `Token::Colon` in `kTokens` with the same display string.
  - UI: added `"."`: `":"` to `alphaMap` so ALPHA + period inserts `:`. Period key's existing `alphaLabel: ":"` now reflects real behaviour.
  - Tests: 8 new assertions covering simple chain, chained store + read, triple chain (last-segment-wins), error-mid-chain abort with earlier-state commit, and stray leading/trailing colons.
  - Surfaced and fixed a pre-existing latent crash in the digit-flush pass (BUG-019) — bare `.` was throwing `std::invalid_argument` from `std::stod` and propagating out of the engine, aborting the process. Wrapped in try/catch + `parseFailed` flag returning ERR:SYNTAX.
- **Trade-offs:** Chose recursive `evaluate()` over a flatter "preprocess into segments, run sequentially in a loop" approach because the existing function has a lot of local state and refactoring for non-recursion would have been a bigger change for no behavioural difference. The recursion depth is bounded by the number of `:` tokens in a single expression — won't blow the stack for any plausible input.
- **Notes:** Closes the chain-of-statements gap. Natural next: the `?` and `"` ALPHA labels (Input/Disp commands and string literals) are still aspirational — they need real TI-BASIC support. Not on the immediate queue.

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

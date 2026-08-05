# TI-BASIC Programming — Design & Roadmap

Status legend (matches [ROADMAP.md](../ROADMAP.md)): ✅ done · 🚧 in
progress · 🔜 next · 📅 planned · 💭 idea / maybe.

TI-BASIC is the last major unbuilt Tux-TI83 feature and the capability the
project is named after. It is large enough to warrant its own design doc.
This file is the single source of truth for the plan; the
[ROADMAP.md](../ROADMAP.md) "Programming" section links here.

**Scope decision (2026-08-01): _pragmatic subset first._** We target the
common command set that makes real programs runnable, ship it in small
milestones, then expand. Exotic commands, in-program graphics, `.8xp`
import, and `getKey` game loops are explicitly deferred to later phases.

---

## 1. Goals & non-goals

**Goals**

- Write, name, save, edit, and run programs in the GUI.
- A useful language subset: sequential statements, `Sto`, `Disp`/`Output`,
  the full control-flow set, `Input`/`Prompt`, `Lbl`/`Goto`, sub-program
  calls — enough to write real utilities (quadratic solver, unit
  converters, guessing games, loops).
- Reuse the existing expression evaluator for every value/condition, so all
  current math (scalars, lists, matrices, complex, functions) works inside
  programs for free.
- Run the same interpreter **headless** in the CLI/REPL (batch execution of
  a program from a file / stdin).

**Non-goals (at least initially)**

- Byte-for-byte `.8xp` compatibility (deferred to P6).
- Assembly programs (`Asm(`), archived/flash apps.
- Exact TI-83 timing / cycle behaviour.
- Full string library on day one (strings arrive in P4, after control flow).

---

## 2. Architecture

### 2.1 The interpreter layer

A new **`Interpreter`** component sits **above** `MathStateMachine`, never
inside it. The math engine stays a pure single-expression evaluator; the
interpreter orchestrates statements and calls
`MathStateMachine::evaluate()` for the expression and condition parts.

```
          ┌─────────────────────────────────────────┐
          │  UIController (Qt bridge)                │
          │   • PRGM store, editor state             │
          │   • drives the run-loop, routes I/O      │
          └───────────────┬─────────────────────────┘
                          │ steps / resumes
          ┌───────────────▼─────────────────────────┐
          │  Interpreter                             │
          │   • program counter, block/loop stack    │
          │   • label table, call stack              │
          │   • output buffer, run status            │
          └───────────────┬─────────────────────────┘
                          │ evaluate(expr) for values/conditions
          ┌───────────────▼─────────────────────────┐
          │  MathStateMachine (unchanged core)       │
          │   • tokeniser, shunting-yard, registries │
          └─────────────────────────────────────────┘
```

The interpreter shares the engine's global registries (`varRegistry`,
`listRegistry`, `matrixRegistry`, `lastResult`/`Ans`), so `5→A` inside a
program and `A` on the home screen refer to the same `A`.

### 2.2 Resumable execution (the foundational commitment)

GUI code cannot block waiting for `Input`. The interpreter therefore runs
in **steps** and reports a status after each pause point:

```cpp
enum class RunStatus {
  Running,     // more to do; call step() again
  Output,      // emitted text to the output buffer; UI should render
  NeedInput,   // Input/Prompt — waiting for a value/string
  NeedKey,     // Pause/getKey/Menu — waiting for a keypress/choice
  Done,        // program finished
  Error        // ERR:… (with line number)
};
```

Execution state — program counter, the block/loop stack, the label table,
the call stack, and the pending Input target — lives in the `Interpreter`
object, not on the C++ call stack. So a program can suspend at any I/O
point and resume when the UI supplies input. The UIController drives a
loop: `while (step() == Running) {}`, then acts on the pause status.

The CLI/REPL uses the **same** interpreter but resolves `NeedInput`/
`NeedKey` synchronously from stdin, so programs run headless too.

### 2.3 Program storage

A program is a **name + ordered list of source lines**. Statements are
separated by newlines or `:` (colon). Stored as source text (re-tokenised
per line at run time via the existing tokeniser), which keeps editing
simple and sidesteps a binary format.

- `std::map<QString, QStringList>` (name → lines) in the controller.
- Persisted in the state JSON and named saves (extends
  `buildStateJson`/`applyStateJson`).
- Program names: 1–8 chars, `A`–`Z`/`0`–`9`, matching the TI-83.

### 2.4 Statement model

Each line is one statement, classified by its leading token:

- **Bare expression** — evaluate, store to `Ans`, and (unless suppressed)
  echo to output, like the home screen.
- **Store** — `expr→VAR` (already handled by `evaluate()`).
- **Keyword statement** — `Disp`, `If`, `For(`, `Goto`, … dispatched by the
  interpreter, with the remainder of the line parsed as the keyword's
  arguments (often themselves expressions evaluated via the engine).

Control-flow structures use a **block stack**: `If…Then`/`For(`/`While`/
`Repeat` push a frame; `End` pops the matching one. `Lbl`/`Goto` use a
one-pass **label table** built when the program is first run.

### 2.5 New value type: strings

Real I/O needs text. `CalculationResult`/`Operand` gain a **string** kind
(alongside scalar/list/matrix/complex). Introduced in **P4**, not before —
control flow (P3) is numeric-only and doesn't need it.

- `Str1`–`Str9` registry + `"…"` string literals.
- Ops: concatenation (`+`), `sub(`, `length(`, `inString(`, `expr(` (parse a
  string as an expression), and number→string for `Disp`.

---

## 3. Phases (pragmatic subset first)

Each phase is independently shippable and GUI-verifiable. Suggested first
milestone: **P0 + P1 + P2** — write, save, and run a program that computes
and `Disp`s results.

### P0 — Scaffolding 🚧 (core landed 2026-08-01)
- ✅ New pure-C++ `interpreter` library (`interpreter/`), linked by
  `graph_ui` + the test binary — reusable headless (CLI) and testable
  without Qt.
- ✅ `Interpreter`: `RunStatus` enum (`Running`/`Output`/`NeedInput`/
  `NeedKey`/`Done`/`Error`), statement splitter (lines → statements on
  top-level `:`), and the resumable `step()` / `run()` loop. P0 executes no
  statements yet — a program advances its program counter to `Done`.
- ✅ `ProgramStore` (name → source lines): put / has / get / names / remove.
- ✅ 21 unit tests (splitter, load/flatten, run-to-Done, reset, store).
- 📅 Program persistence in state JSON / named saves — deferred to P1 (no
  way to create a program until the editor exists).
- Milestone reached: store a program and run it to completion.

### P1 — Program editor (UI) ✅ (landed 2026-08-01)
- ✅ **PRGM popup** opened via **2ND + `√(`** (resolves open decision #1 —
  this keypad has no dedicated PRGM key). List mode (RUN / EDIT / ✕ per
  program + NEW) and edit mode (name field + multi-line source editor +
  SAVE / CANCEL).
- ✅ **Editor model:** chose a **freeform multi-line text editor** (source
  text) over a keypad-driven line-list — pragmatic and robust, since
  programs are stored as source and re-tokenised per line at run time.
  Typed with the physical keyboard (`->` for →, `Disp`, `sin(`, … as text).
- ✅ `ProgramStore` wired into `UIController`: `programNames` /
  `programText` / `saveProgram` / `deleteProgram` / `normalizeProgramName`
  / `runProgram`. Names normalised to A–Z/0–9, ≤8 chars.
- ✅ **Persistence** — programs serialise into the state JSON (and named
  saves) via `buildStateJson`/`applyStateJson`; cleared by RESET.
- ✅ P1 RUN loads the program into an `Interpreter` and runs to `Done`,
  recording completion in history. (Real `Disp` output + a run view = P2.)
- ✅ 16 controller tests (CRUD, run, persistence round-trip).
- Milestone reached: author, save, edit, delete, and persist a program.

### P2 — Sequential interpreter core 📅
- Execute line-by-line: bare expressions, `Sto`, `Disp` (numeric,
  multi-arg), `ClrHome`, `Pause` (numeric), `Stop`.
- A scrollable **run/output view** (the program's "home screen").
- Wire EXEC to run the selected program.
- Milestone: a program like `5→A : A²→B : Disp B` runs and shows `25`.

### P3 — Control flow 📅
- `If` / `If…Then…Else…End`, `For(var,start,end[,step])…End`,
  `While…End`, `Repeat…End` with correct nested-`End` matching.
- `Lbl` / `Goto` (label table + jump).
- Conditions reuse the existing relational/boolean operators.
- Milestone: loops and branches (FizzBuzz, a counter, a guarded solver).

### P4 — Strings + real interaction 📅
- **String type** (§2.5): `Str1`–`Str9`, `"…"` literals, concat, `sub(`,
  `length(`, `inString(`, `expr(`.
- `Disp` (strings + mixed), `Output(row,col,value)`.
- `Input` (var/list), `Input "prompt",var`, `Prompt var` — resumable.
- `Menu(` — titled choice list that jumps to labels.
- Milestone: an interactive program (prompt for input, branch on it, print
  a labelled result).

### P5 — Program control & robustness 📅
- `Return`, `prgmNAME` sub-program calls (call stack; recursion depth cap),
  `DelVar`.
- `getKey` (real-time key code polling).
- **Break / interrupt** (an ON-equivalent to stop a running/looping
  program) and runtime error reporting with the **line number**, with
  jump-to-line in the editor.
- In-editor **command paste** menu (PRGM ▸ CTL / I/O tabs) so keywords
  don't have to be typed by hand.
- Milestone: multi-program projects; a runaway loop is interruptible;
  errors point at the offending line.

### P6 — Advanced / optional 💭
- Graphics from programs (reuse the existing DRAW primitives: `Line(`,
  `Horizontal`, `Vertical`, `Pt-On(`, `Text(`; add `Circle(` as needed),
  `getKey` game loops, `Pause` on a graph.
- `.8xp` import — parse real TI-83 program files (cross-references the
  Connectivity roadmap's `.8xp` item). Requires a token-value mapping.
- `Repeat`/`While` performance, `rand`-seeded games, `menu`-driven apps.

---

## 4. Command coverage (initial subset)

| Group | In the subset (P2–P5) | Deferred (P6 / later) |
|---|---|---|
| Sequential | expression, `Sto` (`→`), `:` separator | — |
| Output | `Disp`, `Output(`, `ClrHome`, `Pause` | `Disp` of graphs |
| Control | `If`/`Then`/`Else`/`End`, `For(`, `While`, `Repeat`, `End` | — |
| Jumps | `Lbl`, `Goto` | — |
| Input | `Input`, `Prompt`, `Menu(`, `getKey` | `getKey` game loops |
| Program | `Stop`, `Return`, `prgmNAME`, `DelVar` | `Archive`/`Unarchive` |
| Strings | `Str1`–`Str9`, `"…"`, `sub(`, `length(`, `inString(`, `expr(` | full string library |
| Graphics | — | `Line(`, `Circle(`, `Text(`, `Pt-On(` in programs |
| Interop | `.8xp` import | link-cable transmit |

---

## 5. Testing strategy

- **Interpreter unit tests** run headless (no Qt UI) — feed a program as
  source, run to completion with scripted `Input`/`getKey` responses, and
  assert on the output buffer and final registry state. This mirrors how
  the CLI runs programs, so one harness covers both.
- Golden programs per phase: P2 arithmetic, P3 loops/branches, P4
  interactive I/O with scripted input, P5 sub-calls + error/line reporting.
- Keep the existing 629-test math suite green throughout — the engine core
  must not regress.

---

## 6. Open design decisions (resolve as we reach them)

1. **PRGM menu home** — which key/soft-key opens PRGM (the TI-83 uses a
   dedicated PRGM key we don't have). Decide in P1.
2. **Output view vs. history** — does program output render in the existing
   history pane, a dedicated run view, or reuse the LCD? Decide in P2.
3. **Editor model** — line-list editor (each line edited with the keypad)
   vs. a freeform text area. Line-list is closer to the TI-83 and reuses
   the tokeniser; leaning that way.
4. **Break key** — what interrupts a running program in the GUI (ESC? a
   dedicated on-screen STOP?). Decide in P5.
5. **String storage in state JSON** — schema for `Str1`–`Str9` + programs.

---

## 7. Build order recommendation

Ship **P0 → P1 → P2** as the first milestone (author + run + `Disp`), then
**P3** (control flow) as the second — those two together already make the
feature genuinely useful. **P4** (strings + interaction) is the third and
largest single step. **P5** hardens it; **P6** is optional polish.

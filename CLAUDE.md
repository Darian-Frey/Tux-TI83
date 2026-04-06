# CLAUDE.md — Tux-TI83 Session Context

## Project Identity

**Tux-TI83** — A Linux-native graphing calculator application reimagining the TI-83 Plus.
- Repo: https://github.com/Darian-Frey/Tux-TI83
- Stack: C++20 · Qt 6.5+ · QML · CMake 3.16+
- Build: `chmod +x build.sh && ./build.sh`
- Target: Linux-first desktop application

## Current Architecture

```
Tux-TI83/
├── app/              # QML UI layer — primary focus of this session
├── core_math/        # C++ math engine (recursive descent parser + shunting-yard evaluator)
├── graph_ui/         # Graphing engine (multi-function plot, pan/zoom, Z-logic)
├── build/            # CMake build output
├── CMakeLists.txt
└── build.sh
```

### Core capabilities already implemented
- Recursive descent parser / shunting-yard evaluator
- Multi-function graphing (Y1, Y2, Y3) with pan and scroll-zoom
- Matrix operations: addition, scalar multiply, matrix multiply, 3×3 editor
- Relational and boolean logic operators
- SCHEMA_V5 state machine architecture (capsule-based memory)

**Do not modify `core_math/` unless explicitly asked.** The math engine is solid — all work in this session is UI-layer only.

---

## Active Mission: Interface Redesign

The previous QML interface used the Nord palette but lacked structural hierarchy, making it feel unfinished. The redesign has been fully specified. Implement it faithfully.

### Design System

**Palette — retain Nord base, add semantic roles:**

```
Background layers:
  --bg-shell:    #1e2030   (calculator body)
  --bg-display:  #0b1120   (LCD panel — always dark)
  --bg-surface:  #252840   (key surface / cards)
  --bg-section:  #1a1c2e   (section dividers)

Text:
  --text-primary:   #e2e8f0
  --text-secondary: #a0aec0
  --text-muted:     #4a5568
  --text-display:   #e2e8f0  (LCD main readout)
  --text-expr:      #3b82f6  (expression history line)
  --text-result:    #4ade80  (computed result — green flash)
  --text-error:     #f87171

Semantic key colours:
  Operators (+−×÷):  #1e3a5f border / #163354 bg   (blue family)
  ENTER:             #14532d border / #0f3d20 bg   (green family)
  CLEAR / danger:    inherit text-primary, --text-error on label
  2ND:               #78350f border / #4c1d02 bg   (amber family)
  Function keys:     --bg-surface, slightly lighter than numeric keys
  Numeric keys:      --bg-display darkened  (#111827)
```

**Typography:**
- Display readout: `'Courier New', monospace` — 30px, right-aligned
- Expression history: `monospace` — 11px, right-aligned, --text-expr colour
- Section labels: 8px, letter-spacing 0.12em, ALL CAPS, --text-muted
- Key labels: 11px, font-weight 500 for numbers/ops, 10px for function keys
- Header brand: monospaced, 10px, letter-spacing 0.14em — renders as "TUX·TI83"

**Layout structure (top to bottom):**
1. Header strip — brand name left, mode indicator right (NORMAL / DEG / RAD)
2. LCD display panel — expression line (top, small) + result line (large)
3. Soft-key row — Y= · WINDOW · ZOOM · TRACE · GRAPH (5 equal columns, 1px gap grid)
4. Section: CONTROL — 2ND · MODE · ⌫ · ALPHA · CLEAR
5. Section: SCIENTIFIC — MATH · sin( · cos( · tan( · ^ / x² · √( · ln( · log( · π
6. Section: NUMERIC — standard 4×5 grid + operators right column + ENTER bottom-right

Each section has: an 8px uppercase label + full-width hairline rule above the keypad.

**Key anatomy:**
- Height: 36px
- Border radius: 6px
- Border: 0.5px solid (semantic colour or neutral)
- Hover: lighten background 8%
- Active/press: `scale(0.91)` with 60ms transition

---

## QML Implementation Notes

- Use `Rectangle` + `Text` for keys — do not rely on `Button` from Controls 2 (harder to style consistently)
- The LCD display panel is a `Rectangle` with `color: "#0b1120"` — this is intentional and permanent, not a theme variable
- Section dividers: a `Row` containing a `Text` label and a `Rectangle` hairline, anchored full width
- Soft-key row: a `Row` with equal-width `Rectangle` items separated by 1px `Rectangle` spacers and a shared background
- All key colours defined as `readonly property` values in a top-level `QtObject` style singleton — no hardcoded colours scattered through component files
- The cursor blink on the display: a `Rectangle` 2×28px, `SequentialAnimation` on `opacity` toggling 0↔1 every 500ms, stopped while `justEvaled` is true

### State machine (display logic)
```
State: INPUTTING
  - display-expr line: empty
  - display-result: current expression text (growing)
  - cursor: visible and blinking

State: EVALUATED  (after ENTER)
  - display-expr line: "expression ="
  - display-result: formatted result in --text-result (green)
  - cursor: hidden
  - Next digit/function keypress: clears expr, returns to INPUTTING
  - Next operator keypress: appends to result value, returns to INPUTTING

State: ERROR
  - display-expr line: failed expression
  - display-result: "ERR:SYNTAX" in --text-error
  - cursor: hidden
```

### Expression evaluation bridge
The QML layer calls into `core_math` via a registered C++ `QObject` — do not re-implement evaluation logic in QML/JS. The bridge object should expose:
```cpp
Q_INVOKABLE QString evaluate(const QString &expression);
Q_INVOKABLE QString lastError() const;
```

---

## Key Mapping (keyboard shortcuts)
```
0–9, .          → digit input
+ - * /         → operators (map to display symbols +, −, ×, ÷)
^               → power operator
( )             → parentheses
Enter / =       → evaluate
Backspace       → delete last character
Escape          → CLEAR
s               → sin(
c               → cos(
t               → tan(
l               → log(
n               → ln(
r               → √(
p               → π
```

---

## Build & Dev Workflow

```bash
# Full rebuild
./build.sh

# Incremental (from build/)
cd build && make -j$(nproc)

# Run
./build/Tux-TI83
```

Qt 6.5+ required. Ensure `qmake`/`cmake` resolves to Qt6, not Qt5.
On Linux Mint: `sudo apt install qt6-base-dev qt6-declarative-dev cmake`

---

## Immediate Task Queue

When this session begins, work through these in order:

1. **Create `app/Style.qml`** — style singleton with all colours, sizes, and font specs as `readonly property` values. This is the single source of truth for the entire UI.

2. **Rebuild `app/Main.qml`** — implement the full layout structure described above. Replace any existing layout wholesale; do not patch the old one.

3. **Implement `app/components/CalcKey.qml`** — reusable key component accepting `label`, `keyType` (enum: numeric / operator / function / control / enter / second), and `onPressed` signal.

4. **Implement `app/components/Display.qml`** — the LCD panel component with expression line, result line, and cursor animation.

5. **Implement `app/components/SoftKeyRow.qml`** — the five soft-key buttons above the main keypad.

6. **Wire keyboard shortcuts** — `Keys.onPressed` handler in the root `ApplicationWindow`.

7. **Verify build compiles and runs** after each component is added. Do not batch all components and build once at the end.

---

## Constraints

- Do not break the `core_math` API
- Do not modify `CMakeLists.txt` unless a new QML file requires registration
- All new QML files go in `app/` or `app/components/`
- Commit message format: `ui: <what changed>` (e.g. `ui: add Style singleton`)
- If something in `core_math` appears buggy during UI wiring, flag it with a `// FIXME:` comment and continue — do not fix math engine issues during a UI session

## Bug Discovery Workflow

When you find a bug **anywhere** in the codebase (not just `core_math`):

1. **Do not silently fix it.** Tell the user what you found, where, and why
   it's a bug. The user decides whether to fix now, defer, or leave alone.
2. **Log it in [BUGS.md](BUGS.md)** using the template at the top of that
   file. Assign the next sequential `BUG-NNN` id. Even bugs the user chooses
   to fix immediately should be logged (then moved to the "Fixed" section
   when resolved).
3. This rule applies regardless of session focus — UI sessions still log
   math-engine bugs, math sessions still log UI bugs.

## Improvement Workflow

When you notice code that **works but could be improved** — opportunities
for refactoring, clarity, reuse, simpler architecture, or making future
roadmap work easier — apply the same rule:

1. **Do not silently apply the change.** Tell the user what you noticed,
   where, what you'd propose, and what trade-offs are involved. The user
   decides whether to apply now, defer until later in the roadmap, or
   decline.
2. **Log it in [IMPROVEMENTS.md](IMPROVEMENTS.md)** using the template at
   the top of that file. Assign the next sequential `IMP-NNN` id. Applied
   improvements move to the "Applied" section; declined ones move to
   "Declined" with the reason.
3. This is the dual of the bug workflow: bugs are things that are *broken*,
   improvements are things that *work but could be better*. Both deserve
   transparency before action.

## Project Ledger Maintenance

Tux-TI83 has three living catalogue files that together form the project's
complete ledger:

| File | Tracks |
|---|---|
| [ROADMAP.md](ROADMAP.md) | Features and capabilities — what to build, organised by area |
| [BUGS.md](BUGS.md) | Things that are broken |
| [IMPROVEMENTS.md](IMPROVEMENTS.md) | Things that work but could be better |

These files **must** be kept in sync with the actual state of the code as
work progresses. Drift between code and ledger makes the project lose
track of itself. Specifically:

1. **When you start work on a feature**, move its [ROADMAP.md](ROADMAP.md)
   entry from 📅/🔜 to 🚧.
2. **When you finish a feature**, move it from 🚧 to ✅. Note any caveats
   inline (e.g. "engine done, UI exposure pending"). If a whole phase is
   complete, note the date in the phase header.
3. **When the user adds a feature request**, add it to
   [ROADMAP.md](ROADMAP.md) as 📅 (or 🔜 if it's the immediate next step).
4. **When you find a bug**, log it in [BUGS.md](BUGS.md) per the Bug
   Discovery Workflow above.
5. **When you fix a bug**, move it to the Fixed section of
   [BUGS.md](BUGS.md) with the date and a brief note about the fix.
6. **When you notice an improvement opportunity**, log it in
   [IMPROVEMENTS.md](IMPROVEMENTS.md) per the Improvement Workflow above.
7. **When you apply an improvement**, move it to the Applied section.
8. **Cross-reference between files when relevant.** A ROADMAP item that's
   blocked by a bug should link to the bug entry. An improvement that
   would close a bug should mention it. The goal is one coherent ledger,
   not three disconnected files.

These updates are not optional housekeeping — they're part of any change.
If you mark a step done in a chat response without updating the
corresponding entries in these files, the ledger is already drifting.

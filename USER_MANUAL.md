# Tux-TI83 User Manual

A day-to-day operational guide to Tux-TI83 — the GUI, the CLI binaries,
and every feature currently shipping. If you're looking for project
architecture or contributor workflows, see [README.md](README.md) and
[CLAUDE.md](CLAUDE.md) instead.

> **Status:** actively maintained. Core calculation, variables, matrices,
> lists, statistics/regressions, calculus, and graphing (including polar)
> all ship today. A few sections still flag *"planned content"* for
> worked examples and screenshots. See [ROADMAP.md](ROADMAP.md) for the
> full, current feature ledger.

## Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Interface Overview](#interface-overview)
4. [Basic Calculations](#basic-calculations)
5. [The Keypad](#the-keypad)
6. [The MATH Menu](#the-math-menu)
7. [Variables & Storage](#variables--storage)
8. [Matrices](#matrices)
9. [Lists & Statistics](#lists--statistics)
10. [Graph Mode](#graph-mode)
11. [The MODE Menu](#the-mode-menu)
12. [Keyboard Shortcuts](#keyboard-shortcuts)
13. [CLI Usage](#cli-usage-one-shot)
14. [REPL Usage](#repl-usage-interactive)
15. [Ans and Conversions](#ans-and-conversions)
16. [Error Messages](#error-messages)
17. [Tips & Tricks](#tips--tricks)
18. [Troubleshooting](#troubleshooting)
19. [Glossary](#glossary)
20. [Appendix: Function Reference](#appendix-function-reference)

---

## Introduction

Tux-TI83 is a Linux desktop calculator that reimagines the Texas
Instruments TI-83 Plus. If you've used a TI-83 before, most of what
you know carries over — the same function names, the same error
labels, the same operator precedence. If you haven't, this manual
walks you through the essentials.

The project ships four binaries:

| Binary | Use case |
|---|---|
| `tux_ti83` | Graphical calculator (primary) |
| `tux_ti83_cli` | One-shot command-line evaluation |
| `tux_ti83_repl` | Interactive command-line calculator |
| `tux_ti83_tests` | Developer regression suite (skip unless hacking) |

Most users stick with the GUI. Power users and shell-script authors
will find the CLI binaries useful.

## Getting Started

After [building the project](README.md#build):

```bash
./build/tux_ti83              # GUI
./build/tux_ti83_cli "2+2"    # one-shot → 4
./build/tux_ti83_repl         # interactive prompt
```

The GUI window opens immediately; see [Interface Overview](#interface-overview)
for what you're looking at.

## Interface Overview

*Screenshots and per-region labelling planned.* In brief, top to bottom:

- **Header strip** — brand name, mode indicators (`NORMAL`, `DEG`)
- **Function selector** — `Y1` / `Y2` / `Y3` tabs (which function slot
  you're editing)
- **LCD display** — top line shows the last evaluated expression,
  bottom line shows the live input or the computed result
- **Soft-key row** — `Y=` · `WINDOW` · `ZOOM` · `TRACE` · `GRAPH`
- **CONTROL section** — `2ND`, `MODE`, `⌫`, `ALPHA`, `CLEAR`
- **SCIENTIFIC section** — trig functions, logarithms, constants,
  `MATH` menu
- **NUMERIC section** — digits, operators, `Ans`, `ENTER`
- **History panel** (right-side column) — every expression you've
  evaluated, most recent at top

## Basic Calculations

### Entering an expression

Click the keypad or type on your keyboard. For `2+2`:

1. Press `2`
2. Press `+`
3. Press `2`
4. Press `ENTER` (or keyboard Enter)

The result `4` appears in green on the main line of the LCD. The top
line shows `2+2 =` in blue so you can see what you just evaluated.

### Chaining with Ans

`Ans` holds the last successful result. To use it, press the `Ans`
key (NUMERIC row 4 column 4):

```
5 × 3         → 15
Ans + 100     → 115
Ans / 5       → 23
```

### Fractions and decimals

By default, results display as decimals. To convert a result to its
fraction form (when one exists), open the MATH menu and click `▶Frac`.
Click `▶Dec` to convert back. Irrational results (`e`, `π`, `√2`…)
silently remain decimal — no exact fraction exists.

### Clearing

- `CLEAR` (red) — empties the current expression, back to a blank slate
- `⌫` (backspace) — deletes the last token

## The Keypad

*Detailed walk-through with screenshots planned.* At a glance:

- **CONTROL** — `2ND` and `ALPHA` modifiers, `MODE` menu, delete (`⌫`),
  `CLEAR`
- **CURSOR** — `HOME`, `←`, `→`, `END` for in-expression editing, plus
  `STO▸`
- **SCIENTIFIC** — math functions with dedicated keys (`sin(`, `cos(`,
  `tan(`, `√(`, `ln(`, `log(`), the `^` operator, the constants `π`
  and `e`, plus the `MATH` menu and `MATRX` popup triggers
- **NUMERIC** — digits, the four standard operators, `x²`, `Ans`,
  parentheses, comma, the `X` variable, unary `(-)`, and `ENTER`

Most keys carry a **2ND** function (amber, top-left corner) and an
**ALPHA** letter (green, top-right). Press `2ND` then the key for the
amber label, or `ALPHA` then the key for the green letter. `2ND` then
`ALPHA` engages **A-LOCK** for typing several letters in a row.

## The MATH Menu

The `MATH` key (top-left of SCIENTIFIC) opens a popup listing
additional functions that don't have dedicated keypad keys:

| Entry | Description | Arity |
|---|---|---|
| `abs(` | Absolute value | Unary |
| `int(` | Floor (largest integer ≤ x) | Unary |
| `iPart(` | Integer part (truncation toward zero) | Unary |
| `fPart(` | Fractional part (`x − iPart(x)`) | Unary |
| `round(` | Round x to n decimal places | Binary |
| `min(` | Minimum of two values | Binary |
| `max(` | Maximum of two values | Binary |
| `mod(` | Modulo | Binary |
| `nCr(` | Combinations (n choose r) | Binary |
| `nPr(` | Permutations | Binary |
| `!` | Factorial (unary postfix) | Unary |
| `sinh(`, `cosh(`, `tanh(` | Hyperbolic | Unary |
| `asinh(`, `acosh(`, `atanh(` | Inverse hyperbolic | Unary |
| `e^(` | Exponential | Unary |
| `sgn(` | Sign (−1 / 0 / +1) | Unary |
| `fnInt(` | Numeric integral `fnInt(expr, var, a, b)` | Calculus |
| `nDeriv(` | Numeric derivative `nDeriv(expr, var, x[, h])` | Calculus |
| `sum(` / `prod(` | 4-arg summation/product, **or** 1-arg list reduction | Overloaded |
| `seq(` | Generate a list: `seq(expr, var, start, end[, step])` | List |
| `mean(`, `median(`, `stdDev(`, `variance(` | List statistics | List |
| `▶Frac` | Post-hoc: convert last result to fraction | Action |
| `▶Dec` | Post-hoc: convert last result to decimal | Action |
| `→ (STO)` | Store to a variable or list | Action |

`sum(`/`prod(`/`min(`/`max(` are **overloaded**: a single list argument
(e.g. `sum(L1)`) reduces the list; the multi-argument forms
(`sum(X,X,1,4)`, `min(3,7)`) keep their original meaning.

*Worked examples planned.*

## Variables & Storage

Tux-TI83 remembers your work between sessions and across expressions.

**Scalar variables `A`–`Z`.** Store a value with `STO▸` (the arrow):

```
5→A            → 5        (stores 5 in A)
A+3→A          → 8        (A is now 8)
A²             → 64
```

On the keyboard, press **Shift+letter** for a variable (bare lowercase
letters are function shortcuts), or `|` / `->` for `STO▸`. `X` doubles as
the graphing sweep variable and as an ordinary scalar on the home screen.

**`Ans`.** The last successful result — see [Ans and Conversions](#ans-and-conversions).

**Last-entry recall.** `2ND` + `ENTER` walks backward through a 10-deep
history of what you typed, so you can fix a typo or reuse an expression.

**Y-VARS.** Reference `Y1`, `Y2`, `Y3` from another expression — define
`Y1 = X²`, then `Y2 = Y1+10` plots `X²+10`. The argument form `Y1(3)`
evaluates `Y1` at `X = 3`. Self-reference and cycles raise `ERR:RECURSION`.

**Persistence.** Scalars, matrices, lists, `Y=` buffers, the viewport, and
MODE settings are saved to `~/.local/state/tux-ti83/state.json` (both
periodically and on clean exit) and restored on the next launch. The
**RESET** button in the MODE popup wipes everything back to defaults.

## Matrices

Press the `MATRX` key (SCIENTIFIC section, row 2). The popup has three
tabs:

- **NAMES** — click `[A]`–`[E]` to insert it at the cursor
- **MATH** — insert `det(`, `T(` (transpose), `rref(`, or `^-1` (inverse)
- **EDIT** — the matrix editor v2: pick a matrix with the `[A]`–`[E]`
  selector, set its dimensions with the `R`/`C` steppers (up to 6×6), fill
  the grid, and click `SAVE TO [x]`. Existing values are read back when
  you open a matrix, so you can edit rather than retype.

Operations currently supported:

| Operation | Example |
|---|---|
| Addition / subtraction | `[A]+[B]` / `[A]-[B]` (same dimensions) |
| Scalar multiplication | `3*[A]` or `[A]*3` |
| Matrix multiplication | `[A]*[B]` (conformable) |
| Determinant | `det([A])` (square only) |
| Transpose | `T([A])` |
| Inverse | `[A]^-1` (square, non-singular) |
| Reduced row-echelon | `rref([A])` |

Errors surface as `ERR:INVALID DIM` (mismatched shapes),
`ERR:DATA TYPE` (mixing matrix and scalar where not allowed),
`ERR:SINGULAR MAT` (inverting a singular matrix), or `ERR:UNDEFINED`
(matrix referenced before editing). The engine backs `[A]`–`[J]`; the UI
currently exposes `[A]`–`[E]`.

## Lists & Statistics

**Lists `L1`–`L6`** hold sequences of numbers. Type a literal with
`{` and `}` (2ND+`(` / 2ND+`)`), and store with `STO▸`:

```
{1,2,3}→L1     → {1,2,3}
L1+10          → {11,12,13}    (scalar broadcasts)
L1+L1          → {2,4,6}       (element-wise; equal lengths)
2L1            → {2,4,6}
```

Insert a list name with `2ND` + a number key (`2ND`+`1` → `L1`, …).
Element-wise arithmetic (`+ − × ÷ ^`) requires equal lengths
(`ERR:INVALID DIM` otherwise); a scalar operand broadcasts.

**The Stat list editor** opens with `2ND` + `MATRX` (labelled `STAT`).
Pick a list, set its length, and fill in values. It also hosts the
statistics buttons.

**List functions** (also in the MATH menu): `sum(`, `prod(`, `mean(`,
`median(`, `min(`, `max(`, `stdDev(`, `variance(` reduce a list to a
scalar. `seq(expr, var, start, end[, step])` builds a list — so the
classic `sum(seq(X²,X,1,4))` → `30` works.

**1-Var Stats.** In the Stat editor, select a list and click
`1-VAR STATS`. A results screen shows n, x̄, Σx, Σx², Sx (sample sd),
σx (population sd), minX, Q1, Med, Q3, maxX.

**2-Var Stats & regression.** Put your X data in `L1` and Y data in `L2`,
then use the editor's buttons:

- `2-VAR L1,L2` — the two-variable summary plus **LinReg** (slope `a`,
  intercept `b`, correlation `r`, `r²`)
- `REGRESSIONS ▸` — a menu of `QuadReg`, `CubicReg`, `ExpReg`, `LnReg`,
  `PwrReg` (each reports its coefficients and `R²` / `r`)

Exp/Ln/Pwr models require positive data where the maths demands it
(`ERR:DOMAIN` otherwise); mismatched list lengths give `ERR:INVALID DIM`.

## Graph Mode

Press the `GRAPH` soft-key (top-right of the soft-key row) to switch
the bottom half of the calculator from keypad to graph canvas.

To plot a function:

1. Use the `Y1` / `Y2` / `Y3` tab at the top to pick a slot
2. Enter an expression using `X` as the variable (e.g., `X^2-2`)
3. Press `GRAPH`

The canvas shows all three Y slots' curves simultaneously, each in
its own colour. Axis labels and grid lines scale dynamically with the
viewport.

**Interacting with the graph:**
- Click-drag to **pan**
- Scroll-wheel to **zoom** centred on the cursor

**Changing the viewport:**
Press the `WINDOW` soft-key to open the viewport editor. Enter
Xmin / Xmax / Ymin / Ymax values. The `ZOOM` soft-key offers presets:
ZStandard (`-10..10`), ZoomFit (auto-scale Y), Zoom In/Out, ZSquare,
ZTrig, ZDecimal, and ZInteger.

**Trace.** The `TRACE` soft-key drops a crosshair on the active curve
with a live X/Y readout; `←`/`→` step along the curve and `↑`/`↓` cycle
between Y1/Y2/Y3.

**Table.** `2ND` + `GRAPH` opens the `TABLE` view — a scrollable
`X | Y1 | Y2 | Y3` grid. `2ND` + `WINDOW` opens `TBLSET` to set the table
start and step.

**Polar mode.** Set MODE → Graph → **Pol** to plot `r = f(θ)`. The three
slots become `r1`/`r2`/`r3`; enter the function using `X` as the angle θ
(e.g. `r1 = 4sin(3X)` draws a rose). See [The MODE Menu](#the-mode-menu).

Press `Y=` to return to the keypad.

*Planned: a full Y-editor with on/off toggles and function styles (thick /
dotted / shaded).*

## The MODE Menu

The `MODE` key (CONTROL section) opens the settings popup. Wired rows take
effect immediately and are reflected in the header indicator:

| Row | Options | Effect |
|---|---|---|
| **Angle** | Radian / Degree | How trig functions interpret their input |
| **Notation** | Normal / Sci / Eng | Number display format |
| **Decimal** | Float / Fix 0–9 | Fixed decimal places |
| **Base** | Dec / Hex / Oct / Bin | Integer results shown in the chosen base (`0xFF`, `0o77`, `0b1010`); non-integers fall back to decimal |
| **Graph** | Func / Pol | Cartesian `y=f(x)` or polar `r=f(θ)` |
| **Draw** | Connected / Dot | How graph curves are drawn |

`RESET` (in this popup) restores factory defaults and clears all saved
state. The remaining rows (Plot, Complex, Screen, and Graph's Par/Seq)
are shown greyed — placeholders for features not yet built.

## Keyboard Shortcuts

```
0–9, .                  digit entry
A–Z (Shift+letter)      scalar variables
+ − * /                 operators (converted to Unicode for display)
^                       power
( ) ,                   parens and argument separator
!                       factorial (postfix)
Enter / =               evaluate
Backspace               delete last token
Escape                  CLEAR
← → Home End            move the in-expression cursor
s / c / t               sin( / cos( / tan(
l / n / r               log( / ln( / √(
p                       π
|  (or ->)              STO▸
```

Bare lowercase letters are function shortcuts (`s` → `sin(`); press
**Shift+letter** for a scalar variable (`A`–`Z`), and type `Y1`/`Y2`/`Y3`
directly for the Y-VARS. On the on-screen keypad, `2ND` reaches the amber
labels — including `{` / `}` (2ND+`(` / `)`) and `L1`–`L6` (2ND+`1`–`6`).

Functions without a dedicated shortcut (abs, round, min, max,
hyperbolics, nCr, calculus, list functions, …) come from the MATH menu.

## CLI Usage (one-shot)

For quick one-line calculations or shell scripting:

```bash
$ ./build/tux_ti83_cli "2+2"
4
$ ./build/tux_ti83_cli "sin(0)"
0
$ ./build/tux_ti83_cli "nCr(52, 5)"
2598960
```

Output is ANSI-coloured (green for results, red for errors) when
stdout is a tty; plain when piped or redirected.

**Shell gotcha:** wrap expressions containing `!` in **single quotes**:

```bash
$ ./build/tux_ti83_cli '5!+3!'      # correct → 126
$ ./build/tux_ti83_cli "5!+3!"      # bash: event not found
```

(Bash's history expansion fires on `!` even inside double quotes.)

Exit codes:
- `0` — success
- `1` — evaluation error (e.g., `ERR:DIVIDE BY 0`)
- `2` — usage error (no expression given)

## REPL Usage (interactive)

```bash
$ ./build/tux_ti83_repl
Tux-TI83 REPL — type :quit (or Ctrl+D) to exit,
                Ans recalls the last result.
> 2+2
4
> Ans*3
12
> nCr(52, 5)
2598960
> 5!+3!
126
> :quit
```

Exit via `:quit`, `:q`, or Ctrl+D. `Ans` persists across lines.

Because stdin bypasses bash's arg-parsing, there's no history-expansion
issue — `!` and other special characters can be typed freely.

## Ans and Conversions

### Ans recall

Every successful ENTER writes the result to `Ans`. To reuse it:

- **GUI:** press the `Ans` CalcKey (NUMERIC row 4 column 4)
- **REPL:** type `Ans` in your next expression

Errors do **not** overwrite `Ans`. If your last calculation errored,
`Ans` still holds the previous good result.

### ▶Frac / ▶Dec

These are post-hoc display conversions — they re-format the current
result without re-evaluating anything:

```
1÷3           → 0.3333333333   (default: decimal)
(MATH → ▶Frac) → 1/3             (re-displayed as fraction)
(MATH → ▶Dec)  → 0.3333333333   (re-displayed as decimal)
```

`▶Frac` silently does nothing when the result is irrational — there's
no exact fraction within tolerance for `e`, `π`, `√2`, etc.

## Error Messages

TI-83-style error labels:

| Label | Meaning |
|---|---|
| `ERR:SYNTAX` | Expression couldn't be parsed (mismatched parens, missing operand, unknown function) |
| `ERR:DIVIDE BY 0` | Attempted to divide by zero; also thrown by `mod(x, 0)` |
| `ERR:NONREAL ANS` | Operation would produce a non-real result — `√(-1)`, `log(-5)`, `ln(0)` |
| `ERR:DOMAIN` | Input outside the function's valid domain — `asin(2)`, `(-5)!`, `acosh(0.5)`, `nCr(5, 6)` |
| `ERR:INVALID DIM` | Dimension mismatch — adding a 2×2 to a 3×3, unequal-length lists, a backwards `seq` range, etc. |
| `ERR:DATA TYPE` | Type mismatch — mixing a matrix/list and a scalar where not allowed, or a store-target mismatch (`5→L1`, `{1,2}→A`) |
| `ERR:UNDEFINED` | Referenced a matrix or list before storing values into it |
| `ERR:SINGULAR MAT` | Tried to invert a singular (non-invertible) matrix |
| `ERR:RECURSION` | A Y-VAR references itself directly or through a cycle |

## Tips & Tricks

*More patterns coming — this section will grow with common idioms.*

- **Integer check:** `x - int(x)` equals 0 iff `x` is an integer
  (positive or negative).
- **Exact fraction when possible:** after ENTER shows a decimal,
  open MATH and click `▶Frac` to see the rational form. If nothing
  changes, the value is irrational.
- **Chained Ans:** each ENTER updates Ans, so you can build a
  calculation incrementally — `1+1` ENTER, `Ans*2` ENTER, `Ans+5`
  ENTER walks through `2 → 4 → 9`.
- **REPL for batch eval:** pipe a file of expressions to
  `tux_ti83_repl` via stdin for quick bulk evaluation.

## Troubleshooting

### `bash: event not found` when CLI expression contains `!`

Bash's history expansion. Wrap in single quotes:

```bash
./build/tux_ti83_cli '5!+3!'
```

Or use `tux_ti83_repl` — it reads stdin, bypassing bash parsing.

### `⌫` key label shows as a blank square

Font fallback — your system's monospace font is missing the U+232B
glyph. Harmless; the key still functions normally. Installing a
fuller Unicode font (e.g., DejaVu Sans Mono, Noto Sans Mono) resolves
the display.

### Keyboard shortcuts stop working mid-session

Click anywhere on the calculator window background to restore focus
to the root layout. Certain Qt widget interactions (like focus
entering a TextField inside a popup) can transfer focus away.

*More troubleshooting entries coming — graph rendering issues, build
errors, Qt module detection, etc.*

## Glossary

- **Token** — the smallest input unit the calculator recognises
  (e.g., `5`, `+`, `sin(`, `[A]`, `,`). The tokeniser turns your
  typed string into a sequence of tokens before evaluation.
- **Buffer** — the sequence of tokens currently staged for
  evaluation. `CLEAR` empties it; `ENTER` evaluates it.
- **Display state** — the calculator's UI mode at any moment:
  **INPUTTING** (actively typing), **EVALUATED** (showing a computed
  result), **ERROR** (showing an `ERR:…` label).
- **Ans** — the last successful evaluation result; updated by each
  successful ENTER.
- **Soft key** — one of `Y=`, `WINDOW`, `ZOOM`, `TRACE`, `GRAPH` along
  the top of the keypad. Named for the hardware-calculator convention
  where the five keys below the screen change meaning based on context.

## Appendix: Function Reference

*Planned: an alphabetical table of every function and operator with
syntax, arity, domain, example, and cross-references. Until that
lands, the [README.md features list](README.md#features) and
[ROADMAP.md](ROADMAP.md) together cover everything that ships.*

---

*This manual tracks the shipping feature set. Sections marked "planned"
(worked examples, screenshots, the full function-reference appendix) will
be expanded over time; contributions welcome — see [CLAUDE.md](CLAUDE.md)
for the development workflow.*

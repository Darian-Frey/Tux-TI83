# Tux-TI83 User Manual

A day-to-day operational guide to Tux-TI83 — the GUI, the CLI binaries,
and every feature currently shipping. If you're looking for project
architecture or contributor workflows, see [README.md](README.md) and
[CLAUDE.md](CLAUDE.md) instead.

> **Status:** skeleton. Each section has enough content to orient you,
> but several sections are marked *"planned content"* — they'll be
> fleshed out over time with worked examples and screenshots. See the
> `USER_MANUAL.md` entry under **Tooling & testing** in
> [ROADMAP.md](ROADMAP.md) for progress.

## Contents

1. [Introduction](#introduction)
2. [Getting Started](#getting-started)
3. [Interface Overview](#interface-overview)
4. [Basic Calculations](#basic-calculations)
5. [The Keypad](#the-keypad)
6. [The MATH Menu](#the-math-menu)
7. [Matrices](#matrices)
8. [Graph Mode](#graph-mode)
9. [Keyboard Shortcuts](#keyboard-shortcuts)
10. [CLI Usage](#cli-usage-one-shot)
11. [REPL Usage](#repl-usage-interactive)
12. [Ans and Conversions](#ans-and-conversions)
13. [Error Messages](#error-messages)
14. [Tips & Tricks](#tips--tricks)
15. [Troubleshooting](#troubleshooting)
16. [Glossary](#glossary)
17. [Appendix: Function Reference](#appendix-function-reference)

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

- **CONTROL** — modifier keys (some planned: `2ND`, `ALPHA`), delete /
  clear, mode menu (planned)
- **SCIENTIFIC** — math functions with dedicated keys (`sin(`, `cos(`,
  `tan(`, `√(`, `ln(`, `log(`), the `^` operator, the constants `π`
  and `e`, plus the `MATH` menu and `MATRX` popup triggers
- **NUMERIC** — digits, the four standard operators, `x²`, `Ans`,
  parentheses, comma, the `X` variable, unary `(-)`, and `ENTER`

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
| `▶Frac` | Post-hoc: convert last result to fraction | Action |
| `▶Dec` | Post-hoc: convert last result to decimal | Action |

*Worked examples planned.*

## Matrices

Press the `MATRX` key (SCIENTIFIC section, row 2). The popup has three
tabs:

- **NAMES** — click `[A]`, `[B]`, or `[C]` to insert it at the cursor
- **MATH** — click `det(` to insert the determinant function
- **EDIT** — 3×3 grid editor for matrix `[A]`; fill cells, click
  `SAVE TO [A]` to commit

Operations currently supported:

| Operation | Example |
|---|---|
| Addition | `[A]+[B]` (same dimensions required) |
| Subtraction | `[A]-[B]` |
| Scalar multiplication | `3*[A]` or `[A]*3` |
| Matrix multiplication | `[A]*[B]` (conformable) |
| Determinant | `det([A])` (square only) |

Errors surface as `ERR:INVALID DIM` (mismatched shapes),
`ERR:DATA TYPE` (mixing matrix and scalar where not allowed), or
`ERR:UNDEFINED` (matrix referenced before editing).

*Worked examples planned: solving `Ax = b`, computing area via cross
product, etc.*

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
Xmin / Xmax / Ymin / Ymax values. Two shortcuts:
- `ZSTD` — reset to `-10..10` on both axes
- `ZFIT` — auto-scale Y to fit the current functions

Press `Y=` to return to the keypad.

*Planned: function styles (thick / dotted / shaded), trace mode, tables.*

## Keyboard Shortcuts

```
0–9, .                  digit entry
+ − * /                 operators (converted to Unicode for display)
^                       power
( ) ,                   parens and argument separator
!                       factorial (postfix)
Enter / =               evaluate
Backspace               delete last token
Escape                  CLEAR
s / c / t               sin( / cos( / tan(
l / n / r               log( / ln( / √(
p                       π
```

Single-letter shortcuts are unconditional — pressing `s` immediately
inserts `sin(` regardless of context. A future **ALPHA modifier**
([IMP-003](IMPROVEMENTS.md)) will let bare letters be typed as
literals for variable names; until then, letter keys are reserved for
function shortcuts.

Functions without a dedicated shortcut (abs, round, min, max,
hyperbolics, nCr, nPr, etc.) must come from the MATH menu.

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
| `ERR:INVALID DIM` | Matrix dimension mismatch — adding a 2×2 to a 3×3, etc. |
| `ERR:DATA TYPE` | Type mismatch, usually mixing a matrix and a scalar in an op that doesn't support it |
| `ERR:UNDEFINED` | Referenced a matrix (`[B]`, `[C]`) before editing values into it |

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

*This skeleton is a starting point. Sections marked "planned" will be
expanded over time; contributions welcome — see [CLAUDE.md](CLAUDE.md)
for the development workflow.*

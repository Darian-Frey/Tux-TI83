# Tux-TI83 User Manual

A day-to-day operational guide to Tux-TI83 — the GUI, the CLI binaries,
and every feature currently shipping. If you're looking for project
architecture or contributor workflows, see [README.md](README.md) and
[CLAUDE.md](CLAUDE.md) instead.

> **Status:** actively maintained. Core calculation, variables, matrices
> (including typed literals), lists, statistics/regressions, probability
> distributions, calculus, complex numbers, and graphing in all four modes
> (Func / Par / Pol / Seq) ship today, along with the full MODE menu and
> three UI themes (Dark / Light / Amber). A few sections still flag
> *"planned content"* for worked examples and screenshots. See
> [ROADMAP.md](ROADMAP.md) for the full, current feature ledger.

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
11. [Programming (TI-BASIC)](#programming-ti-basic)
12. [The MODE Menu](#the-mode-menu)
13. [Keyboard Shortcuts](#keyboard-shortcuts)
14. [CLI Usage](#cli-usage-one-shot)
15. [REPL Usage](#repl-usage-interactive)
16. [Ans and Conversions](#ans-and-conversions)
17. [Complex Numbers](#complex-numbers)
18. [Error Messages](#error-messages)
19. [Tips & Tricks](#tips--tricks)
20. [Troubleshooting](#troubleshooting)
21. [Glossary](#glossary)
22. [Appendix: Function Reference](#appendix-function-reference)

---

## Introduction

Tux-TI83 is a Linux desktop calculator that reimagines the Texas
Instruments TI-83 Plus. If you've used a TI-83 before, most of what
you know carries over — the same function names, the same error
labels, the same operator precedence. If you haven't, this manual
walks you through the essentials. It also includes a full **TI-BASIC**
programming environment — write, run, and save programs on the calculator,
including ones that graph and draw (see
[Programming (TI-BASIC)](#programming-ti-basic)).

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

- **Header strip** — brand name; mode indicators for notation
  (`NORMAL` / `SCI` / `ENG`), angle (`RAD` / `DEG`), graph mode
  (`POL` / `PAR` / `SEQ` when not in Func), and complex mode (`a+bi` /
  `re^θi` when not Real)
- **Function selector** — `Y1` / `Y2` / `Y3` tabs (which function slot
  you're editing; the engine has ten slots, `Y1`–`Y9`, `Y0`)
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

**Brackets.** All three bracket pairs live on the `(` / `)` keys:
primary `(` `)`, `2ND` → `{` `}` (list literals), `ALPHA` → `[` `]`
(matrix literals). The displaced letters `K` / `L` moved to `ALPHA` +
`π` / `e`.

**Y-VARS.** `2ND` + `X` opens the **Y-VARS picker** — buttons `Y1`–`Y0`
that insert a function-reference token (see [Variables & Storage](#variables--storage)).

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

**Probability distributions** are also available (MATH menu / by name):
`normalpdf(`, `normalcdf(`, `invNorm(`, `binompdf(`, `binomcdf(`,
`poissonpdf(`, `poissoncdf(`, `geometpdf(`, `geometcdf(`, `tpdf(`,
`tcdf(`, `χ²pdf(`, `χ²cdf(`, `Fpdf(`, `Fcdf(`. The `…pdf`/`…cdf` forms
with a trailing `count` argument return a list.

**Random functions:** `rand`, `randInt(`, `randNorm(`, `randBin(` (a
trailing count gives a list), and `randM(r,c)` for a random matrix.

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

**Y-VARS.** Reference any of the ten function slots `Y1`–`Y9`, `Y0` from
another expression — define `Y1 = X²`, then `Y2 = Y1+10` plots `X²+10`.
The argument form `Y5(3)` evaluates `Y5` at `X = 3`. Self-reference and
cycles raise `ERR:RECURSION`.

You can also **store an expression into a slot** from the home screen:
`<expr>→Yn` saves the expression (not its value) into that function
buffer, so it plots and shows in the Y= editor. It reports `Done`.

To insert a `Yn` token on-screen, use the **Y-VARS picker** (`2ND` + `X`).
On the physical keyboard, press the letter `Y` then a digit (`Y` `1` →
the `Y1` token). Entering "Y1" any other way inserts the *letter-Y
variable* times 1.

**Persistence.** Scalars, matrices, lists, `Y=` buffers, the viewport, and
all MODE settings (including the theme) are saved to
`~/.local/state/tux-ti83/state.json` (both periodically and on clean exit)
and restored on the next launch. You can also keep **named snapshots**
under `~/.local/state/tux-ti83/saves/<name>.t83` — export/import a named
state to switch between different working setups. The **RESET** button in
the MODE popup wipes everything back to defaults.

## Matrices

Press the `MATRX` key (SCIENTIFIC section, row 2). The popup has three
tabs:

- **NAMES** — click `[A]`–`[E]` to insert a matrix reference, or `[` / `]`
  to insert the literal brackets
- **MATH** — insert matrix functions (see the table below)
- **EDIT** — the matrix editor v2: pick a matrix with the `[A]`–`[E]`
  selector, set its dimensions with the `R`/`C` steppers (up to 6×6), fill
  the grid, and click `SAVE TO [x]`. Existing values are read back when
  you open a matrix, so you can edit rather than retype.

**Typed matrix literals.** You can also type a matrix directly:
`[[1,2][3,4]]` (outer brackets wrap the matrix; each `[…]` is a row;
elements are comma-separated and may be expressions). Enter the brackets
via `ALPHA` + `(` / `)`, the physical `[` / `]` keys, or the NAMES tab.
Store a matrix into a register with `→`: `[[1,2][3,4]]→[A]`.

Operations currently supported:

| Operation | Example |
|---|---|
| Addition / subtraction | `[A]+[B]` / `[A]-[B]` (same dimensions) |
| Scalar multiplication | `3*[A]` or `[A]*3` |
| Matrix multiplication | `[A]*[B]` (conformable) |
| Determinant | `det([A])` (square only) |
| Transpose | `T([A])` |
| Inverse | `[A]^-1` (square, non-singular) |
| Row-echelon / reduced | `ref([A])` / `rref([A])` |
| Identity | `identity(n)` → n×n identity |
| Dimensions | `dim([A])` → `{rows,cols}` (or list length) |
| Augment | `augment([A],[B])` (equal rows) or list‖list |
| Random matrix | `randM(r,c)` → ints in [−9,9] |
| List → matrix | `List▶Matr(L1,…,Ln)` → each list a column |
| Matrix → list | `Matr▶List([A],col)` → that column as a list |

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

**List ↔ matrix.** `List▶Matr(L1,…,Ln)` builds a matrix with each list as
a column; `Matr▶List([A],col)` extracts a column back out as a list.
Store the results with `→[A]` / `→Ln`.

## Graph Mode

Press the `GRAPH` soft-key (top-right of the soft-key row) to switch
the bottom half of the calculator from keypad to graph canvas.

To plot a function:

1. Use the `Y1` / `Y2` / `Y3` tab at the top to pick a slot
2. Enter an expression using `X` as the variable (e.g., `X^2-2`)
3. Press `GRAPH`

The canvas shows every enabled function slot's curve simultaneously,
each in its own colour. Axis labels and grid lines scale with the
viewport.

**The Y= editor.** Press `Y=` from the keypad to open the editor for all
ten slots (`Y1`–`Y9`, `Y0`). Each row has an **on/off** toggle (disabled
slots don't plot) and a **line style** cycle (thin / thick / dotted).

**Interacting with the graph:**
- Click-drag to **pan**
- Scroll-wheel to **zoom** centred on the cursor

**Changing the viewport:**
Press the `WINDOW` soft-key to open the viewport editor: Xmin / Xmax /
Ymin / Ymax, plus **Xscl / Yscl** (axis tick spacing) and **Xres** (Func
sample stride, 1–8; higher = coarser/faster). Par/Pol modes add a
`T`/`θ` window (min/max/step); Seq mode adds `nMax` and the initial
terms. The `ZOOM` soft-key offers presets: ZStandard (`-10..10`),
ZoomFit (auto-scale Y), Zoom In/Out, ZSquare, ZTrig, ZDecimal, ZInteger.

**Trace.** The `TRACE` soft-key drops a crosshair on the active curve
with a live readout; `←`/`→` step along the curve and `↑`/`↓` cycle
between slots.

**FORMAT** (`2ND` + `ZOOM`): toggle **Grid**, **Axes**, **Coord** (the
trace readout), and **Label**; choose **RectGC / PolarGC** (trace readout
as `X=/Y=` or `R=/θ=`); and **ExprOn / ExprOff** (show the traced
function's equation top-left).

**DRAW** (`2ND` + `TRACE`): overlay a `Line`, `Horizontal`, `Vertical`,
`Pt-On`, or `Text`, delete overlays one at a time, or `ClrDraw` them all.

**Table.** `2ND` + `GRAPH` opens the `TABLE` view — a scrollable
`X | Y1 | Y2 | Y3` grid. `2ND` + `WINDOW` opens `TBLSET` to set the table
start and step.

**Graph modes** (MODE → Graph): **Func** (`y=f(x)`), **Par**
(parametric `X1T`/`Y1T`, …), **Pol** (`r=f(θ)`, e.g. `r1 = 4sin(3X)`
where `X` stands in for θ), and **Seq** (sequences `u`/`v`/`w`, with
`Ans` as the previous term for recursion). See
[The MODE Menu](#the-mode-menu).

Press `Y=` to return to the keypad.

## Programming (TI-BASIC)

Tux-TI83 includes a **TI-BASIC** interpreter — you can write, save, and run
programs on the calculator, just like the real thing. Programs run on the
"home screen" (a run/output view) and can also drive the graph.

### Opening the program manager

Press **2ND + √(** — the `√(` key's second function is **PRGM** — to open the
program manager. From there you can **RUN** a program, **EDIT** its source,
delete it (**✕**), or create one with **NEW** (name: A–Z / 0–9, up to 8
characters). Programs persist across restarts, saved with the rest of the
calculator's state.

### The editor

A program is plain source text — **one statement per line**, or several on a
line separated by a colon `:`. The store arrow `→` is entered with the **STO▸**
key or by typing `->`.

Rather than type keywords by hand, tap **⌨ COMMANDS** to open a palette with
four tabs — **CTL** (control flow), **I/O** (input/output), **STR** (strings),
**FN** (functions and `→`) — and click any keyword to insert it at the cursor.
Press **SAVE** to store, **CANCEL** to discard.

### Running a program

**RUN** opens the run view — the program's output screen. Depending on what it
does you may see:

- **output** from `Disp` / `Output(` (scrolling, monospace);
- an **input box** at `Input` / `Prompt` (type a value, press ENTER);
- a **▶ CONTINUE** button at `Pause`;
- a **menu** of options at `Menu(`;
- a **■ STOP** button while it runs — press it to interrupt a program (e.g. a
  runaway loop); the program halts with `ERR:BREAK`.

Other buttons: **COPY** copies the output to the clipboard, **◀ PRGM** returns
to the manager, and after an error **✎ EDIT LINE n** jumps into the editor at
the offending line.

### Command reference

**Display & output**

| Command | Does |
|---|---|
| `Disp value[,value…]` | Print each value on its own line (numbers or strings) |
| `Output(row,col,value)` | Print `value` at a fixed row (1–8), column (1–16) |
| `ClrHome` | Clear the output screen |

**Input & interaction**

| Command | Does |
|---|---|
| `Input VAR` / `Input "prompt",VAR` | Pause for a value into `VAR` (number, or raw text into a `StrN`) |
| `Prompt VAR` | Like `Input` with an automatic `VAR=?` prompt |
| `Pause` | Wait for CONTINUE |
| `getKey` | Code of the key pressed since the last poll, or `0` (non-blocking). Arrows = 24/25/26/34 (←↑→↓), `ENTER`=105, `CLEAR`=45, `DEL`=23, digits 0–9 |
| `Menu("title","opt",Lbl,…)` | Show a menu; the chosen option jumps to its `Lbl` |

**Control flow**

| Command | Does |
|---|---|
| `If cond` | Run the next statement only if `cond` is true |
| `If cond` / `Then` … `Else` … `End` | Block form with an optional `Else` |
| `For(VAR,start,end[,step])` … `End` | Counted loop |
| `While cond` … `End` | Loop while `cond` holds |
| `Repeat cond` … `End` | Loop until `cond` becomes true (body always runs once) |
| `Lbl name` / `Goto name` | Label and jump |
| `Stop` | End the program |

**Program control**

| Command | Does |
|---|---|
| `prgmNAME` | Run another program as a sub-routine, then continue |
| `Return` | Return from a sub-program (ends the run in the main program) |
| `DelVar VAR` | Reset a scalar to 0 / clear a `StrN` |

**Strings**

`Str1`–`Str9` hold text; `"…"` are string literals; `+` concatenates.

| Function | Returns |
|---|---|
| `length(str)` | Character count |
| `sub(str,begin,count)` | Substring (1-based) |
| `inString(str,sub[,start])` | 1-based position, or 0 if not found |
| `expr(str)` | The string evaluated as an expression |

**Graphing** (see also [Graph Mode](#graph-mode))

| Command | Does |
|---|---|
| `"X²"→Y1` (or bare `X²→Y1`) | Store a function into a `Y=` slot |
| `n→Xmin` / `Xmax` / `Ymin` / `Ymax` / `Xscl` / `Yscl` | Set a window variable |
| `FnOn [n]` / `FnOff [n]` | Enable / disable a function (all if no number) |
| `ZStandard` / `ZoomFit` | Reset / fit the window |
| `DispGraph` | Show the graph |
| `ClrDraw` | Clear all drawings |
| `Line(x1,y1,x2,y2)` | Draw a line (graph coordinates) |
| `Circle(x,y,r)` | Draw a circle |
| `Horizontal y` / `Vertical x` | Draw a full-width / full-height line |
| `Pt-On(x,y)` | Draw a point |
| `Text(x,y,value)` | Draw text at a graph position |

Any graphics command switches to the graph automatically. When drawings are
present, a **✕ CLR** button appears at the top-right of the graph to clear them
all in one tap (the DRAW menu, **2ND+TRACE**, can delete individual drawings).

### Examples

A greeting that asks for your name:

```
Input "NAME?",Str1
Disp "HELLO "+Str1
```

Sum the numbers 1 to N:

```
Prompt N
0→S
For(I,1,N)
S+I→S
End
Disp S
```

Set up and show a parabola with a circle drawn on it:

```
"X²"→Y1
-6→Xmin
6→Xmax
ClrDraw
Circle(0,4,3)
DispGraph
```

### Notes

- **Interrupting:** press **■ STOP** during a run to stop a program (a
  runaway loop is stopped this way) — it ends with `ERR:BREAK`.
- **Errors** report the source line and offer **✎ EDIT LINE n** to jump there.
- Assignments (`5→A`) are silent, as on the TI-83 — use `Disp` to show a value.
- The math engine is shared with the home screen, so every function you can
  type there works in a program too.

## The MODE Menu

The `MODE` key (CONTROL section) opens the settings popup. Wired rows take
effect immediately and are reflected in the header indicator:

| Row | Options | Effect |
|---|---|---|
| **Angle** | Radian / Degree | How trig functions interpret their input |
| **Notation** | Normal / Sci / Eng | Number display format |
| **Decimal** | Float / Fix 0–9 | Fixed decimal places |
| **Base** | Dec / Hex / Oct / Bin | Integer results in the chosen base (`0xFF`, `0o77`, `0b1010`); non-integers fall back to decimal |
| **Graph** | Func / Par / Pol / Seq | Cartesian, parametric, polar, or sequence graphing |
| **Draw** | Connected / Dot | How graph curves are drawn |
| **Plot** | Sequential / Simul | Draw-animation order — one curve fully, or all curves together |
| **Complex** | Real / a+bi / re^θi | Whether non-real results are allowed, and how they display |
| **Screen** | Full / Horiz / G-T | Single view, graph-over-keypad, or graph-beside-table |
| **Theme** | Dark / Light / Amber | App-wide UI theme (see below) |

Every row is wired and takes effect immediately. `RESET` (in this popup)
restores factory defaults and clears all saved state.

### Themes

Three UI themes, chosen from **MODE → Theme** and remembered across
launches:

- **Dark** — the default (Nord-ish dark).
- **Light** — a light calculator body with dark text.
- **Amber** — an orange-on-black terminal look (outlined keys, amber text).

The LCD panel stays a dark "screen" in every theme (authentic to a real
calculator); its readout text is themed to match.

## Keyboard Shortcuts

```
0–9, .                  digit entry
A–Z (Shift+letter)      scalar variables
+ − * /                 operators (converted to Unicode for display)
^                       power
( ) ,                   parens and argument separator
[ ]                     matrix-literal brackets
{ }                     list-literal braces
!                       factorial (postfix)
i                       imaginary unit
Enter / =               evaluate
Backspace               delete last token
Escape                  CLEAR
← → Home End            move the in-expression cursor
s / c / t               sin( / cos( / tan(
l / n / r               log( / ln( / √(
p                       π
Y then digit            Y-VARS token (Y 1 → Y1)
|  (or ->)              STO▸
```

Bare lowercase letters are function shortcuts (`s` → `sin(`); press
**Shift+letter** for a scalar variable (`A`–`Z`). Type the letter `Y`
followed by a digit for a Y-VARS token (`Y` `1` → `Y1`). On the on-screen
keypad, `2ND` reaches the amber labels — `{` / `}` (2ND+`(` / `)`) and
`L1`–`L6` (2ND+`1`–`6`) — and `ALPHA` + `(` / `)` gives `[` / `]`.

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
$ ./build/tux_ti83_cli "det([[1,2][3,4]])"
-2
```

Typed matrix literals work in the CLI/REPL too, so matrix math is fully
available there (`[[1,2][3,4]]^-1`, `[[1,2][3,4]]*[[5,6][7,8]]`, …).
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

## Complex Numbers

Set **MODE → Complex** to `a+bi` (rectangular) or `re^θi` (polar) to
allow non-real results; the default `Real` rejects them with
`ERR:NONREAL ANS`. The imaginary unit is `i` (keyboard `i`).

```
i^2                 → -1
(2+3i)+(1-i)        → 3+2i
(2+3i)(2-3i)        → 13
√(-4)               → 2i        (in a+bi mode; ERR:NONREAL ANS in Real)
```

Complex-aware functions include `+ − × ÷ ^`, `√(`, `ln(`, `e^(`, the trig
functions, plus `conj(`, `real(`, `imag(`, and `angle(`. The `re^θi`
mode displays results in polar form; the header shows the active complex
mode.

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
| `ERR:UNDEFINED` | Referenced a matrix or list before storing values into it; or a program called a `prgmNAME` that doesn't exist |
| `ERR:SINGULAR MAT` | Tried to invert a singular (non-invertible) matrix |
| `ERR:RECURSION` | A Y-VAR references itself directly or through a cycle |
| `ERR:BREAK` | A running program was interrupted (**■ STOP**), or a runaway loop hit the step limit |
| `ERR:LABEL` | A program's `Goto` / `Menu(` target `Lbl` doesn't exist |
| `ERR:ARGUMENT` | A program command was given the wrong number of arguments |
| `ERR:MEMORY` | Program sub-calls (`prgmNAME`) nested too deep (runaway recursion) |

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

# Tux-TI83 🐧🔢

A Linux-native graphing calculator reimagining the TI-83 Plus, built with
**C++20**, **Qt 6.5+/QML**, and a decoupled component architecture.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)

## Overview

Tux-TI83 is a desktop calculator that mirrors the TI-83 Plus's math
engine, expression parser, matrix and list algebra, statistics, and
function graphing while giving them a modern Qt6/QML front-end. It's
structured as three decoupled modules plus four binaries:

| Module | Purpose |
|---|---|
| [`core_math/`](core_math/) | Pure C++20 math engine — recursive-descent parser, shunting-yard evaluator, matrix/list algebra, deferred-evaluation framework (calculus + `seq`), continued-fraction rational simplification. No Qt dependencies. |
| [`graph_ui/`](graph_ui/) | Qt-based controller + graph rendering. Exposes a `UIController` QObject bridge that maintains calculator state, drives the display state machine, manages Y1/Y2/Y3 buffers, persistence, and the statistics/regression computations. |
| [`app/`](app/) | QML application shell, component library (keys, display, popups, canvas), Nord-derived theme, keyboard shortcut handler. |

Four executables come out of the build:

| Binary | What it is |
|---|---|
| `tux_ti83` | The GUI calculator. `./build/tux_ti83` launches it. |
| `tux_ti83_cli` | One-shot CLI: `tux_ti83_cli "2+2"` prints `4` and exits. |
| `tux_ti83_repl` | Interactive REPL — prompt-per-line, `Ans` recall, `:quit` to exit. |
| `tux_ti83_tests` | Regression test suite (**899** assertions, also wired into CTest). |

## Features

### Calculator math
- Arithmetic: `+ − × ÷ ^`, unary negation, parentheses, π, e
- Implicit multiplication by juxtaposition — `2π`, `2(3)`, `5X`, `2sin(0)`
- Powers & roots: `^`, `x²`, `√(`, nth-root `ˣ√` (2ND+^), `e^(`
- Trig: `sin`, `cos`, `tan` and inverses `asin`, `acos`, `atan` (Radian/Degree aware)
- Hyperbolic: `sinh`, `cosh`, `tanh` and inverses `asinh`, `acosh`, `atanh`
- Logarithms: `log` (base 10), `ln`
- Number functions: `abs`, `int` (floor), `iPart`, `fPart`, `round(x, n)`, `sgn`
- `min(a, b)`, `max(a, b)`, `mod(a, b)`
- Combinatorics: `nCr(n, r)`, `nPr(n, r)`, `!` (factorial)
- `Ans` recall, last-entry recall (2ND+ENTER), `▶Frac` / `▶Dec` conversions
- `:` statement separator (`5→A:A+1`)
- Base display modes: DEC / HEX / OCT / BIN (MODE menu)
- TI-83-style error labels: `ERR:DIVIDE BY 0`, `ERR:NONREAL ANS`, `ERR:DOMAIN`, `ERR:INVALID DIM`, `ERR:DATA TYPE`, `ERR:UNDEFINED`, `ERR:SYNTAX`, `ERR:RECURSION`, `ERR:SINGULAR MAT`

### Variables & storage
- 26 scalar variables `A`–`Z`, store with `STO▸` (`5→A`)
- `Ans` auto-populated after every successful ENTER
- `Y-VARS` recall — reference `Y1`/`Y2`/`Y3` from another expression, with
  cycle detection; explicit-argument form `Y1(3)`
- **Persistent state across runs** — scalars, matrices, lists, Y= buffers,
  viewport, and MODE settings saved to `~/.local/state/tux-ti83/state.json`
  (periodic + on clean exit) and reloaded on launch
- Factory **RESET** (MODE popup) clears all state

### Calculus
- Numeric integration `fnInt(expr, var, a, b)` — composite Simpson's rule
- Numeric derivative `nDeriv(expr, var, x[, h])` — symmetric difference
- `sum(expr, var, start, end)` / `prod(expr, var, start, end)`
- `seq(expr, var, start, end[, step])` — generates a list; enables the
  authentic `sum(seq(...))` summation form
- Nested calls supported (e.g. `fnInt(fnInt(1, Y, 0, X), X, 0, 1)`)

### Lists & statistics
- Lists `L1`–`L6`, `{1,2,3}` literals, `STO▸` to a list
- Element-wise arithmetic `+ − × ÷ ^` with scalar broadcasting
- **Stat list editor** — selector, variable length, value read-back
- List functions: `sum(`, `prod(`, `mean(`, `median(`, `min(`, `max(`,
  `stdDev(`, `variance(`
- **1-Var Stats** — n, x̄, Σx, Σx², Sx, σx, minX, Q1, Med, Q3, maxX
- **2-Var Stats + LinReg** — correlation `r`, `r²`, slope, intercept
- **Regressions** — `QuadReg`, `CubicReg`, `ExpReg`, `LnReg`, `PwrReg`
  (least-squares, with `R²`)

### Graphing
- Simultaneous plot of up to 3 functions (Y1, Y2, Y3)
- **Polar mode** (`r = f(θ)`) — MODE → Graph → Pol
- Pan (click-drag) and zoom (scroll-wheel)
- WINDOW popup (Xmin/Xmax/Ymin/Ymax), ZStandard / ZoomFit / Zoom In-Out /
  ZSquare / ZTrig / ZDecimal / ZInteger presets (ZOOM popup)
- **TRACE** — movable crosshair with X/Y readout, curve-cycling
- **TABLE** / **TBLSET** — tabular X | Y1 | Y2 | Y3 view (2ND+GRAPH)
- Connected / Dot draw modes
- Dynamic gridlines and axis labels scaled to the current viewport

### Matrices
- `[A]`–`[E]` exposed in the UI (engine supports `[A]`–`[J]`)
- **Matrix editor v2** — matrix selector, variable dimensions (up to 6×6),
  reads existing values back for editing
- Add, subtract, scalar-multiply, matrix-multiply
- Determinant `det(`, transpose `T(`, inverse `^-1` (Gauss-Jordan),
  reduced row-echelon `rref(`
- Dimension-mismatch errors surface cleanly (`ERR:INVALID DIM`)

### Modes & menus
- **MODE** menu — Angle (Radian/Degree), Notation (Normal/Sci/Eng),
  Decimal (Float/Fix N), Base (Dec/Hex/Oct/Bin), Graph (Func/Pol), Draw
  (Connected/Dot); live header indicator
- **2ND** / **ALPHA** modifier systems with on-key sub-labels, one-shot and
  ALPHA-lock, insert/overwrite mode, in-expression cursor editing
- **MATH** menu, **MATRX** editor, **STAT** list editor (2ND+MATRX),
  **CATALOG** browser (2ND+0), logic/TEST menu (2ND+MATH)

### Logic & boolean
- Relational: `=`, `≠`, `<`, `>`, `≤`, `≥`
- Boolean: `and`, `or`, `xor`, `not`

### CLI & scripting
- Tokeniser accepts both Unicode (`×`, `÷`, `−`) and ASCII (`*`, `/`, `-`) operators
- Context-aware `-` disambiguation — unary negation vs binary subtraction
- ANSI-coloured output (green result, red error, blue prompt) when stdout is a tty; plain output when piped

## Build

### Run without building — AppImage

The quickest way to run the GUI on any modern x86-64 Linux desktop, with no
system Qt install required:

```bash
chmod +x Tux-TI83-x86_64.AppImage
./Tux-TI83-x86_64.AppImage
```

Build the AppImage yourself with `./packaging/build-appimage.sh` (needs
`linuxdeploy`, `linuxdeploy-plugin-qt`, and `appimagetool` on `PATH`); see
[`packaging/README.md`](packaging/README.md). To build from source instead:

### Prerequisites

- **Compiler:** GCC 11+ or Clang 14+
- **Framework:** Qt 6.5+ (Core, Gui, Qml, Quick, QuickControls2)
- **Build system:** CMake 3.21+

On Mint / Ubuntu, the full Qt6 runtime module set needed is:

```bash
sudo apt install qt6-base-dev qt6-declarative-dev cmake build-essential \
                 qml6-module-qtquick qml6-module-qtquick-controls \
                 qml6-module-qtquick-layouts qml6-module-qtquick-window \
                 qml6-module-qtquick-templates qml6-module-qtqml-workerscript
```

### Commands

```bash
git clone https://github.com/Darian-Frey/Tux-TI83.git
cd Tux-TI83
chmod +x build.sh
./build.sh             # builds all four binaries and launches the GUI
```

After a successful build:

```bash
./build/tux_ti83             # GUI calculator
./build/tux_ti83_cli "2+2"   # one-shot CLI
./build/tux_ti83_repl        # interactive REPL
./build/tux_ti83_tests       # regression suite
```

**Shell note:** wrap CLI expressions containing `!` in **single quotes** to
defeat bash's history expansion:
```bash
./build/tux_ti83_cli '5!+3!'      # correct → 126
./build/tux_ti83_cli "5!+3!"      # bash: event not found
```
The REPL bypasses bash parsing entirely — just type `5!+3!` at the prompt.
Note the CLI/REPL run in separate processes with fresh state, so matrices
and lists set in one invocation don't persist to the next.

### Running the tests

```bash
./build/tux_ti83_tests            # plain execution
cd build && ctest                 # via CTest (also registered)
```

## Keyboard shortcuts (GUI)

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

2ND combinations mirror the TI-83 keytops — e.g. **2ND+( / )** → `{` / `}`,
**2ND+1…6** → `L1…L6`, **2ND+MATRX** → the Stat list editor, **2ND+GRAPH**
→ TABLE. Function names without a shortcut (`abs`, `round`, `mod`,
hyperbolics, `nCr`, calculus, list functions, …) live in the **MATH** menu.

See [USER_MANUAL.md](USER_MANUAL.md) for the full keypad and menu guide.

## Design language

The UI uses a Nord-derived palette with a semantic role overlay; all
colours live in [app/qml/Style.qml](app/qml/Style.qml) as a `QtObject`
singleton:

| Role | Colour |
|---|---|
| Calculator shell | `#1e2030` |
| LCD panel | `#0b1120` (permanent — always dark) |
| Key surface | `#252840` |
| Section divider / hairline | `#1a1c2e` |
| Primary text | `#e2e8f0` |
| Result text (green) | `#4ade80` |
| Expression history (blue) | `#3b82f6` |
| Error text (red) | `#f87171` |
| Operator keys | bg `#163354` / border `#1e3a5f` |
| ENTER | bg `#0f3d20` / border `#14532d` |
| 2ND | bg `#4c1d02` / border `#78350f` |

## Project documentation

| File | Purpose |
|---|---|
| [USER_MANUAL.md](USER_MANUAL.md) | End-user manual — keypad layout, menus, graphing, lists & stats, CLI/REPL usage, error messages. |
| [ROADMAP.md](ROADMAP.md) | Features and capabilities — done, in-flight, and planned, organised by area |
| [BUGS.md](BUGS.md) | Known bugs and fix history |
| [IMPROVEMENTS.md](IMPROVEMENTS.md) | Code-quality improvements (applied + suggested) |
| [CLAUDE.md](CLAUDE.md) | Session context and workflow rules for AI agents helping with development |

## License

MIT.

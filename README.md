# Tux-TI83 🐧🔢

A Linux-native graphing calculator reimagining the TI-83 Plus, built with
**C++20**, **Qt 6.5+/QML**, and a decoupled component architecture.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)

## Overview

Tux-TI83 is a desktop calculator that mirrors the TI-83 Plus's math
engine, expression parser, matrix algebra, and function graphing while
giving them a modern Qt6/QML front-end. It's structured as three
decoupled modules plus four binaries:

| Module | Purpose |
|---|---|
| [`core_math/`](core_math/) | Pure C++20 math engine — recursive-descent parser, shunting-yard evaluator, matrix ops, continued-fraction rational simplification. No Qt dependencies. |
| [`graph_ui/`](graph_ui/) | Qt-based controller + graph rendering. Exposes a `UIController` QObject bridge that maintains calculator state, drives the display state machine, and manages Y1/Y2/Y3 function buffers. |
| [`app/`](app/) | QML application shell, component library (keys, display, popups, canvas), Nord-derived theme, keyboard shortcut handler. |

Four executables come out of the build:

| Binary | What it is |
|---|---|
| `tux_ti83` | The GUI calculator. `./build/tux_ti83` launches it. |
| `tux_ti83_cli` | One-shot CLI: `tux_ti83_cli "2+2"` prints `4` and exits. |
| `tux_ti83_repl` | Interactive REPL — prompt-per-line, `Ans` recall, `:quit` to exit. |
| `tux_ti83_tests` | Regression test suite (currently **113** assertions). |

## Features

### Calculator math
- Arithmetic: `+ − × ÷ ^`, unary negation, parentheses, π, e
- Trig: `sin`, `cos`, `tan` and inverses `asin`, `acos`, `atan`
- Hyperbolic: `sinh`, `cosh`, `tanh` and inverses `asinh`, `acosh`, `atanh`
- Logarithms: `log` (base 10), `ln`
- Square root `√` with domain-error propagation
- Number functions: `abs`, `int` (floor), `iPart` (truncation toward zero), `fPart`, `round(x, n)`
- `min(a, b)`, `max(a, b)`, `mod(a, b)`
- Combinatorics: `nCr(n, r)`, `nPr(n, r)`, `!` (factorial)
- `Ans` recall — auto-populated from every successful ENTER
- `▶Frac` / `▶Dec` post-hoc conversions between fraction and decimal representations
- TI-83-style error labels: `ERR:DIVIDE BY 0`, `ERR:NONREAL ANS`, `ERR:DOMAIN`, `ERR:INVALID DIM`, `ERR:DATA TYPE`, `ERR:UNDEFINED`, `ERR:SYNTAX`

### Graphing
- Simultaneous plot of up to 3 functions (Y1, Y2, Y3)
- Pan (click-drag) and zoom (scroll-wheel) on the canvas
- Viewport editor (WINDOW popup) for Xmin/Xmax/Ymin/Ymax
- ZStandard and ZoomFit one-click actions
- Dynamic gridlines and axis labels scaled to the current viewport

### Matrices
- Add, subtract, scalar-multiply, matrix-multiply
- Registry for [A], [B], [C] (engine supports [A]–[J])
- 3×3 grid editor with NAMES / MATH / EDIT tabs
- Determinant (`det`) via recursive Laplace expansion
- Dimension-mismatch errors surface cleanly (`ERR:INVALID DIM`)

### Logic & boolean
- Relational: `=`, `≠`, `<`, `>`, `≤`, `≥`
- Boolean: `and`, `or`, `xor`, `not`

### CLI & scripting
- Tokeniser accepts both Unicode (`×`, `÷`, `−`) and ASCII (`*`, `/`, `-`) operators
- Context-aware `-` disambiguation — unary negation vs binary subtraction
- ANSI-coloured output (green result, red error, blue prompt) when stdout is a tty; plain output when piped

## Build

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

### Running the tests

```bash
./build/tux_ti83_tests            # plain execution
cd build && ctest                 # via CTest (also registered)
```

## Keyboard shortcuts (GUI)

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

Function names without a shortcut key (`abs`, `round`, `min`/`max`, `mod`,
hyperbolics, `nCr`, `nPr`, `▶Frac`, `▶Dec`, …) are available from the
**MATH** menu.

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
| [USER_MANUAL.md](USER_MANUAL.md) | End-user manual — keypad layout, MATH/MATRX/WINDOW menus, graph mode, CLI/REPL usage, error messages. Currently a skeleton; sections flagged *"planned"* will be fleshed out over time. |
| [ROADMAP.md](ROADMAP.md) | Features and capabilities — what to build, organised by area |
| [BUGS.md](BUGS.md) | Known bugs and fix history |
| [IMPROVEMENTS.md](IMPROVEMENTS.md) | Code-quality improvements (applied + suggested) |
| [CLAUDE.md](CLAUDE.md) | Session context and workflow rules for AI agents helping with development |

## License

MIT.

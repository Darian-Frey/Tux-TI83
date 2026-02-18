# Tux-TI83 🐧🔢

A high-performance, graph-addressable TI-83 emulator/calculator hybrid built with **C++20**, **Qt6/QML**, and the **SCHEMA_V5** architectural protocol.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Protocol](https://img.shields.io/badge/protocol-SCHEMA__V5-blueviolet)

## 🚀 Overview

Tux-TI83 is designed to bridge the gap between traditional graphing calculators and modern computing power. It treats mathematical state as a graph-addressable machine, allowing for complex matrix operations, real-time function graphing, and modular logic evaluation.

### Core Architecture
* **Engine:** Custom Recursive Descent Parser / Shunting-Yard Evaluator.
* **Protocol:** SCHEMA_V5 (State Machine Logic + Capsule-Based Memory).
* **UI:** Nord-themed QML interface with high-density information transfer.
* **Math:** Polymorphic stack supporting both Scalars and Matrices ([A] through [E]).

---

## ✨ Key Features

### 📊 Graphing Engine
* **Multi-Function Plotting:** Graph up to 3 functions simultaneously (Y1, Y2, Y3).
* **Interactive Viewport:** Real-time Pan (Click-Drag) and Zoom (Scroll-Wheel).
* **Z-Logic:** Quick-access ZStandard and Zoom Fit settings.
* **Coordinate Mapping:** High-contrast grid with dynamic axis labeling.

### 🧮 Matrix Processing
* **Operations:** Supports Matrix Addition, Scalar Multiplication, and Matrix-Matrix Multiplication.
* **Grid Editor:** Interactive 3x3 UI for defining matrix data.
* **Registry:** Persistently store and recall matrices [A], [B], and [C].

### 🛠 Logic & Boolean
* **Relational:** =, ≠, <, >, <=, >=.
* **Boolean:** and, or, not, xor.
* **Fractional:** ▶Frac conversion for scalar results.

---

## 🛠 Installation & Build

### Prerequisites
* **Compiler:** GCC 11+ or Clang 14+
* **Framework:** Qt 6.5+ (Core, Gui, Qml, Quick, QuickControls2)
* **Build System:** CMake 3.16+

### Commands

# Clone the repository
git clone https://github.com/yourusername/Tux-TI83.git
cd Tux-TI83

# Run the build script
chmod +x build.sh
./build.sh

---

## 🎨 Design Language
The project uses the **Nord Palette** for reduced eye strain and high readability:
* **Polar Night:** #2E3440 (Primary Background)
* **Snow Storm:** #ECEFF4 (Primary Text)
* **Frost Blue:** #88C0D0 (Selection/Active)
* **Aurora Green:** #A3BE8C (Success/Save)
* **Aurora Red:** #BF616A (Deletes/Errors)

---

## 📅 Roadmap
- [x] Matrix Engine Integration
- [x] Interactive Graphing Viewport
- [ ] **Next:** Modular Component Refactoring
- [ ] Determinant & Transpose Logic
- [ ] List Processing (L1 - L6)
- [ ] Program Scripting Mode

---

## 📜 License
This project is licensed under the MIT License.

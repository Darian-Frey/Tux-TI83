// Shared helpers for the Tux-TI83 command-line binaries.
//
// Both `tux_ti83_cli` (one-shot) and `tux_ti83_repl` (interactive)
// drive the same UIController and want identical result formatting,
// error reporting, and tty-aware ANSI colouring. This header centralises
// that so the two entry points stay in sync.
//
// Header-only; each helper is `inline` so both TUs can include without
// ODR violations.

#pragma once

#include "ui_controller.hpp"
#include <QString>
#include <iostream>
#include <string>
#include <unistd.h>

namespace tux_ti83 {
namespace cli {

// ANSI SGR escape sequences. Only emitted when stdout is a tty.
constexpr const char *kReset = "\033[0m";
constexpr const char *kRed   = "\033[31m";
constexpr const char *kGreen = "\033[32m";
constexpr const char *kBlue  = "\033[34m";

inline bool isTty() { return ::isatty(STDOUT_FILENO) != 0; }

inline void printResult(const UIController &c) {
  bool isError = (c.displayState() == UIController::Error);
  const char *colour = isError ? kRed : kGreen;
  bool colorize = isTty();
  if (colorize) std::cout << colour;
  std::cout << c.currentDisplay().toStdString();
  if (colorize) std::cout << kReset;
  std::cout << '\n';
}

// Evaluate one line through the controller. Returns true on success,
// false on tokenisation failure or evaluation error. Prints the result
// (or ERR:...) to stdout, optionally coloured.
inline bool evaluateLine(UIController &c, const QString &line) {
  c.processInput(QStringLiteral("CLEAR")); // fresh expression buffer
  if (!c.processExpression(line)) {
    bool colorize = isTty();
    if (colorize) std::cout << kRed;
    std::cout << "ERR:TOKENISE";
    if (colorize) std::cout << kReset;
    std::cout << '\n';
    return false;
  }
  c.processInput(QStringLiteral("ENTER"));
  printResult(c);
  return c.displayState() != UIController::Error;
}

} // namespace cli
} // namespace tux_ti83

// Tux-TI83 command-line interface.
//
// Two modes:
//   tux_ti83_cli "2+2"          one-shot: evaluate, print result, exit
//   tux_ti83_cli                REPL: prompt-per-line, Ctrl+D / :quit to exit
//
// Both modes drive the exact same UIController used by the GUI (via
// processExpression + processInput("ENTER")), so behaviour is identical
// to clicking the keys in the desktop app — same parser, same evaluator,
// same state machine, same Ans recall between lines.
//
// Output is colourised when stdout is a tty (green for results, red for
// errors, blue for the REPL prompt). When piped or redirected, output is
// plain so it composes cleanly with other shell tools.

#include "ui_controller.hpp"
#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <iostream>
#include <string>
#include <unistd.h>

using tux_ti83::UIController;

namespace {

// ANSI SGR escape sequences. Only emitted when stdout is a tty.
constexpr const char *kReset = "\033[0m";
constexpr const char *kRed   = "\033[31m";
constexpr const char *kGreen = "\033[32m";
constexpr const char *kBlue  = "\033[34m";

bool gIsTty = false;

void printResult(const UIController &c) {
  bool isError = (c.displayState() == UIController::Error);
  const char *colour = isError ? kRed : kGreen;
  if (gIsTty)
    std::cout << colour;
  std::cout << c.currentDisplay().toStdString();
  if (gIsTty)
    std::cout << kReset;
  std::cout << '\n';
}

// Evaluate one line through the controller. Returns the controller's
// success/failure so REPL mode can know whether to keep going.
bool evaluateLine(UIController &c, const QString &line) {
  c.processInput(QStringLiteral("C")); // fresh expression buffer
  if (!c.processExpression(line)) {
    if (gIsTty)
      std::cout << kRed;
    std::cout << "ERR:TOKENISE";
    if (gIsTty)
      std::cout << kReset;
    std::cout << '\n';
    return false;
  }
  c.processInput(QStringLiteral("ENTER"));
  printResult(c);
  return c.displayState() != UIController::Error;
}

int runOneShot(UIController &c, int argc, char *argv[]) {
  QString expr;
  for (int i = 1; i < argc; ++i) {
    if (i > 1)
      expr += ' ';
    expr += QString::fromUtf8(argv[i]);
  }
  return evaluateLine(c, expr) ? 0 : 1;
}

int runRepl(UIController &c) {
  if (gIsTty) {
    std::cout << "Tux-TI83 CLI — type :quit (or Ctrl+D) to exit, "
              << "Ans recalls the last result.\n";
  }
  std::string line;
  while (true) {
    if (gIsTty)
      std::cout << kBlue << "> " << kReset << std::flush;
    if (!std::getline(std::cin, line))
      break; // EOF
    if (line == ":quit" || line == ":q")
      break;
    if (line.empty())
      continue;
    evaluateLine(c, QString::fromUtf8(line));
  }
  if (gIsTty)
    std::cout << '\n';
  return 0;
}

} // namespace

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  gIsTty = isatty(STDOUT_FILENO);
  UIController c;
  return (argc > 1) ? runOneShot(c, argc, argv) : runRepl(c);
}

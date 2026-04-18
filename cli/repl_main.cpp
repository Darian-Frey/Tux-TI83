// Tux-TI83 interactive REPL.
//
//   tux_ti83_repl          prompt-per-line, Ctrl+D / :quit to exit
//
// Reads expressions from stdin one line at a time. Because stdin
// bypasses bash's arg-parsing, there's no history-expansion gotcha —
// type `!` freely. `Ans` recalls the previous result across lines.
//
// For shell-scripted one-shot evaluation, use `tux_ti83_cli` instead.

#include "cli_common.hpp"
#include <QCoreApplication>
#include <QString>

using tux_ti83::UIController;
using tux_ti83::cli::evaluateLine;
using tux_ti83::cli::isTty;
using tux_ti83::cli::kBlue;
using tux_ti83::cli::kReset;

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  UIController c;

  if (isTty()) {
    std::cout << "Tux-TI83 REPL — type :quit (or Ctrl+D) to exit,\n"
              << "                Ans recalls the last result.\n";
  }

  std::string line;
  while (true) {
    if (isTty())
      std::cout << kBlue << "> " << kReset << std::flush;
    if (!std::getline(std::cin, line))
      break; // EOF
    if (line == ":quit" || line == ":q")
      break;
    if (line.empty())
      continue;
    evaluateLine(c, QString::fromUtf8(line));
  }
  if (isTty())
    std::cout << '\n';
  return 0;
}

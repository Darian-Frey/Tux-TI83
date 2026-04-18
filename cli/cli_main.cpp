// Tux-TI83 one-shot command-line calculator.
//
//   tux_ti83_cli EXPRESSION        evaluate EXPRESSION, print result, exit
//
// For interactive use (prompt, Ans recall between lines, no shell-
// expansion gotchas), run `tux_ti83_repl` instead.
//
// Shell note: bash's history expansion bites on `!` even inside
// double quotes, so wrap expressions containing `!` in single quotes:
//
//   tux_ti83_cli '5!+3!'            # correct
//   tux_ti83_cli "5!+3!"            # bash: event not found
//
// Exit codes: 0 success, 1 evaluation error, 2 usage error.

#include "cli_common.hpp"
#include <QCoreApplication>
#include <QString>

using tux_ti83::UIController;
using tux_ti83::cli::evaluateLine;

namespace {

void printUsage() {
  std::cerr
      << "usage: tux_ti83_cli EXPRESSION\n"
      << "\n"
      << "  Evaluate EXPRESSION through the Tux-TI83 math engine and\n"
      << "  print the result to stdout.\n"
      << "\n"
      << "  Examples:\n"
      << "    tux_ti83_cli \"2+2\"\n"
      << "    tux_ti83_cli \"sin(0)\"\n"
      << "    tux_ti83_cli \"nCr(52, 5)\"\n"
      << "\n"
      << "  Wrap expressions containing `!` in SINGLE quotes — bash's\n"
      << "  history expansion fires even inside double quotes:\n"
      << "    tux_ti83_cli '5!+3!'\n"
      << "\n"
      << "  For an interactive prompt with Ans recall between lines,\n"
      << "  use `tux_ti83_repl` instead (stdin bypasses shell expansion).\n";
}

} // namespace

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  if (argc < 2) {
    printUsage();
    return 2;
  }
  UIController c;
  QString expr;
  for (int i = 1; i < argc; ++i) {
    if (i > 1) expr += ' ';
    expr += QString::fromUtf8(argv[i]);
  }
  return evaluateLine(c, expr) ? 0 : 1;
}

// Tux-TI83 math regression tests.
//
// Drives UIController programmatically — same code path as the GUI and
// the CLI. Each test feeds an expression string, presses ENTER, and
// asserts on the resulting display string. Plain assertions; no
// external test framework. Exit code: 0 on full pass, 1 on any failure.

#include "ui_controller.hpp"
#include <QCoreApplication>
#include <QString>
#include <iostream>
#include <string>

using tux_ti83::UIController;

namespace {

int gPassed = 0;
int gFailed = 0;
std::string gCurrentSection;

void section(const std::string &name) {
  gCurrentSection = name;
  std::cout << "\n=== " << name << " ===\n";
}

void check(const std::string &description, const QString &actual,
           const QString &expected) {
  bool ok = (actual == expected);
  if (ok) {
    ++gPassed;
    std::cout << "  PASS  " << description << '\n';
  } else {
    ++gFailed;
    std::cout << "  FAIL  " << description << '\n'
              << "        expected: " << expected.toStdString() << '\n'
              << "        actual:   " << actual.toStdString() << '\n';
  }
}

// Boolean predicate variant — for assertions that don't reduce to a
// QString equality (e.g. "result starts with [[", "result is a matrix").
void checkTrue(const std::string &description, bool condition) {
  if (condition) {
    ++gPassed;
    std::cout << "  PASS  " << description << '\n';
  } else {
    ++gFailed;
    std::cout << "  FAIL  " << description << '\n';
  }
}

// Reset + evaluate. Does NOT reset Ans/lastResult — tests that depend
// on Ans-not-being-set should be the first in their section, or use
// a fresh controller.
QString eval(UIController &c, const QString &expr) {
  c.processInput(QStringLiteral("C"));
  c.processExpression(expr);
  c.processInput(QStringLiteral("ENTER"));
  return c.currentDisplay();
}

// Evaluate without resetting first — used for chained tests where
// the previous result feeds the next via Ans.
QString evalChained(UIController &c, const QString &expr) {
  c.processExpression(expr);
  c.processInput(QStringLiteral("ENTER"));
  return c.currentDisplay();
}

} // namespace

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  UIController c;

  section("Basic arithmetic");
  check("2+2", eval(c, "2+2"), "4");
  check("3-7", eval(c, "3-7"), "-4");
  check("3×4", eval(c, "3×4"), "12");
  check("10÷4", eval(c, "10÷4"), "2.5");
  check("2^10", eval(c, "2^10"), "1024");
  check("Order of operations: 2+3*4", eval(c, "2+3*4"), "14");
  check("Parens override: (2+3)*4", eval(c, "(2+3)*4"), "20");

  section("Constants");
  check("π evaluates to ≈3.14159", eval(c, "π"), QString::number(M_PI));
  check("e evaluates to ≈2.71828", eval(c, "e"), QString::number(M_E));

  section("Trig (sin / cos / tan)");
  check("sin(0) = 0", eval(c, "sin(0)"), "0");
  check("cos(0) = 1", eval(c, "cos(0)"), "1");
  check("tan(0) = 0", eval(c, "tan(0)"), "0");

  section("Inverse trig (BUG-004 — was silently no-op)");
  check("asin(0) = 0", eval(c, "asin(0)"), "0");
  check("acos(1) = 0", eval(c, "acos(1)"), "0");
  check("atan(0) = 0", eval(c, "atan(0)"), "0");
  check("asin(2) → ERR:DOMAIN", eval(c, "asin(2)"), "ERR:DOMAIN");
  check("acos(-2) → ERR:DOMAIN", eval(c, "acos(-2)"), "ERR:DOMAIN");

  section("Log / ln");
  check("log(100) = 2", eval(c, "log(100)"), "2");
  check("ln(1) = 0", eval(c, "ln(1)"), "0");
  check("log(-5) → ERR:NONREAL ANS (BUG-007)", eval(c, "log(-5)"),
        "ERR:NONREAL ANS");
  check("ln(0) → ERR:NONREAL ANS (BUG-007)", eval(c, "ln(0)"),
        "ERR:NONREAL ANS");

  section("Square root (BUG-006 — was silently 0 for negatives)");
  check("√(16) = 4", eval(c, "√(16)"), "4");
  check("√(0) = 0", eval(c, "√(0)"), "0");
  check("√(-4) → ERR:NONREAL ANS", eval(c, "√(-4)"), "ERR:NONREAL ANS");

  section("Division by zero (BUG-005)");
  check("5÷0 → ERR:DIVIDE BY 0", eval(c, "5÷0"), "ERR:DIVIDE BY 0");
  check("0÷0 → ERR:DIVIDE BY 0", eval(c, "0÷0"), "ERR:DIVIDE BY 0");

  section("Power right-associativity (BUG-009)");
  check("2^3^2 = 512 (right-assoc, not 64)", eval(c, "2^3^2"), "512");
  check("2^2^3 = 256", eval(c, "2^2^3"), "256");

  section("Unary negation (BUG-014)");
  check("-5 = -5 (keyboard `-` at start)", eval(c, "-5"), "-5");
  check("--5 = 5 (double negation)", eval(c, "--5"), "5");
  check("abs(-5) = 5", eval(c, "abs(-5)"), "5");
  check("int(-3.7) = -4 (floor)", eval(c, "int(-3.7)"), "-4");
  check("iPart(-3.7) = -3 (trunc toward 0)", eval(c, "iPart(-3.7)"), "-3");
  check("fPart(-3.7) = -0.7", eval(c, "fPart(-3.7)"), "-0.7");
  check("3*-4 = -12 (Sub after operator → Neg)", eval(c, "3*-4"), "-12");
  check("-3^2 = -9 (Pow binds tighter than Neg)", eval(c, "-3^2"), "-9");
  check("-3*4 = -12", eval(c, "-3*4"), "-12");
  check("3-4 = -1 (binary sub stays binary)", eval(c, "3-4"), "-1");

  section("Number functions (Phase B Wave 1)");
  check("abs(5) = 5", eval(c, "abs(5)"), "5");
  check("int(3.7) = 3", eval(c, "int(3.7)"), "3");
  check("iPart(3.7) = 3", eval(c, "iPart(3.7)"), "3");
  check("fPart(3.7) = 0.7", eval(c, "fPart(3.7)"), "0.7");

  section("Binary functions (Phase B Wave 2)");
  check("min(3, 5) = 3", eval(c, "min(3, 5)"), "3");
  check("max(3, 5) = 5", eval(c, "max(3, 5)"), "5");
  check("min(-3, 5) = -3 (negation works inside)", eval(c, "min(-3, 5)"),
        "-3");
  check("max(-3, -7) = -3", eval(c, "max(-3, -7)"), "-3");
  check("mod(10, 3) = 1", eval(c, "mod(10, 3)"), "1");
  check("mod(7, 0) → ERR:DIVIDE BY 0", eval(c, "mod(7, 0)"),
        "ERR:DIVIDE BY 0");
  check("round(0.123, 2) = 0.12", eval(c, "round(0.123, 2)"), "0.12");
  check("round(3.5, 0) = 4 (rounds away from zero)",
        eval(c, "round(3.5, 0)"), "4");
  check("round(0.005, 2) ≈ 0.01", eval(c, "round(0.005, 2)"), "0.01");
  // Nested: max of two function results
  check("max(sin(0), cos(0)) = 1", eval(c, "max(sin(0), cos(0))"), "1");
  // Nested binary
  check("min(max(1, 2), 5) = 2", eval(c, "min(max(1, 2), 5)"), "2");
  // BUG-016: the same expression formed with no whitespace — this was
  // the exact user-reported reproduction. Must produce 1, not SYNTAX.
  check("max(sin(0),cos(0)) = 1 (BUG-016)",
        eval(c, "max(sin(0),cos(0))"), "1");
  check("sin(max(0, 1)) = sin(1) (BUG-016 sibling)",
        eval(c, "sin(max(0, 1))"), QString::number(std::sin(1.0)));
  check("abs(min(-5, -3)) = 5 (BUG-016 sibling)",
        eval(c, "abs(min(-5, -3))"), "5");

  section("Combinatorics (nCr, nPr — Phase B)");
  check("nCr(5, 2) = 10", eval(c, "nCr(5, 2)"), "10");
  check("nCr(5, 0) = 1", eval(c, "nCr(5, 0)"), "1");
  check("nCr(5, 5) = 1", eval(c, "nCr(5, 5)"), "1");
  check("nCr(10, 3) = 120", eval(c, "nCr(10, 3)"), "120");
  check("nCr(10, 7) = 120 (symmetry)", eval(c, "nCr(10, 7)"), "120");
  check("nPr(5, 2) = 20", eval(c, "nPr(5, 2)"), "20");
  check("nPr(5, 0) = 1", eval(c, "nPr(5, 0)"), "1");
  check("nPr(5, 5) = 120", eval(c, "nPr(5, 5)"), "120");
  check("nPr(10, 3) = 720", eval(c, "nPr(10, 3)"), "720");
  // Domain errors
  check("nCr(5, 6) → ERR:DOMAIN (r > n)",
        eval(c, "nCr(5, 6)"), "ERR:DOMAIN");
  check("nCr(-1, 2) → ERR:DOMAIN",
        eval(c, "nCr(-1, 2)"), "ERR:DOMAIN");
  check("nCr(5, -1) → ERR:DOMAIN",
        eval(c, "nCr(5, -1)"), "ERR:DOMAIN");
  check("nCr(5.5, 2) → ERR:DOMAIN (non-integer)",
        eval(c, "nCr(5.5, 2)"), "ERR:DOMAIN");
  check("nPr(5, 2.3) → ERR:DOMAIN",
        eval(c, "nPr(5, 2.3)"), "ERR:DOMAIN");
  // Composition
  check("nCr(nCr(4, 2), 2) = 15",
        eval(c, "nCr(nCr(4, 2), 2)"), "15");

  section("Hyperbolic functions (Phase B)");
  check("sinh(0) = 0", eval(c, "sinh(0)"), "0");
  check("cosh(0) = 1", eval(c, "cosh(0)"), "1");
  check("tanh(0) = 0", eval(c, "tanh(0)"), "0");
  check("asinh(0) = 0", eval(c, "asinh(0)"), "0");
  check("acosh(1) = 0", eval(c, "acosh(1)"), "0");
  check("atanh(0) = 0", eval(c, "atanh(0)"), "0");
  check("cosh(1) ≈ 1.543", eval(c, "cosh(1)"), QString::number(std::cosh(1.0)));
  check("sinh(1) ≈ 1.175", eval(c, "sinh(1)"), QString::number(std::sinh(1.0)));
  // Inverse identities
  check("sinh(asinh(3)) = 3", eval(c, "sinh(asinh(3))"), "3");
  check("tanh(atanh(0.5)) = 0.5",
        eval(c, "tanh(atanh(0.5))"), QString::number(std::tanh(std::atanh(0.5))));
  // Domain errors
  check("acosh(0.5) → ERR:DOMAIN (requires x ≥ 1)",
        eval(c, "acosh(0.5)"), "ERR:DOMAIN");
  check("acosh(-1) → ERR:DOMAIN",
        eval(c, "acosh(-1)"), "ERR:DOMAIN");
  check("atanh(1) → ERR:DOMAIN (requires |x| < 1)",
        eval(c, "atanh(1)"), "ERR:DOMAIN");
  check("atanh(-1) → ERR:DOMAIN",
        eval(c, "atanh(-1)"), "ERR:DOMAIN");
  check("atanh(2) → ERR:DOMAIN",
        eval(c, "atanh(2)"), "ERR:DOMAIN");

  section("toFraction default behaviour (BUG-013, BUG-015)");
  check("1÷3 displays as decimal, not 1/3", eval(c, "1÷3"),
        QString::number(1.0 / 3.0));
  check("e displays as decimal, not 1457/536", eval(c, "e"),
        QString::number(M_E));
  check("π displays as decimal", eval(c, "π"), QString::number(M_PI));

  section("Ans recall (Phase B)");
  c.processInput(QStringLiteral("C"));
  // First result: 5
  check("set up: 2+3 = 5", evalChained(c, "2+3"), "5");
  // Ans should now be 5
  check("Ans+10 = 15", eval(c, "Ans+10"), "15");
  check("Ans*2 = 30 (uses prior Ans=15)", eval(c, "Ans*2"), "30");
  // Errors don't overwrite Ans
  c.processInput(QStringLiteral("C"));
  c.processExpression("5÷0");
  c.processInput(QStringLiteral("ENTER"));
  check("After error, Ans still recalls last good (30)", eval(c, "Ans"), "30");

  section("▶Frac / ▶Dec post-hoc conversions (BUG-015)");
  c.processInput(QStringLiteral("C"));
  evalChained(c, "1÷3");
  c.processInput(QStringLiteral("▶Frac"));
  check("1÷3 then ▶Frac → 1/3", c.currentDisplay(), "1/3");
  c.processInput(QStringLiteral("▶Dec"));
  check("then ▶Dec returns to decimal", c.currentDisplay(),
        QString::number(1.0 / 3.0));
  // ▶Frac on irrational silently leaves decimal
  c.processInput(QStringLiteral("C"));
  evalChained(c, "e");
  QString eDecimal = c.currentDisplay();
  c.processInput(QStringLiteral("▶Frac"));
  check("▶Frac on e silently leaves decimal (irrational)",
        c.currentDisplay(), eDecimal);

  section("Matrix subtraction (BUG-008)");
  // Set up [A] = [[1,2,3],[4,5,6],[7,8,9]] and [B] = [[1,1,1],[1,1,1],[1,1,1]]
  c.updateMatrix("[A]", 3, 3,
                 QVariantList{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});
  c.updateMatrix("[B]", 3, 3,
                 QVariantList{1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0});
  QString matResult = eval(c, "[A]-[B]");
  checkTrue("[A]-[B] returns a matrix (not Type Error)",
            matResult.startsWith("[["));

  section("Matrix dimension mismatch (BUG-010, BUG-011)");
  c.updateMatrix("[A]", 2, 2, QVariantList{1.0, 2.0, 3.0, 4.0});
  c.updateMatrix("[B]", 3, 3,
                 QVariantList{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});
  check("[A]+[B] mismatched dims → ERR:INVALID DIM (BUG-010)",
        eval(c, "[A]+[B]"), "ERR:INVALID DIM");
  check("[A]×[B] non-conformable → ERR:INVALID DIM (BUG-011)",
        eval(c, "[A]×[B]"), "ERR:INVALID DIM");

  section("Empty input");
  check("empty expression → ERR:SYNTAX", eval(c, ""), "ERR:SYNTAX");

  std::cout << "\n----------------------------------------\n"
            << "Total: " << (gPassed + gFailed) << "  Passed: " << gPassed
            << "  Failed: " << gFailed << '\n';
  return gFailed == 0 ? 0 : 1;
}

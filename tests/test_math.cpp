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
  c.processInput(QStringLiteral("CLEAR"));
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
  check("π evaluates to ≈3.14159", eval(c, "π"), UIController::formatScalar(M_PI));
  check("e evaluates to ≈2.71828", eval(c, "e"), UIController::formatScalar(M_E));

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
        eval(c, "sin(max(0, 1))"), UIController::formatScalar(std::sin(1.0)));
  check("abs(min(-5, -3)) = 5 (BUG-016 sibling)",
        eval(c, "abs(min(-5, -3))"), "5");

  section("Factorial (Phase B)");
  check("0! = 1", eval(c, "0!"), "1");
  check("1! = 1", eval(c, "1!"), "1");
  check("5! = 120", eval(c, "5!"), "120");
  check("10! = 3628800", eval(c, "10!"), "3628800");
  check("5!+3! = 126", eval(c, "5!+3!"), "126");
  check("5!/5 = 24", eval(c, "5!/5"), "24");
  // Precedence: factorial binds tighter than ^
  check("2^3! = 64 (= 2^(3!) = 2^6)", eval(c, "2^3!"), "64");
  check("5!^2 = 14400", eval(c, "5!^2"), "14400");
  // Unary negation and factorial
  check("-5! = -120 (= -(5!))", eval(c, "-5!"), "-120");
  check("(-5)! → ERR:DOMAIN", eval(c, "(-5)!"), "ERR:DOMAIN");
  // Domain errors
  check("3.5! → ERR:DOMAIN (non-integer)",
        eval(c, "3.5!"), "ERR:DOMAIN");
  check("(-1)! → ERR:DOMAIN",
        eval(c, "(-1)!"), "ERR:DOMAIN");
  // Composition with combinatorics
  check("nCr(5, 2) × 3! = 60", eval(c, "nCr(5, 2)*3!"), "60");

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
  check("cosh(1) ≈ 1.543", eval(c, "cosh(1)"), UIController::formatScalar(std::cosh(1.0)));
  check("sinh(1) ≈ 1.175", eval(c, "sinh(1)"), UIController::formatScalar(std::sinh(1.0)));
  // Inverse identities
  check("sinh(asinh(3)) = 3", eval(c, "sinh(asinh(3))"), "3");
  check("tanh(atanh(0.5)) = 0.5",
        eval(c, "tanh(atanh(0.5))"), UIController::formatScalar(std::tanh(std::atanh(0.5))));
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
        UIController::formatScalar(1.0 / 3.0));
  check("e displays as decimal, not 1457/536", eval(c, "e"),
        UIController::formatScalar(M_E));
  check("π displays as decimal", eval(c, "π"), UIController::formatScalar(M_PI));

  section("Ans recall (Phase B)");
  c.processInput(QStringLiteral("CLEAR"));
  // First result: 5
  check("set up: 2+3 = 5", evalChained(c, "2+3"), "5");
  // Ans should now be 5
  check("Ans+10 = 15", eval(c, "Ans+10"), "15");
  check("Ans*2 = 30 (uses prior Ans=15)", eval(c, "Ans*2"), "30");
  // Errors don't overwrite Ans
  c.processInput(QStringLiteral("CLEAR"));
  c.processExpression("5÷0");
  c.processInput(QStringLiteral("ENTER"));
  check("After error, Ans still recalls last good (30)", eval(c, "Ans"), "30");

  section("▶Frac / ▶Dec post-hoc conversions (BUG-015)");
  c.processInput(QStringLiteral("CLEAR"));
  evalChained(c, "1÷3");
  c.processInput(QStringLiteral("▶Frac"));
  check("1÷3 then ▶Frac → 1/3", c.currentDisplay(), "1/3");
  c.processInput(QStringLiteral("▶Dec"));
  check("then ▶Dec returns to decimal", c.currentDisplay(),
        UIController::formatScalar(1.0 / 3.0));
  // ▶Frac on irrational silently leaves decimal
  c.processInput(QStringLiteral("CLEAR"));
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

  section("Matrix inverse and rref (Phase B)");
  // Inverse of 2×2 [[1,2],[3,4]] is [[-2,1],[1.5,-0.5]].
  c.updateMatrix("[A]", 2, 2, QVariantList{1.0, 2.0, 3.0, 4.0});
  check("[A]^-1 for [[1,2],[3,4]]",
        eval(c, "[A]^-1"), "[[-2,1][1.5,-0.5]]");
  // Inverse of identity is identity.
  c.updateMatrix("[A]", 2, 2, QVariantList{1.0, 0.0, 0.0, 1.0});
  check("[I]^-1 = [I]",
        eval(c, "[A]^-1"), "[[1,0][0,1]]");
  // Multiplying a matrix by its inverse should yield identity.
  c.updateMatrix("[A]", 2, 2, QVariantList{1.0, 2.0, 3.0, 4.0});
  check("[A]*[A]^-1 = I",
        eval(c, "[A]*[A]^-1"), "[[1,0][0,1]]");
  // Singular matrix: [[1,2],[2,4]] has det 0.
  c.updateMatrix("[A]", 2, 2, QVariantList{1.0, 2.0, 2.0, 4.0});
  check("singular [A]^-1 → ERR:SINGULAR MAT",
        eval(c, "[A]^-1"), "ERR:SINGULAR MAT");
  // Non-square inverse fails.
  c.updateMatrix("[A]", 2, 3, QVariantList{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  check("non-square [A]^-1 → ERR:INVALID DIM",
        eval(c, "[A]^-1"), "ERR:INVALID DIM");

  // rref of a 3×3 matrix with rank 2: rows 3 is a linear combination,
  // so rref gives a zero row at the bottom.
  c.updateMatrix("[A]", 3, 3,
                 QVariantList{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});
  check("rref([[1,2,3][4,5,6][7,8,9]]) = [[1,0,-1][0,1,2][0,0,0]]",
        eval(c, "rref([A])"), "[[1,0,-1][0,1,2][0,0,0]]");
  // rref of identity is identity.
  c.updateMatrix("[A]", 3, 3,
                 QVariantList{1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0});
  check("rref(I) = I",
        eval(c, "rref([A])"), "[[1,0,0][0,1,0][0,0,1]]");
  // rref on scalar → type error.
  check("rref(5) → ERR:DATA TYPE",
        eval(c, "rref(5)"), "ERR:DATA TYPE");

  section("Matrix transpose (Phase B)");
  // 2×3 [A] = [[1, 2, 3], [4, 5, 6]]
  c.updateMatrix("[A]", 2, 3,
                 QVariantList{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  check("T([A]) of 2×3 → 3×2 [[1,4],[2,5],[3,6]]",
        eval(c, "T([A])"), "[[1,4][2,5][3,6]]");
  // Double transpose is identity
  check("T(T([A])) = [A]",
        eval(c, "T(T([A]))"), "[[1,2,3][4,5,6]]");
  // Transpose of scalar → type error
  check("T(5) → ERR:DATA TYPE",
        eval(c, "T(5)"), "ERR:DATA TYPE");
  // Transpose of square matrix
  c.updateMatrix("[B]", 2, 2, QVariantList{1.0, 2.0, 3.0, 4.0});
  check("T([B]) of square 2×2",
        eval(c, "T([B])"), "[[1,3][2,4]]");

  section("Matrix dimension mismatch (BUG-010, BUG-011)");
  c.updateMatrix("[A]", 2, 2, QVariantList{1.0, 2.0, 3.0, 4.0});
  c.updateMatrix("[B]", 3, 3,
                 QVariantList{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0});
  check("[A]+[B] mismatched dims → ERR:INVALID DIM (BUG-010)",
        eval(c, "[A]+[B]"), "ERR:INVALID DIM");
  check("[A]×[B] non-conformable → ERR:INVALID DIM (BUG-011)",
        eval(c, "[A]×[B]"), "ERR:INVALID DIM");

  section("Scalar variables and STO");
  // Fresh controller so we don't inherit values from earlier sections.
  // MathStateMachine::varRegistry is a process-global static, so reset
  // the two letters we'll touch to zero up front.
  tux_ti83::MathStateMachine::varRegistry.fill(0.0);

  // Unset variables default to 0 (TI-83 power-on state).
  check("A with no store → 0", eval(c, "A"), "0");
  check("B+3 with no store → 3", eval(c, "B+3"), "3");

  // Basic store. The display shows the stored value, and subsequent
  // reads return it.
  check("5→A returns 5", eval(c, "5→A"), "5");
  check("A after 5→A is 5", eval(c, "A"), "5");

  // Arithmetic before store, then read-back.
  check("A+3→A returns 8", eval(c, "A+3→A"), "8");
  check("A after A+3→A is 8", eval(c, "A"), "8");

  // Independence — writing to A doesn't touch B.
  check("B still 0 after all A mutations", eval(c, "B"), "0");

  // ASCII alias for the arrow.
  check("7->C returns 7 (ASCII arrow alias)", eval(c, "7->C"), "7");
  check("C after 7->C is 7", eval(c, "C"), "7");

  // Errors don't mutate state: after a DIVIDE BY 0 attempt on D,
  // D should still be its previous value (0, since untouched).
  check("1÷0→D → ERR:DIVIDE BY 0", eval(c, "1÷0→D"), "ERR:DIVIDE BY 0");
  check("D unchanged after failed store", eval(c, "D"), "0");

  // Malformed: Sto not followed by a variable.
  check("5→ alone → ERR:SYNTAX", eval(c, "5→"), "ERR:SYNTAX");
  check("5→5 → ERR:SYNTAX", eval(c, "5→5"), "ERR:SYNTAX");

  // Matrix-to-scalar-var is a type error.
  c.updateMatrix("[A]", 2, 2, QVariantList{1.0, 2.0, 3.0, 4.0});
  check("[A]→E → ERR:DATA TYPE", eval(c, "[A]→E"), "ERR:DATA TYPE");

  // X as calc-mode scalar: stored via Sto, read back via the registry
  // (not the graph-mode xValue=0 default that used to win).
  check("9→X returns 9", eval(c, "9→X"), "9");
  check("X+1 after 9→X is 10", eval(c, "X+1"), "10");

  section("Angle mode (MODE menu)");
  // Default is Radian — previously-run trig tests already proved this.
  // Flip to Degree and confirm standard reference values work.
  tux_ti83::MathStateMachine::angleMode = tux_ti83::AngleMode::Degree;

  check("sin(0°) = 0", eval(c, "sin(0)"), "0");
  check("sin(30°) = 0.5", eval(c, "sin(30)"), "0.5");
  check("sin(90°) = 1", eval(c, "sin(90)"), "1");
  check("cos(0°) = 1", eval(c, "cos(0)"), "1");
  check("cos(60°) = 0.5", eval(c, "cos(60)"), "0.5");
  check("cos(90°) = 0 (tol)", eval(c, "cos(90)"), "6.123233996e-17");
  check("tan(45°) = 1", eval(c, "tan(45)"), "1");

  // Inverse trig: result should be in degrees now.
  check("asin(1) in deg = 90", eval(c, "asin(1)"), "90");
  check("acos(0) in deg = 90", eval(c, "acos(0)"), "90");
  check("atan(1) in deg = 45", eval(c, "atan(1)"), "45");

  // Restore radians before subsequent tests (and make sure we're back).
  tux_ti83::MathStateMachine::angleMode = tux_ti83::AngleMode::Radian;
  check("after mode restore: sin(0 rad) = 0", eval(c, "sin(0)"), "0");
  check("after mode restore: asin(1) rad ≈ π/2",
        eval(c, "asin(1)"), UIController::formatScalar(M_PI / 2.0));

  section("Last-entry recall (2ND+ENTER)");
  {
    // Fresh controller so the entry-history ring buffer starts empty.
    UIController rc;

    // Baseline: recall with nothing in history is a no-op. Display
    // should remain empty after the call.
    rc.recallLastEntry();
    check("recall on empty history leaves display empty",
          rc.currentDisplay(), "");

    // Push three entries, then cycle back through them. After each
    // recall the display shows the recalled expression; ENTER then
    // re-evaluates and pushes the same entry again. For the cycle
    // tests we just inspect currentDisplay() without pressing ENTER.
    evalChained(rc, "1+2");  // result 3 — pushes [1,+,2]
    rc.processInput(QStringLiteral("CLEAR"));
    evalChained(rc, "4×5");  // result 20 — pushes [4,×,5]
    rc.processInput(QStringLiteral("CLEAR"));
    evalChained(rc, "7-3");  // result 4 — pushes [7,-,3]

    // After CLEAR the display is empty. First 2ND+ENTER brings back
    // the most recent entry.
    rc.processInput(QStringLiteral("CLEAR"));
    rc.recallLastEntry();
    check("first recall shows last entry (7-3)",
          rc.currentDisplay(), "7−3");

    // Second recall walks further back.
    rc.recallLastEntry();
    check("second recall shows 2nd-last entry (4×5)",
          rc.currentDisplay(), "4×5");

    rc.recallLastEntry();
    check("third recall shows 3rd-last entry (1+2)",
          rc.currentDisplay(), "1+2");

    // Extra recalls past the beginning of history clamp — the oldest
    // entry stays displayed rather than disappearing.
    rc.recallLastEntry();
    check("recall past oldest clamps on the oldest entry",
          rc.currentDisplay(), "1+2");

    // Any non-recall input resets the cycle so the next 2ND+ENTER
    // starts from the most recent entry again.
    rc.processInput(QStringLiteral("CLEAR"));
    rc.recallLastEntry();
    check("after non-recall input, cycle restarts at last entry",
          rc.currentDisplay(), "7−3");

    // Recalled expression can be evaluated — this also re-pushes it
    // to history (a real TI-83 stores every ENTER).
    rc.processInput(QStringLiteral("ENTER"));
    check("recalled 7-3 evaluates to 4",
          rc.currentDisplay(), "4");

    // Pressing ENTER on an empty buffer should NOT push to history.
    // Verify by entering something, clearing, pressing ENTER (empty),
    // then recalling — the last pushed entry should still be what we
    // stored, not the empty one.
    UIController rc2;
    evalChained(rc2, "9+1");           // pushes [9,+,1]
    rc2.processInput(QStringLiteral("CLEAR"));
    rc2.processInput(QStringLiteral("ENTER")); // empty — must not push
    rc2.recallLastEntry();
    check("empty ENTER does not pollute history",
          rc2.currentDisplay(), "9+1");
  }

  section("Cursor movement within expression");
  {
    UIController cc;
    // Build the expression "1+2" one token at a time, then walk the
    // cursor left and insert `×9` between "1" and "+", producing
    // "1×9+2" which evaluates to 11.
    cc.processInput(QStringLiteral("CLEAR"));
    cc.processInput(QStringLiteral("1"));
    cc.processInput(QStringLiteral("+"));
    cc.processInput(QStringLiteral("2"));
    check("pre-move display is 1+2",
          cc.currentDisplay(), "1+2");

    cc.moveCursorLeft();  // cursor now between + and 2
    cc.moveCursorLeft();  // cursor now between 1 and +
    cc.processInput(QStringLiteral("×"));
    cc.processInput(QStringLiteral("9"));
    check("after mid-insert display is 1×9+2",
          cc.currentDisplay(), "1×9+2");

    cc.processInput(QStringLiteral("ENTER"));
    check("1×9+2 evaluates to 11",
          cc.currentDisplay(), "11");

    // Mid-expression backspace: build "12+3", move cursor one left
    // (between + and 3), backspace removes the +, giving "123".
    cc.processInput(QStringLiteral("CLEAR"));
    cc.processExpression("12+3");
    cc.moveCursorLeft();  // cursor between + and 3
    cc.processInput(QStringLiteral("DEL"));  // removes the +
    check("mid-backspace of + from 12+3 gives 123",
          cc.currentDisplay(), "123");

    // Cursor clamping — Left from 0 and Right past end are no-ops.
    cc.processInput(QStringLiteral("CLEAR"));
    cc.processExpression("5");
    cc.moveCursorLeft();
    cc.moveCursorLeft();  // over-clamp: no crash, cursor stays at 0
    cc.processInput(QStringLiteral("7"));  // inserts at head
    check("over-clamped-left cursor inserts at head (75)",
          cc.currentDisplay(), "75");

    // Home / End behaviour — after Home, insertions land at the front;
    // after End, they land at the back.
    cc.processInput(QStringLiteral("CLEAR"));
    cc.processExpression("ABC");  // three variable tokens (uppercase)
    cc.moveCursorHome();
    cc.processInput(QStringLiteral("1"));
    check("Home then 1 prepends to ABC → 1ABC",
          cc.currentDisplay(), "1ABC");
    cc.moveCursorEnd();
    cc.processInput(QStringLiteral("2"));
    check("End then 2 appends to 1ABC → 1ABC2",
          cc.currentDisplay(), "1ABC2");

    // Cursor moves are no-ops outside Inputting state.
    cc.processInput(QStringLiteral("CLEAR"));
    cc.processExpression("2+3");
    cc.processInput(QStringLiteral("ENTER"));  // state → Evaluated
    const int offsetBefore = cc.cursorOffset();
    cc.moveCursorLeft();
    check("moveCursorLeft is a no-op outside Inputting",
          QString::number(cc.cursorOffset()), QString::number(offsetBefore));
  }

  section("Logic operator menu (2ND+MATH)");
  // These operators existed on the engine but had no UI exposure until
  // the LogicMenuPopup and the ASCII aliases in kTokens landed.
  check("3≤5 (Unicode ≤)", eval(c, "3≤5"), "1");
  check("5≤3 (Unicode ≤)", eval(c, "5≤3"), "0");
  check("3<=5 (ASCII alias)", eval(c, "3<=5"), "1");
  check("3≥5 (Unicode ≥)", eval(c, "3≥5"), "0");
  check("5>=3 (ASCII alias)", eval(c, "5>=3"), "1");
  check("1 xor 0 = 1", eval(c, "1 xor 0"), "1");
  check("1 xor 1 = 0", eval(c, "1 xor 1"), "0");
  check("0 xor 0 = 0", eval(c, "0 xor 0"), "0");

  section("Empty input");
  check("empty expression → ERR:SYNTAX", eval(c, ""), "ERR:SYNTAX");

  std::cout << "\n----------------------------------------\n"
            << "Total: " << (gPassed + gFailed) << "  Passed: " << gPassed
            << "  Failed: " << gFailed << '\n';
  return gFailed == 0 ? 0 : 1;
}

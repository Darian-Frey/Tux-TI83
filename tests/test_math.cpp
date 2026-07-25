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

  section("Notation + Decimal (MODE menu)");
  using tux_ti83::MathStateMachine;
  using tux_ti83::NumberNotation;

  // Normal + Float is the historical default — sanity-check it matches
  // what the long list of earlier tests has been asserting.
  MathStateMachine::notation    = NumberNotation::Normal;
  MathStateMachine::fixDecimals = -1;
  check("Normal Float: 1÷3 ≈ 0.333333…",
        eval(c, "1÷3"), UIController::formatScalar(1.0 / 3.0));
  check("Normal Float: 10! = 3628800 (integer form)",
        eval(c, "10!"), "3628800");

  // Normal + Fix N: fixed decimal places, no exponent.
  MathStateMachine::fixDecimals = 2;
  check("Normal Fix 2: 1÷3 → 0.33",
        eval(c, "1÷3"), "0.33");
  check("Normal Fix 2: 1 → 1.00",
        eval(c, "1"), "1.00");
  MathStateMachine::fixDecimals = 0;
  check("Normal Fix 0: 1÷3 → 0",
        eval(c, "1÷3"), "0");

  // Sci mode — uppercase E, Qt's native 'E' formatter.
  MathStateMachine::notation    = NumberNotation::Sci;
  MathStateMachine::fixDecimals = -1;
  check("Sci Float: 12345 → 1.23450…E+04",
        eval(c, "12345"), "1.234500000E+04");
  MathStateMachine::fixDecimals = 2;
  check("Sci Fix 2: 12345 → 1.23E+04",
        eval(c, "12345"), "1.23E+04");

  // Eng mode: exponent is always a multiple of 3.
  MathStateMachine::notation    = NumberNotation::Eng;
  MathStateMachine::fixDecimals = 3;
  check("Eng Fix 3: 12345 → 12.345E3",
        eval(c, "12345"), "12.345E3");
  check("Eng Fix 3: 0.005 → 5.000E-3",
        eval(c, ".005"), "5.000E-3");
  check("Eng Fix 3: 0 → 0.000E0",
        eval(c, "0"), "0.000E0");

  // Restore defaults so the remaining assertions in the file see the
  // Normal + Float environment they were written against.
  MathStateMachine::notation    = NumberNotation::Normal;
  MathStateMachine::fixDecimals = -1;

  section("Statement separator (`:`)");
  {
    UIController sc;
    tux_ti83::MathStateMachine::varRegistry.fill(0.0);

    // Two simple expressions chained: result is the last segment.
    check("1+2:3+4 returns 7", eval(sc, "1+2:3+4"), "7");

    // Chained store + read: A is set in the first segment, read in the
    // second. This is the canonical TI-83 idiom.
    check("5→A:A+1 returns 6", eval(sc, "5→A:A+1"), "6");
    check("A persists after chained store", eval(sc, "A"), "5");

    // Triple chain — verify the last value comes through.
    check("1:2:3 returns 3", eval(sc, "1:2:3"), "3");

    // Error mid-chain aborts; later segments don't run. The first
    // store DOES commit because errors short-circuit but earlier
    // segments have already mutated the registry.
    check("9→B:1÷0:7→B → ERR:DIVIDE BY 0",
          eval(sc, "9→B:1÷0:7→B"), "ERR:DIVIDE BY 0");
    check("B was set by first segment before error",
          eval(sc, "B"), "9");

    // Stray colons are tolerated (leading/trailing).
    check(":5 returns 5", eval(sc, ":5"), "5");
    check("5: returns 5", eval(sc, "5:"), "5");
  }

  section("Insert vs. overwrite mode (2ND + DEL)");
  {
    UIController ic;

    // Default insert mode: typing "1+2" produces "1+2" (3 tokens).
    ic.processInput(QStringLiteral("CLEAR"));
    ic.processExpression("1+2");
    check("default insert mode: display is 1+2",
          ic.currentDisplay(), "1+2");

    // Move cursor to between 1 and +; toggle to overwrite; type 9.
    // The + at the cursor position should be replaced, giving 192.
    ic.moveCursorLeft();          // cursor between + and 2
    ic.moveCursorLeft();          // cursor between 1 and +
    ic.toggleInsertMode();
    ic.processInput(QStringLiteral("9"));
    check("overwrite mid-expression: + replaced with 9 → 192",
          ic.currentDisplay(), "192");

    // Continued overwrite at the next position replaces the 2.
    ic.processInput(QStringLiteral("0"));
    check("overwrite continues: 2 replaced with 0 → 190",
          ic.currentDisplay(), "190");

    // Typing past the end falls back to append (matches TI-83 OVR
    // behaviour at the tail).
    ic.processInput(QStringLiteral("8"));
    check("overwrite past end appends → 1908",
          ic.currentDisplay(), "1908");

    // Toggle back to insert mode; new tokens splice in.
    ic.toggleInsertMode();
    ic.moveCursorHome();
    ic.processInput(QStringLiteral("7"));
    check("back to insert: 7 prepended → 71908",
          ic.currentDisplay(), "71908");

    // insertMode is exposed as a Q_PROPERTY — verify it round-trips.
    checkTrue("insertMode reads as true after toggle-back",
              ic.insertMode());
    ic.toggleInsertMode();
    checkTrue("insertMode reads as false after second toggle",
              !ic.insertMode());
  }

  section("Nth root (2ND + ^)");
  check("3ˣ√27 = 3 (cubic root)",  eval(c, "3ˣ√27"), "3");
  check("4ˣ√16 = 2 (4th root)",    eval(c, "4ˣ√16"), "2");
  check("2ˣ√9 = 3 (square root)",  eval(c, "2ˣ√9"),  "3");
  check("3ˣ√-8 = -2 (odd root of negative)",
        eval(c, "3ˣ√-8"), "-2");
  check("xroot ASCII alias: 3 xroot 27 = 3",
        eval(c, "3 xroot 27"), "3");

  // Domain edges.
  check("0ˣ√5 → ERR:DOMAIN (n=0 undefined)",
        eval(c, "0ˣ√5"), "ERR:DOMAIN");
  check("2ˣ√-9 → ERR:NONREAL ANS (even root of negative)",
        eval(c, "2ˣ√-9"), "ERR:NONREAL ANS");

  // Right-associativity: same precedence as Pow.
  // 2 ˣ√ 3 ˣ√ 64  ==  2 ˣ√ (3rd root of 64)  ==  2 ˣ√ 4  ==  2.
  check("2ˣ√3ˣ√64 = 2 (right-assoc)",
        eval(c, "2ˣ√3ˣ√64"), "2");

  section("Implicit multiplication by juxtaposition (IMP-005)");
  check("2π = 2 × π",
        eval(c, "2π"), UIController::formatScalar(2.0 * M_PI));
  check("2(3+4) = 14",     eval(c, "2(3+4)"),     "14");
  check("(3)(4) = 12",     eval(c, "(3)(4)"),     "12");
  check("(1+2)(3+4) = 21", eval(c, "(1+2)(3+4)"), "21");
  check("2sin(0) = 0",     eval(c, "2sin(0)"),    "0");
  check("3!2 = 12 (factorial juxtaposed with number)",
        eval(c, "3!2"), "12");
  check("2π3 = 6π (chained juxtaposition)",
        eval(c, "2π3"), UIController::formatScalar(6.0 * M_PI));
  check("π² = π × π",
        eval(c, "π^2"), UIController::formatScalar(M_PI * M_PI));

  // Variable juxtaposition: 5A with A = 4 should give 20.
  tux_ti83::MathStateMachine::varRegistry.fill(0.0);
  evalChained(c, "4→A");
  check("5A with A=4 = 20",
        eval(c, "5A"), "20");
  check("2A+3A = 20 (mixed juxtaposition + binary)",
        eval(c, "2A+3A"), "20");

  // Regression: 2-3 must still parse as subtraction, not 2*(-3).
  check("2-3 stays as subtraction → -1",
        eval(c, "2-3"), "-1");

  section("Y-VARS recall (IMP-036)");
  {
    UIController yc;
    // Reset varRegistry so any A..Z used inside Y_n bodies start at 0.
    tux_ti83::MathStateMachine::varRegistry.fill(0.0);

    // Populate Y1 = 5 by switching to slot 0 and evaluating. eval()
    // CLEARs the active slot first, so we're safe even if a previous
    // section left state behind.
    yc.setActiveFunction(0);
    check("Y1 := 5", eval(yc, "5"), "5");

    // Cross-slot reference: while active = Y2, evaluating Y1+1 should
    // look up Y1's stored buffer and return 6.
    yc.setActiveFunction(1);
    check("Y1+1 in Y2 with Y1=5 → 6",
          eval(yc, "Y1+1"), "6");

    // X parameter threads through Y-VAR lookups.
    yc.setActiveFunction(0);
    check("Y1 := X+2", eval(yc, "X+2"), "2");  // X reads from registry; A..Z all 0
    yc.setActiveFunction(1);
    eval(yc, "3→X");                            // store 3 in X (registry)
    check("Y1 in Y2 with X=3, Y1=X+2 → 5",
          eval(yc, "Y1"), "5");

    // Self-reference: Y1 = Y1+1 (the buffer references itself). Detect
    // via the cycle guard.
    yc.setActiveFunction(0);
    check("Y1+1 in Y1 → ERR:RECURSION (self)",
          eval(yc, "Y1+1"), "ERR:RECURSION");

    // Empty target: Y3 wasn't written, so Y3 acts as 0.
    yc.setActiveFunction(1);
    check("Y3+5 with Y3 empty → 5",
          eval(yc, "Y3+5"), "5");

    // Mutual cycle Y1=Y2, Y2=Y1. Build the buffers via
    // processExpression (no eval, so no recursion fires during setup),
    // then trigger evaluation from one side and verify the guard
    // catches the loop two hops deep.
    UIController cc;
    cc.setActiveFunction(0);
    cc.processInput(QStringLiteral("CLEAR"));
    cc.processExpression(QStringLiteral("Y2"));
    cc.setActiveFunction(1);
    cc.processInput(QStringLiteral("CLEAR"));
    cc.processExpression(QStringLiteral("Y1"));
    cc.setActiveFunction(0);
    cc.processInput(QStringLiteral("ENTER"));
    check("Y1=Y2, Y2=Y1 → ERR:RECURSION (mutual cycle)",
          cc.currentDisplay(), "ERR:RECURSION");
  }

  section("Exp + Sgn (IMP-041)");
  check("e^(0) = 1", eval(c, "e^(0)"), "1");
  check("e^(1) = e", eval(c, "e^(1)"), UIController::formatScalar(M_E));
  check("e^(ln(5)) = 5 (inverse round-trip)",
        eval(c, "e^(ln(5))"), "5");
  check("sgn(-5) = -1",   eval(c, "sgn(-5)"), "-1");
  check("sgn(0)  =  0",   eval(c, "sgn(0)"),  "0");
  check("sgn(5)  =  1",   eval(c, "sgn(5)"),  "1");
  check("sgn(-3.7) = -1 (non-integer negative)",
        eval(c, "sgn(-3.7)"), "-1");
  check("2sgn(7) = 2 (juxtaposition with implicit mul)",
        eval(c, "2sgn(7)"), "2");

  section("Y-VARS call form: Y1(arg) (IMP-042)");
  {
    UIController yc;
    tux_ti83::MathStateMachine::varRegistry.fill(0.0);

    // Set Y1 = X^2 by switching to slot 0 and evaluating.
    yc.setActiveFunction(0);
    eval(yc, "X^2");
    // Cross-slot: Y2 evaluates Y1(3) → 9
    yc.setActiveFunction(1);
    check("Y1(3) with Y1=X^2 → 9", eval(yc, "Y1(3)"), "9");
    check("Y1(3+1) with Y1=X^2 → 16",
          eval(yc, "Y1(3+1)"), "16");
    check("Y1(-2) with Y1=X^2 → 4 (unary-minus arg)",
          eval(yc, "Y1(-2)"), "4");
    // Argument can itself be a Y-VAR. Set Y3 = X+1 to test:
    yc.setActiveFunction(2);
    eval(yc, "X+1");
    yc.setActiveFunction(1);
    check("Y1(Y3(3)) with Y3=X+1, Y1=X^2 → 16",
          eval(yc, "Y1(Y3(3))"), "16");

    // Empty target → arg ignored, evaluates to 0 (matches bare form).
    // Reset Y3 so it's empty again.
    yc.setActiveFunction(2);
    yc.processInput(QStringLiteral("CLEAR"));
    yc.setActiveFunction(1);
    check("Y3(99) with Y3 empty → 0",
          eval(yc, "Y3(99)"), "0");

    // Recursion across forms: Y1=Y1(0)+1 (call form referencing itself)
    yc.setActiveFunction(0);
    check("Y1(0)+1 in Y1 → ERR:RECURSION",
          eval(yc, "Y1(0)+1"), "ERR:RECURSION");

    // Cross-form cycle: Y1=Y2, Y2=Y1(0)
    UIController cc;
    cc.setActiveFunction(0);
    cc.processInput(QStringLiteral("CLEAR"));
    cc.processExpression(QStringLiteral("Y2"));
    cc.setActiveFunction(1);
    cc.processInput(QStringLiteral("CLEAR"));
    cc.processExpression(QStringLiteral("Y1(0)"));
    cc.setActiveFunction(0);
    cc.processInput(QStringLiteral("ENTER"));
    check("Y1=Y2, Y2=Y1(0) → ERR:RECURSION (mixed cycle)",
          cc.currentDisplay(), "ERR:RECURSION");
  }

  section("Calculus: fnInt / nDeriv / sum / prod (IMP-044)");
  {
    // fnInt — definite integral via composite Simpson's rule. N=100,
    // so closed-form polynomials converge to within the formatter's
    // visible precision; trig integrals match to ~1e-8.
    check("fnInt(X^2, X, 0, 1) → 1/3",
          eval(c, "fnInt(X^2, X, 0, 1)"),
          UIController::formatScalar(1.0 / 3.0));
    check("fnInt(X, X, 0, 10) → 50",
          eval(c, "fnInt(X, X, 0, 10)"), "50");
    check("fnInt(X^3, X, 0, 2) → 4",
          eval(c, "fnInt(X^3, X, 0, 2)"), "4");

    // Order-swap: a > b returns the signed integral.
    check("fnInt(X, X, 1, 0) → -0.5",
          eval(c, "fnInt(X, X, 1, 0)"),
          UIController::formatScalar(-0.5));

    // Degenerate bounds.
    check("fnInt(X, X, 5, 5) → 0",
          eval(c, "fnInt(X, X, 5, 5)"), "0");

    // nDeriv — symmetric finite difference. Default h = 1e-3 → error
    // ≈ h²·f‴(x)/6 for smooth functions; pick targets where the exact
    // derivative is a small integer.
    check("nDeriv(X^2, X, 3) → 6",
          eval(c, "nDeriv(X^2, X, 3)"), "6");
    check("nDeriv(X^3, X, 2) ≈ 12 (Simpson tolerance)",
          eval(c, "nDeriv(X^3, X, 2)"), "12.000001");

    // Explicit h.
    check("nDeriv(X^2, X, 0, 0.5) → 0",
          eval(c, "nDeriv(X^2, X, 0, 0.5)"), "0");

    // h=0 is a domain error.
    check("nDeriv(X, X, 0, 0) → DOMAIN",
          eval(c, "nDeriv(X, X, 0, 0)"), "ERR:DOMAIN");

    // sum — exact arithmetic; integer iteration.
    check("sum(X, X, 1, 10) → 55",
          eval(c, "sum(X, X, 1, 10)"), "55");
    check("sum(X^2, X, 1, 5) → 55",
          eval(c, "sum(X^2, X, 1, 5)"), "55");
    check("sum(2X, X, 1, 4) → 20",
          eval(c, "sum(2X, X, 1, 4)"), "20");

    // Empty range → identity (0 for sum).
    check("sum(X, X, 5, 1) → 0 (empty)",
          eval(c, "sum(X, X, 5, 1)"), "0");

    // Iteration cap — 1,000,000 iterations are above the 100,000 cap.
    // (Don't use `1e6` here: the engine parses `e` as the constant E,
    // not scientific notation, so `1e6` evaluates to ~16.3.)
    check("sum(1, X, 1, 1000000) → DOMAIN (over cap)",
          eval(c, "sum(1, X, 1, 1000000)"), "ERR:DOMAIN");

    // prod.
    check("prod(X, X, 1, 5) → 120 (= 5!)",
          eval(c, "prod(X, X, 1, 5)"), "120");
    check("prod(2, X, 1, 5) → 32 (= 2^5)",
          eval(c, "prod(2, X, 1, 5)"), "32");
    check("prod(X, X, 5, 1) → 1 (empty identity)",
          eval(c, "prod(X, X, 5, 1)"), "1");

    // Bound variable other than X — varRegistry path. Use A so the
    // sampler writes/restores varRegistry[0] without touching xValue.
    check("sum(A^2, A, 1, 3) → 14",
          eval(c, "sum(A^2, A, 1, 3)"), "14");

    // Variables outside the bound name resolve from the registry,
    // restored after the loop. Set A=5, then sum 1..A.
    {
      UIController cc;
      cc.processExpression(QStringLiteral("5→A"));
      cc.processInput(QStringLiteral("ENTER"));
      cc.processInput(QStringLiteral("CLEAR"));
      cc.processExpression(QStringLiteral("sum(X, X, 1, A)"));
      cc.processInput(QStringLiteral("ENTER"));
      check("sum(X, X, 1, A) with A=5 → 15",
            cc.currentDisplay(), "15");
    }

    // Nested calls — outer's slot reservation happens before recursion
    // into args, so each call carries its own K index.
    check("fnInt(fnInt(1, Y, 0, X), X, 0, 1) → 0.5",
          eval(c, "fnInt(fnInt(1, Y, 0, X), X, 0, 1)"),
          UIController::formatScalar(0.5));

    // Deferred expression spanning a built-in-paren function — the
    // earlier opensParenScope fix targets this.
    {
      QString s = eval(c, "fnInt(sin(X), X, 0, π)");
      bool ok = s.startsWith("2") || s.startsWith("1.99");
      check("fnInt(sin(X), X, 0, π) ≈ 2",
            ok ? QString("2 (≈)") : s, "2 (≈)");
    }

    // Syntax errors: variable arg must be a single VarA..VarZ.
    check("fnInt(X, X+1, 0, 1) → SYNTAX",
          eval(c, "fnInt(X, X+1, 0, 1)"), "ERR:SYNTAX");
    check("fnInt(X, 5, 0, 1) → SYNTAX",
          eval(c, "fnInt(X, 5, 0, 1)"), "ERR:SYNTAX");

    // Wrong arity.
    check("fnInt(X, X, 0) → SYNTAX (missing upper)",
          eval(c, "fnInt(X, X, 0)"), "ERR:SYNTAX");
  }

  section("Number base (IMP-043)");
  {
    using tux_ti83::NumberBase;
    // Dec is the historic default — exercise it explicitly so the
    // expectation is documented next to the alternatives.
    MathStateMachine::numberBase = NumberBase::Dec;
    check("Dec: 255 → \"255\"",
          UIController::formatScalar(255.0), "255");

    // Hex / Oct / Bin: positive integers render with prefix + uppercase
    // hex (binary/octal have no case ambiguity).
    MathStateMachine::numberBase = NumberBase::Hex;
    check("Hex: 0 → 0x0",       UIController::formatScalar(0.0),    "0x0");
    check("Hex: 255 → 0xFF",    UIController::formatScalar(255.0),  "0xFF");
    check("Hex: 4096 → 0x1000", UIController::formatScalar(4096.0), "0x1000");

    MathStateMachine::numberBase = NumberBase::Oct;
    check("Oct: 8 → 0o10",      UIController::formatScalar(8.0),    "0o10");
    check("Oct: 63 → 0o77",     UIController::formatScalar(63.0),   "0o77");

    MathStateMachine::numberBase = NumberBase::Bin;
    check("Bin: 5 → 0b101",     UIController::formatScalar(5.0),    "0b101");
    check("Bin: 10 → 0b1010",   UIController::formatScalar(10.0),   "0b1010");

    // Negatives carry a leading minus rather than two's complement
    // padding — easier to read for typical calculator use.
    MathStateMachine::numberBase = NumberBase::Hex;
    check("Hex: -255 → -0xFF",  UIController::formatScalar(-255.0), "-0xFF");

    // Non-integers fall back to the active Notation/Decimal formatter
    // (Normal + Float in this test environment).
    MathStateMachine::notation   = NumberNotation::Normal;
    MathStateMachine::fixDecimals = -1;
    check("Hex: 1.5 falls back to decimal",
          UIController::formatScalar(1.5), "1.5");
    check("Hex: 1÷3 falls back to decimal",
          eval(c, "1÷3"), UIController::formatScalar(1.0 / 3.0));

    // Integer results from the engine route through the same formatter
    // — end-to-end check that `2^8` displays as 0x100 when in Hex mode.
    check("Hex: 2^8 → 0x100", eval(c, "2^8"), "0x100");

    // Restore defaults so subsequent assertions see Dec.
    MathStateMachine::numberBase = NumberBase::Dec;
  }

  section("Lists (Phase C — Wave 1)");
  {
    // Fresh list registry so we don't inherit state from a prior run.
    tux_ti83::MathStateMachine::listRegistry.clear();

    // Literals + display.
    check("{1,2,3} literal", eval(c, "{1,2,3}"), "{1,2,3}");
    check("single-element {5}", eval(c, "{5}"), "{5}");

    // Element-wise arithmetic (equal length).
    check("{1,2,3}+{4,5,6} → {5,7,9}", eval(c, "{1,2,3}+{4,5,6}"), "{5,7,9}");
    check("{5,7,9}-{1,2,3} → {4,5,6}", eval(c, "{5,7,9}-{1,2,3}"), "{4,5,6}");
    check("{1,2,3}*{2,3,4} → {2,6,12}", eval(c, "{1,2,3}*{2,3,4}"), "{2,6,12}");
    check("{6,8,10}/{2,4,5} → {3,2,2}", eval(c, "{6,8,10}/{2,4,5}"), "{3,2,2}");
    check("{2,3,4}^{2,2,2} → {4,9,16}", eval(c, "{2,3,4}^{2,2,2}"), "{4,9,16}");

    // Scalar broadcast (both operand orders).
    check("{1,2,3}+10 → {11,12,13}", eval(c, "{1,2,3}+10"), "{11,12,13}");
    check("10+{1,2,3} → {11,12,13}", eval(c, "10+{1,2,3}"), "{11,12,13}");
    check("{1,2,3}*2 → {2,4,6}", eval(c, "{1,2,3}*2"), "{2,4,6}");
    check("2{1,2,3} implicit mul → {2,4,6}", eval(c, "2{1,2,3}"), "{2,4,6}");
    check("{1,2,3}^2 → {1,4,9}", eval(c, "{1,2,3}^2"), "{1,4,9}");

    // Error paths.
    check("{1,2}+{1,2,3} → ERR:INVALID DIM",
          eval(c, "{1,2}+{1,2,3}"), "ERR:INVALID DIM");
    check("{10,20}/{2,0} → ERR:DIVIDE BY 0",
          eval(c, "{10,20}/{2,0}"), "ERR:DIVIDE BY 0");

    // STO round-trip: list → L1, read back, list arithmetic on the ref.
    check("{1,2,3}→L1 returns the list", eval(c, "{1,2,3}→L1"), "{1,2,3}");
    check("L1 reads back {1,2,3}", eval(c, "L1"), "{1,2,3}");
    check("L1+L1 → {2,4,6}", eval(c, "L1+L1"), "{2,4,6}");
    check("2L1 → {2,4,6}", eval(c, "2L1"), "{2,4,6}");
    check("L1²=L1^2 → {1,4,9}", eval(c, "L1^2"), "{1,4,9}");
    // List → list store.
    check("L1→L2 returns {1,2,3}", eval(c, "L1→L2"), "{1,2,3}");
    check("L2 reads back {1,2,3}", eval(c, "L2"), "{1,2,3}");

    // Undefined list read.
    check("L5 undefined → ERR:UNDEFINED", eval(c, "L5"), "ERR:UNDEFINED");

    // Type mismatches.
    check("5→L1 (scalar to list) → ERR:DATA TYPE",
          eval(c, "5→L1"), "ERR:DATA TYPE");
    check("{1,2}→A (list to scalar) → ERR:DATA TYPE",
          eval(c, "{1,2}→A"), "ERR:DATA TYPE");
    check("sin({0,1}) → ERR:DATA TYPE", eval(c, "sin({0,1})"), "ERR:DATA TYPE");
    check("abs({1,2}) → ERR:DATA TYPE", eval(c, "abs({1,2})"), "ERR:DATA TYPE");
    check("{1,2}! → ERR:DATA TYPE", eval(c, "{1,2}!"), "ERR:DATA TYPE");
    check("min({1,2},3) → ERR:DATA TYPE",
          eval(c, "min({1,2},3)"), "ERR:DATA TYPE");

    // Ans carries a list result forward.
    check("{4,5,6} seeds Ans", eval(c, "{4,5,6}"), "{4,5,6}");
    check("Ans+1 → {5,6,7}", evalChained(c, "Ans+1"), "{5,6,7}");

    // Restore so later sections aren't surprised by leftover L1/L2.
    tux_ti83::MathStateMachine::listRegistry.clear();
  }

  section("List functions (Phase C — Wave 3)");
  {
    tux_ti83::MathStateMachine::listRegistry.clear();

    // Reductions on a literal.
    check("sum({1,2,3,4}) → 10", eval(c, "sum({1,2,3,4})"), "10");
    check("prod({1,2,3,4}) → 24", eval(c, "prod({1,2,3,4})"), "24");
    check("mean({1,2,3,4}) → 2.5", eval(c, "mean({1,2,3,4})"), "2.5");
    check("min({3,1,2}) → 1", eval(c, "min({3,1,2})"), "1");
    check("max({3,9,1}) → 9", eval(c, "max({3,9,1})"), "9");
    check("variance({1,2,3,4}) → 1.666666667",
          eval(c, "variance({1,2,3,4})"), "1.666666667");
    check("stdDev({1,2,3,4}) → 1.290994449",
          eval(c, "stdDev({1,2,3,4})"), "1.290994449");

    // Reductions on a stored list reference.
    check("{2,4,6}→L1", eval(c, "{2,4,6}→L1"), "{2,4,6}");
    check("sum(L1) → 12", eval(c, "sum(L1)"), "12");
    check("mean(L1) → 4", eval(c, "mean(L1)"), "4");

    // Composition + implicit-mul.
    check("sum({1,2,3})+sum({4,5,6}) → 21",
          eval(c, "sum({1,2,3})+sum({4,5,6})"), "21");
    check("2mean({1,2,3}) → 4", eval(c, "2mean({1,2,3})"), "4");

    // Overloads preserved: 4-arg calculus sum, and 2-scalar min/max.
    check("sum(X,X,1,4) still calculus → 10", eval(c, "sum(X,X,1,4)"), "10");
    check("prod(X,X,1,4) still calculus → 24", eval(c, "prod(X,X,1,4)"), "24");
    check("min(3,7) still binary → 3", eval(c, "min(3,7)"), "3");
    check("max(3,7) still binary → 7", eval(c, "max(3,7)"), "7");

    // Type / domain errors.
    check("sum(3) scalar arg → ERR:DATA TYPE", eval(c, "sum(3)"), "ERR:DATA TYPE");
    check("mean(5) scalar arg → ERR:DATA TYPE", eval(c, "mean(5)"), "ERR:DATA TYPE");
    check("stdDev({5}) n<2 → ERR:DOMAIN", eval(c, "stdDev({5})"), "ERR:DOMAIN");
    check("variance({5}) n<2 → ERR:DOMAIN",
          eval(c, "variance({5})"), "ERR:DOMAIN");

    tux_ti83::MathStateMachine::listRegistry.clear();
  }

  section("seq( and median( (Phase C — Wave 3b)");
  {
    tux_ti83::MathStateMachine::listRegistry.clear();

    // seq( — 4-arg (default step 1) and 5-arg (explicit step).
    check("seq(X,X,1,5) → {1,2,3,4,5}", eval(c, "seq(X,X,1,5)"), "{1,2,3,4,5}");
    check("seq(X^2,X,1,4) → {1,4,9,16}", eval(c, "seq(X^2,X,1,4)"), "{1,4,9,16}");
    check("seq(2X,X,1,3) → {2,4,6}", eval(c, "seq(2X,X,1,3)"), "{2,4,6}");
    check("seq(X,X,1,10,2) → {1,3,5,7,9}",
          eval(c, "seq(X,X,1,10,2)"), "{1,3,5,7,9}");
    check("seq(X,X,5,1,-1) → {5,4,3,2,1} (negative step)",
          eval(c, "seq(X,X,5,1,-1)"), "{5,4,3,2,1}");

    // seq( composes with the reductions — the authentic TI-83
    // sum(seq(...)) summation form.
    check("sum(seq(X^2,X,1,4)) → 30", eval(c, "sum(seq(X^2,X,1,4))"), "30");
    check("mean(seq(X,X,1,9)) → 5", eval(c, "mean(seq(X,X,1,9))"), "5");
    // seq result stores into a list slot.
    check("seq(X,X,1,4)→L1 → {1,2,3,4}", eval(c, "seq(X,X,1,4)→L1"), "{1,2,3,4}");
    check("L1 after seq store → {1,2,3,4}", eval(c, "L1"), "{1,2,3,4}");

    // seq( error paths.
    check("seq(X,X,5,1) backwards → ERR:INVALID DIM",
          eval(c, "seq(X,X,5,1)"), "ERR:INVALID DIM");
    check("seq(X,X,1,3,0) step 0 → ERR:DOMAIN",
          eval(c, "seq(X,X,1,3,0)"), "ERR:DOMAIN");

    // median( — odd and even length.
    check("median({3,1,2}) → 2", eval(c, "median({3,1,2})"), "2");
    check("median({1,2,3,4}) → 2.5", eval(c, "median({1,2,3,4})"), "2.5");
    check("median({7,7,7}) → 7", eval(c, "median({7,7,7})"), "7");

    tux_ti83::MathStateMachine::listRegistry.clear();
  }

  section("1-Var Stats (Phase C — Wave 4a)");
  {
    tux_ti83::MathStateMachine::listRegistry.clear();
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-6; };

    // Even n: {1..8}. sumX=36, sumX2=204, mean=4.5, Sx=√6, σx=√5.25,
    // Q1=2.5, Med=4.5, Q3=6.5.
    eval(c, "{1,2,3,4,5,6,7,8}→L1");
    QVariantMap s = c.oneVarStats("L1");
    checkTrue("1-var: no error", s["error"].toString().isEmpty());
    checkTrue("1-var: n = 8", s["n"].toInt() == 8);
    checkTrue("1-var: mean = 4.5", near(s["mean"].toDouble(), 4.5));
    checkTrue("1-var: sumX = 36", near(s["sumX"].toDouble(), 36.0));
    checkTrue("1-var: sumX2 = 204", near(s["sumX2"].toDouble(), 204.0));
    checkTrue("1-var: Sx = sqrt(6)", near(s["Sx"].toDouble(), std::sqrt(6.0)));
    checkTrue("1-var: sigmaX = sqrt(5.25)",
              near(s["sigmaX"].toDouble(), std::sqrt(5.25)));
    checkTrue("1-var: minX = 1", near(s["minX"].toDouble(), 1.0));
    checkTrue("1-var: Q1 = 2.5", near(s["Q1"].toDouble(), 2.5));
    checkTrue("1-var: median = 4.5", near(s["median"].toDouble(), 4.5));
    checkTrue("1-var: Q3 = 6.5", near(s["Q3"].toDouble(), 6.5));
    checkTrue("1-var: maxX = 8", near(s["maxX"].toDouble(), 8.0));

    // Odd n quartiles (median excluded from halves): {1..5} →
    // Q1=1.5, Med=3, Q3=4.5.
    eval(c, "{1,2,3,4,5}→L2");
    QVariantMap s2 = c.oneVarStats("L2");
    checkTrue("1-var odd: Q1 = 1.5", near(s2["Q1"].toDouble(), 1.5));
    checkTrue("1-var odd: median = 3", near(s2["median"].toDouble(), 3.0));
    checkTrue("1-var odd: Q3 = 4.5", near(s2["Q3"].toDouble(), 4.5));

    // Undefined list → error map.
    QVariantMap s3 = c.oneVarStats("L5");
    checkTrue("1-var undefined → error", !s3["error"].toString().isEmpty());

    tux_ti83::MathStateMachine::listRegistry.clear();
  }

  section("2-Var Stats + LinReg (Phase C — Wave 4b)");
  {
    tux_ti83::MathStateMachine::listRegistry.clear();
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-6; };

    // Perfectly linear: Y = 2X + 1 → a=2, b=1, r=1, r²=1.
    eval(c, "{1,2,3,4}→L1");
    eval(c, "{3,5,7,9}→L2");
    QVariantMap s = c.twoVarStats("L1", "L2");
    checkTrue("2-var: no error", s["error"].toString().isEmpty());
    checkTrue("2-var: n = 4", s["n"].toInt() == 4);
    checkTrue("2-var: meanX = 2.5", near(s["meanX"].toDouble(), 2.5));
    checkTrue("2-var: meanY = 6", near(s["meanY"].toDouble(), 6.0));
    checkTrue("2-var: sumXY = 70", near(s["sumXY"].toDouble(), 70.0));
    checkTrue("LinReg: a (slope) = 2", near(s["a"].toDouble(), 2.0));
    checkTrue("LinReg: b (intercept) = 1", near(s["b"].toDouble(), 1.0));
    checkTrue("LinReg: r = 1", near(s["r"].toDouble(), 1.0));
    checkTrue("LinReg: r² = 1", near(s["r2"].toDouble(), 1.0));

    // Non-perfect fit: X={1,2,3,4,5}, Y={2,4,5,4,5} → a=0.6, b=2.2,
    // r²=0.6.
    eval(c, "{1,2,3,4,5}→L3");
    eval(c, "{2,4,5,4,5}→L4");
    QVariantMap s2 = c.twoVarStats("L3", "L4");
    checkTrue("LinReg2: a = 0.6", near(s2["a"].toDouble(), 0.6));
    checkTrue("LinReg2: b = 2.2", near(s2["b"].toDouble(), 2.2));
    checkTrue("LinReg2: r² = 0.6", near(s2["r2"].toDouble(), 0.6));

    // Dimension mismatch.
    eval(c, "{1,2}→L5");
    eval(c, "{1,2,3}→L6");
    QVariantMap sd = c.twoVarStats("L5", "L6");
    checkTrue("2-var dim mismatch → DIM", sd["error"].toString() == "DIM");

    // Undefined lists.
    tux_ti83::MathStateMachine::listRegistry.clear();
    QVariantMap su = c.twoVarStats("L1", "L2");
    checkTrue("2-var undefined → error", !su["error"].toString().isEmpty());

    // Degenerate X (no spread) → regression fields absent, stats present.
    eval(c, "{2,2,2}→L1");
    eval(c, "{1,2,3}→L2");
    QVariantMap sg = c.twoVarStats("L1", "L2");
    checkTrue("2-var degenerate: stats present", sg["n"].toInt() == 3);
    checkTrue("2-var degenerate: no slope", !sg.contains("a"));

    tux_ti83::MathStateMachine::listRegistry.clear();
  }

  section("Regression models (Phase C — Wave 4c)");
  {
    tux_ti83::MathStateMachine::listRegistry.clear();
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-5; };

    // QuadReg: y = 2x² + 3x + 1 (exact → R²=1).
    c.updateList("L1", QVariantList{0, 1, 2, 3});
    c.updateList("L2", QVariantList{1, 6, 15, 28});
    QVariantMap q = c.regression("quad", "L1", "L2");
    checkTrue("QuadReg a = 2", near(q["a"].toDouble(), 2.0));
    checkTrue("QuadReg b = 3", near(q["b"].toDouble(), 3.0));
    checkTrue("QuadReg c = 1", near(q["c"].toDouble(), 1.0));
    checkTrue("QuadReg R² = 1", near(q["r2"].toDouble(), 1.0));

    // CubicReg: y = x³ - 2x² + x + 5 (exact).
    c.updateList("L1", QVariantList{0, 1, 2, 3, 4});
    c.updateList("L2", QVariantList{5, 5, 7, 17, 41});
    QVariantMap cu = c.regression("cubic", "L1", "L2");
    checkTrue("CubicReg a = 1", near(cu["a"].toDouble(), 1.0));
    checkTrue("CubicReg b = -2", near(cu["b"].toDouble(), -2.0));
    checkTrue("CubicReg c = 1", near(cu["c"].toDouble(), 1.0));
    checkTrue("CubicReg d = 5", near(cu["d"].toDouble(), 5.0));
    checkTrue("CubicReg R² = 1", near(cu["r2"].toDouble(), 1.0));

    // ExpReg: y = 3·2ˣ.
    c.updateList("L1", QVariantList{0, 1, 2, 3});
    c.updateList("L2", QVariantList{3, 6, 12, 24});
    QVariantMap e = c.regression("exp", "L1", "L2");
    checkTrue("ExpReg a = 3", near(e["a"].toDouble(), 3.0));
    checkTrue("ExpReg b = 2", near(e["b"].toDouble(), 2.0));
    checkTrue("ExpReg r = 1", near(e["r"].toDouble(), 1.0));

    // PwrReg: y = 2·x³.
    c.updateList("L1", QVariantList{1, 2, 3, 4});
    c.updateList("L2", QVariantList{2, 16, 54, 128});
    QVariantMap p = c.regression("pwr", "L1", "L2");
    checkTrue("PwrReg a = 2", near(p["a"].toDouble(), 2.0));
    checkTrue("PwrReg b = 3", near(p["b"].toDouble(), 3.0));
    checkTrue("PwrReg r = 1", near(p["r"].toDouble(), 1.0));

    // LnReg: y = 2 + 3·ln x.
    c.updateList("L1", QVariantList{1, 2, 3, 4});
    c.updateList("L2", QVariantList{2.0, 2.0 + 3.0 * std::log(2.0),
                                    2.0 + 3.0 * std::log(3.0),
                                    2.0 + 3.0 * std::log(4.0)});
    QVariantMap l = c.regression("ln", "L1", "L2");
    checkTrue("LnReg a = 2", near(l["a"].toDouble(), 2.0));
    checkTrue("LnReg b = 3", near(l["b"].toDouble(), 3.0));
    checkTrue("LnReg r = 1", near(l["r"].toDouble(), 1.0));

    // Domain error: ExpReg needs positive Y.
    c.updateList("L1", QVariantList{1, 2, 3});
    c.updateList("L2", QVariantList{1, -2, 3});
    QVariantMap ed = c.regression("exp", "L1", "L2");
    checkTrue("ExpReg non-positive Y → DOMAIN",
              ed["error"].toString() == "DOMAIN");

    // Dimension mismatch.
    c.updateList("L1", QVariantList{1, 2});
    c.updateList("L2", QVariantList{1, 2, 3});
    QVariantMap dm = c.regression("quad", "L1", "L2");
    checkTrue("reg dim mismatch → DIM", dm["error"].toString() == "DIM");

    tux_ti83::MathStateMachine::listRegistry.clear();
  }

  section("Random functions (Phase C — Wave 5)");
  {
    // Determinism: reseeding reproduces the sequence exactly.
    tux_ti83::MathStateMachine::seedRandom(12345);
    QString r1 = eval(c, "rand");
    QString r2 = eval(c, "randInt(1,6)");
    tux_ti83::MathStateMachine::seedRandom(12345);
    checkTrue("rand deterministic under seed", eval(c, "rand") == r1);
    checkTrue("randInt deterministic under seed", eval(c, "randInt(1,6)") == r2);

    // rand ∈ [0, 1) over many draws.
    bool randOk = true;
    for (int i = 0; i < 200; ++i) {
      double v = eval(c, "rand").toDouble();
      if (v < 0.0 || v >= 1.0) randOk = false;
    }
    checkTrue("rand ∈ [0,1)", randOk);

    // randInt(1,6) ∈ {1..6}, integer.
    bool diceOk = true;
    for (int i = 0; i < 200; ++i) {
      double v = eval(c, "randInt(1,6)").toDouble();
      if (v < 1.0 || v > 6.0 || v != std::floor(v)) diceOk = false;
    }
    checkTrue("randInt(1,6) ∈ [1,6] integers", diceOk);

    // randBin(10,0.5) ∈ [0,10], integer.
    bool binOk = true;
    for (int i = 0; i < 100; ++i) {
      double v = eval(c, "randBin(10,0.5)").toDouble();
      if (v < 0.0 || v > 10.0 || v != std::floor(v)) binOk = false;
    }
    checkTrue("randBin(10,0.5) ∈ [0,10]", binOk);

    // randNorm returns a finite scalar.
    checkTrue("randNorm(5,2) finite",
              std::isfinite(eval(c, "randNorm(5,2)").toDouble()));

    // List form: randInt(1,6,5) → a 5-element list.
    QString lst = eval(c, "randInt(1,6,5)");
    checkTrue("randInt list form is a list",
              lst.startsWith("{") && lst.endsWith("}"));
    checkTrue("randInt(1,6,5) has 5 elements", lst.count(',') == 4);

    // Composition with list reductions — all-1 list has mean 1.
    checkTrue("mean(randInt(1,1,10)) = 1",
              std::abs(eval(c, "mean(randInt(1,1,10))").toDouble() - 1.0) < 1e-9);

    // Domain errors.
    check("randInt(5,1) lo>hi → ERR:DOMAIN", eval(c, "randInt(5,1)"), "ERR:DOMAIN");
    check("randNorm(0,-1) sd≤0 → ERR:DOMAIN", eval(c, "randNorm(0,-1)"), "ERR:DOMAIN");
    check("randBin(-1,0.5) → ERR:DOMAIN", eval(c, "randBin(-1,0.5)"), "ERR:DOMAIN");
    check("randInt(1,6,0) count<1 → ERR:DOMAIN", eval(c, "randInt(1,6,0)"), "ERR:DOMAIN");
  }

  section("Stat plots (Phase C — Wave 5b)");
  {
    tux_ti83::MathStateMachine::listRegistry.clear();
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-9; };

    c.updateList("L1", QVariantList{3, 1, 2});
    c.updateList("L2", QVariantList{30, 10, 20});
    c.setProperty("statPlotOn", true);
    c.setProperty("statPlotXList", "L1");
    c.setProperty("statPlotYList", "L2");

    // Scatter — points in list order.
    c.setProperty("statPlotType", 0);
    QVariantMap sc = c.getStatPlotData();
    checkTrue("scatter: on + no error",
              sc["on"].toBool() && sc["error"].toString().isEmpty());
    QVariantList pts = sc["points"].toList();
    checkTrue("scatter: 3 points", pts.size() == 3);
    checkTrue("scatter: first point (3,30)",
              near(pts[0].toMap()["x"].toDouble(), 3.0) &&
              near(pts[0].toMap()["y"].toDouble(), 30.0));

    // xyLine — points sorted by x.
    c.setProperty("statPlotType", 1);
    QVariantList xy = c.getStatPlotData()["points"].toList();
    checkTrue("xyLine: sorted first x = 1",
              near(xy[0].toMap()["x"].toDouble(), 1.0));
    checkTrue("xyLine: sorted last x = 3",
              near(xy[2].toMap()["x"].toDouble(), 3.0));

    // Histogram — bin counts sum to n.
    c.setProperty("statPlotType", 2);
    QVariantList bins = c.getStatPlotData()["bins"].toList();
    int total = 0;
    for (const auto &b : bins) total += b.toMap()["count"].toInt();
    checkTrue("hist: counts sum to n=3", total == 3);

    // Box — five-number summary of {1,2,3}.
    c.setProperty("statPlotType", 3);
    QVariantMap bx = c.getStatPlotData()["box"].toMap();
    checkTrue("box: min = 1", near(bx["min"].toDouble(), 1.0));
    checkTrue("box: med = 2", near(bx["med"].toDouble(), 2.0));
    checkTrue("box: max = 3", near(bx["max"].toDouble(), 3.0));

    // Scatter with mismatched lists → DIM.
    c.updateList("L2", QVariantList{1, 2});
    c.setProperty("statPlotType", 0);
    checkTrue("scatter dim mismatch → DIM",
              c.getStatPlotData()["error"].toString() == "DIM");

    // Off → on = false, no error.
    c.setProperty("statPlotOn", false);
    checkTrue("plot off → on=false", !c.getStatPlotData()["on"].toBool());

    tux_ti83::MathStateMachine::listRegistry.clear();
  }

  section("Empty input");
  check("empty expression → ERR:SYNTAX", eval(c, ""), "ERR:SYNTAX");

  std::cout << "\n----------------------------------------\n"
            << "Total: " << (gPassed + gFailed) << "  Passed: " << gPassed
            << "  Failed: " << gFailed << '\n';
  return gFailed == 0 ? 0 : 1;
}

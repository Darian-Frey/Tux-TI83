// Tux-TI83 math regression tests.
//
// Drives UIController programmatically — same code path as the GUI and
// the CLI. Each test feeds an expression string, presses ENTER, and
// asserts on the resulting display string. Plain assertions; no
// external test framework. Exit code: 0 on full pass, 1 on any failure.

#include "ui_controller.hpp"
#include "interpreter.hpp"
#include <QCoreApplication>
#include <QDir>
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

  // Isolate persistence BEFORE constructing any UIController. The interpreter
  // tests call saveProgram/deleteProgram, which call saveState() → the on-disk
  // state file (resolved from XDG_STATE_HOME). Point that at a throwaway dir so
  // running the suite never overwrites the user's real ~/.local/state — which
  // it used to, silently wiping saved TI-BASIC programs (BUG-024).
  const QByteArray testStateHome =
      (QDir::tempPath() + QStringLiteral("/tux-ti83-test-state")).toUtf8();
  qputenv("XDG_STATE_HOME", testStateHome);
  QDir().mkpath(QString::fromUtf8(testStateHome) + "/tux-ti83");

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

  section("Matrix/List toolkit (Phase F follow-up)");
  {
    // identity(n) — scalar → n×n identity matrix.
    check("identity(3)", eval(c, "identity(3)"), "[[1,0,0][0,1,0][0,0,1]]");
    check("identity(1)", eval(c, "identity(1)"), "[[1]]");
    check("identity(0) → DOMAIN", eval(c, "identity(0)"), "ERR:DOMAIN");
    check("identity(2.5) → DOMAIN", eval(c, "identity(2.5)"), "ERR:DOMAIN");

    // dim( — matrix → {rows,cols} list; list → length scalar.
    c.updateMatrix("[A]", 2, 3, QVariantList{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
    check("dim(2×3 matrix) = {2,3}", eval(c, "dim([A])"), "{2,3}");
    check("dim(list) = length", eval(c, "dim({4,5,6,7})"), "4");
    check("dim(scalar) → type error", eval(c, "dim(5)"), "ERR:DATA TYPE");

    // ref( — row-echelon: leading 1s, upper-triangular, no back-elim.
    c.updateMatrix("[A]", 3, 3,
                   QVariantList{2.0, 4.0, 6.0, 1.0, 1.0, 1.0, 0.0, 0.0, 5.0});
    check("ref([[2,4,6][1,1,1][0,0,5]]) = [[1,2,3][0,1,2][0,0,1]]",
          eval(c, "ref([A])"), "[[1,2,3][0,1,2][0,0,1]]");
    // Rank-deficient 3×2 → leading 1 then zero rows below.
    c.updateMatrix("[A]", 3, 2,
                   QVariantList{1.0, 2.0, 2.0, 4.0, 0.0, 0.0});
    check("ref of rank-1 3×2 → [[1,2][0,0][0,0]]",
          eval(c, "ref([A])"), "[[1,2][0,0][0,0]]");
    check("ref(scalar) → type error", eval(c, "ref(5)"), "ERR:DATA TYPE");

    // augment( — matrix‖matrix (equal rows) or list‖list.
    c.updateMatrix("[A]", 2, 2, QVariantList{1.0, 2.0, 3.0, 4.0});
    c.updateMatrix("[B]", 2, 2, QVariantList{5.0, 6.0, 7.0, 8.0});
    check("augment(2×2,2×2) → 2×4",
          eval(c, "augment([A],[B])"), "[[1,2,5,6][3,4,7,8]]");
    c.updateMatrix("[B]", 3, 2, QVariantList{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
    check("augment row mismatch → INVALID DIM",
          eval(c, "augment([A],[B])"), "ERR:INVALID DIM");
    check("augment(list,list) concatenates",
          eval(c, "augment({1,2},{3,4,5})"), "{1,2,3,4,5}");
    check("augment(matrix,list) → type error",
          eval(c, "augment([A],{1,2})"), "ERR:DATA TYPE");

    // randM(r,c) — r×c matrix of random ints in [-9,9].
    tux_ti83::MathStateMachine::seedRandom(777);
    QString rm = eval(c, "randM(2,3)");
    tux_ti83::MathStateMachine::seedRandom(777);
    checkTrue("randM deterministic under seed", eval(c, "randM(2,3)") == rm);
    check("dim(randM(4,2)) = {4,2}", eval(c, "dim(randM(4,2))"), "{4,2}");
    check("randM(0,2) → DOMAIN", eval(c, "randM(0,2)"), "ERR:DOMAIN");
    check("randM(2,2.5) → DOMAIN", eval(c, "randM(2,2.5)"), "ERR:DOMAIN");
    // Every entry is an integer within [-9,9] over many draws.
    bool rmOk = true;
    for (int i = 0; i < 50 && rmOk; ++i) {
      c.updateMatrix("[A]", 1, 1, QVariantList{0.0});  // reset target
      QString s = eval(c, "randM(3,3)");
      // Normalise the [[..][..]] grid to comma-separated numbers: turn the
      // "][" row boundary into a comma first, then drop the outer brackets.
      QString nums = s;
      nums.replace("][", ",").remove('[').remove(']');
      for (const QString &p : nums.split(',', Qt::SkipEmptyParts)) {
        double v = p.toDouble();
        if (v < -9.0 || v > 9.0 || v != std::floor(v)) rmOk = false;
      }
    }
    checkTrue("randM entries are integers in [-9,9]", rmOk);

    // List▶Matr(a,b) — two equal-length lists → n×2 matrix (columns).
    check("List▶Matr({1,2,3},{4,5,6}) → [[1,4][2,5][3,6]]",
          eval(c, "List▶Matr({1,2,3},{4,5,6})"), "[[1,4][2,5][3,6]]");
    check("List▶Matr length mismatch → INVALID DIM",
          eval(c, "List▶Matr({1,2},{3,4,5})"), "ERR:INVALID DIM");
    check("List▶Matr with a scalar arg → type error",
          eval(c, "List▶Matr({1,2},5)"), "ERR:DATA TYPE");

    // Matr▶List([A], col) — 1-based column → list.
    c.updateMatrix("[A]", 3, 2,
                   QVariantList{1.0, 4.0, 2.0, 5.0, 3.0, 6.0});
    check("Matr▶List([A],1) → {1,2,3}", eval(c, "Matr▶List([A],1)"), "{1,2,3}");
    check("Matr▶List([A],2) → {4,5,6}", eval(c, "Matr▶List([A],2)"), "{4,5,6}");
    check("Matr▶List column out of range → INVALID DIM",
          eval(c, "Matr▶List([A],3)"), "ERR:INVALID DIM");
    check("Matr▶List(scalar,1) → type error",
          eval(c, "Matr▶List(5,1)"), "ERR:DATA TYPE");

    // Round trip: build a matrix from two lists, pull a column back out.
    check("round trip: Matr▶List(List▶Matr(...),1) → {1,2,3}",
          eval(c, "Matr▶List(List▶Matr({1,2,3},{4,5,6}),1)"), "{1,2,3}");
    check("round trip: column 2 → {4,5,6}",
          eval(c, "Matr▶List(List▶Matr({1,2,3},{4,5,6}),2)"), "{4,5,6}");

    // Variadic List▶Matr: n lists → n columns (not just 2).
    check("List▶Matr 3 lists → 2×3",
          eval(c, "List▶Matr({1,2},{3,4},{5,6})"), "[[1,3,5][2,4,6]]");
    check("List▶Matr 4 single-element lists → 1×4",
          eval(c, "List▶Matr({1},{2},{3},{4})"), "[[1,2,3,4]]");
    check("List▶Matr one list → m×1 column vector",
          eval(c, "List▶Matr({7,8,9})"), "[[7][8][9]]");
    check("List▶Matr mismatched length among 3 → INVALID DIM",
          eval(c, "List▶Matr({1,2},{3,4},{5,6,7})"), "ERR:INVALID DIM");
    check("List▶Matr scalar among 3 args → type error",
          eval(c, "List▶Matr({1,2},9,{5,6})"), "ERR:DATA TYPE");
    // Extract the middle column of a 3-column build.
    check("Matr▶List(3-col build, 2) → {3,4}",
          eval(c, "Matr▶List(List▶Matr({1,2},{3,4},{5,6}),2)"), "{3,4}");
  }

  section("Matrix literals + store (IMP-011)");
  {
    // Typed matrix literals `[[..][..]]` — rows are `[…]`, elements
    // comma-separated. Row adjacency `][` must NOT get an implicit ×.
    check("2×2 literal", eval(c, "[[1,2][3,4]]"), "[[1,2][3,4]]");
    check("2×3 literal", eval(c, "[[1,2,3][4,5,6]]"), "[[1,2,3][4,5,6]]");
    check("column vector 3×1", eval(c, "[[1][2][3]]"), "[[1][2][3]]");
    check("row vector 1×3", eval(c, "[[7,8,9]]"), "[[7,8,9]]");
    check("expression elements", eval(c, "[[1+1,2*3][4,5]]"), "[[2,6][4,5]]");
    check("ragged rows → INVALID DIM", eval(c, "[[1,2][3]]"), "ERR:INVALID DIM");

    // Store a literal into [A], then operate on it (the IMP-011 gap).
    check("store literal → [A] echoes it",
          eval(c, "[[1,2][3,4]]→[A]"), "[[1,2][3,4]]");
    check("stored [A]^-1", eval(c, "[A]^-1"), "[[-2,1][1.5,-0.5]]");
    check("stored det([A])", eval(c, "det([A])"), "-2");
    check("stored [A]*[A]", eval(c, "[A]*[A]"), "[[7,10][15,22]]");

    // Store a *computed* matrix result into another register.
    eval(c, "[[1,2][3,4]]→[A]");   // reset [A]
    eval(c, "[A]+[A]→[B]");
    check("stored [B] = 2[A]", eval(c, "[B]"), "[[2,4][6,8]]");

    // Type mismatch: a scalar can't be stored into a matrix register.
    check("5→[A] → type error", eval(c, "5→[A]"), "ERR:DATA TYPE");

    // GUI bracket-entry: build [[1,2][3,4]] one token at a time via
    // processInput (the on-screen keypad / physical-keyboard path routes
    // each key through processInput, not the string tokeniser).
    c.processInput(QStringLiteral("CLEAR"));
    const char *seq[] = {"[", "[", "1", ",", "2", "]",
                         "[", "3", ",", "4", "]", "]"};
    for (auto s : seq) c.processInput(QString::fromUtf8(s));
    c.processInput(QStringLiteral("ENTER"));
    check("keypad-built [[1,2][3,4]] evaluates",
          c.currentDisplay(), "[[1,2][3,4]]");

    // Restore [A] to the value later matrix sections assume.
    c.updateMatrix("[A]", 2, 2, QVariantList{1.0, 2.0, 3.0, 4.0});
  }

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

  section("Distributions — normal family (Phase C follow-on)");
  {
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-6; };

    // normalpdf — default (0,1) and explicit args agree.
    checkTrue("normalpdf(0) = 1/√(2π)",
              near(eval(c, "normalpdf(0)").toDouble(), 0.39894228040));
    checkTrue("normalpdf default = explicit",
              eval(c, "normalpdf(0)") == eval(c, "normalpdf(0,0,1)"));
    checkTrue("normalpdf(1,0,2) ≈ 0.17603",
              near(eval(c, "normalpdf(1,0,2)").toDouble(), 0.17603266338));

    // normalcdf — 2-arg (defaults) and 4-arg forms.
    checkTrue("normalcdf(-1,1) ≈ 0.6826895",
              near(eval(c, "normalcdf(-1,1)").toDouble(), 0.68268949));
    checkTrue("normalcdf(-1.96,1.96) ≈ 0.95",
              near(eval(c, "normalcdf(-1.96,1.96)").toDouble(), 0.95000421));
    checkTrue("normalcdf(0,1) ≈ 0.3413447",
              near(eval(c, "normalcdf(0,1)").toDouble(), 0.34134475));

    // invNorm — quantiles.
    checkTrue("invNorm(0.975) ≈ 1.959964",
              near(eval(c, "invNorm(0.975)").toDouble(), 1.95996399));
    checkTrue("invNorm(0.5) = 0", near(eval(c, "invNorm(0.5)").toDouble(), 0.0));
    checkTrue("invNorm(0.9,100,15) ≈ 119.223",
              near(eval(c, "invNorm(0.9,100,15)").toDouble(), 119.2232735));

    // Nested calls round-trip (padding recurses into arguments).
    checkTrue("normalcdf(-50, invNorm(0.9)) ≈ 0.9",
              near(eval(c, "normalcdf(-50,invNorm(0.9))").toDouble(), 0.9));

    // Domain errors.
    check("normalpdf σ≤0 → DOMAIN", eval(c, "normalpdf(0,0,-1)"), "ERR:DOMAIN");
    check("normalcdf σ≤0 → DOMAIN", eval(c, "normalcdf(0,1,0,0)"), "ERR:DOMAIN");
    check("invNorm(1) → DOMAIN", eval(c, "invNorm(1)"), "ERR:DOMAIN");
    check("invNorm(0) → DOMAIN", eval(c, "invNorm(0)"), "ERR:DOMAIN");
  }

  section("Distributions — discrete (Phase C follow-on)");
  {
    tux_ti83::MathStateMachine::listRegistry.clear();
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-6; };

    // Binomial — scalar pdf/cdf.
    checkTrue("binompdf(10,0.5,5) = 0.24609375",
              near(eval(c, "binompdf(10,0.5,5)").toDouble(), 0.24609375));
    checkTrue("binomcdf(10,0.5,5) = 0.623046875",
              near(eval(c, "binomcdf(10,0.5,5)").toDouble(), 0.623046875));
    checkTrue("binompdf(5,0.5,10) = 0 (x>n)",
              near(eval(c, "binompdf(5,0.5,10)").toDouble(), 0.0));
    // Binomial — 2-arg list form: 11 values summing to 1.
    checkTrue("sum(binompdf(10,0.5)) = 1",
              near(eval(c, "sum(binompdf(10,0.5))").toDouble(), 1.0));
    checkTrue("binomcdf(10,0.5) last element = 1",
              near(eval(c, "max(binomcdf(10,0.5))").toDouble(), 1.0));

    // Poisson.
    checkTrue("poissonpdf(3,2) ≈ 0.2240418",
              near(eval(c, "poissonpdf(3,2)").toDouble(), 0.22404180));
    checkTrue("poissoncdf(3,2) ≈ 0.4231901",
              near(eval(c, "poissoncdf(3,2)").toDouble(), 0.42319008));

    // Geometric.
    checkTrue("geometpdf(0.5,3) = 0.125",
              near(eval(c, "geometpdf(0.5,3)").toDouble(), 0.125));
    checkTrue("geometcdf(0.5,3) = 0.875",
              near(eval(c, "geometcdf(0.5,3)").toDouble(), 0.875));

    // Domain errors.
    check("binompdf p>1 → DOMAIN", eval(c, "binompdf(10,1.5,3)"), "ERR:DOMAIN");
    check("poissonpdf μ<0 → DOMAIN", eval(c, "poissonpdf(-1,2)"), "ERR:DOMAIN");
    check("geometpdf p=0 → DOMAIN", eval(c, "geometpdf(0,3)"), "ERR:DOMAIN");

    tux_ti83::MathStateMachine::listRegistry.clear();
  }

  section("Distributions — continuous t/χ²/F (Phase C follow-on)");
  {
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-5; };

    // Student's t.
    checkTrue("tpdf(0,1) = 1/π",
              near(eval(c, "tpdf(0,1)").toDouble(), 0.31830988618));
    checkTrue("tcdf(-1e6,0,5) = 0.5 (median)",
              near(eval(c, "tcdf(-1000000,0,5)").toDouble(), 0.5));
    checkTrue("tcdf(-1e3,1.8125,10) ≈ 0.95",
              near(eval(c, "tcdf(-1000,1.8125,10)").toDouble(), 0.95000317));

    // Chi-square (df=2 is Exponential(1/2)).
    checkTrue("χ²pdf(2,2) = 0.5·e⁻¹",
              near(eval(c, "chi2pdf(2,2)").toDouble(), 0.18393972059));
    checkTrue("χ²cdf(0,2,2) = 1−e⁻¹",
              near(eval(c, "chi2cdf(0,2,2)").toDouble(), 0.63212055883));
    // Unicode token also tokenises.
    checkTrue("χ²cdf unicode token → ~1",
              near(eval(c, QString::fromUtf8("χ²cdf(0,1000000,5)")).toDouble(),
                   1.0));

    // F distribution — F(5,5) has median 1, so Fcdf(0,1,5,5)=0.5.
    checkTrue("Fcdf(0,1,5,5) = 0.5",
              near(eval(c, "Fcdf(0,1,5,5)").toDouble(), 0.5));
    checkTrue("Fpdf(1,5,5) ≈ 0.4244132",
              near(eval(c, "Fpdf(1,5,5)").toDouble(), 0.42441318));

    // Domain errors (df params must be > 0).
    check("tpdf ν≤0 → DOMAIN", eval(c, "tpdf(0,0)"), "ERR:DOMAIN");
    check("χ²pdf k≤0 → DOMAIN", eval(c, "chi2pdf(1,-1)"), "ERR:DOMAIN");
    check("Fcdf d1≤0 → DOMAIN", eval(c, "Fcdf(0,2,0,5)"), "ERR:DOMAIN");
  }

  section("MEM menu — memInfo + targeted clears");
  {
    tux_ti83::MathStateMachine::listRegistry.clear();
    tux_ti83::MathStateMachine::matrixRegistry.clear();
    tux_ti83::MathStateMachine::varRegistry.fill(0.0);

    eval(c, "5→A");
    c.updateMatrix("[A]", 2, 2, QVariantList{1, 2, 3, 4});
    c.updateList("L1", QVariantList{1, 2, 3});

    QVariantMap mi = c.memInfo();
    checkTrue("memInfo: 1 var defined", mi["vars"].toInt() == 1);
    checkTrue("memInfo: 1 matrix", mi["matrices"].toInt() == 1);
    checkTrue("memInfo: 1 list", mi["lists"].toInt() == 1);

    c.clearAllLists();
    checkTrue("clearAllLists → 0 lists", c.memInfo()["lists"].toInt() == 0);
    c.clearAllMatrices();
    checkTrue("clearAllMatrices → 0 matrices",
              c.memInfo()["matrices"].toInt() == 0);
    c.clearAllVars();
    checkTrue("clearAllVars → 0 vars", c.memInfo()["vars"].toInt() == 0);
    c.clearEntries();
    checkTrue("clearEntries → 0 entries", c.memInfo()["entries"].toInt() == 0);

    tux_ti83::MathStateMachine::listRegistry.clear();
    tux_ti83::MathStateMachine::matrixRegistry.clear();
    tux_ti83::MathStateMachine::varRegistry.fill(0.0);
  }

  section("Zoom menu completion (Phase D)");
  {
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-6; };
    auto X0 = [&]() { return c.property("xMin").toDouble(); };
    auto X1 = [&]() { return c.property("xMax").toDouble(); };
    auto Y0 = [&]() { return c.property("yMin").toDouble(); };
    auto Y1 = [&]() { return c.property("yMax").toDouble(); };

    // ZBox normalises its corners into the new window.
    c.resetViewport();
    c.zoomBox(3, 4, 1, 2);
    checkTrue("zoomBox → [1,3]×[2,4]",
              near(X0(), 1) && near(X1(), 3) && near(Y0(), 2) && near(Y1(), 4));

    // ZoomPrevious restores the pre-box window (standard −10..10).
    c.zoomPrevious();
    checkTrue("zoomPrevious → back to standard",
              near(X0(), -10) && near(X1(), 10) && near(Y0(), -10) && near(Y1(), 10));

    // ZoomMemory: store a window, change it, recall it.
    c.resetViewport();
    c.zoomBox(0, 0, 5, 5);
    c.zoomStore();
    c.resetViewport();
    c.zoomRecall();
    checkTrue("zoomRecall → stored [0,5]×[0,5]",
              near(X0(), 0) && near(X1(), 5) && near(Y0(), 0) && near(Y1(), 5));

    // ZoomStat fits Xlist (x) / Ylist (y) with 10% padding.
    tux_ti83::MathStateMachine::listRegistry.clear();
    c.updateList("L1", QVariantList{0, 10});
    c.updateList("L2", QVariantList{0, 20});
    c.setProperty("statPlotType", 0);
    c.setProperty("statPlotXList", "L1");
    c.setProperty("statPlotYList", "L2");
    c.zoomStat();
    checkTrue("zoomStat x → [-1,11]", near(X0(), -1) && near(X1(), 11));
    checkTrue("zoomStat y → [-2,22]", near(Y0(), -2) && near(Y1(), 22));

    c.resetViewport();
    tux_ti83::MathStateMachine::listRegistry.clear();
  }

  section("Y-editor: 10 functions + on/off (Phase D)");
  {
    checkTrue("functionCount == 10", c.functionCount() == 10);
    checkTrue("all slots default enabled",
              c.functionEnabled(0) && c.functionEnabled(9));
    checkTrue("default style thin (0)", c.functionStyle(4) == 0);

    // Define Y1 and Y8 (index 7), disable Y1: only Y8 should plot.
    c.setActiveFunction(0); eval(c, "X");
    c.setActiveFunction(7); eval(c, "2X");
    // functionBufferText reflects the plotted BUFFER (drives the trace
    // ExprOn overlay), not the live edit string that eval leaves behind.
    checkTrue("functionBufferText(0) == \"X\"",
              c.functionBufferText(0) == QStringLiteral("X"));
    checkTrue("functionBufferText(7) == \"2X\"",
              c.functionBufferText(7) == QStringLiteral("2X"));
    c.toggleFunctionEnabled(0);  // Y1 off
    QVariantList pts = c.getMultiGraphPoints(10);
    checkTrue("10 slot-lists returned", pts.size() == 10);
    checkTrue("Y1 disabled → empty list", pts[0].toList().isEmpty());
    checkTrue("Y8 enabled+defined → has points", !pts[7].toList().isEmpty());
    c.toggleFunctionEnabled(0);  // back on
    checkTrue("Y1 re-enabled → has points",
              !c.getMultiGraphPoints(10)[0].toList().isEmpty());

    // Style cycles thin → thick → dotted → thin.
    c.cycleFunctionStyle(2);
    checkTrue("style cycles to thick(1)", c.functionStyle(2) == 1);
    c.cycleFunctionStyle(2); c.cycleFunctionStyle(2);
    checkTrue("style wraps back to thin(0)", c.functionStyle(2) == 0);

    // Clean up the buffers we defined.
    c.setActiveFunction(0); c.processInput("CLEAR");
    c.setActiveFunction(7); c.processInput("CLEAR");
    c.setActiveFunction(0);
  }

  section("Xres graph resolution (Phase D)");
  {
    c.setProperty("graphMode", 0);          // Func mode
    c.setActiveFunction(0); eval(c, "X");   // Y1 = X, defined across [-10,10]

    c.setProperty("xres", 1);
    int n1 = c.getMultiGraphPoints(600)[0].toList().size();
    c.setProperty("xres", 4);
    int n4 = c.getMultiGraphPoints(600)[0].toList().size();
    c.setProperty("xres", 8);
    int n8 = c.getMultiGraphPoints(600)[0].toList().size();

    checkTrue("Xres=1 samples finely (~601 points)", n1 > 500);
    checkTrue("Xres=4 yields ~1/4 the points", n4 < n1 && std::abs(n4 - n1 / 4) < 25);
    checkTrue("Xres=8 is coarsest (fewest points)", n8 < n4);

    // Out-of-band Xres is clamped to [1,8] by the sweep, never zero-divides.
    c.setProperty("xres", 0);
    checkTrue("Xres=0 clamped → still samples",
              !c.getMultiGraphPoints(600)[0].toList().isEmpty());

    c.setProperty("xres", 1);
    c.setActiveFunction(0); c.processInput("CLEAR");
  }

  section("Y-VARS store (expr→Yn)");
  {
    // Dedicated controller for a self-contained Y-buffer fixture. (Since
    // IMP-045, yLookup is bound per engine, so this no longer has to be
    // the last-constructed controller — it's just tidy isolation.)
    UIController ys;
    ys.setProperty("graphMode", 0);
    ys.setActiveFunction(0);  // Y1 is the active/home slot

    // Store to a non-active slot: X²→Y2. Reports Done; Y2 now holds X².
    check("X^2→Y2 reports Done", eval(ys, "X^2→Y2"), "Done");
    checkTrue("Y2 buffer holds X^2",
              ys.functionBufferText(1) == QStringLiteral("X^2"));
    checkTrue("Y2 plots after store",
              !ys.getMultiGraphPoints(10)[1].toList().isEmpty());

    // Stored function recalls and composes: Y2(3) → 9.
    ys.setActiveFunction(0);
    check("stored Y2 recall: Y2(3) = 9", eval(ys, "Y2(3)"), "9");

    // Store into the active slot: 5→Y1 echoes the stored expression (its
    // buffer and the home line are the same storage in this app).
    ys.setActiveFunction(0);
    check("5→Y1 (active slot) echoes expr", eval(ys, "5→Y1"), "5");
    checkTrue("Y1 buffer holds 5", ys.functionBufferText(0) == QStringLiteral("5"));

    // Empty LHS (→Y3) is not a store — falls through to normal eval/error.
    ys.setActiveFunction(0);
    checkTrue("bare →Y3 is not treated as a store",
              ys.functionBufferText(2).isEmpty());

    // Keyed form: the on-screen keypad enters "Y3" as the letter Y (VarY)
    // then a digit, i.e. tokens [Sto, VarY, Num3] rather than the fused Y3
    // token. Build that shape via two processExpression calls and confirm
    // it still stores into Y3 (slot 2).
    ys.setActiveFunction(0);
    ys.processInput(QStringLiteral("CLEAR"));
    ys.processExpression(QStringLiteral("2X→Y"));  // …, Sto, VarY
    ys.processExpression(QStringLiteral("3"));      // + Num3  → reads as Y3
    ys.processInput(QStringLiteral("ENTER"));
    check("2X→Y3 (keyed VarY+digit) reports Done", ys.currentDisplay(), "Done");
    checkTrue("Y3 buffer holds 2X", ys.functionBufferText(2) == QStringLiteral("2X"));

    // A plain store to variable Y (no trailing digit) is NOT a Y-store.
    ys.setActiveFunction(0);
    check("7→Y stores to scalar Y (not a function)", eval(ys, "7→Y"), "7");
    checkTrue("Y3 unchanged by 7→Y",
              ys.functionBufferText(2) == QStringLiteral("2X"));
  }

  section("IMP-045: yLookup is per-instance (no cross-controller dangling)");
  {
    // Y-VARS lookup is now bound per MathStateMachine instead of a static
    // capturing `this`. Recall on one controller must survive another
    // controller being constructed AND destroyed — which, under the old
    // static, left the global pointing at freed memory.
    UIController a;
    a.setGraphMode(0);
    a.setActiveFunction(0); eval(a, "X^2");  // a's Y1 = X^2

    {  // throwaway controller, destroyed at the closing brace
      UIController b;
      b.setActiveFunction(0); eval(b, "2X");
    }

    a.setActiveFunction(1);
    check("Y1 recall on `a` works after `b` was destroyed",
          eval(a, "Y1(3)"), "9");
  }

  section("BUG-023: on-screen Y-VARS entry inserts the fused token");
  {
    // The Y-VARS popup inserts a Yn function token via
    // processExpression("Yn"); this test mirrors that path and contrasts
    // it with the broken keypad form (letter-Y variable + digit).
    tux_ti83::MathStateMachine::varRegistry.fill(0.0);
    UIController yp;
    yp.setGraphMode(0);
    yp.setActiveFunction(1); eval(yp, "X^2");  // Y2 = X^2

    // Popup path: insert the fused "Y2" token, then the user types "(3)".
    yp.setActiveFunction(0);
    yp.processInput(QStringLiteral("CLEAR"));
    yp.processExpression(QStringLiteral("Y2"));   // what the popup inserts
    yp.processExpression(QStringLiteral("(3)"));
    yp.processInput(QStringLiteral("ENTER"));
    check("popup-inserted Y2(3) recalls the function → 9",
          yp.currentDisplay(), "9");

    // Broken keypad form: letter-Y variable (unset → 0) times 2 times 3.
    yp.setActiveFunction(0);
    yp.processInput(QStringLiteral("CLEAR"));
    yp.processExpression(QStringLiteral("Y"));     // VarY, not the Y2 token
    yp.processExpression(QStringLiteral("2(3)"));
    yp.processInput(QStringLiteral("ENTER"));
    check("keypad VarY+digit form is NOT a recall → 0",
          yp.currentDisplay(), "0");
  }

  section("DRAW menu overlays (Phase D)");
  {
    c.clrDraw();
    checkTrue("clrDraw → 0 objects", c.getDrawObjects().isEmpty());

    c.drawLine(1, 2, 3, 4);
    c.drawCircle(0, 0, 5);
    c.drawHorizontal(3);
    c.drawVertical(-2);
    c.drawPoint(7, 8);
    c.drawText(1, 1, "hi");
    QVariantList objs = c.getDrawObjects();
    checkTrue("6 draw objects added", objs.size() == 6);
    checkTrue("first is a line at (1,2,3,4)",
              objs[0].toMap()["type"].toString() == "line" &&
              objs[0].toMap()["a"].toDouble() == 1.0 &&
              objs[0].toMap()["d"].toDouble() == 4.0);
    checkTrue("circle radius recorded",
              objs[1].toMap()["type"].toString() == "circle" &&
              objs[1].toMap()["c"].toDouble() == 5.0);
    checkTrue("text string recorded",
              objs[5].toMap()["type"].toString() == "text" &&
              objs[5].toMap()["text"].toString() == "hi");

    // Delete one element (the circle at index 1); the rest shift down.
    c.deleteDrawObject(1);
    QVariantList after = c.getDrawObjects();
    checkTrue("deleteDrawObject → 5 left", after.size() == 5);
    checkTrue("index 1 is now the horiz line",
              after[1].toMap()["type"].toString() == "hline");
    checkTrue("out-of-range delete is a no-op",
              (c.deleteDrawObject(99), c.getDrawObjects().size() == 5));

    c.clrDraw();
    checkTrue("clrDraw clears all", c.getDrawObjects().isEmpty());
  }

  section("Parametric mode (Phase F)");
  {
    // Labels adapt to the graph mode.
    c.setProperty("graphMode", 0);
    checkTrue("Func label: Y1 / Y0",
              c.functionLabel(0) == "Y1" && c.functionLabel(9) == "Y0");
    c.setProperty("graphMode", 2);
    checkTrue("Pol label: r1", c.functionLabel(0) == "r1");
    c.setProperty("graphMode", 1);
    checkTrue("Par label: X1T/Y1T/X5T/Y5T",
              c.functionLabel(0) == "X1T" && c.functionLabel(1) == "Y1T" &&
              c.functionLabel(8) == "X5T" && c.functionLabel(9) == "Y5T");

    // setGraphMode accepts Par(1); rejects out-of-range → Func.
    c.setGraphMode(5);
    checkTrue("graphMode out-of-range(5) rejected → Func(0)",
              c.property("graphMode").toInt() == 0);
    c.setGraphMode(1);
    checkTrue("graphMode Par(1) accepted",
              c.property("graphMode").toInt() == 1);

    // X1T = X (t), Y1T = 2X (2t): the pair plots (t, 2t). Points land at
    // the even (X) slot; the odd (Y) slot stays empty.
    c.setActiveFunction(0); eval(c, "X");
    c.setActiveFunction(1); eval(c, "2X");
    QVariantList pts = c.getMultiGraphPoints(10);
    checkTrue("Par: pair points at slot 0", !pts[0].toList().isEmpty());
    checkTrue("Par: odd (Y) slot 1 empty", pts[1].toList().isEmpty());
    QVariantMap p0 = pts[0].toList()[0].toMap();
    checkTrue("Par: first point (0,0)",
              std::abs(p0["x"].toDouble()) < 1e-9 &&
              std::abs(p0["y"].toDouble()) < 1e-9);
    QVariantList l0 = pts[0].toList();
    QVariantMap pk = l0[l0.size() / 2].toMap();
    checkTrue("Par: y = 2x along the curve",
              std::abs(pk["y"].toDouble() - 2.0 * pk["x"].toDouble()) < 1e-9);

    c.setActiveFunction(0); c.processInput("CLEAR");
    c.setActiveFunction(1); c.processInput("CLEAR");
    c.setGraphMode(0);
    c.setActiveFunction(0);
  }

  section("Sequence mode (Phase F)");
  {
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-9; };
    c.setGraphMode(3);
    checkTrue("graphMode Seq(3) accepted", c.property("graphMode").toInt() == 3);
    checkTrue("Seq labels u/v/w + blank",
              c.functionLabel(0) == "u(n)" && c.functionLabel(1) == "v(n)" &&
              c.functionLabel(2) == "w(n)" && c.functionLabel(3).isEmpty());
    c.setProperty("seqNMax", 10.0);

    // Explicit: u(n) = X² (= n²) — evaluated directly at each n.
    c.setActiveFunction(0); eval(c, "X^2");
    QVariantList u = c.getMultiGraphPoints(10)[0].toList();
    checkTrue("Seq explicit: 10 terms", u.size() == 10);
    checkTrue("Seq explicit: u(1)=1",
              near(u[0].toMap()["x"].toDouble(), 1) &&
              near(u[0].toMap()["y"].toDouble(), 1));
    checkTrue("Seq explicit: u(3)=9", near(u[2].toMap()["y"].toDouble(), 9));

    // Recursive: u(n)=Ans+2, seed u(1)=1 → 1,3,5,7,...
    c.setActiveFunction(0); c.processInput("CLEAR"); eval(c, "Ans+2");
    c.setProperty("seqInitU", 1.0);
    QVariantList r = c.getMultiGraphPoints(10)[0].toList();
    checkTrue("Seq recursive: seed u(1)=1", near(r[0].toMap()["y"].toDouble(), 1));
    checkTrue("Seq recursive: u(2)=3", near(r[1].toMap()["y"].toDouble(), 3));
    checkTrue("Seq recursive: u(3)=5", near(r[2].toMap()["y"].toDouble(), 5));

    // Geometric: u(n)=2·Ans, seed 1 → 1,2,4,8,...
    c.setActiveFunction(0); c.processInput("CLEAR"); eval(c, "2Ans");
    QVariantList g = c.getMultiGraphPoints(10)[0].toList();
    checkTrue("Seq geometric: 1,2,4,8",
              near(g[0].toMap()["y"].toDouble(), 1) &&
              near(g[1].toMap()["y"].toDouble(), 2) &&
              near(g[3].toMap()["y"].toDouble(), 8));

    c.setActiveFunction(0); c.processInput("CLEAR");
    c.setGraphMode(0); c.setActiveFunction(0);
  }

  section("Save/load export (Phase F)");
  {
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-9; };
    tux_ti83::MathStateMachine::varRegistry.fill(0.0);
    tux_ti83::MathStateMachine::listRegistry.clear();
    c.setGraphMode(0);

    // Set some state, export it, mutate, then import → state restored.
    eval(c, "42→A");
    c.updateList("L1", QVariantList{7, 8, 9});
    checkTrue("export writes a save", c.exportState("unittest"));
    checkTrue("save appears in listSaves",
              c.listSaves().contains("unittest"));

    // Mutate away from the saved state.
    eval(c, "0→A");
    c.clearAllLists();
    checkTrue("A mutated to 0", near(eval(c, "A").toDouble(), 0.0));

    // Import restores.
    checkTrue("import succeeds", c.importState("unittest"));
    checkTrue("A restored to 42", near(eval(c, "A").toDouble(), 42.0));
    QVariantList l1 = c.getList("L1");
    checkTrue("L1 restored to {7,8,9}",
              l1.size() == 3 && near(l1[0].toDouble(), 7) &&
              near(l1[2].toDouble(), 9));

    // FORMAT flags coordMode / exprOn + MODE plotMode round-trip through
    // save/load.
    checkTrue("coordMode defaults to 0 (RectGC)",
              c.property("coordMode").toInt() == 0);
    checkTrue("exprOn defaults to true", c.property("exprOn").toBool());
    checkTrue("plotMode defaults to 0 (Sequential)",
              c.property("plotMode").toInt() == 0);
    checkTrue("screenMode defaults to 0 (Full)",
              c.property("screenMode").toInt() == 0);
    checkTrue("theme defaults to 0 (Dark)", c.property("theme").toInt() == 0);
    c.setProperty("coordMode", 1);       // PolarGC
    c.setProperty("exprOn", false);      // ExprOff
    c.setProperty("plotMode", 1);        // Simul
    c.setProperty("screenMode", 2);      // G-T
    c.setProperty("theme", 2);           // Amber
    checkTrue("format save writes", c.exportState("fmt"));
    c.setProperty("coordMode", 0);
    c.setProperty("exprOn", true);
    c.setProperty("plotMode", 0);
    c.setProperty("screenMode", 0);
    c.setProperty("theme", 0);
    checkTrue("format import succeeds", c.importState("fmt"));
    checkTrue("coordMode restored to PolarGC",
              c.property("coordMode").toInt() == 1);
    checkTrue("exprOn restored to false", !c.property("exprOn").toBool());
    checkTrue("plotMode restored to Simul",
              c.property("plotMode").toInt() == 1);
    checkTrue("screenMode restored to G-T",
              c.property("screenMode").toInt() == 2);
    checkTrue("theme restored to 2 (Amber)",
              c.property("theme").toInt() == 2);
    c.deleteSave("fmt");
    c.setProperty("coordMode", 0);       // reset to defaults
    c.setProperty("exprOn", true);
    c.setProperty("plotMode", 0);
    c.setProperty("screenMode", 0);
    c.setProperty("theme", 0);

    // Name sanitisation + missing-save handling.
    checkTrue("blank name rejected", !c.exportState("   "));
    checkTrue("import of missing save fails", !c.importState("nope-xyz"));

    // Clean up the test save.
    c.deleteSave("unittest");
    checkTrue("delete removes it", !c.listSaves().contains("unittest"));

    tux_ti83::MathStateMachine::varRegistry.fill(0.0);
    tux_ti83::MathStateMachine::listRegistry.clear();
  }

  section("Complex numbers (Phase F)");
  {
    using tux_ti83::MathStateMachine;
    using tux_ti83::ComplexMode;
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-6; };

    // Basic entry + arithmetic.
    check("2+3i", eval(c, "2+3i"), "2+3i");
    check("i^2 = -1", eval(c, "i^2"), "-1");
    check("i^3 = -i", eval(c, "i^3"), "-i");
    check("i^4 = 1", eval(c, "i^4"), "1");
    check("3i", eval(c, "3i"), "3i");
    check("(2+3i)(2-3i) = 13", eval(c, "(2+3i)(2-3i)"), "13");
    check("(1+i)^2 = 2i", eval(c, "(1+i)^2"), "2i");
    check("(2+3i)^2 = -5+12i", eval(c, "(2+3i)^2"), "-5+12i");
    check("real parts cancel: 2+2i-2 = 2i", eval(c, "2+2i-2"), "2i");
    check("i/i = 1", eval(c, "i/i"), "1");

    // Complex functions.
    check("conj(2+3i) = 2-3i", eval(c, "conj(2+3i)"), "2-3i");
    check("real(2+3i) = 2", eval(c, "real(2+3i)"), "2");
    check("imag(2+3i) = 3", eval(c, "imag(2+3i)"), "3");
    check("abs(3+4i) = 5", eval(c, "abs(3+4i)"), "5");
    checkTrue("angle(i) = π/2", near(eval(c, "angle(i)").toDouble(), M_PI / 2.0));

    // Ans carries a complex value forward.
    check("seed Ans = 2+3i", eval(c, "2+3i"), "2+3i");
    check("Ans+1 = 3+3i", evalChained(c, "Ans+1"), "3+3i");

    // Complex transcendentals (radian): sin/cos/exp/ln on complex.
    check("sin(i) = i·sinh(1)", eval(c, "sin(i)"), "1.175201194i");
    check("cos(i) = cosh(1) (real via snap)", eval(c, "cos(i)"), "1.543080635");
    check("e^(i) = cos1+isin1", eval(c, "e^(i)"), "0.5403023059+0.8414709848i");
    check("ln(i) = iπ/2", eval(c, "ln(i)"), "1.570796327i");
    check("Euler: e^(iπ) = -1", eval(c, "e^(iπ)"), "-1");

    // √ / ln of a negative: Real errors, a+bi gives a complex result.
    MathStateMachine::complexMode = ComplexMode::Real;
    check("√(-4) Real → ERR:NONREAL ANS", eval(c, "√(-4)"), "ERR:NONREAL ANS");
    MathStateMachine::complexMode = ComplexMode::Rect;
    check("√(-4) a+bi → 2i", eval(c, "√(-4)"), "2i");
    check("√(-1) a+bi → i", eval(c, "√(-1)"), "i");
    check("ln(-1) a+bi → iπ", eval(c, "ln(-1)"), "3.141592654i");
    MathStateMachine::complexMode = ComplexMode::Real;  // restore default
  }

  section("Parametric/polar T-window (Phase F follow-up)");
  {
    auto near = [](double a, double b) { return std::abs(a - b) < 1e-9; };
    c.setGraphMode(1);                    // parametric
    c.setActiveFunction(0); eval(c, "X"); // X1T = t
    c.setActiveFunction(1); eval(c, "X"); // Y1T = t
    c.setProperty("paramTMin", 0.0);
    c.setProperty("paramTMax", 10.0);
    c.setProperty("paramTStep", 1.0);
    QVariantList pts = c.getMultiGraphPoints(600)[0].toList();
    checkTrue("T-window: 11 points (t=0..10 step 1)", pts.size() == 11);
    checkTrue("T-window: first t=0", near(pts[0].toMap()["x"].toDouble(), 0.0));
    checkTrue("T-window: last t=10", near(pts[10].toMap()["x"].toDouble(), 10.0));

    // Non-zero Tmin shifts the start.
    c.setProperty("paramTMin", 3.0);
    c.setProperty("paramTMax", 5.0);
    c.setProperty("paramTStep", 1.0);
    QVariantList p2 = c.getMultiGraphPoints(600)[0].toList();
    checkTrue("T-window: Tmin=3 → first t=3",
              near(p2[0].toMap()["x"].toDouble(), 3.0));
    checkTrue("T-window: 3 points (3,4,5)", p2.size() == 3);

    // Clean up + restore defaults.
    c.setActiveFunction(0); c.processInput("CLEAR");
    c.setActiveFunction(1); c.processInput("CLEAR");
    c.setGraphMode(0); c.setActiveFunction(0);
    c.setProperty("paramTMin", 0.0);
    c.setProperty("paramTMax", 6.283185307179586);
    c.setProperty("paramTStep", 0.02);
  }

  section("Y4–Y0 reference tokens (Phase F follow-up)");
  {
    // Dedicated controller for an isolated Y-buffer fixture. (Since
    // IMP-045, yLookup is per-engine, so this needn't be the last live
    // controller; kept separate purely for a clean fixture.)
    UIController yv;
    tux_ti83::MathStateMachine::varRegistry.fill(0.0);  // X = 0
    yv.setGraphMode(0);

    // Y4 (slot 3) = X²; Y5 (slot 4) = Y4 + 10 (references Y4).
    yv.setActiveFunction(3); eval(yv, "X^2");
    yv.setActiveFunction(4); eval(yv, "Y4+10");
    // Y0 (slot 9) = 7.
    yv.setActiveFunction(9); eval(yv, "7");

    yv.setActiveFunction(0);  // scratch slot for evaluation
    check("Y4(4) = 16", eval(yv, "Y4(4)"), "16");
    check("Y5 references Y4: Y5(3) = 19", eval(yv, "Y5(3)"), "19");
    check("bare Y4 at X=0 = 0", eval(yv, "Y4"), "0");
    check("Y0 = 7", eval(yv, "Y0"), "7");
    check("Y0 + Y4(2) = 11", eval(yv, "Y0+Y4(2)"), "11");

    // Self-reference on a high slot still trips recursion: Y7 = Y7+1.
    yv.setActiveFunction(6); eval(yv, "Y7+1");
    yv.setActiveFunction(0);
    check("Y7 self-ref → ERR:RECURSION", eval(yv, "Y7"), "ERR:RECURSION");
  }

  section("Empty input");
  check("empty expression → ERR:SYNTAX", eval(c, ""), "ERR:SYNTAX");

  section("TI-BASIC interpreter — P0 scaffolding");
  {
    using tux_ti83::Interpreter;
    using tux_ti83::ProgramStore;
    using tux_ti83::RunStatus;

    // Statement splitter: ':' separates statements; blanks dropped; trimmed.
    auto s1 = Interpreter::splitStatements("5\xE2\x86\x92""A:A\xC2\xB2\xE2\x86\x92""B: Disp B");
    checkTrue("splitStatements → 3 statements", s1.size() == 3);
    checkTrue("splitStatements trims + keeps arrow", s1.size() == 3 && s1[2] == "Disp B");
    checkTrue("splitStatements drops blanks",
              Interpreter::splitStatements("A::: B ::").size() == 2);
    checkTrue("splitStatements empty line → 0", Interpreter::splitStatements("   ").empty());

    // load() flattens lines (and inner ':') into one statement list.
    Interpreter interp;
    interp.load({"5\xE2\x86\x92""A", "A+1\xE2\x86\x92""A : Disp A", ""});
    checkTrue("load flattens lines + colons → 3", interp.statementCount() == 3);

    // P0 run: no statements execute yet; the program runs to Done.
    checkTrue("fresh program status is Running", interp.status() == RunStatus::Running);
    checkTrue("run() reaches Done", interp.run() == RunStatus::Done);
    checkTrue("program counter at end", interp.programCounter() == 3);
    checkTrue("no output yet (P0)", interp.output().empty());
    checkTrue("no error", interp.errorLine() == -1);

    // Empty program runs straight to Done.
    Interpreter empty;
    empty.load({});
    checkTrue("empty program → Done immediately", empty.run() == RunStatus::Done);
    checkTrue("empty program has 0 statements", empty.statementCount() == 0);

    // reset() rewinds and re-runs.
    interp.reset();
    checkTrue("reset rewinds pc to 0", interp.programCounter() == 0);
    checkTrue("reset → Running", interp.status() == RunStatus::Running);
    checkTrue("re-run reaches Done", interp.run() == RunStatus::Done);

    // ProgramStore: put / has / get / names / remove.
    ProgramStore store;
    store.put("ADD", {"prompt A", "prompt B", "Disp A+B"});
    store.put("HELLO", {"Disp 42"});
    checkTrue("store has ADD", store.has("ADD"));
    checkTrue("store size 2", store.size() == 2);
    checkTrue("get returns lines", store.get("ADD") != nullptr && store.get("ADD")->size() == 3);
    checkTrue("names sorted", store.names().size() == 2 && store.names()[0] == "ADD");
    checkTrue("remove works", store.remove("HELLO") && !store.has("HELLO"));
    checkTrue("get missing → nullptr", store.get("NOPE") == nullptr);
  }

  section("TI-BASIC programs — P1 (controller CRUD + persistence)");
  {
    UIController pc;
    checkTrue("no programs initially", pc.programNames().isEmpty());

    // Name normalisation: uppercase, strip non-alnum, cap at 8.
    checkTrue("normalize uppercases + strips",
              pc.normalizeProgramName("ab#3xyz") == "AB3XYZ");
    checkTrue("normalize caps at 8",
              pc.normalizeProgramName("abcdefghij") == "ABCDEFGH");
    checkTrue("normalize junk → empty", pc.normalizeProgramName("!!!").isEmpty());

    // Save / query / round-trip (ASCII '->' avoids arrow-encoding noise;
    // the interpreter accepts it as Sto).
    QString saved = pc.saveProgram("add", "5->A\nDisp A\n");
    checkTrue("saveProgram returns normalised name", saved == "ADD");
    checkTrue("programExists after save", pc.programExists("ADD"));
    checkTrue("programNames contains it", pc.programNames().contains("ADD"));
    checkTrue("programText round-trips (trailing blank dropped)",
              pc.programText("ADD") == "5->A\nDisp A");

    pc.saveProgram("BEE", "Disp 1");
    checkTrue("two programs, sorted",
              pc.programNames().size() == 2 && pc.programNames()[0] == "ADD");
    checkTrue("invalid name not saved", pc.saveProgram("###", "x").isEmpty());

    // Run (P2: executes statements, fills programOutput). BEE = "Disp 1".
    pc.runProgram("BEE");
    checkTrue("runProgram Disp 1 → output '1'",
              pc.programOutput().size() == 1 && pc.programOutput().first() == "1");

    // Persistence round-trip via a named save.
    checkTrue("export writes", pc.exportState("prgtest"));
    pc.deleteProgram("ADD");
    pc.deleteProgram("BEE");
    checkTrue("programs cleared", pc.programNames().isEmpty());
    checkTrue("import restores", pc.importState("prgtest"));
    checkTrue("ADD restored", pc.programExists("ADD"));
    checkTrue("BEE restored + body intact",
              pc.programText("BEE") == "Disp 1");
    pc.deleteSave("prgtest");

    pc.deleteProgram("ADD");
    pc.deleteProgram("BEE");
  }

  section("TI-BASIC interpreter — P2 (statement execution)");
  {
    UIController pc;

    // Milestone: 5→A : A²→B : Disp B → 25 (stores are silent; Disp shows it).
    pc.saveProgram("SQ", "5->A\nA^2->B\nDisp B");
    pc.runProgram("SQ");
    checkTrue("milestone: Disp B after squaring → 25",
              !pc.programOutput().isEmpty() && pc.programOutput().last() == "25");

    // Bare expression echoes; Disp with two args → two lines; string literal.
    pc.saveProgram("T2", "3+4\nDisp 10,20\nDisp \"HI\"");
    pc.runProgram("T2");
    QStringList o = pc.programOutput();
    checkTrue("bare expr echoes 7", o.size() == 4 && o[0] == "7");
    checkTrue("Disp 10,20 → two lines", o[1] == "10" && o[2] == "20");
    checkTrue("Disp \"HI\" prints literal text", o[3] == "HI");

    // ClrHome wipes prior output.
    pc.saveProgram("T3", "Disp 1\nClrHome\nDisp 2");
    pc.runProgram("T3");
    checkTrue("ClrHome clears → only '2'",
              pc.programOutput().size() == 1 && pc.programOutput().first() == "2");

    // Stop halts early.
    pc.saveProgram("T4", "Disp 1\nStop\nDisp 999");
    pc.runProgram("T4");
    checkTrue("Stop halts before later output",
              pc.programOutput().size() == 1 && pc.programOutput().first() == "1");

    // Runtime error stops execution and reports the line.
    pc.saveProgram("T5", "Disp 1\nDisp 5/0\nDisp 3");
    pc.runProgram("T5");
    QStringList e = pc.programOutput();
    checkTrue("error stops run + reports line",
              e.size() == 2 && e[0] == "1" &&
              e[1].startsWith("ERR:DIVIDE BY 0"));

    // A shared variable set by a program is visible on the home screen.
    checkTrue("program-set A persists to home screen (A=5)",
              eval(pc, "A") == "5");

    for (const char *n : {"SQ", "T2", "T3", "T4", "T5"})
      pc.deleteProgram(n);
  }

  section("TI-BASIC interpreter — P3 (control flow)");
  {
    UIController pc;
    auto runP = [&](const QString &src) -> QStringList {
      pc.saveProgram("P", src);
      pc.runProgram("P");
      return pc.programOutput();
    };
    auto last = [](const QStringList &o) {
      return o.isEmpty() ? QString() : o.last();
    };

    // Single-statement If (conditions the tokeniser accepts: < > <= >= =).
    checkTrue("If true → next runs",
              last(runP("5->A:If A>3:Disp 7")) == "7");
    {
      QStringList o = runP("2->A:If A>3:Disp 7:Disp 8");
      checkTrue("If false → skips just one statement",
                o.last() == "8" && !o.contains("7"));
    }

    // If / Then / Else / End.
    checkTrue("If-Then true branch",
              last(runP("5->A:If A>3:Then:Disp 99:Else:Disp 11:End")) == "99");
    checkTrue("If-Then else branch",
              last(runP("2->A:If A>3:Then:Disp 99:Else:Disp 11:End")) == "11");

    // For( — ascending, custom step, descending (unary-minus step).
    checkTrue("For sum 1..5 → 15",
              last(runP("0->S:For(I,1,5):S+I->S:End:Disp S")) == "15");
    {
      QStringList o = runP("For(I,0,6,2):Disp I:End");
      checkTrue("For step 2 → 0,2,4,6",
                o.size() == 4 && o[0] == "0" && o[3] == "6");
    }
    checkTrue("For descending step -1 → 3",
              last(runP("9->S:For(I,3,1,-1):S-I->S:End:Disp S")) == "3");

    // While / Repeat (Repeat body always runs once; loops until true).
    checkTrue("While A<4 → A=4",
              last(runP("0->A:While A<4:A+1->A:End:Disp A")) == "4");
    checkTrue("Repeat until A>=3 → A=3",
              last(runP("0->A:Repeat A>=3:A+1->A:End:Disp A")) == "3");

    // Lbl / Goto.
    checkTrue("Goto loop → A=3",
              last(runP("0->A:Lbl P:A+1->A:If A<3:Goto P:Disp A")) == "3");
    checkTrue("Goto to a missing label → ERR:LABEL",
              last(runP("Goto Z:Disp 1")).startsWith("ERR:LABEL"));

    // Nested loops.
    checkTrue("nested For(3)×For(3) → 9",
              last(runP("0->S:For(I,1,3):For(J,1,3):S+1->S:End:End:Disp S")) == "9");

    // Program-computed variable is visible on the home screen.
    runP("42->A");
    checkTrue("program state shared with home screen", eval(pc, "A") == "42");

    pc.deleteProgram("P");
  }

  section("TI-BASIC interpreter — P4 (interactive I/O)");
  {
    UIController pc;

    // Milestone: Prompt → branch → labelled result.
    pc.saveProgram("ASK",
        "Prompt A:If A>3:Then:Disp \"BIG\":Else:Disp \"SMALL\":End");
    pc.runProgram("ASK");
    checkTrue("Prompt pauses for input", pc.programWaitingInput());
    checkTrue("prompt label is A=?", pc.programInputPrompt() == "A=?");
    pc.provideProgramInput("5");
    checkTrue("input resumes + true branch → BIG",
              !pc.programWaitingInput() && pc.programOutput().last() == "BIG");
    checkTrue("Input value stored to A (home screen)", eval(pc, "A") == "5");

    pc.runProgram("ASK");
    pc.provideProgramInput("2");
    checkTrue("else branch → SMALL", pc.programOutput().last() == "SMALL");

    // Input with a prompt string, value used in an expression.
    pc.saveProgram("IN", "Input \"X=\",N:Disp N+1");
    pc.runProgram("IN");
    checkTrue("Input prompt text = X=", pc.programInputPrompt() == "X=");
    pc.provideProgramInput("7");
    checkTrue("Input value used → 8", pc.programOutput().last() == "8");

    // Invalid input re-prompts; a valid one then proceeds.
    pc.runProgram("IN");
    pc.provideProgramInput("@@@");  // unparseable
    checkTrue("bad input keeps waiting", pc.programWaitingInput());
    pc.provideProgramInput("3");
    checkTrue("valid input then proceeds → 4",
              !pc.programWaitingInput() && pc.programOutput().last() == "4");

    // Pause waits for a keypress, then continues.
    pc.saveProgram("PZ", "Disp 1:Pause:Disp 2");
    pc.runProgram("PZ");
    checkTrue("Pause waits for key",
              pc.programWaitingKey() && pc.programOutput().last() == "1");
    pc.resumeProgram();
    checkTrue("resume continues past Pause → 2",
              !pc.programWaitingKey() && pc.programOutput().last() == "2");

    // Input inside a loop (exercises repeated suspend/resume).
    pc.saveProgram("LP", "0->S:For(I,1,3):Input N:S+N->S:End:Disp S");
    pc.runProgram("LP");
    pc.provideProgramInput("10");
    pc.provideProgramInput("20");
    pc.provideProgramInput("30");
    checkTrue("looped Input sums → 60",
              !pc.programWaitingInput() && pc.programOutput().last() == "60");

    for (const char *n : {"ASK", "IN", "PZ", "LP"})
      pc.deleteProgram(n);
  }

  section("TI-BASIC interpreter — P4b (strings)");
  {
    UIController pc;
    auto runP = [&](const QString &src) -> QStringList {
      pc.saveProgram("P", src);
      pc.runProgram("P");
      return pc.programOutput();
    };
    auto last = [](const QStringList &o) {
      return o.isEmpty() ? QString() : o.last();
    };

    // Store + display a string variable.
    checkTrue("store + Disp Str1",
              last(runP("\"HELLO\"->Str1:Disp Str1")) == "HELLO");
    // Concatenation of two Str vars.
    checkTrue("concat Str1+Str2",
              last(runP("\"AB\"->Str1:\"CD\"->Str2:Disp Str1+Str2")) == "ABCD");
    // Literal + variable concatenation.
    checkTrue("literal + var concat",
              last(runP("\"BOB\"->Str1:Disp \"HI \"+Str1")) == "HI BOB");
    // Mixed Disp args: a string label then a number.
    {
      QStringList o = runP("5->A:Disp \"A=\",A");
      checkTrue("Disp string label + number", o.last() == "5" && o.contains("A="));
    }
    // Quote-aware splitting: a ':' inside a string is not a separator.
    checkTrue("':' inside a string isn't a separator",
              last(runP("Disp \"A:B\"")) == "A:B");
    // Type errors.
    checkTrue("number → StrN is a type error",
              last(runP("5->Str1")).startsWith("ERR:DATA TYPE"));
    checkTrue("string + number is a type error",
              last(runP("Disp \"X\"+5")).startsWith("ERR:DATA TYPE"));

    // Str vars persist across runs (in session).
    runP("\"XYZ\"->Str1");
    checkTrue("Str1 persists across runs", last(runP("Disp Str1")) == "XYZ");

    // Input into a string stores the raw typed text (no evaluation).
    pc.saveProgram("NM", "Input \"NAME?\",Str1:Disp \"HELLO \"+Str1");
    pc.runProgram("NM");
    checkTrue("string Input pauses with prompt",
              pc.programWaitingInput() && pc.programInputPrompt() == "NAME?");
    pc.provideProgramInput("SAM");
    checkTrue("string Input → greeting",
              !pc.programWaitingInput() && pc.programOutput().last() == "HELLO SAM");

    pc.deleteProgram("P");
    pc.deleteProgram("NM");
  }

  section("TI-BASIC interpreter — P5 (sub-calls, Return, DelVar)");
  {
    UIController pc;

    // Milestone: a program calls a sub-program that mutates a shared global,
    // then control returns to the caller.
    pc.saveProgram("INC", "A+1->A");
    pc.saveProgram("MAIN", "5->A:prgmINC:Disp A");
    pc.runProgram("MAIN");
    checkTrue("prgmNAME runs sub then returns → 6",
              pc.programOutput().last() == "6");
    checkTrue("sub's global mutation persists (A=6)", eval(pc, "A") == "6");

    // Sub called inside a loop runs each iteration.
    pc.saveProgram("SUM", "0->S:For(I,1,3):prgmADD:End:Disp S");
    pc.saveProgram("ADD", "S+I->S");
    pc.runProgram("SUM");
    checkTrue("sub-call inside a loop → 6", pc.programOutput().last() == "6");

    // Return exits the sub early; the caller then continues.
    pc.saveProgram("SUB", "Disp 1:Return:Disp 2");
    pc.saveProgram("CALL", "prgmSUB:Disp 9");
    pc.runProgram("CALL");
    {
      const QStringList o = pc.programOutput();
      checkTrue("Return skips rest of sub (no '2')", !o.contains("2"));
      checkTrue("Return resumes caller → 1 then 9",
                o.contains("1") && o.last() == "9");
    }

    // Return in the main program ends the run.
    pc.saveProgram("RM", "Disp 1:Return:Disp 2");
    pc.runProgram("RM");
    {
      const QStringList o = pc.programOutput();
      checkTrue("Return in main ends run (only '1')",
                o.last() == "1" && !o.contains("2"));
    }

    // Nested sub-calls (A → B → C) unwind correctly.
    pc.saveProgram("C", "3->Z");
    pc.saveProgram("B", "prgmC:Z+10->Z");
    pc.saveProgram("A", "prgmB:Disp Z");
    pc.runProgram("A");
    checkTrue("nested sub-calls unwind → 13", pc.programOutput().last() == "13");

    // Calling a program that doesn't exist is a loud error, not a silent
    // no-op (matches the TI-83; also makes a mistyped name obvious).
    pc.saveProgram("BADCALL", "Disp 1:prgmNOPE:Disp 2");
    pc.runProgram("BADCALL");
    {
      const QStringList o = pc.programOutput();
      checkTrue("missing sub-program → ERR:UNDEFINED",
                o.last().startsWith("ERR:UNDEFINED") && !o.contains("2"));
    }

    // Unbounded recursion is caught by the depth cap (no hang).
    pc.saveProgram("SELF", "prgmSELF");
    pc.runProgram("SELF");
    checkTrue("runaway recursion → ERR:MEMORY",
              pc.programOutput().last().startsWith("ERR:MEMORY"));

    // DelVar resets a scalar; DelVar on a string clears it.
    pc.saveProgram("DV", "5->A:DelVar A:Disp A");
    pc.runProgram("DV");
    checkTrue("DelVar scalar → 0", pc.programOutput().last() == "0");
    pc.saveProgram("DVS", "\"HI\"->Str1:DelVar Str1:Disp \"[\"+Str1+\"]\"");
    pc.runProgram("DVS");
    checkTrue("DelVar Str1 clears it → []",
              pc.programOutput().last() == "[]");

    for (const char *n : {"INC", "MAIN", "SUM", "ADD", "SUB", "CALL", "RM",
                          "A", "B", "C", "BADCALL", "SELF", "DV", "DVS"})
      pc.deleteProgram(n);
  }

  section("TI-BASIC interpreter — P5b (slice / interrupt / break guard)");
  {
    using tux_ti83::Interpreter;
    using tux_ti83::RunStatus;
    using tux_ti83::EvalResult;

    // Trivial evaluators so statements execute without the real engine.
    auto evZero = [](const std::string &) { return EvalResult{true, 0.0, "0", ""}; };
    auto evTrue = [](const std::string &) { return EvalResult{true, 1.0, "1", ""}; };

    std::vector<std::string> many(100, "0");  // 100 bare statements

    // runSlice runs a bounded number of steps, then can resume.
    Interpreter it;
    it.setEvaluator(evZero);
    it.load(many);
    checkTrue("runSlice(10) leaves it Running", it.runSlice(10) == RunStatus::Running);
    checkTrue("runSlice(10) advanced pc by 10", it.programCounter() == 10);
    checkTrue("runSlice resumes to Done", it.runSlice(1000) == RunStatus::Done);

    // interrupt() stops a Running program with ERR:BREAK.
    Interpreter it2;
    it2.setEvaluator(evZero);
    it2.load(many);
    it2.runSlice(5);
    checkTrue("mid-run is Running", it2.status() == RunStatus::Running);
    it2.interrupt();
    checkTrue("interrupt → Error", it2.status() == RunStatus::Error);
    checkTrue("interrupt → ERR:BREAK", it2.errorMessage() == "ERR:BREAK");

    // interrupt() on a finished program is a no-op.
    Interpreter it3;
    it3.setEvaluator(evZero);
    it3.load({"0"});
    it3.run();
    it3.interrupt();
    checkTrue("interrupt no-op when not Running", it3.status() == RunStatus::Done);

    // A runaway loop is bounded by the lifetime guard → ERR:BREAK, not a hang.
    Interpreter it4;
    it4.setEvaluator(evTrue);
    it4.load({"While 1", "End"});  // infinite loop
    checkTrue("runaway loop hits guard → Error", it4.run() == RunStatus::Error);
    checkTrue("guard → ERR:BREAK", it4.errorMessage() == "ERR:BREAK");
  }

  section("TI-BASIC — P5b getKey (non-blocking key poll)");
  {
    UIController pc;
    auto last = [](const QStringList &o) {
      return o.isEmpty() ? QString() : o.last();
    };

    // No key pending → getKey polls 0.
    pc.saveProgram("GK0", "getKey->K:Disp K");
    pc.runProgram("GK0");
    checkTrue("getKey with no key → 0", last(pc.programOutput()) == "0");

    // getKey composes inside an expression (0 + 7).
    pc.saveProgram("GKX", "getKey+7->K:Disp K");
    pc.runProgram("GKX");
    checkTrue("getKey in an expression → 7", last(pc.programOutput()) == "7");

    // A queued key is returned once, then consumed (the next poll is 0).
    // Pause gives a headless injection point between the reset and the poll.
    pc.saveProgram("GKP", "Pause:getKey->K:getKey->M:Disp K:Disp M");
    pc.runProgram("GKP");
    checkTrue("program paused for a key", pc.programWaitingKey());
    pc.sendProgramKey(25);  // Up arrow
    pc.resumeProgram();
    {
      const QStringList o = pc.programOutput();
      checkTrue("getKey returns the queued key (25)",
                o.size() >= 2 && o[o.size() - 2] == "25");
      checkTrue("getKey is consumed → next poll 0", o.last() == "0");
    }

    // A stale key does not carry into a fresh run.
    pc.sendProgramKey(105);
    pc.runProgram("GK0");
    checkTrue("stale key cleared on new run → 0", last(pc.programOutput()) == "0");

    for (const char *n : {"GK0", "GKX", "GKP"})
      pc.deleteProgram(n);
  }

  section("TI-BASIC — P5b error jump-to-line (source line mapping)");
  {
    UIController pc;

    // The erroring statement's editor line, mapped through a ':'-chain.
    // Line 0 holds two statements; the 5/0 error is on line 1 → line 2 shown.
    pc.saveProgram("ERR1", "Disp 1:Disp 2\nDisp 5/0");
    pc.runProgram("ERR1");
    checkTrue("error maps to source line 1 (not statement 2)",
              pc.programErrorLine() == 1);
    checkTrue("error names the program", pc.programErrorProgram() == "ERR1");

    // A clean run clears the error location.
    pc.saveProgram("OK", "Disp 1");
    pc.runProgram("OK");
    checkTrue("no error → line -1", pc.programErrorLine() == -1);
    checkTrue("no error → empty program name",
              pc.programErrorProgram().isEmpty());

    // An error inside a sub-program points at the sub, at its own line.
    pc.saveProgram("SUBERR", "Disp 9\nDisp 1/0");
    pc.saveProgram("MAINERR", "prgmSUBERR");
    pc.runProgram("MAINERR");
    checkTrue("sub-program error → line within the sub",
              pc.programErrorLine() == 1);
    checkTrue("sub-program error → names the sub",
              pc.programErrorProgram() == "SUBERR");

    for (const char *n : {"ERR1", "OK", "SUBERR", "MAINERR"})
      pc.deleteProgram(n);
  }

  section("TI-BASIC — P4c string functions (length/sub/inString/expr)");
  {
    UIController pc;
    auto runP = [&](const QString &src) -> QStringList {
      pc.saveProgram("P", src);
      pc.runProgram("P");
      return pc.programOutput();
    };
    auto last = [](const QStringList &o) {
      return o.isEmpty() ? QString() : o.last();
    };

    // length(
    checkTrue("length of a Str var",
              last(runP("\"HELLO\"->Str1:Disp length(Str1)")) == "5");
    checkTrue("length of a literal", last(runP("Disp length(\"ABC\")")) == "3");

    // sub( — 1-based (begin, count).
    checkTrue("sub extracts a substring",
              last(runP("\"HELLO\"->Str1:Disp sub(Str1,2,3)")) == "ELL");
    checkTrue("sub result stores into a Str var",
              last(runP("\"HELLO\"->Str1:sub(Str1,1,2)->Str2:Disp Str2")) == "HE");
    checkTrue("sub concatenates as a string",
              last(runP("\"AB\"->Str1:Disp \"X\"+sub(Str1,1,1)")) == "XA");
    checkTrue("sub out of range → ERR:DOMAIN",
              last(runP("\"HI\"->Str1:Disp sub(Str1,1,5)")).startsWith("ERR:DOMAIN"));

    // inString( — position (1-based), 0 if not found, optional start.
    checkTrue("inString finds a substring",
              last(runP("\"HELLO\"->Str1:Disp inString(Str1,\"LL\")")) == "3");
    checkTrue("inString not found → 0",
              last(runP("\"HELLO\"->Str1:Disp inString(Str1,\"Z\")")) == "0");
    checkTrue("inString honours a start index",
              last(runP("\"HELLO\"->Str1:Disp inString(Str1,\"L\",4)")) == "4");

    // expr( — evaluate a string as an expression.
    checkTrue("expr evaluates a string literal",
              last(runP("Disp expr(\"6*7\")")) == "42");
    checkTrue("expr of a Str var into a scalar",
              last(runP("\"3+4\"->Str1:expr(Str1)->A:Disp A")) == "7");

    // Nested: sub with a length( count → whole string.
    checkTrue("nested length inside sub",
              last(runP("\"HELLO\"->Str1:Disp sub(Str1,1,length(Str1))")) == "HELLO");

    // Numeric context: a string function in a condition.
    checkTrue("length in an If condition",
              last(runP("\"HELLO\"->Str1:If length(Str1)>3:Disp 1")) == "1");

    // toString( — number → string (P7).
    checkTrue("toString of a number", last(runP("Disp toString(5)")) == "5");
    checkTrue("toString of an expression",
              last(runP("Disp toString(3+4)")) == "7");
    checkTrue("toString concatenates with a literal",
              last(runP("Disp \"X=\"+toString(42)")) == "X=42");
    checkTrue("toString stores into a Str var",
              last(runP("toString(99)->Str1:Disp Str1")) == "99");
    checkTrue("toString composes with length",
              last(runP("Disp length(toString(123))")) == "3");

    pc.deleteProgram("P");
  }

  section("TI-BASIC — P4c Str-var disk-persistence");
  {
    // Set Str1 in one controller, save, then load in a fresh one and read it
    // back (the test harness redirects state to a temp dir; both instances
    // share that file).
    {
      UIController a;
      a.saveProgram("SET", "\"PERSIST\"->Str1");
      a.runProgram("SET");
      a.saveState();
    }
    {
      UIController b;
      b.loadState();
      b.saveProgram("GET", "Disp Str1");
      b.runProgram("GET");
      checkTrue("Str1 survives save → load",
                b.programOutput().last() == "PERSIST");
      b.deleteProgram("GET");
      b.deleteProgram("SET");
    }
  }

  section("TI-BASIC — P4d Output( positioned text");
  {
    UIController pc;
    auto runP = [&](const QString &src) -> QStringList {
      pc.saveProgram("P", src);
      pc.runProgram("P");
      return pc.programOutput();
    };

    checkTrue("Output at (1,1)", runP("Output(1,1,\"HI\")").value(0) == "HI");
    checkTrue("Output pads to the column",
              runP("Output(1,5,\"X\")").value(0) == "    X");
    checkTrue("two Outputs share a row",
              runP("Output(1,1,\"AB\"):Output(1,4,\"CD\")").value(0) == "AB CD");
    {
      QStringList o = runP("Output(3,1,\"Z\")");
      checkTrue("Output at row 3 pads blank rows above",
                o.size() >= 3 && o.value(2) == "Z" && o.value(0) == "");
    }
    checkTrue("Output of a number", runP("Output(1,1,5+2)").value(0) == "7");
    checkTrue("Output of a Str var",
              runP("\"HI\"->Str1:Output(1,1,Str1)").value(0) == "HI");
    checkTrue("Output overwrites in place",
              runP("Output(1,1,\"HELLO\"):Output(1,1,\"HI\")").value(0) == "HILLO");
    checkTrue("row > 8 → ERR:DOMAIN",
              runP("Output(9,1,\"X\")").last().startsWith("ERR:DOMAIN"));
    checkTrue("col > 16 → ERR:DOMAIN",
              runP("Output(1,17,\"X\")").last().startsWith("ERR:DOMAIN"));
    checkTrue("Output clips at col 16",
              runP("Output(1,15,\"ABC\")").value(0).length() == 16);

    pc.deleteProgram("P");
  }

  section("TI-BASIC — P4e Menu( interactive branch");
  {
    UIController pc;

    pc.saveProgram(
        "M", "Menu(\"PICK\",\"ONE\",A,\"TWO\",B):Lbl A:Disp 1:Stop:Lbl B:Disp 2");
    pc.runProgram("M");
    checkTrue("Menu pauses for a choice", pc.programMenuActive());
    checkTrue("Menu title", pc.programMenuTitle() == "PICK");
    checkTrue("Menu options",
              pc.programMenuOptions().size() == 2 &&
                  pc.programMenuOptions()[0] == "ONE" &&
                  pc.programMenuOptions()[1] == "TWO");
    pc.provideProgramMenuChoice(0);
    checkTrue("choice 1 → Lbl A (Disp 1)",
              !pc.programMenuActive() && pc.programOutput().last() == "1");

    pc.runProgram("M");
    pc.provideProgramMenuChoice(1);
    checkTrue("choice 2 → Lbl B (Disp 2)", pc.programOutput().last() == "2");

    // Missing target label → ERR:LABEL when that option is chosen.
    pc.saveProgram("MBAD", "Menu(\"X\",\"GO\",Z)");
    pc.runProgram("MBAD");
    pc.provideProgramMenuChoice(0);
    checkTrue("missing label → ERR:LABEL",
              pc.programOutput().last().startsWith("ERR:LABEL"));

    // A missing (option,label) pair → ERR:ARGUMENT.
    pc.saveProgram("MARG", "Menu(\"X\",\"ONE\")");
    pc.runProgram("MARG");
    checkTrue("Menu missing a label arg → ERR:ARGUMENT",
              pc.programOutput().last().startsWith("ERR:ARGUMENT"));

    for (const char *n : {"M", "MBAD", "MARG"})
      pc.deleteProgram(n);
  }

  section("TI-BASIC — P6 program-driven graphs");
  {
    UIController pc;

    // Store a function into Y1 (bare and quoted both work).
    pc.saveProgram("GY", "\"X^2\"->Y1");
    pc.runProgram("GY");
    checkTrue("program sets Y1 (quoted)", pc.functionBufferText(0) == "X^2");
    checkTrue("stored function is enabled", pc.functionEnabled(0));
    pc.saveProgram("GY2", "2X->Y2");
    pc.runProgram("GY2");
    checkTrue("program sets Y2 (bare)", pc.functionBufferText(1) == "2X");

    // Window variables.
    pc.saveProgram("GW", "15->Xmax:-3->Ymin:2->Xscl");
    pc.runProgram("GW");
    checkTrue("Xmax set", pc.property("xMax").toDouble() == 15.0);
    checkTrue("Ymin set", pc.property("yMin").toDouble() == -3.0);

    // ZStandard resets the window.
    pc.saveProgram("GZ", "99->Xmax:ZStandard");
    pc.runProgram("GZ");
    checkTrue("ZStandard resets Xmax to 10",
              pc.property("xMax").toDouble() == 10.0);

    // FnOff / FnOn toggles a slot.
    pc.saveProgram("GF", "\"X\"->Y1:FnOff 1");
    pc.runProgram("GF");
    checkTrue("FnOff 1 disables Y1", !pc.functionEnabled(0));
    pc.saveProgram("GF2", "FnOn 1");
    pc.runProgram("GF2");
    checkTrue("FnOn 1 re-enables Y1", pc.functionEnabled(0));

    // DispGraph switches into graph mode.
    pc.saveProgram("GD", "\"X\"->Y1:DispGraph");
    pc.runProgram("GD");
    checkTrue("DispGraph switches to graph mode",
              pc.property("isGraphMode").toBool());

    // A bad function expression → ERR:SYNTAX.
    pc.saveProgram("GBAD", "\"@@@\"->Y1");
    pc.runProgram("GBAD");
    checkTrue("garbage function → ERR:SYNTAX",
              pc.programOutput().last().startsWith("ERR:SYNTAX"));

    for (const char *n : {"GY", "GY2", "GW", "GZ", "GF", "GF2", "GD", "GBAD"})
      pc.deleteProgram(n);
  }

  section("TI-BASIC — P6-2 draw overlay");
  {
    UIController pc;
    auto runP = [&](const QString &src) {
      pc.saveProgram("P", src);
      pc.runProgram("P");
      return pc.getDrawObjects();
    };
    auto type0 = [](const QVariantList &o) {
      return o.isEmpty() ? QString() : o[0].toMap()["type"].toString();
    };

    // Each primitive appends one object of the right type (ClrDraw isolates).
    checkTrue("Line(", [&] { auto o = runP("ClrDraw:Line(0,0,5,5)");
              return o.size() == 1 && type0(o) == "line"; }());
    checkTrue("Circle(", [&] { auto o = runP("ClrDraw:Circle(0,0,3)");
              return o.size() == 1 && type0(o) == "circle"; }());
    checkTrue("Horizontal", [&] { auto o = runP("ClrDraw:Horizontal 4");
              return o.size() == 1 && type0(o) == "hline"; }());
    checkTrue("Vertical", [&] { auto o = runP("ClrDraw:Vertical 2");
              return o.size() == 1 && type0(o) == "vline"; }());
    checkTrue("Pt-On(", [&] { auto o = runP("ClrDraw:Pt-On(1,2)");
              return o.size() == 1 && type0(o) == "point"; }());
    {
      auto o = runP("ClrDraw:Text(0,0,\"HI\")");
      checkTrue("Text( draws text", o.size() == 1 && type0(o) == "text" &&
                o[0].toMap()["text"].toString() == "HI");
    }

    // ClrDraw wipes the overlay.
    checkTrue("ClrDraw clears",
              runP("Line(0,0,1,1):Line(1,1,2,2):ClrDraw").isEmpty());

    // A draw command shows the graph.
    runP("ClrDraw:Line(0,0,1,1)");
    checkTrue("drawing switches to graph mode",
              pc.property("isGraphMode").toBool());

    // Wrong arg count → ERR:ARGUMENT.
    pc.saveProgram("P", "Line(0,0)");
    pc.runProgram("P");
    checkTrue("Line with 2 args → ERR:ARGUMENT",
              pc.programOutput().last().startsWith("ERR:ARGUMENT"));

    pc.deleteProgram("P");
  }

  section("TI-BASIC — comments + break/continue");
  {
    UIController pc;
    auto runP = [&](const QString &src) -> QStringList {
      pc.saveProgram("P", src);
      pc.runProgram("P");
      return pc.programOutput();
    };
    auto last = [](const QStringList &o) {
      return o.isEmpty() ? QString() : o.last();
    };

    // Comments (#) are stripped to end of line.
    {
      QStringList o = runP("Disp 1 # show one");
      checkTrue("trailing # comment stripped", o.size() == 1 && o.last() == "1");
    }
    checkTrue("# inside a string is kept", last(runP("Disp \"a#b\"")) == "a#b");
    {
      pc.saveProgram("P", "# header\nDisp 5");
      pc.runProgram("P");
      checkTrue("full-line comment produces no output",
                pc.programOutput().size() == 1 && pc.programOutput().last() == "5");
    }

    // break exits the innermost loop; continue skips to the next iteration.
    {
      QStringList o = runP("For(I,1,5):If I=3:break:Disp I:End");
      checkTrue("break exits a For loop",
                o.contains("1") && o.contains("2") && !o.contains("3"));
    }
    checkTrue("break in a While loop",
              last(runP("0->I:While 1:I+1->I:If I=3:break:End:Disp I")) == "3");
    checkTrue("continue skips an iteration",
              last(runP("0->S:For(I,1,5):If I=2:continue:S+I->S:End:Disp S")) == "13");
    checkTrue("break outside a loop → ERR:SYNTAX",
              last(runP("break")).startsWith("ERR:SYNTAX"));

    pc.deleteProgram("P");
  }

  section("TI-BASIC — P7 element access (lists + matrices)");
  {
    UIController pc;
    auto runP = [&](const QString &src) -> QStringList {
      pc.saveProgram("P", src);
      pc.runProgram("P");
      return pc.programOutput();
    };
    auto last = [](const QStringList &o) {
      return o.isEmpty() ? QString() : o.last();
    };

    // List element read.
    checkTrue("list element read",
              last(runP("{10,20,30}->L1:Disp L1(2)")) == "20");
    checkTrue("list elements in an expression",
              last(runP("{10,20,30}->L1:Disp L1(1)+L1(3)")) == "40");
    checkTrue("computed index",
              last(runP("{10,20,30}->L1:2->K:Disp L1(K+1)")) == "30");
    // List element write.
    checkTrue("list element write in a loop",
              last(runP("{0,0,0,0,0}->L1:For(I,1,5):I^2->L1(I):End:Disp L1(4)"))
                  == "16");
    checkTrue("store at dim+1 appends",
              last(runP("{5}->L1:99->L1(2):Disp L1(2)")) == "99");
    checkTrue("list element in a condition",
              last(runP("{1,2,3}->L1:If L1(2)=2:Disp 7")) == "7");
    checkTrue("list index out of range → ERR:INVALID DIM",
              last(runP("{1,2,3}->L1:Disp L1(5)")).startsWith("ERR:INVALID DIM"));

    // Matrix element read + write.
    checkTrue("matrix element read",
              last(runP("[[1,2][3,4]]->[A]:Disp [A](2,1)")) == "3");
    checkTrue("matrix element write",
              last(runP("[[1,2][3,4]]->[A]:9->[A](1,1):Disp [A](1,1)")) == "9");
    checkTrue("matrix index out of range → ERR:INVALID DIM",
              last(runP("[[1,2][3,4]]->[A]:Disp [A](3,1)"))
                  .startsWith("ERR:INVALID DIM"));

    pc.deleteProgram("P");
  }

  section("TI-BASIC — P7 local variables");
  {
    UIController pc;
    auto runP = [&](const QString &src) -> QStringList {
      pc.saveProgram("P", src);
      pc.runProgram("P");
      return pc.programOutput();
    };
    auto last = [](const QStringList &o) {
      return o.isEmpty() ? QString() : o.last();
    };

    // A Local starts fresh at 0.
    checkTrue("Local starts at 0", last(runP("9->B:Local B:Disp B")) == "0");

    // A sub-program's Local doesn't clobber the caller's variable.
    pc.saveProgram("LSUB", "Local A:99->A");
    checkTrue("Local protects the caller's A",
              runP("5->A:prgmLSUB:Disp A").last() == "5");

    // The sub sees its own local; the caller's value is restored on return.
    pc.saveProgram("LR", "Local A:100->A:Disp A");
    {
      QStringList o = runP("7->A:prgmLR:Disp A");
      checkTrue("sub sees its local, caller restored",
                o.contains("100") && o.last() == "7");
    }

    // A Local loop counter in a sub doesn't break the caller's loop.
    pc.saveProgram("LADD", "Local I:For(I,1,10):End");
    checkTrue("Local counter doesn't disturb the caller's loop",
              last(runP("0->S:For(I,1,3):prgmLADD:S+I->S:End:Disp S")) == "6");

    for (const char *n : {"LSUB", "LR", "LADD"})
      pc.deleteProgram(n);
    pc.deleteProgram("P");
  }

  section("TI-BASIC — P7 user functions (Define / Return)");
  {
    UIController pc;
    auto runP = [&](const QString &src) -> QStringList {
      pc.saveProgram("P", src);
      pc.runProgram("P");
      return pc.programOutput();
    };
    auto last = [](const QStringList &o) {
      return o.isEmpty() ? QString() : o.last();
    };

    checkTrue("function returns a value",
              last(runP("Define f(A)\nReturn A*A+1\nEnd\nDisp f(3)")) == "10");
    checkTrue("function used inside an expression",
              last(runP("Define f(A)\nReturn A*A+1\nEnd\nDisp f(3)+f(4)")) ==
                  "27");
    checkTrue("multi-statement function with Local + loop",
              last(runP("Define g(N)\nLocal S,I\n0->S\nFor(I,1,N)\nS+I->S\nEnd\n"
                        "Return S\nEnd\nDisp g(5)")) == "15");
    {
      QStringList o =
          runP("Define f(A)\nReturn A*10\nEnd\n7->A\nDisp f(2)\nDisp A");
      checkTrue("parameter doesn't leak (caller's A restored)",
                o.contains("20") && o.last() == "7");
    }
    checkTrue("recursion — factorial(5) = 120",
              last(runP("Define f(N)\nIf N<=1\nReturn 1\nReturn N*f(N-1)\nEnd\n"
                        "Disp f(5)")) == "120");
    checkTrue("wrong argument count → ERR:ARGUMENT",
              last(runP("Define f(A,B)\nReturn A+B\nEnd\nDisp f(3)"))
                  .startsWith("ERR:ARGUMENT"));

    pc.deleteProgram("P");
  }

  section("TI-BASIC — P7 SortA( / SortD(");
  {
    UIController pc;
    auto runP = [&](const QString &src) -> QStringList {
      pc.saveProgram("P", src);
      pc.runProgram("P");
      return pc.programOutput();
    };
    auto last = [](const QStringList &o) {
      return o.isEmpty() ? QString() : o.last();
    };

    checkTrue("SortA sorts ascending",
              last(runP("{3,1,2}->L1:SortA(L1):Disp L1")) == "{1,2,3}");
    checkTrue("SortD sorts descending",
              last(runP("{1,3,2}->L1:SortD(L1):Disp L1")) == "{3,2,1}");
    checkTrue("parallel sort reorders the second list",
              last(runP("{3,1,2}->L1:{30,10,20}->L2:SortA(L1,L2):Disp L2")) ==
                  "{10,20,30}");
    checkTrue("parallel dim mismatch → ERR:INVALID DIM",
              last(runP("{1,2}->L1:{1,2,3}->L2:SortA(L1,L2)"))
                  .startsWith("ERR:INVALID DIM"));

    pc.deleteProgram("P");
  }

  section("TI-BASIC — P7 error trapping (Try/Else/End)");
  {
    UIController pc;
    auto runP = [&](const QString &src) -> QStringList {
      pc.saveProgram("P", src);
      pc.runProgram("P");
      return pc.programOutput();
    };
    auto last = [](const QStringList &o) {
      return o.isEmpty() ? QString() : o.last();
    };

    // An error in the Try block runs the Else handler, then execution resumes.
    {
      QStringList o = runP("Try\nDisp 5/0\nElse\nDisp 99\nEnd\nDisp 1");
      checkTrue("caught error runs the handler",
                o.contains("99") && o.last() == "1");
    }
    // No error → the Else handler is skipped.
    {
      QStringList o = runP("Try\nDisp 5\nElse\nDisp 99\nEnd\nDisp 1");
      checkTrue("no error → handler skipped",
                o.contains("5") && o.last() == "1" && !o.contains("99"));
    }
    // Try with no Else swallows the error and continues.
    checkTrue("Try without Else swallows the error",
              last(runP("Try\nDisp 5/0\nEnd\nDisp 1")) == "1");
    // A For loop inside the Try is unwound when the error is caught.
    {
      QStringList o =
          runP("Try\nFor(I,1,3)\n1/0\nEnd\nElse\nDisp 88\nEnd\nDisp 1");
      checkTrue("For inside Try is unwound on catch",
                o.contains("88") && o.last() == "1");
    }
    // A failing store is caught too.
    checkTrue("caught error in a store",
              last(runP("Try\n5/0->A\nElse\nDisp 77\nEnd")) == "77");

    pc.deleteProgram("P");
  }

  section("TI-BASIC — P7 pixel graphics (Pxl-/Pt-)");
  {
    UIController pc;
    auto run = [&](const QString &src) {
      pc.saveProgram("P", src);
      pc.runProgram("P");
    };

    run("Pxl-On(10,20)");
    checkTrue("Pxl-On sets a pixel", pc.pxlTest(10, 20));
    run("Pxl-Off(10,20)");
    checkTrue("Pxl-Off clears a pixel", !pc.pxlTest(10, 20));

    // Pxl-Test( reads the pixel inside an expression.
    {
      pc.saveProgram("P", "Pxl-On(5,5):Disp Pxl-Test(5,5):Disp Pxl-Test(6,6)");
      pc.runProgram("P");
      const QStringList o = pc.programOutput();
      checkTrue("Pxl-Test reads on/off", o.contains("1") && o.last() == "0");
    }

    run("Pxl-On(70,0)");  // row out of range
    checkTrue("out-of-range Pxl-On ignored", !pc.pxlTest(70, 0));
    run("Pxl-On(1,1):ClrDraw");
    checkTrue("ClrDraw clears pixels", !pc.pxlTest(1, 1));

    // Pt-Off erases a vector point; Pt-Change toggles one.
    run("ClrDraw:Pt-On(1,2):Pt-Off(1,2)");
    checkTrue("Pt-Off erases the point", pc.getDrawObjects().isEmpty());
    run("ClrDraw:Pt-Change(1,2)");
    checkTrue("Pt-Change adds a point", pc.getDrawObjects().size() == 1);
    run("ClrDraw:Pt-Change(1,2):Pt-Change(1,2)");
    checkTrue("Pt-Change toggles off", pc.getDrawObjects().isEmpty());

    pc.deleteProgram("P");
  }

  section("TI-BASIC — P7 graph extras (Pic/DrawF/Tangent/Shade)");
  {
    UIController pc;
    auto run = [&](const QString &src) {
      pc.saveProgram("P", src);
      pc.runProgram("P");
    };

    // StorePic snapshots the drawing; RecallPic overlays it after a ClrDraw.
    run("ClrDraw:Pt-On(1,1):StorePic 1:ClrDraw:RecallPic 1");
    checkTrue("RecallPic restores a saved drawing",
              pc.getDrawObjects().size() == 1);

    // DrawF adds a sampled f(X) curve.
    run("ClrDraw:DrawF X");
    {
      const QVariantList d = pc.getDrawObjects();
      checkTrue("DrawF adds a curve",
                d.size() == 1 && d[0].toMap()["type"].toString() == "curve");
    }

    // Tangent draws a line whose slope is f'(x) (x² at x=2 → 4).
    run("ClrDraw:Tangent(X^2,2)");
    {
      const QVariantList d = pc.getDrawObjects();
      const QVariantMap ln = d.isEmpty() ? QVariantMap() : d[0].toMap();
      const double slope = (ln["d"].toDouble() - ln["b"].toDouble()) /
                           (ln["c"].toDouble() - ln["a"].toDouble());
      checkTrue("Tangent draws a line",
                d.size() == 1 && ln["type"].toString() == "line");
      checkTrue("Tangent slope is f'(2)=4", std::abs(slope - 4.0) < 1e-3);
    }

    // Shade adds a filled region between two curves.
    run("ClrDraw:Shade(0,X^2)");
    {
      const QVariantList d = pc.getDrawObjects();
      checkTrue("Shade adds a shaded region",
                d.size() == 1 && d[0].toMap()["type"].toString() == "shade");
    }

    pc.deleteProgram("P");
  }

  std::cout << "\n----------------------------------------\n"
            << "Total: " << (gPassed + gFailed) << "  Passed: " << gPassed
            << "  Failed: " << gFailed << '\n';
  return gFailed == 0 ? 0 : 1;
}

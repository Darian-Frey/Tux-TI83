#pragma once
#include <array>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace tux_ti83 {

enum class Token {
  Num0,
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,
  Decimal,
  Pi,
  E,
  Add,
  Sub,
  Mul,
  Div,
  Pow,
  // Binary infix nth-root: `n NthRoot x` returns x^(1/n). Same
  // precedence as Pow and right-associative — `2 NthRoot 3 NthRoot 8`
  // groups as `2 NthRoot (3 NthRoot 8)`. DOMAIN error when n is 0;
  // NONREAL ANS for even n on a negative x. Real TI-83 binds 2ND+^
  // to this; `xroot` works as an ASCII alias.
  NthRoot,
  // Implicit multiplication — never typed directly by the user.
  // A preprocessing pass in `evaluate()` synthesises this between
  // juxtaposed value-like tokens (`2π`, `2(3)`, `(3)(4)`, `2sin(x)`,
  // `5X`). Same precedence + behaviour as Mul on the eval side;
  // separate token so the source structure stays inspectable.
  ImplicitMul,
  Sin,
  Cos,
  Tan,
  Log,
  Ln,
  Sqrt,
  ASin,
  ACos,
  ATan,
  Equal,
  NotEqual,
  Less,
  LessEq,
  Greater,
  GreaterEq,
  And,
  Or,
  Xor,
  Not,
  Det,
  // Matrix transpose — unary prefix function that takes a matrix and
  // returns its transpose (rows and columns swapped).
  Transpose,
  // Reduced row-echelon form. Takes a matrix, returns the matrix with
  // row operations applied until each leading coefficient is 1 and is
  // the only non-zero entry in its column (Gauss-Jordan elimination).
  Rref,
  // Unary negation. Semantically distinct from binary Sub so we can
  // honour the TI-83's `(-)` key versus `−` key distinction.
  Neg,
  // Number functions (unary)
  Abs,
  Int,
  IPart,
  FPart,
  // Number functions (binary). Pop two operands during evaluation;
  // arguments are separated by Comma in the source expression.
  Round,
  Min,
  Max,
  Mod,
  // Combinatorics (binary). Arguments must be non-negative integers
  // with r ≤ n; domain errors otherwise.
  NCr,
  NPr,
  // Factorial — unary postfix. Accepts non-negative integers ≤ 170
  // (beyond that a double would overflow). Returns DOMAIN error for
  // negatives, non-integers, or values > 170.
  Fact,
  // Hyperbolic functions (unary). asinh/atanh accept all reals;
  // acosh requires x ≥ 1 (DOMAIN error otherwise); atanh requires
  // -1 < x < 1 (DOMAIN error otherwise).
  Sinh,
  Cosh,
  Tanh,
  ASinh,
  ACosh,
  ATanh,
  LeftParen,
  RightParen,
  // Scalar variables A..Z (26 contiguous tokens). Backing store is
  // MathStateMachine::varRegistry, indexed by `(int)t - (int)VarA`.
  // VarX is dual-purpose: in calc-mode evaluation the controller passes
  // `varRegistry[X_idx]` as xValue so `5→X` then `X+1` reads the stored
  // value; in graph-mode evaluation the controller passes the sweep x,
  // so plotting Y1=X² walks across the window. All other letters always
  // resolve via the registry.
  VarA,
  VarB,
  VarC,
  VarD,
  VarE,
  VarF,
  VarG,
  VarH,
  VarI,
  VarJ,
  VarK,
  VarL,
  VarM,
  VarN,
  VarO,
  VarP,
  VarQ,
  VarR,
  VarS,
  VarT,
  VarU,
  VarV,
  VarW,
  VarX,
  VarY,
  VarZ,
  // Assignment: <expr> → <var>. Evaluate::preprocess consumes the
  // target VarA..VarZ token and records its index in a sidebar; the
  // evaluator pops the top of the stack, writes it to varRegistry,
  // then pushes the value back so it also appears on the display.
  // Lowest precedence (-10) so the whole LHS expression resolves
  // before the store.
  Sto,
  // Statement separator (real TI-83 syntax: `5→A:A+1→A` chains two
  // expressions). The evaluator splits the token stream at every
  // Colon boundary, evaluates each segment in order, and returns
  // the result of the final non-empty segment. Errors in any segment
  // short-circuit — varRegistry mutations from earlier segments stay
  // (matching TI-83 behaviour: state changes commit per-statement).
  Colon,
  // Matrix Specific Tokens
  OpenBracket,
  CloseBracket,
  Comma,
  MatA,
  MatB,
  MatC,
  MatD,
  MatE,
  MatF,
  MatG,
  MatH,
  MatI,
  MatJ,
  // Last-answer recall. Populated by the controller after any successful
  // ENTER; retrieved by the evaluator when it sees this token and pushed
  // onto the operand stack (scalar or matrix, per lastResult.isMatrix).
  Ans,
  // Y-VARS — references to the user-defined function buffers Y1/Y2/Y3.
  // When evaluated, the engine recursively evaluates the referenced
  // buffer at the current xValue. Bare form only (Y1 alone uses
  // current X); explicit-argument form `Y1(3)` is not supported in
  // v1 — it parses as `Y1 * 3` via the existing implicit-mul rule.
  // Self-reference and cross-Y cycles return "Recursion".
  Y1,
  Y2,
  Y3
};

struct Matrix {
  int rows = 0;
  int cols = 0;
  std::vector<double> data;

  double at(int r, int c) const { return data[r * cols + c]; }
  void set(int r, int c, double val) { data[r * cols + c] = val; }
};

struct CalculationResult {
  bool success;
  double value;
  Matrix matrixValue; // Supports matrix-to-matrix results
  bool isMatrix = false;
  std::string error_message;
};

class EOSPrecedence {
public:
  static int precedence(Token t);
  static bool is_left_associative(Token t);
  static bool is_operator(Token t);
  static bool is_function(Token t);
  // True for functions that pop two operands during evaluation
  // (round, min, max, mod). Arguments are comma-separated in the source.
  static bool is_binary_function(Token t);
  // True for functions whose kTokens input string ends in `(` (so the
  // user types e.g. `abs(` as one keystroke and never types a separate
  // `(`). The shunting-yard pushes a synthetic LeftParen alongside
  // these so the matching `)` and any inner commas have a clear scope
  // marker. Functions whose input is bare (sin, cos, ..., neg) instead
  // expect the user to type `(` separately.
  static bool has_built_in_paren(Token t);
};

// Trig-function angle interpretation. Default is Radian (matches
// mathematical convention and preserves prior behaviour). When set to
// Degree, sin/cos/tan convert their input from degrees, and
// asin/acos/atan return degrees. Hyperbolic functions ignore this —
// hyperbolic arguments are dimensionless.
enum class AngleMode { Radian, Degree };

// Number-format notation for scalar results. Normal is the default
// "smart" format ('g' precision trims trailing zeros); Sci forces
// scientific notation (`1.234E5`); Eng forces engineering notation —
// same as Sci but the exponent is always a multiple of 3
// (`123.4E3` rather than `1.234E5`). Interpretation lives in
// UIController::formatScalar so the engine stays format-agnostic.
enum class NumberNotation { Normal, Sci, Eng };

class MathStateMachine {
public:
  CalculationResult evaluate(const std::vector<Token> &graph,
                             double xValue = 0.0);
  static std::string toFraction(double value, double tolerance = 1.0e-9);

  // Matrix Storage
  static std::map<Token, Matrix> matrixRegistry;

  // Scalar variable registry A..Z. Zero-initialised on program start;
  // mutated via Sto. Errors don't overwrite — the evaluator writes to
  // the registry only after a successful arithmetic result.
  static std::array<double, 26> varRegistry;

  // Current angle mode. Process-global static so CLI/REPL/GUI share
  // one setting; reflected in the trig-function evaluation below.
  static AngleMode angleMode;

  // Number-format controls. Both are read by UIController::formatScalar
  // when turning a double into a display string. `fixDecimals` == -1 is
  // the Float mode (default); 0..9 fixes that many decimal places.
  static NumberNotation notation;
  static int fixDecimals;

  // Last successful evaluation result. Updated by the UI controller
  // after every successful ENTER and recalled via Token::Ans. Matches
  // a TI-83's `Ans` behaviour — errors don't overwrite it.
  static CalculationResult lastResult;

  // Y-VARS lookup. The UI controller sets this on construction to a
  // lambda that returns m_functionBuffers[idx] (idx ∈ {0,1,2} for
  // Y1/Y2/Y3). Stays null in headless contexts (CLI/REPL/tests) —
  // the evaluator then treats every Y_n as 0, matching the TI-83
  // behaviour of an empty function slot.
  static std::function<std::vector<Token>(int)> yLookup;
};
} // namespace tux_ti83

#pragma once
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
  VarX,
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
  Ans
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

class MathStateMachine {
public:
  CalculationResult evaluate(const std::vector<Token> &graph,
                             double xValue = 0.0);
  static std::string toFraction(double value, double tolerance = 1.0e-9);

  // Matrix Storage
  static std::map<Token, Matrix> matrixRegistry;

  // Last successful evaluation result. Updated by the UI controller
  // after every successful ENTER and recalled via Token::Ans. Matches
  // a TI-83's `Ans` behaviour — errors don't overwrite it.
  static CalculationResult lastResult;
};
} // namespace tux_ti83

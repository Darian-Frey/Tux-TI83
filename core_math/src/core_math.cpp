#include "capsules/capsule_math.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <stack>
#include <string>

namespace tux_ti83 {

std::map<Token, Matrix> MathStateMachine::matrixRegistry;
std::array<double, 26> MathStateMachine::varRegistry{};
AngleMode MathStateMachine::angleMode = AngleMode::Radian;
NumberNotation MathStateMachine::notation = NumberNotation::Normal;
int MathStateMachine::fixDecimals = -1;  // -1 = Float (no fix)
CalculationResult MathStateMachine::lastResult{true, 0.0, {}, false, ""};

int EOSPrecedence::precedence(Token t) {
  switch (t) {
  case Token::Sin:
  case Token::Cos:
  case Token::Tan:
  case Token::ASin:
  case Token::ACos:
  case Token::ATan:
  case Token::Log:
  case Token::Ln:
  case Token::Sqrt:
  case Token::Not:
  case Token::Det:
  case Token::Transpose:
  case Token::Rref:
  case Token::Abs:
  case Token::Int:
  case Token::IPart:
  case Token::FPart:
  case Token::Round:
  case Token::Min:
  case Token::Max:
  case Token::Mod:
  case Token::NCr:
  case Token::NPr:
  case Token::Sinh:
  case Token::Cosh:
  case Token::Tanh:
  case Token::ASinh:
  case Token::ACosh:
  case Token::ATanh:
    return 4;
  case Token::Pow:
  case Token::NthRoot:
    return 3;
  case Token::Mul:
  case Token::ImplicitMul:
  case Token::Div:
  case Token::Neg: // Same as Mul/Div so −3*4 = (−3)*4 = −12 but −3^2 = −(3^2) = −9.
    return 2;
  case Token::Add:
  case Token::Sub:
    return 1;
  case Token::Equal:
  case Token::NotEqual:
  case Token::Less:
  case Token::LessEq:
  case Token::Greater:
  case Token::GreaterEq:
    return -1;
  case Token::And:
    return -2;
  case Token::Or:
  case Token::Xor:
    return -3;
  case Token::Sto:
    return -10;
  default:
    return 0;
  }
}

bool EOSPrecedence::is_left_associative(Token t) {
  // Pow and NthRoot are the right-associative operators. Everything
  // else is left.
  return t != Token::Pow && t != Token::NthRoot;
}
bool EOSPrecedence::is_operator(Token t) {
  return precedence(t) != 0 && !is_function(t);
}
bool EOSPrecedence::is_function(Token t) {
  // Note: Neg is treated as a function here so the shunting-yard pushes
  // it onto the opStack without popping prior operators (unary prefix
  // behaviour). Its precedence is still honoured when subsequent
  // operators arrive, which is how −3*4 vs −3^2 end up with different
  // groupings.
  return (t == Token::Sin || t == Token::Cos || t == Token::Tan ||
          t == Token::ASin || t == Token::ACos || t == Token::ATan ||
          t == Token::Log || t == Token::Ln || t == Token::Sqrt ||
          t == Token::Not || t == Token::Det || t == Token::Transpose ||
          t == Token::Rref || t == Token::Neg ||
          t == Token::Abs || t == Token::Int ||
          t == Token::IPart || t == Token::FPart ||
          t == Token::Sinh || t == Token::Cosh || t == Token::Tanh ||
          t == Token::ASinh || t == Token::ACosh || t == Token::ATanh ||
          is_binary_function(t));
}

bool EOSPrecedence::is_binary_function(Token t) {
  return (t == Token::Round || t == Token::Min ||
          t == Token::Max || t == Token::Mod ||
          t == Token::NCr || t == Token::NPr);
}

bool EOSPrecedence::has_built_in_paren(Token t) {
  // All functions with kTokens input strings ending in `(`. The shunting-
  // yard pushes a synthetic LeftParen for each; this is the uniform
  // contract that makes nested function-arg scopes work correctly
  // (fixes the `max(sin(0), cos(0))` regression).
  return (t == Token::Sin || t == Token::Cos || t == Token::Tan ||
          t == Token::ASin || t == Token::ACos || t == Token::ATan ||
          t == Token::Log || t == Token::Ln || t == Token::Sqrt ||
          t == Token::Abs || t == Token::Int || t == Token::IPart ||
          t == Token::FPart || t == Token::Det || t == Token::Transpose ||
          t == Token::Rref ||
          t == Token::Round || t == Token::Min ||
          t == Token::Max || t == Token::Mod ||
          t == Token::NCr || t == Token::NPr ||
          t == Token::Sinh || t == Token::Cosh || t == Token::Tanh ||
          t == Token::ASinh || t == Token::ACosh || t == Token::ATanh);
}

// --- MATRIX MATH HELPERS ---

double getDeterminant(const Matrix &m) {
  if (m.rows != m.cols || m.rows == 0)
    return 0.0;
  if (m.rows == 1)
    return m.data[0];
  if (m.rows == 2)
    return m.at(0, 0) * m.at(1, 1) - m.at(0, 1) * m.at(1, 0);

  double det = 0.0;
  for (int j = 0; j < m.cols; j++) {
    Matrix sub;
    sub.rows = m.rows - 1;
    sub.cols = m.cols - 1;
    sub.data.reserve(sub.rows * sub.cols);
    for (int row = 1; row < m.rows; row++) {
      for (int col = 0; col < m.cols; col++) {
        if (col == j)
          continue;
        sub.data.push_back(m.at(row, col));
      }
    }
    det += (j % 2 == 0 ? 1 : -1) * m.at(0, j) * getDeterminant(sub);
  }
  return det;
}

Matrix matrixAdd(const Matrix &a, const Matrix &b) {
  if (a.rows != b.rows || a.cols != b.cols)
    return {};
  Matrix res = a;
  for (size_t i = 0; i < a.data.size(); ++i)
    res.data[i] += b.data[i];
  return res;
}

Matrix matrixMul(const Matrix &a, const Matrix &b) {
  if (a.cols != b.rows)
    return {};
  Matrix res;
  res.rows = a.rows;
  res.cols = b.cols;
  res.data.resize(res.rows * res.cols, 0.0);
  for (int i = 0; i < a.rows; ++i)
    for (int j = 0; j < b.cols; ++j)
      for (int k = 0; k < a.cols; ++k)
        res.data[i * b.cols + j] += a.at(i, k) * b.at(k, j);
  return res;
}

// BUG-008 fix: matrix-matrix subtraction helper. Mirrors matrixAdd shape.
// Caller is responsible for the dimension check (the Sub branch in the
// evaluator does this and returns a Dim Mismatch error before invoking).
Matrix matrixSub(const Matrix &a, const Matrix &b) {
  Matrix res = a;
  for (size_t i = 0; i < a.data.size(); ++i)
    res.data[i] -= b.data[i];
  return res;
}

// Reduce matrix m in-place to reduced row-echelon form via Gauss-Jordan
// elimination. Pivots below 1e-12 are treated as zero (floating-point
// tolerance). After reduction, any cell with magnitude < 1e-12 is
// clamped to exactly 0 so results don't display as "-0" or "5.551e-17".
void rrefInPlace(Matrix &m) {
  int rows = m.rows;
  int cols = m.cols;
  int lead = 0;
  for (int r = 0; r < rows; ++r) {
    if (lead >= cols)
      break;
    int i = r;
    while (std::abs(m.at(i, lead)) < 1e-12) {
      ++i;
      if (i == rows) {
        i = r;
        ++lead;
        if (lead == cols)
          goto clamp;
      }
    }
    if (i != r) {
      for (int j = 0; j < cols; ++j) {
        double tmp = m.at(r, j);
        m.set(r, j, m.at(i, j));
        m.set(i, j, tmp);
      }
    }
    {
      double pivot = m.at(r, lead);
      for (int j = 0; j < cols; ++j)
        m.set(r, j, m.at(r, j) / pivot);
    }
    for (int ri = 0; ri < rows; ++ri) {
      if (ri != r) {
        double factor = m.at(ri, lead);
        for (int j = 0; j < cols; ++j)
          m.set(ri, j, m.at(ri, j) - factor * m.at(r, j));
      }
    }
    ++lead;
  }
clamp:
  for (size_t k = 0; k < m.data.size(); ++k)
    if (std::abs(m.data[k]) < 1e-12)
      m.data[k] = 0.0;
}

// Compute the inverse of a square matrix via Gauss-Jordan on the
// augmented form [A | I]. If reduction succeeds and the left half
// becomes identity, the right half is the inverse. Non-square input
// returns "Dim Mismatch"; singular input returns "SINGULAR MAT".
std::string matrixInverse(const Matrix &a, Matrix &result) {
  if (a.rows != a.cols)
    return "Dim Mismatch";
  int n = a.rows;
  Matrix aug;
  aug.rows = n;
  aug.cols = 2 * n;
  aug.data.resize(n * 2 * n, 0.0);
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      aug.set(i, j, a.at(i, j));
      aug.set(i, j + n, (i == j) ? 1.0 : 0.0);
    }
  }
  rrefInPlace(aug);
  // Left half should be identity if A was invertible. Any deviation
  // beyond 1e-9 means the matrix was singular or nearly so.
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      double expected = (i == j) ? 1.0 : 0.0;
      if (std::abs(aug.at(i, j) - expected) > 1e-9)
        return "SINGULAR MAT";
    }
  }
  result.rows = n;
  result.cols = n;
  result.data.resize(n * n);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      result.set(i, j, aug.at(i, j + n));
  return "";
}

// --- CORE EVALUATOR ---

CalculationResult MathStateMachine::evaluate(const std::vector<Token> &tokens,
                                             double xValue) {
  if (tokens.empty())
    return {false, 0.0, {}, false, "Empty"};

  // Statement separator: split the input on Colon and evaluate each
  // segment recursively. Return the last non-empty segment's result;
  // errors short-circuit (earlier segments' state changes — Sto in
  // particular — commit to the registry as they fire, matching TI-83
  // per-statement semantics). Empty leading/trailing colons (e.g. `:5`
  // or `5:`) are tolerated; truly empty input was already rejected
  // above.
  if (std::any_of(tokens.begin(), tokens.end(),
                  [](Token t) { return t == Token::Colon; })) {
    CalculationResult last{false, 0.0, {}, false, "Empty"};
    std::vector<Token> segment;
    for (Token t : tokens) {
      if (t == Token::Colon) {
        if (!segment.empty()) {
          last = evaluate(segment, xValue);
          if (!last.success)
            return last;
        }
        segment.clear();
      } else {
        segment.push_back(t);
      }
    }
    if (!segment.empty())
      last = evaluate(segment, xValue);
    return last;
  }

  std::vector<double> numericValues;
  std::vector<Token> processedTokens;
  std::string currentNumStr = "";
  bool parseFailed = false;
  auto flushNum = [&]() {
    if (!currentNumStr.empty()) {
      try {
        double v = std::stod(currentNumStr);
        processedTokens.push_back(Token::Num0);
        numericValues.push_back(v);
      } catch (...) {
        // Bare "." or other malformed numeric runs — std::stod throws
        // invalid_argument. Flag for a graceful syntax error instead
        // of letting the exception propagate out of the engine.
        parseFailed = true;
      }
      currentNumStr = "";
    }
  };

  for (auto t : tokens) {
    int val = (int)t;
    if (val >= 0 && val <= 9)
      currentNumStr += std::to_string(val);
    else if (t == Token::Decimal)
      currentNumStr += ".";
    else {
      flushNum();
      processedTokens.push_back(t);
    }
  }
  flushNum();
  if (parseFailed)
    return {false, 0.0, {}, false, "Syntax Error"};

  // Second preprocessing pass: Sto consumes its following VarA..VarZ
  // target and records the letter index in `storeTargets`. The evaluator
  // reads this in source order as each Sto fires, so the sidebar layout
  // mirrors `numericValues` for Num0.
  std::vector<Token> stoTokens;
  std::vector<int> storeTargets;
  stoTokens.reserve(processedTokens.size());
  for (size_t i = 0; i < processedTokens.size(); ++i) {
    if (processedTokens[i] == Token::Sto) {
      if (i + 1 >= processedTokens.size() ||
          processedTokens[i + 1] < Token::VarA ||
          processedTokens[i + 1] > Token::VarZ)
        return {false, 0.0, {}, false, "Syntax Error"};
      storeTargets.push_back(
          (int)processedTokens[i + 1] - (int)Token::VarA);
      stoTokens.push_back(Token::Sto);
      ++i;  // skip the target Var — already consumed.
    } else {
      stoTokens.push_back(processedTokens[i]);
    }
  }

  // Third preprocessing pass: insert ImplicitMul between juxtaposed
  // value-like tokens so `2π`, `2(3+4)`, `(3)(4)`, `2sin(x)`, `5X`
  // all work without an explicit `×`. IMP-005.
  //
  // `valueLikeEnd`: the previous token produces a value that can be
  // the LHS of a multiplication. `valueLikeStart`: the current token
  // begins something that can be the RHS. When both hold, we inject.
  auto valueLikeEnd = [](Token t) {
    return t == Token::Num0 ||
           t == Token::Pi || t == Token::E || t == Token::Ans ||
           t == Token::RightParen || t == Token::Fact ||
           (t >= Token::VarA && t <= Token::VarZ) ||
           (t >= Token::MatA && t <= Token::MatJ);
  };
  auto valueLikeStart = [](Token t) {
    return t == Token::Num0 ||
           t == Token::Pi || t == Token::E || t == Token::Ans ||
           t == Token::LeftParen ||
           (t >= Token::VarA && t <= Token::VarZ) ||
           (t >= Token::MatA && t <= Token::MatJ) ||
           EOSPrecedence::is_function(t);
  };
  std::vector<Token> finalTokens;
  finalTokens.reserve(stoTokens.size() * 2);
  for (size_t i = 0; i < stoTokens.size(); ++i) {
    if (i > 0 && valueLikeEnd(stoTokens[i - 1]) &&
        valueLikeStart(stoTokens[i])) {
      // Neg is treated as a function by is_function (for shunting-yard
      // unary handling), but `2-3` should never become `2 ImplicitMul
      // (Neg 3)` — the Sub-vs-Neg disambiguation lives in the UI's
      // insertToken, so by the time we see Neg here the user did
      // intend a unary minus. Still skip injection on Neg to keep
      // `2-3` → `2 - 3` rather than `2 * (-3)`. The latter is
      // numerically identical here but loses source structure.
      if (stoTokens[i] != Token::Neg)
        finalTokens.push_back(Token::ImplicitMul);
    }
    finalTokens.push_back(stoTokens[i]);
  }

  std::vector<std::pair<Token, double>> rpn;
  std::stack<Token> opStack;
  int numIdx = 0;
  for (auto t : finalTokens) {
    if (t == Token::Num0)
      rpn.push_back({t, numericValues[numIdx++]});
    else if ((t >= Token::MatA && t <= Token::MatJ) ||
             (t >= Token::VarA && t <= Token::VarZ) ||
             t == Token::Pi || t == Token::E || t == Token::Ans)
      rpn.push_back({t, 0.0});
    else if (t == Token::Fact) {
      // Unary postfix. Its operand is already in rpn ahead of this,
      // so we can emit Fact directly. Immediate emission gives `!` a
      // higher effective precedence than any binary operator, matching
      // the mathematical convention that factorial binds tightest.
      rpn.push_back({t, 0.0});
    }
    else if (EOSPrecedence::is_function(t) || t == Token::LeftParen) {
      opStack.push(t);
      // Functions with a built-in opening paren (input strings like
      // "abs(", "min(") never see a separate LeftParen pushed by the
      // user typing `(`. Push a synthetic one so the matching `)` and
      // any inner commas have a clear scope marker — the legacy
      // RightParen handler ("pop until LeftParen, then pop the
      // function above") and the simple "Comma pops until LeftParen"
      // rule both work uniformly across function styles.
      if (EOSPrecedence::has_built_in_paren(t))
        opStack.push(Token::LeftParen);
    }
    else if (t == Token::RightParen) {
      while (!opStack.empty() && opStack.top() != Token::LeftParen) {
        rpn.push_back({opStack.top(), 0.0});
        opStack.pop();
      }
      if (!opStack.empty())
        opStack.pop();
      if (!opStack.empty() && EOSPrecedence::is_function(opStack.top())) {
        rpn.push_back({opStack.top(), 0.0});
        opStack.pop();
      }
    } else if (t == Token::Comma) {
      // Argument separator for binary functions like round(x, n). Pops
      // operators (including unary functions inside the current
      // argument) until the matching LeftParen of the enclosing
      // function. The synthetic LeftParen pushed by built-in-paren
      // functions makes this work uniformly. The comma itself isn't
      // pushed anywhere; it's just a structural marker.
      while (!opStack.empty() && opStack.top() != Token::LeftParen) {
        rpn.push_back({opStack.top(), 0.0});
        opStack.pop();
      }
    } else {
      // BUG-009 fix: respect right-associativity in the precedence loop.
      // Left-associative operators pop the stack on `>=`; right-associative
      // operators (currently just Pow) pop on strict `>`. Before this fix
      // `is_left_associative` was declared but never called, and `2^3^2`
      // evaluated as `(2^3)^2 = 64` instead of `2^(3^2) = 512`.
      while (!opStack.empty() && opStack.top() != Token::LeftParen) {
        Token top = opStack.top();
        int topPrec = EOSPrecedence::precedence(top);
        int tPrec = EOSPrecedence::precedence(t);
        bool shouldPop = EOSPrecedence::is_left_associative(top)
                             ? (topPrec >= tPrec)
                             : (topPrec > tPrec);
        if (!shouldPop)
          break;
        rpn.push_back({top, 0.0});
        opStack.pop();
      }
      opStack.push(t);
    }
  }
  while (!opStack.empty()) {
    rpn.push_back({opStack.top(), 0.0});
    opStack.pop();
  }

  struct Operand {
    bool isMat;
    double val;
    Matrix mat;
  };
  std::stack<Operand> stack;
  auto toB = [](double v) { return std::abs(v) > 1e-9; };

  int storeIdx = 0;
  for (auto &node : rpn) {
    Token t = node.first;
    if (t == Token::Num0)
      stack.push({false, node.second, {}});
    else if (t >= Token::VarA && t <= Token::VarZ) {
      if (t == Token::VarX)
        stack.push({false, xValue, {}});
      else
        stack.push({false,
                    varRegistry[(int)t - (int)Token::VarA], {}});
    }
    else if (t == Token::Pi)
      stack.push({false, M_PI, {}});
    else if (t == Token::E)
      stack.push({false, M_E, {}});
    else if (t == Token::Ans) {
      // Recall the last successful evaluation result. Defaults to the
      // scalar 0 on first use (matches TI-83 power-on state).
      stack.push({lastResult.isMatrix, lastResult.value, lastResult.matrixValue});
    } else if (t == Token::Sto) {
      // Write the top-of-stack value into varRegistry[target] and push
      // it back so the display reflects the stored value. Scalar only —
      // matrices have their own registry and `[A]..[J]` syntax.
      if (stack.empty())
        return {false, 0.0, {}, false, "Error"};
      Operand v = stack.top();
      stack.pop();
      if (v.isMat)
        return {false, 0.0, {}, false, "Type Error"};
      varRegistry[storeTargets[storeIdx++]] = v.val;
      stack.push(v);
    } else if (t >= Token::MatA && t <= Token::MatJ) {
      if (matrixRegistry.count(t))
        stack.push({true, 0.0, matrixRegistry[t]});
      else
        return {false, 0.0, {}, false, "Undefined Matrix"};
    } else if (t == Token::Fact) {
      if (stack.empty())
        return {false, 0.0, {}, false, "Error"};
      Operand a = stack.top();
      stack.pop();
      if (a.isMat)
        return {false, 0.0, {}, false, "Type Error"};
      double val = a.val;
      // Non-negative integers only, capped at 170 (171! overflows double).
      if (val < 0.0 || val != std::floor(val) || val > 170.0)
        return {false, 0.0, {}, false, "DOMAIN"};
      double result = 1.0;
      for (int i = 2; i <= static_cast<int>(val); ++i)
        result *= i;
      stack.push({false, result, {}});
    } else if (EOSPrecedence::is_function(t)) {
      // Binary functions (round, min, max, mod) — pop two operands.
      if (EOSPrecedence::is_binary_function(t)) {
        if (stack.size() < 2)
          return {false, 0.0, {}, false, "Error"};
        Operand b = stack.top();
        stack.pop();
        Operand a = stack.top();
        stack.pop();
        if (a.isMat || b.isMat)
          return {false, 0.0, {}, false, "Type Error"};
        double result = 0.0;
        if (t == Token::Round) {
          // round(x, n) — round x to n decimal places. n is truncated to
          // an integer if fractional. Negative n rounds to powers of 10.
          double pow10 = std::pow(10.0, std::trunc(b.val));
          result = std::round(a.val * pow10) / pow10;
        } else if (t == Token::Min) {
          result = std::min(a.val, b.val);
        } else if (t == Token::Max) {
          result = std::max(a.val, b.val);
        } else if (t == Token::Mod) {
          if (b.val == 0.0)
            return {false, 0.0, {}, false, "DIVIDE BY 0"};
          result = std::fmod(a.val, b.val);
        } else if (t == Token::NCr || t == Token::NPr) {
          // Both require 0 ≤ r ≤ n with n and r non-negative integers.
          double n = a.val, r = b.val;
          if (n < 0.0 || r < 0.0 || r > n ||
              n != std::floor(n) || r != std::floor(r) || n > 170.0)
            return {false, 0.0, {}, false, "DOMAIN"};
          // Compute iteratively, multiplying and dividing together to
          // avoid overflowing intermediate factorials even for large n.
          if (t == Token::NCr) {
            // Use min(r, n-r) to keep the loop short.
            double k = std::min(r, n - r);
            result = 1.0;
            for (int i = 0; i < static_cast<int>(k); ++i) {
              result *= (n - i);
              result /= (i + 1);
            }
          } else {
            // nPr = n * (n-1) * ... * (n-r+1)
            result = 1.0;
            for (int i = 0; i < static_cast<int>(r); ++i)
              result *= (n - i);
          }
        }
        stack.push({false, result, {}});
        continue;
      }
      // Unary functions — pop one operand.
      if (stack.empty())
        return {false, 0.0, {}, false, "Error"};
      Operand a = stack.top();
      stack.pop();

      // Handle functions based on type
      if (t == Token::Det) {
        if (!a.isMat)
          return {false, 0.0, {}, false, "Type Error"};
        if (a.mat.rows != a.mat.cols)
          return {false, 0.0, {}, false, "Dim Mismatch"};
        stack.push({false, getDeterminant(a.mat), {}});
      } else if (t == Token::Transpose) {
        if (!a.isMat)
          return {false, 0.0, {}, false, "Type Error"};
        Matrix result;
        result.rows = a.mat.cols;
        result.cols = a.mat.rows;
        result.data.resize(result.rows * result.cols, 0.0);
        for (int i = 0; i < a.mat.rows; ++i)
          for (int j = 0; j < a.mat.cols; ++j)
            result.set(j, i, a.mat.at(i, j));
        stack.push({true, 0.0, result});
      } else if (t == Token::Rref) {
        if (!a.isMat)
          return {false, 0.0, {}, false, "Type Error"};
        Matrix result = a.mat;
        rrefInPlace(result);
        stack.push({true, 0.0, result});
      } else {
        if (a.isMat)
          return {false, 0.0, {}, false, "Type Error"};
        double v = a.val;
        // Trig angle-mode conversions. Degree mode scales sin/cos/tan
        // inputs by π/180 on the way in, and asin/acos/atan outputs by
        // 180/π on the way out. Radian mode (the default) is a no-op.
        // Hyperbolic functions ignore this — their argument isn't an
        // angle.
        const double degToRad = M_PI / 180.0;
        const double radToDeg = 180.0 / M_PI;
        const bool deg = (angleMode == AngleMode::Degree);
        if (t == Token::Sin)
          v = std::sin(deg ? v * degToRad : v);
        else if (t == Token::Cos)
          v = std::cos(deg ? v * degToRad : v);
        else if (t == Token::Tan)
          v = std::tan(deg ? v * degToRad : v);
        else if (t == Token::ASin) {
          // BUG-004 fix: ASin/ACos/ATan were declared as functions and
          // accepted by the parser, but the dispatch chain had no
          // branches — they silently returned the input unchanged.
          if (v < -1.0 || v > 1.0)
            return {false, 0.0, {}, false, "DOMAIN"};
          v = std::asin(v);
          if (deg) v *= radToDeg;
        } else if (t == Token::ACos) {
          if (v < -1.0 || v > 1.0)
            return {false, 0.0, {}, false, "DOMAIN"};
          v = std::acos(v);
          if (deg) v *= radToDeg;
        } else if (t == Token::ATan) {
          v = std::atan(v); // domain is all reals
          if (deg) v *= radToDeg;
        } else if (t == Token::Sqrt) {
          // BUG-006 fix: was silently returning 0 for negative input.
          if (v < 0.0)
            return {false, 0.0, {}, false, "NONREAL ANS"};
          v = std::sqrt(v);
        } else if (t == Token::Log) {
          // BUG-007 fix: was silently returning -HUGE_VAL for non-positive.
          if (v <= 0.0)
            return {false, 0.0, {}, false, "NONREAL ANS"};
          v = std::log10(v);
        } else if (t == Token::Ln) {
          if (v <= 0.0)
            return {false, 0.0, {}, false, "NONREAL ANS"};
          v = std::log(v);
        } else if (t == Token::Not)
          v = toB(v) ? 0.0 : 1.0;
        else if (t == Token::Neg)
          v = -v;
        else if (t == Token::Abs)
          v = std::abs(v);
        else if (t == Token::Int)
          // TI-83 `int(` is floor, not truncation toward zero.
          // `iPart(` is the truncation variant (separate token below).
          v = std::floor(v);
        else if (t == Token::IPart)
          v = std::trunc(v);
        else if (t == Token::FPart)
          v = v - std::trunc(v);
        else if (t == Token::Sinh)
          v = std::sinh(v);
        else if (t == Token::Cosh)
          v = std::cosh(v);
        else if (t == Token::Tanh)
          v = std::tanh(v);
        else if (t == Token::ASinh)
          v = std::asinh(v); // all reals in domain
        else if (t == Token::ACosh) {
          if (v < 1.0)
            return {false, 0.0, {}, false, "DOMAIN"};
          v = std::acosh(v);
        } else if (t == Token::ATanh) {
          if (v <= -1.0 || v >= 1.0)
            return {false, 0.0, {}, false, "DOMAIN"};
          v = std::atanh(v);
        }
        stack.push({false, v, {}});
      }
    } else {
      if (stack.size() < 2)
        return {false, 0.0, {}, false, "Error"};
      Operand b = stack.top();
      stack.pop();
      Operand a = stack.top();
      stack.pop();

      if (t == Token::Add) {
        if (a.isMat && b.isMat) {
          // BUG-010 fix: matrixAdd silently returned an empty Matrix on
          // dimension mismatch, which then got pushed as a "result" with
          // rows = cols = 0. Now we check dims here and propagate.
          if (a.mat.rows != b.mat.rows || a.mat.cols != b.mat.cols)
            return {false, 0.0, {}, false, "Dim Mismatch"};
          stack.push({true, 0.0, matrixAdd(a.mat, b.mat)});
        } else if (!a.isMat && !b.isMat)
          stack.push({false, a.val + b.val, {}});
        else
          return {false, 0.0, {}, false, "Type Error"};
      } else if (t == Token::Sub) {
        // BUG-008 fix: matrix-matrix subtraction was missing entirely.
        if (a.isMat && b.isMat) {
          if (a.mat.rows != b.mat.rows || a.mat.cols != b.mat.cols)
            return {false, 0.0, {}, false, "Dim Mismatch"};
          stack.push({true, 0.0, matrixSub(a.mat, b.mat)});
        } else if (!a.isMat && !b.isMat)
          stack.push({false, a.val - b.val, {}});
        else
          return {false, 0.0, {}, false, "Type Error"};
      } else if (t == Token::Mul || t == Token::ImplicitMul) {
        if (a.isMat && b.isMat) {
          // BUG-011 fix: matrixMul silently returned an empty Matrix when
          // a.cols != b.rows. Now we check the conformability rule here
          // and return ERR:INVALID DIM via IMP-006's propagation.
          if (a.mat.cols != b.mat.rows)
            return {false, 0.0, {}, false, "Dim Mismatch"};
          stack.push({true, 0.0, matrixMul(a.mat, b.mat)});
        } else if (a.isMat && !b.isMat) {
          Matrix m = a.mat;
          for (auto &v : m.data)
            v *= b.val;
          stack.push({true, 0.0, m});
        } else if (!a.isMat && b.isMat) {
          Matrix m = b.mat;
          for (auto &v : m.data)
            v *= a.val;
          stack.push({true, 0.0, m});
        } else
          stack.push({false, a.val * b.val, {}});
      } else if (t == Token::Div) {
        if (!a.isMat && !b.isMat) {
          // BUG-005 fix: division by zero was silently returning 0.
          if (b.val == 0.0)
            return {false, 0.0, {}, false, "DIVIDE BY 0"};
          stack.push({false, a.val / b.val, {}});
        } else
          return {false, 0.0, {}, false, "Type Error"};
      } else if (t == Token::Pow) {
        if (!a.isMat && !b.isMat) {
          stack.push({false, std::pow(a.val, b.val), {}});
        } else if (a.isMat && !b.isMat && b.val == -1.0) {
          // TI-83 convention: `[A]^-1` inverts a square matrix.
          // Any other matrix-with-scalar power is unsupported for now.
          Matrix inv;
          std::string err = matrixInverse(a.mat, inv);
          if (!err.empty())
            return {false, 0.0, {}, false, err};
          stack.push({true, 0.0, inv});
        } else {
          return {false, 0.0, {}, false, "Type Error"};
        }
      } else if (t == Token::NthRoot) {
        // `a NthRoot b` = b^(1/a). Scalars only. n=0 is undefined
        // (would be division by zero); even root of negative gives
        // ERR:NONREAL ANS — matching TI-83's real-only mode.
        if (a.isMat || b.isMat)
          return {false, 0.0, {}, false, "Type Error"};
        if (a.val == 0.0)
          return {false, 0.0, {}, false, "DOMAIN"};
        const bool nIsInt = (a.val == std::floor(a.val));
        const bool nIsEven = nIsInt &&
            (static_cast<long long>(a.val) % 2 == 0);
        if (b.val < 0.0 && nIsEven)
          return {false, 0.0, {}, false, "NONREAL ANS"};
        // For odd integer n on a negative b, std::pow returns NaN
        // because of the fractional exponent — handle via the sign +
        // |x|^(1/n) factoring.
        double result;
        if (b.val < 0.0 && nIsInt) {
          result = -std::pow(-b.val, 1.0 / a.val);
        } else {
          result = std::pow(b.val, 1.0 / a.val);
        }
        stack.push({false, result, {}});
      } else if (t == Token::Equal)
        stack.push({false, std::abs(a.val - b.val) < 1e-9 ? 1.0 : 0.0, {}});
      else if (t == Token::NotEqual)
        stack.push({false, std::abs(a.val - b.val) > 1e-9 ? 1.0 : 0.0, {}});
      else if (t == Token::Less)
        stack.push({false, a.val < b.val ? 1.0 : 0.0, {}});
      else if (t == Token::LessEq)
        stack.push({false, a.val <= b.val ? 1.0 : 0.0, {}});
      else if (t == Token::Greater)
        stack.push({false, a.val > b.val ? 1.0 : 0.0, {}});
      else if (t == Token::GreaterEq)
        stack.push({false, a.val >= b.val ? 1.0 : 0.0, {}});
      else if (t == Token::And)
        stack.push({false, (toB(a.val) && toB(b.val)) ? 1.0 : 0.0, {}});
      else if (t == Token::Or)
        stack.push({false, (toB(a.val) || toB(b.val)) ? 1.0 : 0.0, {}});
      else if (t == Token::Xor)
        stack.push({false, (toB(a.val) ^ toB(b.val)) ? 1.0 : 0.0, {}});
    }
  }

  if (stack.empty())
    return {false, 0.0, {}, false, "Error"};
  Operand res = stack.top();
  return {true, res.val, res.mat, res.isMat, ""};
}

std::string MathStateMachine::toFraction(double value, double tolerance) {
  if (std::isinf(value) || std::isnan(value))
    return "";
  double x = value;
  long long n1 = 1, d1 = 0, n2 = 0, d2 = 1;
  double b = x;
  // BUG-013 fix: track whether the continued-fraction loop actually
  // converged within tolerance. Previously the function returned
  // whatever convergent survived the 10-iteration limit, which for
  // irrationals (e, π, √(2), ...) produced a misleading near-fraction
  // like e → 1457/536.
  bool converged = false;
  for (int i = 0; i < 10; ++i) {
    long long a = std::floor(b);
    long long aux_n = n1;
    n1 = a * n1 + n2;
    n2 = aux_n;
    long long aux_d = d1;
    d1 = a * d1 + d2;
    d2 = aux_d;
    if (std::abs(x - (double)n1 / d1) < tolerance) {
      converged = true;
      break;
    }
    if (std::abs(b - a) < 1.0e-12) {
      // Continued-fraction expansion terminated exactly (integer or
      // exact rational). Treat as converged.
      converged = true;
      break;
    }
    b = 1.0 / (b - a);
  }
  if (!converged)
    return "";
  if (d1 == 1)
    return std::to_string(n1);
  return std::to_string(n1) + "/" + std::to_string(d1);
}

} // namespace tux_ti83

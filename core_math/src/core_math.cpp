#include "capsules/capsule_math.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stack>
#include <string>

namespace tux_ti83 {

std::map<Token, Matrix> MathStateMachine::matrixRegistry;
std::map<Token, std::vector<double>> MathStateMachine::listRegistry;
std::array<double, 26> MathStateMachine::varRegistry{};
AngleMode MathStateMachine::angleMode = AngleMode::Radian;
NumberNotation MathStateMachine::notation = NumberNotation::Normal;
int MathStateMachine::fixDecimals = -1;  // -1 = Float (no fix)
NumberBase MathStateMachine::numberBase = NumberBase::Dec;
CalculationResult MathStateMachine::lastResult{true, 0.0, {}, false, ""};
std::function<std::vector<Token>(int)> MathStateMachine::yLookup;

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
  case Token::Exp:
  case Token::Sgn:
  case Token::Y1Call:
  case Token::Y2Call:
  case Token::Y3Call:
  case Token::FnIntCall:
  case Token::NDerivCall:
  case Token::SumCall:
  case Token::ProdCall:
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
          t == Token::Exp || t == Token::Sgn ||
          t == Token::Y1Call || t == Token::Y2Call || t == Token::Y3Call ||
          t == Token::FnIntCall || t == Token::NDerivCall ||
          t == Token::SumCall || t == Token::ProdCall ||
          t == Token::Sinh || t == Token::Cosh || t == Token::Tanh ||
          t == Token::ASinh || t == Token::ACosh || t == Token::ATanh ||
          t == Token::Mean || t == Token::StdDev || t == Token::Variance ||
          t == Token::ListSum || t == Token::ListProd || t == Token::Median ||
          t == Token::SeqCall ||
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
          t == Token::FPart || t == Token::Exp || t == Token::Sgn ||
          t == Token::Y1Call || t == Token::Y2Call || t == Token::Y3Call ||
          t == Token::FnIntCall || t == Token::NDerivCall ||
          t == Token::SumCall || t == Token::ProdCall ||
          t == Token::Det || t == Token::Transpose ||
          t == Token::Rref ||
          t == Token::Round || t == Token::Min ||
          t == Token::Max || t == Token::Mod ||
          t == Token::NCr || t == Token::NPr ||
          t == Token::Sinh || t == Token::Cosh || t == Token::Tanh ||
          t == Token::ASinh || t == Token::ACosh || t == Token::ATanh ||
          t == Token::Mean || t == Token::StdDev || t == Token::Variance ||
          t == Token::ListSum || t == Token::ListProd || t == Token::Median ||
          t == Token::SeqCall);
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

// --- DEFERRED-EVALUATION FRAMEWORK ---
//
// fnInt/nDeriv/sum/prod each take an unevaluated expression as their
// first argument. The shunting-yard evaluates everything eagerly, so to
// keep argument 1 unevaluated we run a preprocessing pass over the raw
// source tokens that:
//   - finds each surface-level FnInt/NDeriv/Sum/Prod token,
//   - locates the matching `)`,
//   - splits the parenthesised contents by top-level commas,
//   - recursively rewrites every argument (so nested calls resolve too),
//   - captures argument 0 (the deferred expression) and argument 1 (the
//     bound variable, which must be a single VarA..VarZ token) into a
//     thread-local side table keyed by a sequential index `K`,
//   - emits a synthetic *Call token followed by the eager argument
//     subexpressions, comma-separated, with `K` encoded as raw digit
//     tokens as the final argument. The shunting-yard then treats the
//     synthetic call exactly like any built-in-paren function: it pops
//     its arguments off the operand stack (top = K, then bounds in
//     source order) and consults the side table.
//
// The side table is `thread_local` and shared across nested
// `evaluate()` invocations via a depth-counter RAII guard — Y-VARS
// recursion (which calls back into `evaluate()` with a different token
// buffer) inherits the parent's table, and the deferred-call evaluator
// branches likewise re-enter `evaluate()` on the captured expression
// without losing track of which K corresponds to which call. The
// outermost `evaluate()` clears the table on entry and exit.

namespace {

struct DeferredCall {
  std::vector<Token> expr;  // raw (pre-flush) tokens — recursive evaluate() handles its own digit flush
  int varIdx;               // 0..25 for VarA..VarZ
};

static thread_local std::vector<DeferredCall> g_deferred;
static thread_local int g_evalDepth = 0;

struct EvalGuard {
  bool outermost;
  EvalGuard() {
    outermost = (g_evalDepth == 0);
    if (outermost) g_deferred.clear();
    ++g_evalDepth;
  }
  ~EvalGuard() {
    --g_evalDepth;
    if (outermost) g_deferred.clear();
  }
};

// Paren-scope tracking treats every built-in-paren function token
// (sin(, abs(, round(, fnInt(, ...) as opening a +1 depth, matching the
// synthetic LeftParen the shunting-yard pushes for those tokens. Without
// this, `nDeriv(sin(X), X, 0)` would consume the `)` from `sin(X)` as
// nDeriv's closer, and the outer rewrite would fail with bogus arg
// counts. The surface-level deferred-call tokens get an explicit branch
// because they're rewritten out before shunting-yard sees them and so
// don't appear in `has_built_in_paren`, but they still open a scope on
// the source side.
bool opensParenScope(Token t) {
  return t == Token::LeftParen ||
         t == Token::FnInt || t == Token::NDeriv ||
         t == Token::Sum   || t == Token::Prod   || t == Token::Seq ||
         EOSPrecedence::has_built_in_paren(t);
}

std::vector<std::vector<Token>> splitByComma(const std::vector<Token> &tokens,
                                              int lo, int hi) {
  std::vector<std::vector<Token>> parts;
  std::vector<Token> current;
  int depth = 0;
  for (int i = lo; i < hi; ++i) {
    Token t = tokens[i];
    // Brace scopes count too, so the commas inside a `{…}` list literal
    // argument (e.g. sum({1,2,3})) aren't mistaken for argument
    // separators.
    if (opensParenScope(t) || t == Token::LeftBrace) ++depth;
    else if (t == Token::RightParen || t == Token::RightBrace) --depth;
    if (depth == 0 && t == Token::Comma) {
      parts.push_back(std::move(current));
      current.clear();
    } else {
      current.push_back(t);
    }
  }
  parts.push_back(std::move(current));
  return parts;
}

// Emit the digits of a non-negative integer as Num0..Num9 tokens.
void appendIntTokens(std::vector<Token> &out, int K) {
  std::string s = std::to_string(K);
  for (char c : s)
    out.push_back(static_cast<Token>(static_cast<int>(Token::Num0) + (c - '0')));
}

std::vector<Token> rewriteDeferredCalls(const std::vector<Token> &tokens,
                                        bool &ok);

// Rewrite the contents of one call's argument list. Each argument is
// recursively passed through `rewriteDeferredCalls` so nested deferred
// calls inside any argument get resolved.
std::vector<std::vector<Token>> rewriteArgs(
    const std::vector<std::vector<Token>> &args, bool &ok) {
  std::vector<std::vector<Token>> result;
  result.reserve(args.size());
  for (auto &a : args) {
    auto r = rewriteDeferredCalls(a, ok);
    if (!ok) return {};
    result.push_back(std::move(r));
  }
  return result;
}

std::vector<Token> rewriteDeferredCalls(const std::vector<Token> &tokens,
                                        bool &ok) {
  std::vector<Token> out;
  out.reserve(tokens.size());
  for (size_t i = 0; i < tokens.size(); ++i) {
    Token t = tokens[i];
    const bool isDeferred = (t == Token::FnInt || t == Token::NDeriv ||
                              t == Token::Sum   || t == Token::Prod   ||
                              t == Token::Seq);
    if (!isDeferred) {
      out.push_back(t);
      continue;
    }
    // Source form is `Func(`: a built-in-paren function, so the user
    // never types a separate LeftParen. The kTokens input string
    // "fnInt(" landed Token::FnInt — and the shunting-yard would push
    // a synthetic LeftParen at that point. The user's actual `)` then
    // closes the call. Locate that closing `)`: it is the matching
    // RightParen to a *virtual* LeftParen positioned right after our
    // token. Equivalently, scan forward tracking depth, starting at
    // depth 1 immediately after Token::FnInt.
    int depth = 1;  // FnInt/NDeriv/Sum/Prod's built-in paren is already open
    int rParen = -1;
    for (size_t j = i + 1; j < tokens.size(); ++j) {
      if (opensParenScope(tokens[j])) ++depth;
      else if (tokens[j] == Token::RightParen) {
        --depth;
        if (depth == 0) { rParen = static_cast<int>(j); break; }
      }
    }
    if (rParen < 0) { ok = false; return out; }

    // Split the contents (i+1 .. rParen) by top-level commas.
    auto args = splitByComma(tokens, static_cast<int>(i) + 1, rParen);

    // sum(/prod( overload (Phase C): a single argument is the LIST
    // reduction (sum / product of a list), not the 4-arg calculus form.
    // Emit the unary ListSum/ListProd around the (recursively rewritten)
    // argument and skip the deferred-call machinery entirely.
    if ((t == Token::Sum || t == Token::Prod) && args.size() == 1) {
      auto rArgs = rewriteArgs(args, ok);
      if (!ok) return out;
      out.push_back(t == Token::Sum ? Token::ListSum : Token::ListProd);
      for (auto x : rArgs[0])
        out.push_back(x);
      out.push_back(Token::RightParen);
      i = rParen;  // continue past the original `)`
      continue;
    }

    // Argument count gate. fnInt/sum/prod want exactly 4; nDeriv accepts
    // 3 (default h) or 4 (explicit h); seq accepts 4 (default step 1) or
    // 5 (explicit step).
    if (t == Token::NDeriv) {
      if (args.size() < 3 || args.size() > 4) { ok = false; return out; }
    } else if (t == Token::Seq) {
      if (args.size() < 4 || args.size() > 5) { ok = false; return out; }
    } else {
      if (args.size() != 4) { ok = false; return out; }
    }

    // Recursively rewrite every argument so nested deferred calls
    // (inside the deferred expression or the eager bounds) resolve.
    auto rArgs = rewriteArgs(args, ok);
    if (!ok) return out;

    // arg1 must be a single VarA..VarZ token after recursion.
    if (rArgs[1].size() != 1 ||
        rArgs[1][0] < Token::VarA || rArgs[1][0] > Token::VarZ) {
      ok = false; return out;
    }
    const int varIdx = static_cast<int>(rArgs[1][0]) -
                       static_cast<int>(Token::VarA);

    // Allocate the side-table slot. The slot's index `K` will be
    // emitted into the stream as the synthetic call's final operand;
    // the evaluator pops K and indexes into g_deferred.
    const int K = static_cast<int>(g_deferred.size());
    g_deferred.push_back({std::move(rArgs[0]), varIdx});

    // Emit synthetic call. The *Call token has has_built_in_paren=true
    // so the shunting-yard pushes a synthetic LeftParen for us — we
    // only emit the comma-separated arguments and the closing `)`.
    Token callTok = (t == Token::FnInt)  ? Token::FnIntCall :
                    (t == Token::NDeriv) ? Token::NDerivCall :
                    (t == Token::Sum)    ? Token::SumCall :
                    (t == Token::Prod)   ? Token::ProdCall :
                                           Token::SeqCall;
    out.push_back(callTok);

    auto emitArg = [&](const std::vector<Token> &arg) {
      for (auto x : arg) out.push_back(x);
    };

    if (t == Token::Seq) {
      // start, end, step (default 1 if omitted), K.
      emitArg(rArgs[2]);
      out.push_back(Token::Comma);
      emitArg(rArgs[3]);
      out.push_back(Token::Comma);
      if (rArgs.size() == 5) {
        emitArg(rArgs[4]);
      } else {
        out.push_back(Token::Num1);
      }
      out.push_back(Token::Comma);
    } else if (t == Token::FnInt || t == Token::Sum || t == Token::Prod) {
      // lower, upper, K.
      emitArg(rArgs[2]);
      out.push_back(Token::Comma);
      emitArg(rArgs[3]);
      out.push_back(Token::Comma);
    } else {
      // nDeriv: point, h (default 0.001 if omitted), K.
      emitArg(rArgs[2]);
      out.push_back(Token::Comma);
      if (rArgs.size() == 4) {
        emitArg(rArgs[3]);
      } else {
        out.push_back(Token::Num0);
        out.push_back(Token::Decimal);
        out.push_back(Token::Num0);
        out.push_back(Token::Num0);
        out.push_back(Token::Num1);
      }
      out.push_back(Token::Comma);
    }
    appendIntTokens(out, K);
    out.push_back(Token::RightParen);

    i = rParen;  // continue past the original `)`
  }
  return out;
}

} // namespace

// --- CORE EVALUATOR ---

CalculationResult MathStateMachine::evaluate(const std::vector<Token> &tokensIn,
                                             double xValue) {
  // RAII guard: at the outermost evaluate() entry, clears the deferred
  // table; nested calls (Y-VARS recursion, deferred-call handlers
  // recursing on their captured expression) inherit and append. The
  // table is cleared again on the outermost exit so no state leaks
  // between top-level evaluations.
  EvalGuard depthGuard;

  // Rewrite deferred calls out of the source token stream before any
  // other preprocessing. The output stream contains no surface-level
  // FnInt/NDeriv/Sum/Prod tokens; their synthetic *Call replacements
  // flow through digit-flush / Sto / Y_n / implicit-mul / shunting-yard
  // exactly like any other built-in-paren function.
  bool rewriteOk = true;
  std::vector<Token> rewritten = rewriteDeferredCalls(tokensIn, rewriteOk);
  if (!rewriteOk)
    return {false, 0.0, {}, false, "Syntax Error"};
  const std::vector<Token> &tokens = rewritten;

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
        processedTokens.push_back(Token::NumLiteral);
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
  // Store targets recorded as the raw target Token so both scalar
  // variables (VarA..VarZ) and lists (L1..L6) can be assigned. The
  // evaluator reads this in source order as each Sto fires.
  std::vector<Token> storeTargets;
  stoTokens.reserve(processedTokens.size());
  for (size_t i = 0; i < processedTokens.size(); ++i) {
    if (processedTokens[i] == Token::Sto) {
      const bool hasTarget = (i + 1 < processedTokens.size());
      const Token tgt = hasTarget ? processedTokens[i + 1] : Token::Sto;
      const bool isVar = (tgt >= Token::VarA && tgt <= Token::VarZ);
      const bool isList = (tgt >= Token::L1 && tgt <= Token::L6);
      if (!hasTarget || (!isVar && !isList))
        return {false, 0.0, {}, false, "Syntax Error"};
      storeTargets.push_back(tgt);
      stoTokens.push_back(Token::Sto);
      ++i;  // skip the target — already consumed.
    } else {
      stoTokens.push_back(processedTokens[i]);
    }
  }

  // Y-VARS call-form rewrite: collapse [Y_n, LeftParen] into a single
  // Y_nCall token. Done before implicit-mul so we don't wedge an
  // ImplicitMul between Y_n and its argument's `(`. The call form is
  // a unary function with built-in paren — the argument expression
  // then flows through the standard function-arg pipeline. IMP-042.
  std::vector<Token> ycTokens;
  ycTokens.reserve(stoTokens.size());
  for (size_t i = 0; i < stoTokens.size(); ++i) {
    Token t = stoTokens[i];
    const bool isLeafYn = (t == Token::Y1 || t == Token::Y2 || t == Token::Y3);
    if (isLeafYn &&
        i + 1 < stoTokens.size() &&
        stoTokens[i + 1] == Token::LeftParen) {
      const Token callForm =
          (t == Token::Y1) ? Token::Y1Call :
          (t == Token::Y2) ? Token::Y2Call :
                             Token::Y3Call;
      ycTokens.push_back(callForm);
      ++i;  // skip the LeftParen — the call form has built-in paren
    } else {
      ycTokens.push_back(t);
    }
  }

  // Third preprocessing pass: insert ImplicitMul between juxtaposed
  // value-like tokens so `2π`, `2(3+4)`, `(3)(4)`, `2sin(x)`, `5X`
  // all work without an explicit `×`. IMP-005.
  //
  // `valueLikeEnd`: the previous token produces a value that can be
  // the LHS of a multiplication. `valueLikeStart`: the current token
  // begins something that can be the RHS. When both hold, we inject.
  auto isYn = [](Token t) {
    return t == Token::Y1 || t == Token::Y2 || t == Token::Y3;
  };
  auto valueLikeEnd = [&isYn](Token t) {
    return t == Token::NumLiteral ||
           t == Token::Pi || t == Token::E || t == Token::Ans ||
           t == Token::RightParen || t == Token::Fact ||
           t == Token::RightBrace ||
           (t >= Token::VarA && t <= Token::VarZ) ||
           (t >= Token::MatA && t <= Token::MatJ) ||
           (t >= Token::L1 && t <= Token::L6) ||
           isYn(t);
  };
  auto valueLikeStart = [&isYn](Token t) {
    return t == Token::NumLiteral ||
           t == Token::Pi || t == Token::E || t == Token::Ans ||
           t == Token::LeftParen ||
           t == Token::LeftBrace ||
           (t >= Token::VarA && t <= Token::VarZ) ||
           (t >= Token::MatA && t <= Token::MatJ) ||
           (t >= Token::L1 && t <= Token::L6) ||
           isYn(t) ||
           EOSPrecedence::is_function(t);
  };
  std::vector<Token> finalTokens;
  finalTokens.reserve(ycTokens.size() * 2);
  for (size_t i = 0; i < ycTokens.size(); ++i) {
    if (i > 0 && valueLikeEnd(ycTokens[i - 1]) &&
        valueLikeStart(ycTokens[i])) {
      // Neg is treated as a function by is_function (for shunting-yard
      // unary handling), but `2-3` should never become `2 ImplicitMul
      // (Neg 3)` — the Sub-vs-Neg disambiguation lives in the UI's
      // insertToken, so by the time we see Neg here the user did
      // intend a unary minus. Still skip injection on Neg to keep
      // `2-3` → `2 - 3` rather than `2 * (-3)`. The latter is
      // numerically identical here but loses source structure.
      if (ycTokens[i] != Token::Neg)
        finalTokens.push_back(Token::ImplicitMul);
    }
    finalTokens.push_back(ycTokens[i]);
  }

  std::vector<std::pair<Token, double>> rpn;
  std::stack<Token> opStack;
  // Element counts for open list-literal `{` scopes — parallel to the
  // LeftBrace markers on opStack. Starts at 1 (one element before any
  // comma); each top-level comma inside the braces bumps it.
  std::stack<int> braceCounts;
  int numIdx = 0;
  for (auto t : finalTokens) {
    if (t == Token::NumLiteral)
      rpn.push_back({t, numericValues[numIdx++]});
    else if ((t >= Token::MatA && t <= Token::MatJ) ||
             (t >= Token::VarA && t <= Token::VarZ) ||
             (t >= Token::L1 && t <= Token::L6) ||
             t == Token::Pi || t == Token::E || t == Token::Ans ||
             t == Token::Y1 || t == Token::Y2 || t == Token::Y3)
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
      // Argument separator for binary functions like round(x, n) AND the
      // element separator inside a `{...}` list literal. Pops operators
      // until the enclosing scope marker (LeftParen for a function,
      // LeftBrace for a list). The comma itself isn't pushed — it's just
      // a structural marker. When the enclosing scope is a brace, bump
      // that brace's element count.
      while (!opStack.empty() && opStack.top() != Token::LeftParen &&
             opStack.top() != Token::LeftBrace) {
        rpn.push_back({opStack.top(), 0.0});
        opStack.pop();
      }
      if (!opStack.empty() && opStack.top() == Token::LeftBrace &&
          !braceCounts.empty())
        braceCounts.top()++;
    } else if (t == Token::LeftBrace) {
      // Open a list literal. Marker on the operator stack + a fresh
      // element counter (1 = the element before any comma).
      opStack.push(t);
      braceCounts.push(1);
    } else if (t == Token::RightBrace) {
      // Close a list literal: flush operators back to the LeftBrace,
      // drop the marker, and emit a MakeList carrying the element count.
      while (!opStack.empty() && opStack.top() != Token::LeftBrace) {
        rpn.push_back({opStack.top(), 0.0});
        opStack.pop();
      }
      if (opStack.empty() || braceCounts.empty())
        return {false, 0.0, {}, false, "Syntax Error"};  // unmatched }
      opStack.pop();  // remove the LeftBrace marker
      const int count = braceCounts.top();
      braceCounts.pop();
      rpn.push_back({Token::MakeList, static_cast<double>(count)});
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
    // Phase C lists. Defaulted so existing brace-init sites
    // ({isMat, val, mat}) still compile — the rest zero-fills.
    bool isList = false;
    std::vector<double> list;
  };
  std::stack<Operand> stack;
  auto toB = [](double v) { return std::abs(v) > 1e-9; };

  int storeIdx = 0;
  // Y-VAR cycle guard, hoisted to function scope so both the bare
  // and call-form branches share one set. Cross-form cycles (e.g.
  // bare Y1 reaches Y2(...) which reaches bare Y1) still trip.
  // `static thread_local` so the set persists across nested
  // recursive evaluate() calls in the same thread without leaking
  // state between top-level evaluations.
  static thread_local std::set<int> activeYn;
  for (auto &node : rpn) {
    Token t = node.first;
    if (t == Token::NumLiteral)
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
      stack.push({lastResult.isMatrix, lastResult.value, lastResult.matrixValue,
                  lastResult.isList, lastResult.listValue});
    } else if (t == Token::Y1 || t == Token::Y2 || t == Token::Y3) {
      // Y-VARS bare form — recursively evaluate the referenced buffer
      // at the current xValue. Cycle guard via `activeYn` (declared
      // at function scope above; shared with the call form).
      const int yIdx = (t == Token::Y1) ? 0
                     : (t == Token::Y2) ? 1
                                        : 2;
      if (activeYn.count(yIdx))
        return {false, 0.0, {}, false, "Recursion"};
      if (!yLookup) {
        stack.push({false, 0.0, {}});
        continue;
      }
      std::vector<Token> buf = yLookup(yIdx);
      if (buf.empty()) {
        stack.push({false, 0.0, {}});
        continue;
      }
      activeYn.insert(yIdx);
      MathStateMachine sub;
      CalculationResult subRes = sub.evaluate(buf, xValue);
      activeYn.erase(yIdx);
      if (!subRes.success) return subRes;
      if (subRes.isMatrix)
        return {false, 0.0, {}, false, "Type Error"};
      stack.push({false, subRes.value, {}});
    } else if (t == Token::Y1Call || t == Token::Y2Call ||
               t == Token::Y3Call) {
      // Y-VARS explicit-argument form — pop the argument from the
      // operand stack and recursively evaluate the referenced buffer
      // with X = arg. Shares the activeYn cycle guard with the bare
      // form so mixed cycles are caught. IMP-042.
      if (stack.empty())
        return {false, 0.0, {}, false, "Error"};
      Operand argOp = stack.top();
      stack.pop();
      if (argOp.isMat)
        return {false, 0.0, {}, false, "Type Error"};
      const double argX = argOp.val;
      const int yIdx = (t == Token::Y1Call) ? 0
                     : (t == Token::Y2Call) ? 1
                                            : 2;
      if (activeYn.count(yIdx))
        return {false, 0.0, {}, false, "Recursion"};
      if (!yLookup) {
        stack.push({false, 0.0, {}});
        continue;
      }
      std::vector<Token> buf = yLookup(yIdx);
      if (buf.empty()) {
        stack.push({false, 0.0, {}});
        continue;
      }
      activeYn.insert(yIdx);
      MathStateMachine sub;
      CalculationResult subRes = sub.evaluate(buf, argX);
      activeYn.erase(yIdx);
      if (!subRes.success) return subRes;
      if (subRes.isMatrix)
        return {false, 0.0, {}, false, "Type Error"};
      stack.push({false, subRes.value, {}});
    } else if (t == Token::FnIntCall || t == Token::NDerivCall ||
               t == Token::SumCall   || t == Token::ProdCall) {
      // Deferred-evaluation calculus call. The three operands on the
      // stack are (top → bottom): K (side-table index), second-bound,
      // first-bound. Bound interpretation:
      //   FnIntCall  → first=lower, second=upper
      //   NDerivCall → first=point, second=h
      //   Sum/Prod   → first=start, second=end
      if (stack.size() < 3)
        return {false, 0.0, {}, false, "Error"};
      Operand kOp = stack.top(); stack.pop();
      Operand bOp = stack.top(); stack.pop();
      Operand aOp = stack.top(); stack.pop();
      if (kOp.isMat || bOp.isMat || aOp.isMat)
        return {false, 0.0, {}, false, "Type Error"};
      const int K = static_cast<int>(std::llround(kOp.val));
      if (K < 0 || K >= static_cast<int>(g_deferred.size()))
        return {false, 0.0, {}, false, "Error"};
      const std::vector<Token> &expr = g_deferred[K].expr;
      const int vIdx = g_deferred[K].varIdx;
      const int xIdx = static_cast<int>(Token::VarX) -
                       static_cast<int>(Token::VarA);

      // Sampler: evaluate `expr` with the bound variable set to `v`.
      // When the bound variable is X we also pass `v` as the recursive
      // call's xValue so graph-mode X resolution sees the loop value;
      // for other letters the registry write is enough. The previous
      // varRegistry value is restored after every sample so callers
      // outside the loop see no side effect. The underlying engine
      // error string is captured so the handler can propagate it
      // verbatim (e.g. Recursion / DIVIDE BY 0) — losing it to a
      // generic "Error" was masking real failures behind ERR:SYNTAX.
      std::string sampleErr;
      auto sample = [&](double v, bool &okSamp) -> double {
        double prev = varRegistry[vIdx];
        varRegistry[vIdx] = v;
        MathStateMachine sub;
        CalculationResult r = sub.evaluate(expr, (vIdx == xIdx) ? v : xValue);
        varRegistry[vIdx] = prev;
        if (!r.success) { okSamp = false; sampleErr = r.error_message; return 0.0; }
        if (r.isMatrix)  { okSamp = false; sampleErr = "Type Error"; return 0.0; }
        okSamp = true;
        return r.value;
      };
      auto sampleFail = [&]() {
        return CalculationResult{false, 0.0, {}, false, sampleErr};
      };

      double result = 0.0;
      bool okSamp = true;

      if (t == Token::FnIntCall) {
        double a = aOp.val, b = bOp.val;
        if (a == b) {
          result = 0.0;
        } else {
          bool flipSign = false;
          if (a > b) { std::swap(a, b); flipSign = true; }
          const int N = 100;  // even — composite Simpson's needs even subintervals
          const double h = (b - a) / N;
          double f0 = sample(a, okSamp); if (!okSamp) return sampleFail();
          double fN = sample(b, okSamp); if (!okSamp) return sampleFail();
          double acc = f0 + fN;
          for (int i = 1; i < N; ++i) {
            double xi = a + i * h;
            double vi = sample(xi, okSamp);
            if (!okSamp) return sampleFail();
            acc += (i % 2 == 1) ? 4.0 * vi : 2.0 * vi;
          }
          result = (h / 3.0) * acc;
          if (flipSign) result = -result;
        }
      } else if (t == Token::NDerivCall) {
        const double xPt = aOp.val;
        const double h = bOp.val;
        if (h == 0.0)
          return {false, 0.0, {}, false, "DOMAIN"};
        double fp = sample(xPt + h, okSamp); if (!okSamp) return sampleFail();
        double fm = sample(xPt - h, okSamp); if (!okSamp) return sampleFail();
        result = (fp - fm) / (2.0 * h);
      } else {
        // sum / prod — integer iteration, inclusive. TI-83 convention
        // floors fractional bounds; an empty range (start > end)
        // returns the identity element (0 for sum, 1 for prod). Cap
        // the iteration count so a runaway `sum(X, X, 1, 1e12)` can't
        // wedge the engine.
        const long long start = static_cast<long long>(std::floor(aOp.val));
        const long long end   = static_cast<long long>(std::floor(bOp.val));
        const long long span  = (end >= start) ? (end - start + 1) : 0;
        constexpr long long kIterCap = 100000;
        if (span > kIterCap)
          return {false, 0.0, {}, false, "DOMAIN"};
        if (t == Token::SumCall) {
          double acc = 0.0;
          for (long long k = start; k <= end; ++k) {
            double v = sample(static_cast<double>(k), okSamp);
            if (!okSamp) return sampleFail();
            acc += v;
          }
          result = acc;
        } else {
          double acc = 1.0;
          for (long long k = start; k <= end; ++k) {
            double v = sample(static_cast<double>(k), okSamp);
            if (!okSamp) return sampleFail();
            acc *= v;
          }
          result = acc;
        }
      }

      stack.push({false, result, {}});
    } else if (t == Token::SeqCall) {
      // seq(expr, var, start, end, step) → list. Operands on the stack
      // (top → bottom): K (side-table index), step, end, start.
      if (stack.size() < 4)
        return {false, 0.0, {}, false, "Error"};
      Operand kOp = stack.top(); stack.pop();
      Operand stepOp = stack.top(); stack.pop();
      Operand endOp = stack.top(); stack.pop();
      Operand startOp = stack.top(); stack.pop();
      if (kOp.isMat || stepOp.isMat || endOp.isMat || startOp.isMat ||
          kOp.isList || stepOp.isList || endOp.isList || startOp.isList)
        return {false, 0.0, {}, false, "Type Error"};
      const int K = static_cast<int>(std::llround(kOp.val));
      if (K < 0 || K >= static_cast<int>(g_deferred.size()))
        return {false, 0.0, {}, false, "Error"};
      const std::vector<Token> &expr = g_deferred[K].expr;
      const int vIdx = g_deferred[K].varIdx;
      const int xIdx = static_cast<int>(Token::VarX) -
                       static_cast<int>(Token::VarA);
      const double start = startOp.val, end = endOp.val, step = stepOp.val;
      if (step == 0.0)
        return {false, 0.0, {}, false, "DOMAIN"};
      // Count elements up-front (v = start + i*step) rather than
      // accumulating, so float drift can't shift the endpoint. A
      // backwards/empty range is ERR:INVALID DIM.
      const long long count =
          static_cast<long long>(std::floor((end - start) / step + 1e-9)) + 1;
      if (count < 1)
        return {false, 0.0, {}, false, "Dim Mismatch"};
      constexpr long long kIterCap = 100000;
      if (count > kIterCap)
        return {false, 0.0, {}, false, "DOMAIN"};
      std::vector<double> outList;
      outList.reserve(static_cast<size_t>(count));
      for (long long i = 0; i < count; ++i) {
        const double v = start + static_cast<double>(i) * step;
        const double prev = varRegistry[vIdx];
        varRegistry[vIdx] = v;
        MathStateMachine sub;
        CalculationResult r = sub.evaluate(expr, (vIdx == xIdx) ? v : xValue);
        varRegistry[vIdx] = prev;
        if (!r.success) return r;
        if (r.isMatrix || r.isList)
          return {false, 0.0, {}, false, "Type Error"};
        outList.push_back(r.value);
      }
      Operand o;
      o.isMat = false;
      o.val = 0.0;
      o.isList = true;
      o.list = std::move(outList);
      stack.push(o);
    } else if (t == Token::Sto) {
      // Write the top-of-stack value into its target registry and push
      // it back so the display reflects the stored value. Target type
      // must match the value: a scalar var takes a scalar, an L1..L6
      // list takes a list. A mismatch is ERR:DATA TYPE (matches TI-83).
      if (stack.empty())
        return {false, 0.0, {}, false, "Error"};
      Operand v = stack.top();
      stack.pop();
      const Token tgt = storeTargets[storeIdx++];
      if (tgt >= Token::VarA && tgt <= Token::VarZ) {
        if (v.isMat || v.isList)
          return {false, 0.0, {}, false, "Type Error"};
        varRegistry[(int)tgt - (int)Token::VarA] = v.val;
      } else {  // L1..L6
        if (!v.isList)
          return {false, 0.0, {}, false, "Type Error"};
        listRegistry[tgt] = v.list;
      }
      stack.push(v);
    } else if (t >= Token::L1 && t <= Token::L6) {
      // List leaf — resolve from the registry. Absent slot is undefined.
      auto it = listRegistry.find(t);
      if (it == listRegistry.end())
        return {false, 0.0, {}, false, "Undefined List"};
      Operand o;
      o.isMat = false;
      o.val = 0.0;
      o.isList = true;
      o.list = it->second;
      stack.push(o);
    } else if (t == Token::MakeList) {
      // Assemble a list literal: pop `count` operands (pushed in source
      // order, so they come off the stack reversed) into a list. Nested
      // lists / matrices as elements are rejected.
      const int count = static_cast<int>(std::llround(node.second));
      if (count <= 0 || static_cast<int>(stack.size()) < count)
        return {false, 0.0, {}, false, "Syntax Error"};
      std::vector<double> lst(static_cast<size_t>(count));
      for (int i = count - 1; i >= 0; --i) {
        Operand op = stack.top();
        stack.pop();
        if (op.isMat || op.isList)
          return {false, 0.0, {}, false, "Type Error"};
        lst[static_cast<size_t>(i)] = op.val;
      }
      Operand o;
      o.isMat = false;
      o.val = 0.0;
      o.isList = true;
      o.list = std::move(lst);
      stack.push(o);
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
      if (a.isMat || a.isList)
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
      // min(/max( list overload (Phase C): when the top operand is a
      // list, this is the single-arg list reduction (min/max element),
      // not the two-scalar form. Peek before the binary pop so `min(L1)`
      // and `min(a,b)` both work. Mixing a scalar and a list, or two
      // lists, in one min/max call is not supported (Wave 3 limitation)
      // — the list operand ends up on top and gets reduced alone.
      if ((t == Token::Min || t == Token::Max) && !stack.empty() &&
          stack.top().isList) {
        Operand a = stack.top();
        stack.pop();
        if (a.list.empty())
          return {false, 0.0, {}, false, "DOMAIN"};
        double r = a.list[0];
        for (double v : a.list)
          r = (t == Token::Min) ? std::min(r, v) : std::max(r, v);
        stack.push({false, r, {}});
        continue;
      }
      // Binary functions (round, min, max, mod) — pop two operands.
      if (EOSPrecedence::is_binary_function(t)) {
        if (stack.size() < 2)
          return {false, 0.0, {}, false, "Error"};
        Operand b = stack.top();
        stack.pop();
        Operand a = stack.top();
        stack.pop();
        if (a.isMat || b.isMat || a.isList || b.isList)
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

      // List reductions (Phase C Wave 3) — these REQUIRE a list operand
      // and return a scalar. Handled before the generic list-reject
      // guard below.
      if (t == Token::ListSum || t == Token::ListProd ||
          t == Token::Mean || t == Token::StdDev ||
          t == Token::Variance || t == Token::Median) {
        if (!a.isList)
          return {false, 0.0, {}, false, "Type Error"};
        const std::vector<double> &L = a.list;
        const size_t n = L.size();
        double res = 0.0;
        if (t == Token::ListSum) {
          for (double v : L) res += v;
        } else if (t == Token::ListProd) {
          res = 1.0;
          for (double v : L) res *= v;
        } else if (t == Token::Mean) {
          if (n == 0)
            return {false, 0.0, {}, false, "DOMAIN"};
          double s = 0.0;
          for (double v : L) s += v;
          res = s / static_cast<double>(n);
        } else if (t == Token::Median) {
          if (n == 0)
            return {false, 0.0, {}, false, "DOMAIN"};
          std::vector<double> s = L;
          std::sort(s.begin(), s.end());
          res = (n % 2 == 1) ? s[n / 2]
                             : (s[n / 2 - 1] + s[n / 2]) / 2.0;
        } else {  // StdDev / Variance — sample (n-1 denominator)
          if (n < 2)
            return {false, 0.0, {}, false, "DOMAIN"};
          double s = 0.0;
          for (double v : L) s += v;
          const double m = s / static_cast<double>(n);
          double ss = 0.0;
          for (double v : L) ss += (v - m) * (v - m);
          const double var = ss / static_cast<double>(n - 1);
          res = (t == Token::Variance) ? var : std::sqrt(var);
        }
        stack.push({false, res, {}});
        continue;
      }

      // No other unary function accepts a list operand (element-wise
      // function mapping over lists is a later wave). Reject rather than
      // silently reading a.val (which is 0 for a list).
      if (a.isList)
        return {false, 0.0, {}, false, "Type Error"};

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
        else if (t == Token::Exp)
          v = std::exp(v);
        else if (t == Token::Sgn)
          v = (v > 0.0) ? 1.0 : (v < 0.0 ? -1.0 : 0.0);
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

      // List arithmetic (Phase C): element-wise +,-,*,/,^ with scalar
      // broadcasting. list⊕list requires equal length; matrices are
      // rejected. Other operators (comparisons/logic) on lists aren't
      // supported yet. Handled ahead of the scalar/matrix branches so a
      // single `continue` skips them once a list operand is involved.
      if (a.isList || b.isList) {
        if (a.isMat || b.isMat)
          return {false, 0.0, {}, false, "Type Error"};
        const bool arith =
            (t == Token::Add || t == Token::Sub || t == Token::Mul ||
             t == Token::ImplicitMul || t == Token::Div || t == Token::Pow);
        if (!arith)
          return {false, 0.0, {}, false, "Type Error"};
        if (a.isList && b.isList && a.list.size() != b.list.size())
          return {false, 0.0, {}, false, "Dim Mismatch"};
        const size_t n = a.isList ? a.list.size() : b.list.size();
        Operand out;
        out.isMat = false;
        out.val = 0.0;
        out.isList = true;
        out.list.resize(n);
        for (size_t i = 0; i < n; ++i) {
          const double av = a.isList ? a.list[i] : a.val;
          const double bv = b.isList ? b.list[i] : b.val;
          double r = 0.0;
          if (t == Token::Add)
            r = av + bv;
          else if (t == Token::Sub)
            r = av - bv;
          else if (t == Token::Mul || t == Token::ImplicitMul)
            r = av * bv;
          else if (t == Token::Div) {
            if (bv == 0.0)
              return {false, 0.0, {}, false, "DIVIDE BY 0"};
            r = av / bv;
          } else  // Pow
            r = std::pow(av, bv);
          out.list[i] = r;
        }
        stack.push(out);
        continue;
      }

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
  return {true, res.val, res.mat, res.isMat, "", res.isList, res.list};
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

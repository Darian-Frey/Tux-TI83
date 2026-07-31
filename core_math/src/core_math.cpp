#include "capsules/capsule_math.hpp"
#include <algorithm>
#include <cmath>
#include <complex>
#include <map>
#include <set>
#include <stack>
#include <string>

namespace tux_ti83 {

std::map<Token, Matrix> MathStateMachine::matrixRegistry;
std::map<Token, std::vector<double>> MathStateMachine::listRegistry;
std::mt19937 MathStateMachine::rng{std::random_device{}()};

void MathStateMachine::seedRandom(unsigned int seed) { rng.seed(seed); }
std::array<double, 26> MathStateMachine::varRegistry{};
AngleMode MathStateMachine::angleMode = AngleMode::Radian;
NumberNotation MathStateMachine::notation = NumberNotation::Normal;
int MathStateMachine::fixDecimals = -1;  // -1 = Float (no fix)
NumberBase MathStateMachine::numberBase = NumberBase::Dec;
ComplexMode MathStateMachine::complexMode = ComplexMode::Real;
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
  case Token::Identity:
  case Token::Dim:
  case Token::Ref:
  case Token::Augment:
  case Token::RandM:
  case Token::ListToMatr:
  case Token::MatrToList:
  case Token::Y1Call:
  case Token::Y2Call:
  case Token::Y3Call:
  case Token::Y4Call:
  case Token::Y5Call:
  case Token::Y6Call:
  case Token::Y7Call:
  case Token::Y8Call:
  case Token::Y9Call:
  case Token::Y0Call:
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
          t == Token::Identity || t == Token::Dim || t == Token::Ref ||
          (t >= Token::Y1Call && t <= Token::Y0Call) ||
          t == Token::FnIntCall || t == Token::NDerivCall ||
          t == Token::SumCall || t == Token::ProdCall ||
          t == Token::Sinh || t == Token::Cosh || t == Token::Tanh ||
          t == Token::ASinh || t == Token::ACosh || t == Token::ATanh ||
          t == Token::Mean || t == Token::StdDev || t == Token::Variance ||
          t == Token::ListSum || t == Token::ListProd || t == Token::Median ||
          t == Token::SeqCall ||
          t == Token::RandInt || t == Token::RandNorm || t == Token::RandBin ||
          t == Token::RandIntList || t == Token::RandNormList ||
          t == Token::RandBinList ||
          t == Token::NormalPdf || t == Token::NormalCdf ||
          t == Token::InvNorm ||
          t == Token::BinomPdf || t == Token::BinomCdf ||
          t == Token::BinomPdfList || t == Token::BinomCdfList ||
          t == Token::PoissonPdf || t == Token::PoissonCdf ||
          t == Token::GeometPdf || t == Token::GeometCdf ||
          t == Token::TPdf || t == Token::TCdf ||
          t == Token::ChiPdf || t == Token::ChiCdf ||
          t == Token::FPdf || t == Token::FCdf ||
          t == Token::Conj || t == Token::RealPart ||
          t == Token::ImagPart || t == Token::Angle ||
          is_binary_function(t));
}

bool EOSPrecedence::is_binary_function(Token t) {
  return (t == Token::Round || t == Token::Min ||
          t == Token::Max || t == Token::Mod ||
          t == Token::NCr || t == Token::NPr ||
          t == Token::Augment || t == Token::RandM ||
          t == Token::ListToMatr || t == Token::MatrToList);
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
          t == Token::Identity || t == Token::Dim || t == Token::Ref ||
          (t >= Token::Y1Call && t <= Token::Y0Call) ||
          t == Token::FnIntCall || t == Token::NDerivCall ||
          t == Token::SumCall || t == Token::ProdCall ||
          t == Token::Det || t == Token::Transpose ||
          t == Token::Rref ||
          t == Token::Round || t == Token::Min ||
          t == Token::Max || t == Token::Mod ||
          t == Token::NCr || t == Token::NPr ||
          t == Token::Augment || t == Token::RandM ||
          t == Token::ListToMatr || t == Token::MatrToList ||
          t == Token::Sinh || t == Token::Cosh || t == Token::Tanh ||
          t == Token::ASinh || t == Token::ACosh || t == Token::ATanh ||
          t == Token::Mean || t == Token::StdDev || t == Token::Variance ||
          t == Token::ListSum || t == Token::ListProd || t == Token::Median ||
          t == Token::SeqCall ||
          t == Token::RandInt || t == Token::RandNorm || t == Token::RandBin ||
          t == Token::RandIntList || t == Token::RandNormList ||
          t == Token::RandBinList ||
          t == Token::NormalPdf || t == Token::NormalCdf ||
          t == Token::InvNorm ||
          t == Token::BinomPdf || t == Token::BinomCdf ||
          t == Token::BinomPdfList || t == Token::BinomCdfList ||
          t == Token::PoissonPdf || t == Token::PoissonCdf ||
          t == Token::GeometPdf || t == Token::GeometCdf ||
          t == Token::TPdf || t == Token::TCdf ||
          t == Token::ChiPdf || t == Token::ChiCdf ||
          t == Token::FPdf || t == Token::FCdf ||
          t == Token::Conj || t == Token::RealPart ||
          t == Token::ImagPart || t == Token::Angle);
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

// Row-echelon form (not reduced): forward Gaussian elimination with
// partial pivoting. Each pivot row is normalised to a leading 1 and
// used to zero the entries BELOW it only — unlike rrefInPlace, rows
// above a pivot are left untouched, yielding the upper-triangular
// echelon form the TI-83's `ref(` returns.
void refInPlace(Matrix &m) {
  int rows = m.rows;
  int cols = m.cols;
  int lead = 0;
  for (int r = 0; r < rows; ++r) {
    if (lead >= cols)
      break;
    // Partial pivot: pick the largest-magnitude entry in this column at
    // or below row r for numerical stability.
    int pivRow = r;
    double best = std::abs(m.at(r, lead));
    for (int i = r + 1; i < rows; ++i) {
      double v = std::abs(m.at(i, lead));
      if (v > best) { best = v; pivRow = i; }
    }
    if (best < 1e-12) {
      // No pivot in this column — advance to the next column, same row.
      ++lead;
      --r;
      continue;
    }
    if (pivRow != r) {
      for (int j = 0; j < cols; ++j) {
        double tmp = m.at(r, j);
        m.set(r, j, m.at(pivRow, j));
        m.set(pivRow, j, tmp);
      }
    }
    double pivot = m.at(r, lead);
    for (int j = 0; j < cols; ++j)
      m.set(r, j, m.at(r, j) / pivot);
    for (int ri = r + 1; ri < rows; ++ri) {
      double factor = m.at(ri, lead);
      for (int j = 0; j < cols; ++j)
        m.set(ri, j, m.at(ri, j) - factor * m.at(r, j));
    }
    ++lead;
  }
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

// Select the scalar vs list variant of randInt/randNorm/randBin by
// counting arguments: a 3rd (`count`) argument switches to the list
// form. Only the function token is swapped in place — the arguments are
// left untouched, so no re-emission is needed. Runs before the
// shunting-yard while the comma structure is intact.
std::vector<Token> rewriteRandCalls(const std::vector<Token> &tokens) {
  std::vector<Token> out = tokens;
  for (size_t i = 0; i < out.size(); ++i) {
    Token listVariant;
    switch (out[i]) {
      case Token::RandInt:  listVariant = Token::RandIntList;  break;
      case Token::RandNorm: listVariant = Token::RandNormList; break;
      case Token::RandBin:  listVariant = Token::RandBinList;  break;
      default: continue;
    }
    // These are built-in-paren functions, so the scope is already open
    // right after the token. Count top-level commas up to the match.
    int depth = 1, commas = 0;
    for (size_t j = i + 1; j < out.size(); ++j) {
      const Token u = out[j];
      if (opensParenScope(u) || u == Token::LeftBrace) ++depth;
      else if (u == Token::RightParen || u == Token::RightBrace) {
        if (--depth == 0) break;
      } else if (u == Token::Comma && depth == 1) {
        ++commas;
      }
    }
    if (commas == 2) out[i] = listVariant;  // 3 args → list form
  }
  return out;
}

// binompdf/binomcdf select the whole-distribution list form when called
// with 2 arguments (n, p) instead of 3 (n, p, x). Mirrors
// rewriteRandCalls but with the opposite threshold: 1 top-level comma
// (2 args) → the …List variant.
std::vector<Token> rewriteBinomCalls(const std::vector<Token> &tokens) {
  std::vector<Token> out = tokens;
  for (size_t i = 0; i < out.size(); ++i) {
    Token listVariant;
    switch (out[i]) {
      case Token::BinomPdf: listVariant = Token::BinomPdfList; break;
      case Token::BinomCdf: listVariant = Token::BinomCdfList; break;
      default: continue;
    }
    int depth = 1, commas = 0;
    for (size_t j = i + 1; j < out.size(); ++j) {
      const Token u = out[j];
      if (opensParenScope(u) || u == Token::LeftBrace) ++depth;
      else if (u == Token::RightParen || u == Token::RightBrace) {
        if (--depth == 0) break;
      } else if (u == Token::Comma && depth == 1) {
        ++commas;
      }
    }
    if (commas == 1) out[i] = listVariant;  // 2 args → list form
  }
  return out;
}

// Pad the optional (μ, σ) arguments of the normal-distribution functions
// so the evaluator always sees a fixed arity. normalpdf/invNorm → 3
// args, normalcdf → 4. A missing final argument defaults to σ=1; any
// other missing argument defaults to μ=0 (the last slot is always σ).
std::vector<Token> rewriteDistCalls(const std::vector<Token> &tokens) {
  std::vector<Token> out;
  out.reserve(tokens.size());
  for (size_t i = 0; i < tokens.size(); ++i) {
    const Token t = tokens[i];
    int target = 0;
    if (t == Token::NormalPdf || t == Token::InvNorm) target = 3;
    else if (t == Token::NormalCdf) target = 4;
    else { out.push_back(t); continue; }

    int depth = 1;
    int rp = -1;
    for (size_t j = i + 1; j < tokens.size(); ++j) {
      if (opensParenScope(tokens[j]) || tokens[j] == Token::LeftBrace) ++depth;
      else if (tokens[j] == Token::RightParen ||
               tokens[j] == Token::RightBrace) {
        if (--depth == 0) { rp = static_cast<int>(j); break; }
      }
    }
    if (rp < 0) { out.push_back(t); continue; }  // malformed — leave as-is

    auto args = splitByComma(tokens, static_cast<int>(i) + 1, rp);
    out.push_back(t);
    for (int k = 0; k < target; ++k) {
      if (k > 0) out.push_back(Token::Comma);
      if (k < static_cast<int>(args.size()) && !args[k].empty()) {
        // Recurse so a nested distribution call in an argument (e.g.
        // normalcdf(-50, invNorm(0.9))) gets padded too.
        auto padded = rewriteDistCalls(args[k]);
        for (Token x : padded) out.push_back(x);
      } else {
        out.push_back(k == target - 1 ? Token::Num1 : Token::Num0);
      }
    }
    out.push_back(Token::RightParen);
    i = static_cast<size_t>(rp);
  }
  return out;
}

// Inverse of the standard-normal CDF (quantile) via Acklam's rational
// approximation — accurate to ~1e-9 over p ∈ (0, 1).
double invNormStd(double p) {
  static const double a[] = {-3.969683028665376e+01, 2.209460984245205e+02,
                             -2.759285104469687e+02, 1.383577518672690e+02,
                             -3.066479806614716e+01, 2.506628277459239e+00};
  static const double b[] = {-5.447609879822406e+01, 1.615858368580409e+02,
                             -1.556989798598866e+02, 6.680131188771972e+01,
                             -1.328068155288572e+01};
  static const double c[] = {-7.784894002430293e-03, -3.223964580411365e-01,
                             -2.400758277161838e+00, -2.549732539343734e+00,
                             4.374664141464968e+00, 2.938163982698783e+00};
  static const double d[] = {7.784695709041462e-03, 3.224671290700398e-01,
                             2.445134137142996e+00, 3.754408661907416e+00};
  const double plow = 0.02425, phigh = 1.0 - plow;
  if (p < plow) {
    double q = std::sqrt(-2.0 * std::log(p));
    return (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
           ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
  } else if (p <= phigh) {
    double q = p - 0.5, r = q * q;
    return (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) *
           q / (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1.0);
  }
  double q = std::sqrt(-2.0 * std::log(1.0 - p));
  return -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
         ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
}

// Binomial coefficient C(n, k) computed multiplicatively to avoid
// overflowing an intermediate factorial. 0 for k outside [0, n].
double nCk(long long n, long long k) {
  if (k < 0 || k > n) return 0.0;
  if (k > n - k) k = n - k;
  double r = 1.0;
  for (long long i = 0; i < k; ++i)
    r = r * static_cast<double>(n - i) / static_cast<double>(i + 1);
  return r;
}

// P(X = x) for Binomial(n, p): C(n,x) pˣ (1-p)^(n-x). 0 outside [0, n].
double binomPdfVal(long long n, double p, long long x) {
  if (x < 0 || x > n) return 0.0;
  return nCk(n, x) * std::pow(p, static_cast<double>(x)) *
         std::pow(1.0 - p, static_cast<double>(n - x));
}

// P(X = x) for Poisson(μ): e^{-μ} μˣ / x!, built iteratively for stability.
double poissonPdfVal(double mu, long long x) {
  if (x < 0) return 0.0;
  double t = std::exp(-mu);
  for (long long i = 1; i <= x; ++i) t *= mu / static_cast<double>(i);
  return t;
}

// --- Special functions for continuous-distribution CDFs (NR-style) ---

// Regularized lower incomplete gamma P(a, x) = γ(a,x)/Γ(a). Series for
// x < a+1, continued fraction (for Q) otherwise. Used by χ²cdf.
double gammaP(double a, double x) {
  if (x <= 0.0 || a <= 0.0) return 0.0;
  const double gln = std::lgamma(a);
  if (x < a + 1.0) {
    double ap = a, del = 1.0 / a, sum = del;
    for (int nn = 0; nn < 300; ++nn) {
      ap += 1.0;
      del *= x / ap;
      sum += del;
      if (std::abs(del) < std::abs(sum) * 1e-15) break;
    }
    return sum * std::exp(-x + a * std::log(x) - gln);
  }
  const double FPMIN = 1e-300;
  double b = x + 1.0 - a, c = 1.0 / FPMIN, d = 1.0 / b, h = d;
  for (int i = 1; i <= 300; ++i) {
    const double an = -i * (i - a);
    b += 2.0;
    d = an * d + b; if (std::abs(d) < FPMIN) d = FPMIN;
    c = b + an / c;  if (std::abs(c) < FPMIN) c = FPMIN;
    d = 1.0 / d;
    const double del = d * c;
    h *= del;
    if (std::abs(del - 1.0) < 1e-15) break;
  }
  const double Q = std::exp(-x + a * std::log(x) - gln) * h;
  return 1.0 - Q;
}

// Continued fraction for the incomplete beta (Lentz's method).
double betacf(double a, double b, double x) {
  const double FPMIN = 1e-300, EPS = 1e-15;
  const double qab = a + b, qap = a + 1.0, qam = a - 1.0;
  double c = 1.0, d = 1.0 - qab * x / qap;
  if (std::abs(d) < FPMIN) d = FPMIN;
  d = 1.0 / d;
  double h = d;
  for (int m = 1; m <= 300; ++m) {
    const int m2 = 2 * m;
    double aa = m * (b - m) * x / ((qam + m2) * (a + m2));
    d = 1.0 + aa * d; if (std::abs(d) < FPMIN) d = FPMIN;
    c = 1.0 + aa / c; if (std::abs(c) < FPMIN) c = FPMIN;
    d = 1.0 / d; h *= d * c;
    aa = -(a + m) * (qab + m) * x / ((a + m2) * (qap + m2));
    d = 1.0 + aa * d; if (std::abs(d) < FPMIN) d = FPMIN;
    c = 1.0 + aa / c; if (std::abs(c) < FPMIN) c = FPMIN;
    d = 1.0 / d;
    const double del = d * c;
    h *= del;
    if (std::abs(del - 1.0) < EPS) break;
  }
  return h;
}

// Regularized incomplete beta I_x(a, b). Used by tcdf and Fcdf.
double betai(double a, double b, double x) {
  if (x <= 0.0) return 0.0;
  if (x >= 1.0) return 1.0;
  const double bt = std::exp(std::lgamma(a + b) - std::lgamma(a) -
                             std::lgamma(b) + a * std::log(x) +
                             b * std::log(1.0 - x));
  if (x < (a + 1.0) / (a + b + 2.0))
    return bt * betacf(a, b, x) / a;
  return 1.0 - bt * betacf(b, a, 1.0 - x) / b;
}

// CDF evaluated at a single point for each continuous distribution.
double chiCdfAt(double x, double k)  { return (x <= 0.0) ? 0.0 : gammaP(k / 2.0, x / 2.0); }
double fCdfAt(double x, double d1, double d2) {
  return (x <= 0.0) ? 0.0 : betai(d1 / 2.0, d2 / 2.0, d1 * x / (d1 * x + d2));
}
double tCdfAt(double x, double nu) {
  const double ib = betai(nu / 2.0, 0.5, nu / (nu + x * x));
  return (x >= 0.0) ? (1.0 - 0.5 * ib) : (0.5 * ib);
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
  rewritten = rewriteRandCalls(rewritten);
  rewritten = rewriteBinomCalls(rewritten);
  rewritten = rewriteDistCalls(rewritten);
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
    const bool isLeafYn = (t >= Token::Y1 && t <= Token::Y0);
    if (isLeafYn &&
        i + 1 < stoTokens.size() &&
        stoTokens[i + 1] == Token::LeftParen) {
      const Token callForm = static_cast<Token>(
          static_cast<int>(Token::Y1Call) +
          (static_cast<int>(t) - static_cast<int>(Token::Y1)));
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
    return t >= Token::Y1 && t <= Token::Y0;
  };
  auto valueLikeEnd = [&isYn](Token t) {
    return t == Token::NumLiteral ||
           t == Token::Pi || t == Token::E || t == Token::Ans ||
           t == Token::ImagI ||
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
           t == Token::ImagI ||
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
           t == Token::ImagI ||
             t == Token::Rand ||
             (t >= Token::Y1 && t <= Token::Y0))
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
    // Defaulted so default-constructed `Operand o;` (used by the list
    // and complex code) is a well-formed real 0 — otherwise isMat/val
    // would be uninitialised garbage. Brace-init sites still work.
    bool isMat = false;
    double val = 0.0;
    Matrix mat;
    // Phase C lists.
    bool isList = false;
    std::vector<double> list;
    // Complex (Phase F): `val` = real part, `imag` = imaginary part.
    double imag = 0.0;
  };
  // Is this operand a (non-real) complex number?
  auto isComplex = [](const Operand &o) { return o.imag != 0.0; };
  auto toC = [](const Operand &o) { return std::complex<double>(o.val, o.imag); };
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
    else if (t == Token::ImagI) {
      Operand o;  // the imaginary unit i = 0 + 1i
      o.imag = 1.0;
      stack.push(o);
    }
    else if (t == Token::Rand)
      stack.push({false,
                  std::uniform_real_distribution<double>(0.0, 1.0)(rng), {}});
    else if (t == Token::Ans) {
      // Recall the last successful evaluation result. Defaults to the
      // scalar 0 on first use (matches TI-83 power-on state).
      stack.push({lastResult.isMatrix, lastResult.value, lastResult.matrixValue,
                  lastResult.isList, lastResult.listValue, lastResult.imag});
    } else if (t >= Token::Y1 && t <= Token::Y0) {
      // Y-VARS bare form — recursively evaluate the referenced buffer
      // at the current xValue. Cycle guard via `activeYn` (declared
      // at function scope above; shared with the call form).
      const int yIdx = static_cast<int>(t) - static_cast<int>(Token::Y1);
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
    } else if (t >= Token::Y1Call && t <= Token::Y0Call) {
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
      const int yIdx = static_cast<int>(t) - static_cast<int>(Token::Y1Call);
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
      // Random functions (Phase C Wave 5). Scalar 2-arg forms and 3-arg
      // list forms, handled explicitly (custom arity) before the generic
      // binary/unary dispatch below. A helper draws one sample given the
      // two distribution parameters.
      auto randSample = [&](Token kind, double p1, double p2,
                            bool &okDraw) -> double {
        okDraw = true;
        if (kind == Token::RandInt || kind == Token::RandIntList) {
          long long lo = static_cast<long long>(std::floor(p1));
          long long hi = static_cast<long long>(std::floor(p2));
          if (lo > hi) { okDraw = false; return 0.0; }
          return static_cast<double>(
              std::uniform_int_distribution<long long>(lo, hi)(rng));
        }
        if (kind == Token::RandNorm || kind == Token::RandNormList) {
          if (p2 <= 0.0) { okDraw = false; return 0.0; }  // sd must be > 0
          return std::normal_distribution<double>(p1, p2)(rng);
        }
        // RandBin: p1 = trials n (≥0 integer), p2 = probability [0,1].
        long long trials = static_cast<long long>(std::floor(p1));
        if (trials < 0 || p2 < 0.0 || p2 > 1.0) { okDraw = false; return 0.0; }
        return static_cast<double>(
            std::binomial_distribution<long long>(trials, p2)(rng));
      };

      if (t == Token::RandInt || t == Token::RandNorm ||
          t == Token::RandBin) {
        if (stack.size() < 2)
          return {false, 0.0, {}, false, "Error"};
        Operand b = stack.top(); stack.pop();
        Operand a = stack.top(); stack.pop();
        if (a.isMat || b.isMat || a.isList || b.isList)
          return {false, 0.0, {}, false, "Type Error"};
        bool okDraw = true;
        double v = randSample(t, a.val, b.val, okDraw);
        if (!okDraw) return {false, 0.0, {}, false, "DOMAIN"};
        stack.push({false, v, {}});
        continue;
      }
      if (t == Token::RandIntList || t == Token::RandNormList ||
          t == Token::RandBinList) {
        if (stack.size() < 3)
          return {false, 0.0, {}, false, "Error"};
        Operand cOp = stack.top(); stack.pop();  // count (3rd arg)
        Operand b = stack.top(); stack.pop();
        Operand a = stack.top(); stack.pop();
        if (a.isMat || b.isMat || cOp.isMat ||
            a.isList || b.isList || cOp.isList)
          return {false, 0.0, {}, false, "Type Error"};
        const long long count = static_cast<long long>(std::floor(cOp.val));
        if (count < 1 || count > 100000)
          return {false, 0.0, {}, false, "DOMAIN"};
        std::vector<double> lst;
        lst.reserve(static_cast<size_t>(count));
        for (long long k = 0; k < count; ++k) {
          bool okDraw = true;
          double v = randSample(t, a.val, b.val, okDraw);
          if (!okDraw) return {false, 0.0, {}, false, "DOMAIN"};
          lst.push_back(v);
        }
        Operand o;
        o.isMat = false; o.val = 0.0; o.isList = true;
        o.list = std::move(lst);
        stack.push(o);
        continue;
      }

      // Normal distribution family (padded to fixed arity by
      // rewriteDistCalls). σ must be > 0; scalars only.
      if (t == Token::NormalPdf || t == Token::InvNorm) {
        if (stack.size() < 3)
          return {false, 0.0, {}, false, "Error"};
        Operand sOp = stack.top(); stack.pop();  // σ
        Operand mOp = stack.top(); stack.pop();  // μ
        Operand xOp = stack.top(); stack.pop();  // x or area
        if (sOp.isMat || mOp.isMat || xOp.isMat ||
            sOp.isList || mOp.isList || xOp.isList)
          return {false, 0.0, {}, false, "Type Error"};
        const double sigma = sOp.val, mu = mOp.val, x = xOp.val;
        if (sigma <= 0.0)
          return {false, 0.0, {}, false, "DOMAIN"};
        if (t == Token::NormalPdf) {
          const double z = (x - mu) / sigma;
          stack.push({false,
                      std::exp(-0.5 * z * z) /
                          (sigma * std::sqrt(2.0 * M_PI)),
                      {}});
        } else {  // InvNorm — x is the area, must be in (0,1)
          if (x <= 0.0 || x >= 1.0)
            return {false, 0.0, {}, false, "DOMAIN"};
          stack.push({false, mu + sigma * invNormStd(x), {}});
        }
        continue;
      }
      if (t == Token::NormalCdf) {
        if (stack.size() < 4)
          return {false, 0.0, {}, false, "Error"};
        Operand sOp = stack.top(); stack.pop();  // σ
        Operand mOp = stack.top(); stack.pop();  // μ
        Operand uOp = stack.top(); stack.pop();  // upper
        Operand lOp = stack.top(); stack.pop();  // lower
        if (sOp.isMat || mOp.isMat || uOp.isMat || lOp.isMat ||
            sOp.isList || mOp.isList || uOp.isList || lOp.isList)
          return {false, 0.0, {}, false, "Type Error"};
        const double sigma = sOp.val, mu = mOp.val;
        if (sigma <= 0.0)
          return {false, 0.0, {}, false, "DOMAIN"};
        auto Phi = [&](double v) {
          return 0.5 * (1.0 + std::erf((v - mu) / (sigma * std::sqrt(2.0))));
        };
        stack.push({false, Phi(uOp.val) - Phi(lOp.val), {}});
        continue;
      }

      // Discrete distributions (Phase C follow-on). Cap on n / x sums so
      // a giant parameter can't wedge the engine.
      constexpr long long kDistCap = 100000;
      // Pop a fixed count of scalar operands (top → bottom into vals[0..]),
      // rejecting matrices/lists. Returns false on type error.
      auto popScalars = [&](int count, std::vector<double> &vals) -> bool {
        vals.assign(static_cast<size_t>(count), 0.0);
        for (int k = 0; k < count; ++k) {
          Operand o = stack.top(); stack.pop();
          if (o.isMat || o.isList) return false;
          vals[static_cast<size_t>(k)] = o.val;  // vals[0] = top
        }
        return true;
      };

      if (t == Token::BinomPdf || t == Token::BinomCdf) {
        if (stack.size() < 3) return {false, 0.0, {}, false, "Error"};
        std::vector<double> v;  // v[0]=x, v[1]=p, v[2]=n
        if (!popScalars(3, v)) return {false, 0.0, {}, false, "Type Error"};
        const long long n = static_cast<long long>(std::floor(v[2]));
        const double p = v[1];
        const long long x = static_cast<long long>(std::floor(v[0]));
        if (n < 0 || n > kDistCap || p < 0.0 || p > 1.0)
          return {false, 0.0, {}, false, "DOMAIN"};
        if (t == Token::BinomPdf) {
          stack.push({false, binomPdfVal(n, p, x), {}});
        } else {
          double acc = 0.0;
          for (long long k = 0; k <= x && k <= n; ++k) acc += binomPdfVal(n, p, k);
          stack.push({false, (x >= n) ? 1.0 : (x < 0 ? 0.0 : acc), {}});
        }
        continue;
      }
      if (t == Token::BinomPdfList || t == Token::BinomCdfList) {
        if (stack.size() < 2) return {false, 0.0, {}, false, "Error"};
        std::vector<double> v;  // v[0]=p, v[1]=n
        if (!popScalars(2, v)) return {false, 0.0, {}, false, "Type Error"};
        const long long n = static_cast<long long>(std::floor(v[1]));
        const double p = v[0];
        if (n < 0 || n > kDistCap || p < 0.0 || p > 1.0)
          return {false, 0.0, {}, false, "DOMAIN"};
        std::vector<double> lst;
        lst.reserve(static_cast<size_t>(n + 1));
        double cum = 0.0;
        for (long long k = 0; k <= n; ++k) {
          const double pk = binomPdfVal(n, p, k);
          cum += pk;
          lst.push_back(t == Token::BinomCdfList ? cum : pk);
        }
        Operand o; o.isMat = false; o.val = 0.0; o.isList = true;
        o.list = std::move(lst);
        stack.push(o);
        continue;
      }
      if (t == Token::PoissonPdf || t == Token::PoissonCdf) {
        if (stack.size() < 2) return {false, 0.0, {}, false, "Error"};
        std::vector<double> v;  // v[0]=x, v[1]=μ
        if (!popScalars(2, v)) return {false, 0.0, {}, false, "Type Error"};
        const double mu = v[1];
        const long long x = static_cast<long long>(std::floor(v[0]));
        if (mu < 0.0) return {false, 0.0, {}, false, "DOMAIN"};
        if (t == Token::PoissonPdf) {
          stack.push({false, poissonPdfVal(mu, x), {}});
        } else {
          if (x < 0) { stack.push({false, 0.0, {}}); continue; }
          if (x > kDistCap) return {false, 0.0, {}, false, "DOMAIN"};
          double acc = 0.0;
          for (long long k = 0; k <= x; ++k) acc += poissonPdfVal(mu, k);
          stack.push({false, acc, {}});
        }
        continue;
      }
      if (t == Token::GeometPdf || t == Token::GeometCdf) {
        if (stack.size() < 2) return {false, 0.0, {}, false, "Error"};
        std::vector<double> v;  // v[0]=x, v[1]=p
        if (!popScalars(2, v)) return {false, 0.0, {}, false, "Type Error"};
        const double p = v[1];
        const long long x = static_cast<long long>(std::floor(v[0]));
        if (p <= 0.0 || p > 1.0) return {false, 0.0, {}, false, "DOMAIN"};
        if (x < 1) { stack.push({false, 0.0, {}}); continue; }
        if (t == Token::GeometPdf)
          stack.push({false, std::pow(1.0 - p, static_cast<double>(x - 1)) * p, {}});
        else  // cdf: 1 − (1−p)^x
          stack.push({false, 1.0 - std::pow(1.0 - p, static_cast<double>(x)), {}});
        continue;
      }

      // Continuous distributions (t / χ² / F). Fixed arity; df params > 0.
      if (t == Token::TPdf) {
        if (stack.size() < 2) return {false, 0.0, {}, false, "Error"};
        std::vector<double> v;  // v[0]=ν, v[1]=x
        if (!popScalars(2, v)) return {false, 0.0, {}, false, "Type Error"};
        const double nu = v[0], x = v[1];
        if (nu <= 0.0) return {false, 0.0, {}, false, "DOMAIN"};
        const double coef = std::exp(std::lgamma((nu + 1.0) / 2.0) -
                                     std::lgamma(nu / 2.0)) /
                            std::sqrt(nu * M_PI);
        stack.push({false,
                    coef * std::pow(1.0 + x * x / nu, -(nu + 1.0) / 2.0), {}});
        continue;
      }
      if (t == Token::TCdf) {
        if (stack.size() < 3) return {false, 0.0, {}, false, "Error"};
        std::vector<double> v;  // v[0]=ν, v[1]=upper, v[2]=lower
        if (!popScalars(3, v)) return {false, 0.0, {}, false, "Type Error"};
        const double nu = v[0];
        if (nu <= 0.0) return {false, 0.0, {}, false, "DOMAIN"};
        stack.push({false, tCdfAt(v[1], nu) - tCdfAt(v[2], nu), {}});
        continue;
      }
      if (t == Token::ChiPdf) {
        if (stack.size() < 2) return {false, 0.0, {}, false, "Error"};
        std::vector<double> v;  // v[0]=k, v[1]=x
        if (!popScalars(2, v)) return {false, 0.0, {}, false, "Type Error"};
        const double k = v[0], x = v[1];
        if (k <= 0.0) return {false, 0.0, {}, false, "DOMAIN"};
        if (x <= 0.0) { stack.push({false, 0.0, {}}); continue; }
        stack.push({false,
                    std::exp((k / 2.0 - 1.0) * std::log(x) - x / 2.0 -
                             (k / 2.0) * std::log(2.0) - std::lgamma(k / 2.0)),
                    {}});
        continue;
      }
      if (t == Token::ChiCdf) {
        if (stack.size() < 3) return {false, 0.0, {}, false, "Error"};
        std::vector<double> v;  // v[0]=k, v[1]=upper, v[2]=lower
        if (!popScalars(3, v)) return {false, 0.0, {}, false, "Type Error"};
        const double k = v[0];
        if (k <= 0.0) return {false, 0.0, {}, false, "DOMAIN"};
        stack.push({false, chiCdfAt(v[1], k) - chiCdfAt(v[2], k), {}});
        continue;
      }
      if (t == Token::FPdf) {
        if (stack.size() < 3) return {false, 0.0, {}, false, "Error"};
        std::vector<double> v;  // v[0]=d2, v[1]=d1, v[2]=x
        if (!popScalars(3, v)) return {false, 0.0, {}, false, "Type Error"};
        const double d2 = v[0], d1 = v[1], x = v[2];
        if (d1 <= 0.0 || d2 <= 0.0) return {false, 0.0, {}, false, "DOMAIN"};
        if (x <= 0.0) { stack.push({false, 0.0, {}}); continue; }
        const double lg = std::lgamma((d1 + d2) / 2.0) -
                          std::lgamma(d1 / 2.0) - std::lgamma(d2 / 2.0);
        stack.push({false,
                    std::exp(lg) * std::pow(d1 / d2, d1 / 2.0) *
                        std::pow(x, d1 / 2.0 - 1.0) *
                        std::pow(1.0 + d1 * x / d2, -(d1 + d2) / 2.0),
                    {}});
        continue;
      }
      if (t == Token::FCdf) {
        if (stack.size() < 4) return {false, 0.0, {}, false, "Error"};
        std::vector<double> v;  // v[0]=d2, v[1]=d1, v[2]=upper, v[3]=lower
        if (!popScalars(4, v)) return {false, 0.0, {}, false, "Type Error"};
        const double d2 = v[0], d1 = v[1];
        if (d1 <= 0.0 || d2 <= 0.0) return {false, 0.0, {}, false, "DOMAIN"};
        stack.push({false, fCdfAt(v[2], d1, d2) - fCdfAt(v[3], d1, d2), {}});
        continue;
      }

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
      // Matrix/List toolkit (Phase F follow-up). Handled ahead of the
      // generic binary/unary blocks because these produce matrix/list
      // results and accept matrix/list operands the generic blocks reject.
      if (t == Token::Identity) {
        if (stack.empty()) return {false, 0.0, {}, false, "Error"};
        Operand a = stack.top(); stack.pop();
        if (a.isMat || a.isList) return {false, 0.0, {}, false, "Type Error"};
        double nd = a.val;
        if (nd < 1.0 || nd > 99.0 || nd != std::floor(nd))
          return {false, 0.0, {}, false, "DOMAIN"};
        int n = static_cast<int>(nd);
        Matrix result;
        result.rows = n; result.cols = n;
        result.data.assign(static_cast<size_t>(n) * n, 0.0);
        for (int i = 0; i < n; ++i) result.set(i, i, 1.0);
        stack.push({true, 0.0, result});
        continue;
      }
      if (t == Token::Dim) {
        if (stack.empty()) return {false, 0.0, {}, false, "Error"};
        Operand a = stack.top(); stack.pop();
        if (a.isMat) {
          // Matrix → {rows, cols} list.
          Operand o; o.isList = true;
          o.list = {static_cast<double>(a.mat.rows),
                    static_cast<double>(a.mat.cols)};
          stack.push(o);
          continue;
        }
        if (a.isList) {
          stack.push({false, static_cast<double>(a.list.size()), {}});
          continue;
        }
        return {false, 0.0, {}, false, "Type Error"};  // scalar has no dim
      }
      if (t == Token::Ref) {
        if (stack.empty()) return {false, 0.0, {}, false, "Error"};
        Operand a = stack.top(); stack.pop();
        if (!a.isMat) return {false, 0.0, {}, false, "Type Error"};
        Matrix result = a.mat;
        refInPlace(result);
        stack.push({true, 0.0, result});
        continue;
      }
      if (t == Token::RandM) {
        if (stack.size() < 2) return {false, 0.0, {}, false, "Error"};
        Operand b = stack.top(); stack.pop();
        Operand a = stack.top(); stack.pop();
        if (a.isMat || b.isMat || a.isList || b.isList)
          return {false, 0.0, {}, false, "Type Error"};
        double rd = a.val, cd = b.val;
        if (rd < 1.0 || cd < 1.0 || rd > 99.0 || cd > 99.0 ||
            rd != std::floor(rd) || cd != std::floor(cd))
          return {false, 0.0, {}, false, "DOMAIN"};
        int r = static_cast<int>(rd), cN = static_cast<int>(cd);
        Matrix result;
        result.rows = r; result.cols = cN;
        result.data.resize(static_cast<size_t>(r) * cN);
        std::uniform_int_distribution<int> dist(-9, 9);
        for (auto &v : result.data) v = static_cast<double>(dist(rng));
        stack.push({true, 0.0, result});
        continue;
      }
      if (t == Token::Augment) {
        if (stack.size() < 2) return {false, 0.0, {}, false, "Error"};
        Operand b = stack.top(); stack.pop();
        Operand a = stack.top(); stack.pop();
        if (a.isMat && b.isMat) {
          // Horizontal concatenation — requires equal row counts.
          if (a.mat.rows != b.mat.rows)
            return {false, 0.0, {}, false, "Dim Mismatch"};
          Matrix result;
          result.rows = a.mat.rows;
          result.cols = a.mat.cols + b.mat.cols;
          result.data.resize(static_cast<size_t>(result.rows) * result.cols);
          for (int i = 0; i < result.rows; ++i) {
            for (int j = 0; j < a.mat.cols; ++j)
              result.set(i, j, a.mat.at(i, j));
            for (int j = 0; j < b.mat.cols; ++j)
              result.set(i, a.mat.cols + j, b.mat.at(i, j));
          }
          stack.push({true, 0.0, result});
          continue;
        }
        if (a.isList && b.isList) {
          Operand o; o.isList = true;
          o.list = a.list;
          o.list.insert(o.list.end(), b.list.begin(), b.list.end());
          stack.push(o);
          continue;
        }
        return {false, 0.0, {}, false, "Type Error"};  // no mixed forms
      }
      if (t == Token::ListToMatr) {
        // List▶Matr(Lα, Lβ) — two equal-length lists become the two
        // columns of an n×2 matrix. Value-producing (store with →[C]).
        if (stack.size() < 2) return {false, 0.0, {}, false, "Error"};
        Operand b = stack.top(); stack.pop();
        Operand a = stack.top(); stack.pop();
        if (!a.isList || !b.isList)
          return {false, 0.0, {}, false, "Type Error"};
        if (a.list.size() != b.list.size())
          return {false, 0.0, {}, false, "Dim Mismatch"};
        if (a.list.empty())
          return {false, 0.0, {}, false, "Dim Mismatch"};
        int n = static_cast<int>(a.list.size());
        Matrix result;
        result.rows = n; result.cols = 2;
        result.data.resize(static_cast<size_t>(n) * 2);
        for (int i = 0; i < n; ++i) {
          result.set(i, 0, a.list[static_cast<size_t>(i)]);
          result.set(i, 1, b.list[static_cast<size_t>(i)]);
        }
        stack.push({true, 0.0, result});
        continue;
      }
      if (t == Token::MatrToList) {
        // Matr▶List([A], col) — extract 1-based column `col` of the matrix
        // as a list. Value-producing (store with →Ln).
        if (stack.size() < 2) return {false, 0.0, {}, false, "Error"};
        Operand b = stack.top(); stack.pop();
        Operand a = stack.top(); stack.pop();
        if (!a.isMat || b.isMat || b.isList)
          return {false, 0.0, {}, false, "Type Error"};
        double cd = b.val;
        if (cd < 1.0 || cd != std::floor(cd) || cd > a.mat.cols)
          return {false, 0.0, {}, false, "Dim Mismatch"};
        int col = static_cast<int>(cd) - 1;
        Operand o; o.isList = true;
        o.list.resize(static_cast<size_t>(a.mat.rows));
        for (int i = 0; i < a.mat.rows; ++i)
          o.list[static_cast<size_t>(i)] = a.mat.at(i, col);
        stack.push(o);
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

      // Complex functions (Phase F) — conj/real/imag/angle are defined
      // for real operands too. Then, for a complex operand, handle the
      // functions that accept complex (abs/neg/sqrt) and reject the rest.
      if (t == Token::Conj) {
        Operand o; o.val = a.val; o.imag = -a.imag; stack.push(o); continue;
      }
      if (t == Token::RealPart) { stack.push({false, a.val, {}}); continue; }
      if (t == Token::ImagPart) { stack.push({false, a.imag, {}}); continue; }
      if (t == Token::Angle) {
        double ang = std::atan2(a.imag, a.val);
        if (MathStateMachine::angleMode == AngleMode::Degree)
          ang = ang * 180.0 / M_PI;
        stack.push({false, ang, {}}); continue;
      }
      if (isComplex(a)) {
        // abs is the magnitude (a real result); everything else maps a
        // complex → complex via std::complex. Complex trig/exp/log are
        // radian-only (the angle-mode conversion is real-only).
        if (t == Token::Abs) {
          stack.push({false, std::hypot(a.val, a.imag), {}}); continue;
        }
        const std::complex<double> z = toC(a);
        std::complex<double> r;
        bool ok = true;
        if (t == Token::Neg)        r = -z;
        else if (t == Token::Sqrt)  r = std::sqrt(z);
        else if (t == Token::Sin)   r = std::sin(z);
        else if (t == Token::Cos)   r = std::cos(z);
        else if (t == Token::Tan)   r = std::tan(z);
        else if (t == Token::ASin)  r = std::asin(z);
        else if (t == Token::ACos)  r = std::acos(z);
        else if (t == Token::ATan)  r = std::atan(z);
        else if (t == Token::Exp)   r = std::exp(z);   // e^(
        else if (t == Token::Ln)    r = std::log(z);
        else if (t == Token::Log)   r = std::log10(z);
        else if (t == Token::Sinh)  r = std::sinh(z);
        else if (t == Token::Cosh)  r = std::cosh(z);
        else if (t == Token::Tanh)  r = std::tanh(z);
        else ok = false;
        if (!ok)
          return {false, 0.0, {}, false, "Type Error"};
        Operand o; o.val = r.real(); o.imag = r.imag(); stack.push(o);
        continue;
      }
      // √ / ln / log of a negative real → complex when not in Real mode.
      if (a.val < 0.0 &&
          MathStateMachine::complexMode != ComplexMode::Real &&
          (t == Token::Sqrt || t == Token::Ln || t == Token::Log)) {
        const std::complex<double> z(a.val, 0.0);
        const std::complex<double> r = (t == Token::Sqrt) ? std::sqrt(z)
                                     : (t == Token::Ln)   ? std::log(z)
                                                          : std::log10(z);
        Operand o; o.val = r.real(); o.imag = r.imag(); stack.push(o);
        continue;
      }

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

      // Complex arithmetic (Phase F): when either operand has an
      // imaginary part, compute + - * / ^ (and == / ≠) with std::complex.
      // Real operands keep the existing real code paths untouched.
      if (isComplex(a) || isComplex(b)) {
        if (a.isMat || b.isMat || a.isList || b.isList)
          return {false, 0.0, {}, false, "Type Error"};
        const std::complex<double> ca = toC(a), cb = toC(b);
        auto pushC = [&](std::complex<double> z) {
          Operand o;
          o.val = z.real();
          o.imag = z.imag();
          stack.push(o);
        };
        if (t == Token::Add)                                 pushC(ca + cb);
        else if (t == Token::Sub)                            pushC(ca - cb);
        else if (t == Token::Mul || t == Token::ImplicitMul) pushC(ca * cb);
        else if (t == Token::Div) {
          if (cb == std::complex<double>(0.0, 0.0))
            return {false, 0.0, {}, false, "DIVIDE BY 0"};
          pushC(ca / cb);
        } else if (t == Token::Pow) {
          // Integer exponents via repeated multiplication — exact, no
          // exp/log round-off (so i² = -1, not -1 + 1e-16 i).
          if (b.imag == 0.0 && b.val == std::floor(b.val) &&
              std::abs(b.val) <= 64.0) {
            const long long n = static_cast<long long>(b.val);
            std::complex<double> r(1.0, 0.0);
            const std::complex<double> base =
                (n >= 0) ? ca : (std::complex<double>(1.0, 0.0) / ca);
            for (long long k = 0; k < std::llabs(n); ++k) r *= base;
            pushC(r);
          } else {
            pushC(std::pow(ca, cb));
          }
        }
        else if (t == Token::Equal)
          stack.push({false, (ca == cb) ? 1.0 : 0.0, {}});
        else if (t == Token::NotEqual)
          stack.push({false, (ca != cb) ? 1.0 : 0.0, {}});
        else
          return {false, 0.0, {}, false, "Type Error"};  // <,>,and,... on complex
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
  return {true, res.val, res.mat, res.isMat, "", res.isList, res.list, res.imag};
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

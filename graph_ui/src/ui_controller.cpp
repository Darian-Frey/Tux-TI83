#include "ui_controller.hpp"
#include <algorithm>
#include <cmath>
#include <map>

namespace tux_ti83 {

namespace {

// Single source of truth for the calculator's token vocabulary.
// Adding a new token = adding one row here. The forward (input → token)
// and reverse (token → display string) maps are derived from this table
// at first lookup, so no hand-mirroring is required.
//
// `displayStr` already includes the opening paren for function tokens, so
// the auto-paren heuristic that used to live in processInput is gone — the
// display formatting is now data-driven, which closes the entire class of
// "single-character function names render without their paren" bugs by
// construction (BUG-002 was an instance of this).
struct TokenSpec {
  const char *input;      // string accepted by processInput
  Token       token;      // matching token enum value
  const char *displayStr; // text appended to the display when this token lands
};

constexpr TokenSpec kTokens[] = {
    // Digits
    {"0", Token::Num0, "0"}, {"1", Token::Num1, "1"},
    {"2", Token::Num2, "2"}, {"3", Token::Num3, "3"},
    {"4", Token::Num4, "4"}, {"5", Token::Num5, "5"},
    {"6", Token::Num6, "6"}, {"7", Token::Num7, "7"},
    {"8", Token::Num8, "8"}, {"9", Token::Num9, "9"},

    // Operators
    {"+", Token::Add, "+"}, {"−", Token::Sub, "−"},
    {"×", Token::Mul, "×"}, {"÷", Token::Div, "÷"},
    {"^", Token::Pow, "^"},

    // ASCII aliases for the Unicode operators. Keyboard typists and the
    // CLI binary feed these directly; the QML keyboard handler also
    // pre-converts but we accept either form. Display string matches
    // the Unicode form so the calculator looks consistent.
    {"-", Token::Sub, "−"},
    {"*", Token::Mul, "×"},
    {"/", Token::Div, "÷"},

    // Unary negation. Distinct from Sub — the UI's `(-)` key sends
    // "neg" while the `−` key and keyboard `-` send Sub (with
    // disambiguation in insertToken promoting to Neg in unary contexts).
    {"neg", Token::Neg, "−"},

    // Punctuation
    {"(", Token::LeftParen, "("},
    {")", Token::RightParen, ")"},
    {".", Token::Decimal, "."},

    // Constants & variables
    {"π", Token::Pi, "π"},
    {"e", Token::E,  "e"},

    // Scalar variables A–Z. `X` doubles as the graph independent
    // variable (same token, same symbol); in calc mode the controller
    // passes `varRegistry[X]` as xValue so `5→X` then `X+1` reads the
    // stored value, while graph mode passes the sweep x.
    {"A", Token::VarA, "A"}, {"B", Token::VarB, "B"}, {"C", Token::VarC, "C"},
    {"D", Token::VarD, "D"}, {"E", Token::VarE, "E"}, {"F", Token::VarF, "F"},
    {"G", Token::VarG, "G"}, {"H", Token::VarH, "H"}, {"I", Token::VarI, "I"},
    {"J", Token::VarJ, "J"}, {"K", Token::VarK, "K"}, {"L", Token::VarL, "L"},
    {"M", Token::VarM, "M"}, {"N", Token::VarN, "N"}, {"O", Token::VarO, "O"},
    {"P", Token::VarP, "P"}, {"Q", Token::VarQ, "Q"}, {"R", Token::VarR, "R"},
    {"S", Token::VarS, "S"}, {"T", Token::VarT, "T"}, {"U", Token::VarU, "U"},
    {"V", Token::VarV, "V"}, {"W", Token::VarW, "W"}, {"X", Token::VarX, "X"},
    {"Y", Token::VarY, "Y"}, {"Z", Token::VarZ, "Z"},

    // Assignment arrow — typed `→` or the ASCII alias `->`. Display
    // always renders the Unicode form. Preprocessing in the engine
    // consumes the following VarA..VarZ token and records the target.
    {"→",  Token::Sto, "→"},
    {"->", Token::Sto, "→"},

    // Last-answer recall
    {"Ans", Token::Ans, "Ans"},

    // Functions — inputs include the opening paren, so the buffer
    // always has just the function token (no separate LeftParen).
    // The shunting-yard pushes a synthetic LeftParen for all of these,
    // giving a uniform scope marker that nests correctly.
    {"sin(",  Token::Sin,  "sin("},
    {"cos(",  Token::Cos,  "cos("},
    {"tan(",  Token::Tan,  "tan("},
    {"asin(", Token::ASin, "asin("},
    {"acos(", Token::ACos, "acos("},
    {"atan(", Token::ATan, "atan("},
    {"log(",  Token::Log,  "log("},
    {"ln(",   Token::Ln,   "ln("},
    {"√(",    Token::Sqrt, "√("},
    {"det(",  Token::Det,  "det("},
    {"T(",    Token::Transpose, "T("},
    {"rref(", Token::Rref, "rref("},

    // Number functions (unary)
    {"abs(",   Token::Abs,   "abs("},
    {"int(",   Token::Int,   "int("},
    {"iPart(", Token::IPart, "iPart("},
    {"fPart(", Token::FPart, "fPart("},

    // Number functions (binary) — Wave 2
    {"round(", Token::Round, "round("},
    {"min(",   Token::Min,   "min("},
    {"max(",   Token::Max,   "max("},
    {"mod(",   Token::Mod,   "mod("},

    // Combinatorics (binary)
    {"nCr(",   Token::NCr,   "nCr("},
    {"nPr(",   Token::NPr,   "nPr("},

    // Factorial — unary postfix
    {"!", Token::Fact, "!"},

    // Hyperbolic functions (unary)
    {"sinh(",  Token::Sinh,  "sinh("},
    {"cosh(",  Token::Cosh,  "cosh("},
    {"tanh(",  Token::Tanh,  "tanh("},
    {"asinh(", Token::ASinh, "asinh("},
    {"acosh(", Token::ACosh, "acosh("},
    {"atanh(", Token::ATanh, "atanh("},

    // Argument separator for binary/n-ary functions. Without this entry
    // the `,` CalcKey was inert; now it inserts Token::Comma which the
    // shunting-yard treats as a function-argument separator.
    {",", Token::Comma, ","},

    // Comparators / boolean
    {"=",   Token::Equal,    "="},
    {"≠",   Token::NotEqual, "≠"},
    {"<",   Token::Less,     "<"},
    {">",   Token::Greater,  ">"},
    {"and", Token::And,      "and"},
    {"or",  Token::Or,       "or"},
    {"not", Token::Not,      "not"},

    // Matrices
    {"[A]", Token::MatA, "[A]"},
    {"[B]", Token::MatB, "[B]"},
    {"[C]", Token::MatC, "[C]"},
};

// Lazy-built lookup maps. Pointers are stable because kTokens has static
// storage duration.
const std::map<QString, const TokenSpec *> &inputToSpec() {
  static const std::map<QString, const TokenSpec *> map = [] {
    std::map<QString, const TokenSpec *> m;
    for (const auto &s : kTokens)
      m.emplace(QString::fromUtf8(s.input), &s);
    return m;
  }();
  return map;
}

const std::map<int, const TokenSpec *> &tokenToSpec() {
  static const std::map<int, const TokenSpec *> map = [] {
    std::map<int, const TokenSpec *> m;
    for (const auto &s : kTokens)
      m.emplace(static_cast<int>(s.token), &s);
    return m;
  }();
  return map;
}

} // anonymous namespace

UIController::UIController(QObject *parent) : QObject(parent), m_activeIdx(0) {
  m_functionBuffers.resize(3);
  m_displayStrings.resize(3, "");
}

void UIController::setAngleMode(int m) {
  // Clamp to the two valid values. Anything else becomes Radian — the
  // safer default, matches mathematical convention.
  AngleMode newMode = (m == 1) ? AngleMode::Degree : AngleMode::Radian;
  if (MathStateMachine::angleMode == newMode)
    return;
  MathStateMachine::angleMode = newMode;
  emit angleModeChanged();
}

QString UIController::currentDisplay() const {
  return m_displayStrings[m_activeIdx];
}

QString UIController::formatScalar(double value) {
  // 'g' drops trailing zeros; precision 10 shows enough digits that
  // values up to ~10^9 stay in plain-integer form (10! = 3,628,800
  // no longer renders as "3.6288e+06").
  return QString::number(value, 'g', 10);
}

// ── processInput dispatcher ───────────────────────────────────
//
// Thin switch over the input string. Each branch delegates to a private
// helper that owns one concern. New input categories should be added by
// extending the dispatch table here, not by growing the helpers.
void UIController::processInput(const QString &input) {
  // Control sentinels use multi-character strings so they can't collide
  // with single-letter variable inputs (A..Z). "CLEAR" replaced the old
  // "C" sentinel once VarC landed; "C" on its own now inserts the VarC
  // token via the insertToken path below.
  if (input == "CLEAR") {
    clearAll();
    return;
  }
  if (input == "DEL") {
    backspace();
    return;
  }
  if (input == "ENTER") {
    evaluate();
    return;
  }
  if (input == "▶Frac") {
    convertDisplayToFraction();
    return;
  }
  if (input == "▶Dec") {
    convertDisplayToDecimal();
    return;
  }
  insertToken(input);
}

// CLEAR — full reset, force INPUTTING state.
void UIController::clearAll() {
  auto &currentBuf = m_functionBuffers[m_activeIdx];
  auto &currentStr = m_displayStrings[m_activeIdx];

  currentStr = "";
  currentBuf.clear();
  bool stateChanged =
      (m_displayState != Inputting || !m_displayExpression.isEmpty());
  m_displayState = Inputting;
  m_displayExpression = "";
  if (stateChanged)
    emit displayStateChanged();
  emit displayChanged();
}

// DEL — backspace one token in INPUTTING; behaves like CLEAR after
// an evaluation or error (pressing backspace on a result is treated
// as "abandon this result, go back to fresh input").
void UIController::backspace() {
  auto &currentBuf = m_functionBuffers[m_activeIdx];
  auto &currentStr = m_displayStrings[m_activeIdx];

  if (m_displayState != Inputting) {
    currentStr = "";
    currentBuf.clear();
    m_displayState = Inputting;
    m_displayExpression = "";
    emit displayStateChanged();
    emit displayChanged();
    return;
  }
  if (currentBuf.empty())
    return;

  currentBuf.pop_back();

  // Rebuild the display string from the surviving tokens via the unified
  // token table. No more hand-mirrored revMap.
  currentStr = "";
  const auto &rev = tokenToSpec();
  for (auto t : currentBuf) {
    auto it = rev.find(static_cast<int>(t));
    if (it != rev.end())
      currentStr += QString::fromUtf8(it->second->displayStr);
  }
  emit displayChanged();
}

// ENTER — evaluate the buffer, transition to EVALUATED or ERROR.
void UIController::evaluate() {
  auto &currentBuf = m_functionBuffers[m_activeIdx];
  auto &currentStr = m_displayStrings[m_activeIdx];

  // Snapshot expression for the top "expression =" display line before
  // currentStr is overwritten with the result.
  m_displayExpression = currentStr;
  QString entry =
      "Y" + QString::number(m_activeIdx + 1) + ": " + currentStr + " = ";

  MathStateMachine msm;
  // In calc mode, X behaves like any other scalar variable — resolve
  // it from the registry so `5→X: X+1` gives 6. Graph-mode evaluation
  // (plot sweeps) passes its own xValue and bypasses this path.
  const double xIndex = static_cast<double>((int)Token::VarX - (int)Token::VarA);
  CalculationResult result = msm.evaluate(currentBuf,
      MathStateMachine::varRegistry[static_cast<size_t>(xIndex)]);
  if (result.success) {
    if (result.isMatrix) {
      QString matStr = "[[";
      for (int i = 0; i < result.matrixValue.rows; ++i) {
        for (int j = 0; j < result.matrixValue.cols; ++j) {
          matStr += QString::number(result.matrixValue.at(i, j));
          if (j < result.matrixValue.cols - 1)
            matStr += ",";
        }
        if (i < result.matrixValue.rows - 1)
          matStr += "][";
      }
      currentStr = matStr + "]]";
    } else {
      // BUG-015 fix: default scalar display is decimal. Users get the
      // fraction form on demand via the ▶Frac MATH-menu entry.
      currentStr = formatScalar(result.value);
    }
    m_displayState = Evaluated;
    // Remember this result for the next Token::Ans recall. Errors do not
    // overwrite Ans (matches TI-83 behaviour), so this assignment is
    // only inside the success branch.
    MathStateMachine::lastResult = result;
  } else {
    // IMP-006: Map the engine's `error_message` classification to a
    // TI-83-style display string. The engine already categorises
    // failures by string in CalculationResult.error_message; we just
    // pick the conventional ERR:* label here. Anything unrecognised
    // falls through to ERR:SYNTAX as the safe default.
    QString msg = QString::fromStdString(result.error_message);
    if (msg == "DIVIDE BY 0")
      currentStr = "ERR:DIVIDE BY 0";
    else if (msg == "NONREAL ANS")
      currentStr = "ERR:NONREAL ANS";
    else if (msg == "DOMAIN")
      currentStr = "ERR:DOMAIN";
    else if (msg == "Type Error")
      currentStr = "ERR:DATA TYPE";
    else if (msg == "Dim Mismatch")
      currentStr = "ERR:INVALID DIM";
    else if (msg == "Undefined Matrix")
      currentStr = "ERR:UNDEFINED";
    else if (msg == "SINGULAR MAT")
      currentStr = "ERR:SINGULAR MAT";
    else
      currentStr = "ERR:SYNTAX";
    m_displayState = Error;
  }
  entry += currentStr;
  m_history.prepend(entry);
  emit historyChanged();
  emit displayChanged();
  emit displayStateChanged();
}

// Convert the current scalar result to its fraction form (if one exists
// within the toFraction tolerance). No-op unless we're in Evaluated
// state with a scalar result. Adds an "Ans▶Frac = …" history entry so
// the conversion is visible both on the display and in history.
void UIController::convertDisplayToFraction() {
  if (m_displayState != Evaluated || MathStateMachine::lastResult.isMatrix)
    return;
  std::string fracStr = MathStateMachine::toFraction(MathStateMachine::lastResult.value);
  if (fracStr.empty())
    return; // Irrational or unconvertible — silently leave the decimal.
  auto &currentStr = m_displayStrings[m_activeIdx];
  currentStr = QString::fromStdString(fracStr);
  m_history.prepend("Y" + QString::number(m_activeIdx + 1) +
                    ": Ans▶Frac = " + currentStr);
  emit historyChanged();
  emit displayChanged();
}

// Convert the current scalar result back to its raw decimal form.
// Mirror of convertDisplayToFraction; uses the stored lastResult.value
// rather than trying to parse the current display string.
void UIController::convertDisplayToDecimal() {
  if (m_displayState != Evaluated || MathStateMachine::lastResult.isMatrix)
    return;
  auto &currentStr = m_displayStrings[m_activeIdx];
  currentStr = formatScalar(MathStateMachine::lastResult.value);
  m_history.prepend("Y" + QString::number(m_activeIdx + 1) +
                    ": Ans▶Dec = " + currentStr);
  emit historyChanged();
  emit displayChanged();
}

// Token input. Looks up via the unified table; unknown inputs are silently
// ignored and must NOT disturb display state (otherwise an unbound key
// would prematurely flush an EVALUATED result on click).
void UIController::insertToken(const QString &input) {
  const auto &fwd = inputToSpec();
  auto it = fwd.find(input);
  if (it == fwd.end())
    return;

  auto &currentBuf = m_functionBuffers[m_activeIdx];
  auto &currentStr = m_displayStrings[m_activeIdx];

  // State-machine reset: if a stale EVALUATED/ERROR result is on screen,
  // clear it before appending the new token. Implements the spec rule
  // "next digit/function keypress: clears expr, returns to INPUTTING".
  if (m_displayState != Inputting) {
    currentBuf.clear();
    currentStr = "";
    m_displayState = Inputting;
    m_displayExpression = "";
    emit displayStateChanged();
  }

  const TokenSpec *spec = it->second;
  Token tokenToInsert = spec->token;

  // BUG-014 extension: keyboard `-` and the on-screen `−` key both map
  // to Token::Sub, but that's only correct for binary subtraction. When
  // a `−` arrives with no left operand available (empty buffer, right
  // after `(`, right after an operator, right after a function token),
  // it's really unary negation — promote it to Token::Neg so the parser
  // sees the correct shape. The `(-)` CalcKey bypasses this path
  // entirely by sending `"neg"` directly, so this only affects the
  // keyboard / `−` key ambiguity.
  if (tokenToInsert == Token::Sub) {
    bool isUnary = currentBuf.empty();
    if (!isUnary) {
      Token prev = currentBuf.back();
      isUnary = (prev == Token::LeftParen ||
                 prev == Token::Comma ||
                 prev == Token::Add || prev == Token::Sub ||
                 prev == Token::Mul || prev == Token::Div ||
                 prev == Token::Pow ||
                 EOSPrecedence::is_function(prev));
    }
    if (isUnary)
      tokenToInsert = Token::Neg;
  }

  currentBuf.push_back(tokenToInsert);
  currentStr += QString::fromUtf8(spec->displayStr);
  emit displayChanged();
}

// Longest-match tokeniser. Matches against the kTokens forward table
// plus a small set of control verbs that aren't real tokens but the
// dispatcher recognises (▶Frac, ▶Dec). Returns an empty list if any
// non-whitespace character fails to match.
QStringList UIController::tokenize(const QString &expr) {
  static const QStringList kVerbs = {QStringLiteral("▶Frac"),
                                     QStringLiteral("▶Dec")};
  // QString::length() returns qsizetype in Qt6; using qsizetype here
  // avoids a std::min/max type-deduction failure on 64-bit Linux.
  static const qsizetype kMaxKeyLen = [] {
    qsizetype m = 0;
    for (const auto &kv : inputToSpec())
      m = std::max(m, kv.first.length());
    for (const auto &v : kVerbs)
      m = std::max(m, v.length());
    return m;
  }();

  QStringList tokens;
  qsizetype i = 0;
  while (i < expr.length()) {
    if (expr[i].isSpace()) {
      ++i;
      continue;
    }
    qsizetype matchLen = 0;
    QString matchKey;
    qsizetype upper = std::min(kMaxKeyLen, expr.length() - i);
    for (qsizetype len = upper; len > 0; --len) {
      QString candidate = expr.mid(i, len);
      if (inputToSpec().count(candidate) > 0 || kVerbs.contains(candidate)) {
        matchLen = len;
        matchKey = candidate;
        break;
      }
    }
    if (matchLen == 0)
      return QStringList(); // unrecognised character
    tokens.append(matchKey);
    i += matchLen;
  }
  return tokens;
}

bool UIController::processExpression(const QString &expr) {
  QStringList tokens = tokenize(expr);
  if (tokens.isEmpty() && !expr.trimmed().isEmpty())
    return false;
  for (const QString &t : tokens)
    processInput(t);
  return true;
}

void UIController::updateMatrix(const QString &name, int rows, int cols,
                                const QVariantList &values) {
  Matrix mat;
  mat.rows = rows;
  mat.cols = cols;
  for (const auto &v : values)
    mat.data.push_back(v.toDouble());
  if (name == "[A]")
    MathStateMachine::matrixRegistry[Token::MatA] = mat;
  else if (name == "[B]")
    MathStateMachine::matrixRegistry[Token::MatB] = mat;
  else if (name == "[C]")
    MathStateMachine::matrixRegistry[Token::MatC] = mat;
}

void UIController::zoomFit() {
  double minVal = 1e308, maxVal = -1e308;
  bool found = false;
  MathStateMachine msm;
  for (const auto &buffer : m_functionBuffers) {
    if (buffer.empty())
      continue;
    for (int i = 0; i <= 100; ++i) {
      double x = m_xMin + (i * (m_xMax - m_xMin) / 100.0);
      CalculationResult res = msm.evaluate(buffer, x);
      if (res.success && std::isfinite(res.value)) {
        minVal = std::min(minVal, res.value);
        maxVal = std::max(maxVal, res.value);
        found = true;
      }
    }
  }
  if (found) {
    double margin = (maxVal - minVal) * 0.1;
    if (std::abs(maxVal - minVal) < 1e-9)
      margin = 1.0;
    m_yMin = minVal - margin;
    m_yMax = maxVal + margin;
    emit viewportChanged();
  }
}

QVariantList UIController::getMultiGraphPoints(int resolution) {
  QVariantList allFunctions;
  double step = (m_xMax - m_xMin) / resolution;
  MathStateMachine msm;
  for (size_t f = 0; f < m_functionBuffers.size(); ++f) {
    if (m_functionBuffers[f].empty())
      continue;
    QVariantList points;
    for (int i = 0; i <= resolution; ++i) {
      double x = m_xMin + (i * step);
      CalculationResult res = msm.evaluate(m_functionBuffers[f], x);
      if (res.success && !res.isMatrix) {
        QVariantMap pt;
        pt["x"] = x;
        pt["y"] = res.value;
        points.append(pt);
      }
    }
    allFunctions.append(QVariant::fromValue(points));
  }
  return allFunctions;
}

void UIController::pan(double dx, double dy, double vw, double vh) {
  double rx = m_xMax - m_xMin, ry = m_yMax - m_yMin;
  m_xMin -= dx * (rx / vw);
  m_xMax -= dx * (rx / vw);
  m_yMin += dy * (ry / vh);
  m_yMax += dy * (ry / vh);
  emit viewportChanged();
}

void UIController::zoom(double f, double mx, double my, double vw, double vh) {
  double wx = m_xMin + (mx / vw) * (m_xMax - m_xMin),
         wy = m_yMax - (my / vh) * (m_yMax - m_yMin);
  m_xMin = wx + (m_xMin - wx) * f;
  m_xMax = wx + (m_xMax - wx) * f;
  m_yMin = wy + (m_yMin - wy) * f;
  m_yMax = wy + (m_yMax - wy) * f;
  emit viewportChanged();
}

} // namespace tux_ti83

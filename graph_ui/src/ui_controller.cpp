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

    // Punctuation
    {"(", Token::LeftParen, "("},
    {")", Token::RightParen, ")"},
    {".", Token::Decimal, "."},

    // Constants & variables
    {"π", Token::Pi, "π"},
    {"e", Token::E,  "e"},
    {"X", Token::VarX, "X"},

    // Last-answer recall
    {"Ans", Token::Ans, "Ans"},

    // Functions — displayStr includes the opening paren
    {"sin",  Token::Sin,  "sin("},
    {"cos",  Token::Cos,  "cos("},
    {"tan",  Token::Tan,  "tan("},
    {"asin", Token::ASin, "asin("},
    {"acos", Token::ACos, "acos("},
    {"atan", Token::ATan, "atan("},
    {"log",  Token::Log,  "log("},
    {"ln",   Token::Ln,   "ln("},
    {"√",    Token::Sqrt, "√("},
    {"det(", Token::Det,  "det("},

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

QString UIController::currentDisplay() const {
  return m_displayStrings[m_activeIdx];
}

// ── processInput dispatcher ───────────────────────────────────
//
// Thin switch over the input string. Each branch delegates to a private
// helper that owns one concern. New input categories should be added by
// extending the dispatch table here, not by growing the helpers.
void UIController::processInput(const QString &input) {
  if (input == "C") {
    clearAll();
    return;
  }
  if (input == "DEL") {
    backspace();
    return;
  }
  if (input == "ENTER" || input == "▶Frac") {
    evaluate();
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
  CalculationResult result = msm.evaluate(currentBuf);
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
      std::string fracStr = MathStateMachine::toFraction(result.value);
      currentStr = (fracStr.empty()) ? QString::number(result.value)
                                     : QString::fromStdString(fracStr);
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
  currentBuf.push_back(spec->token);
  currentStr += QString::fromUtf8(spec->displayStr);
  emit displayChanged();
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

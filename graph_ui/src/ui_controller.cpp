#include "ui_controller.hpp"
#include "crash_logger.hpp"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QFileInfo>
#include <algorithm>
#include <cmath>
#include <limits>
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
    // Binary nth-root (2ND+^ on a real TI-83). `xroot` is the ASCII
    // alias for keyboard / CLI users.
    {"ˣ√",    Token::NthRoot, "ˣ√"},
    {"xroot", Token::NthRoot, "ˣ√"},

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

    // Y-VARS — bare references to the user's function buffers. Bare
    // form only in v1 (Y1 alone uses the current xValue);
    // `Y1(3)` parses as `Y1 * 3` via implicit-mul, not as an X
    // override (TI-83 semantics are richer here — see IMP-036 notes).
    {"Y1", Token::Y1, "Y1"},
    {"Y2", Token::Y2, "Y2"},
    {"Y3", Token::Y3, "Y3"},

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
    {"e^(",    Token::Exp,   "e^("},
    {"sgn(",   Token::Sgn,   "sgn("},

    // Number functions (binary) — Wave 2
    {"round(", Token::Round, "round("},
    {"min(",   Token::Min,   "min("},
    {"max(",   Token::Max,   "max("},
    {"mod(",   Token::Mod,   "mod("},

    // Combinatorics (binary)
    {"nCr(",   Token::NCr,   "nCr("},
    {"nPr(",   Token::NPr,   "nPr("},

    // Deferred-evaluation calculus functions. The engine's preprocessing
    // pass rewrites these into synthetic *Call tokens with a thread-local
    // side table holding the unevaluated expression; the original
    // surface tokens still need entries here so the user can type or
    // CATALOG-insert them.
    {"fnInt(",  Token::FnInt,  "fnInt("},
    {"nDeriv(", Token::NDeriv, "nDeriv("},
    {"sum(",    Token::Sum,    "sum("},
    {"prod(",   Token::Prod,   "prod("},

    // List reductions (Phase C Wave 3). sum(/prod(/min(/max( are the
    // same tokens as above/below, overloaded by arity in the engine;
    // mean/stdDev/variance are list-only.
    {"mean(",     Token::Mean,     "mean("},
    {"stdDev(",   Token::StdDev,   "stdDev("},
    {"variance(", Token::Variance, "variance("},
    {"median(",   Token::Median,   "median("},
    // seq(expr,var,start,end[,step]) — generates a list (Wave 3b).
    {"seq(",      Token::Seq,      "seq("},

    // Random functions (Phase C Wave 5). `rand` is a bare value; the
    // others take 2 args (scalar) or 3 (list, via the arg-count rewrite).
    {"rand",      Token::Rand,     "rand"},
    {"randInt(",  Token::RandInt,  "randInt("},
    {"randNorm(", Token::RandNorm, "randNorm("},
    {"randBin(",  Token::RandBin,  "randBin("},

    // Distributions (Phase C follow-on). μ/σ optional (default 0/1).
    {"normalpdf(", Token::NormalPdf, "normalpdf("},
    {"normalcdf(", Token::NormalCdf, "normalcdf("},
    {"invNorm(",   Token::InvNorm,   "invNorm("},
    // Discrete distributions. binompdf/binomcdf take (n,p,x) or (n,p).
    {"binompdf(",   Token::BinomPdf,   "binompdf("},
    {"binomcdf(",   Token::BinomCdf,   "binomcdf("},
    {"poissonpdf(", Token::PoissonPdf, "poissonpdf("},
    {"poissoncdf(", Token::PoissonCdf, "poissoncdf("},
    {"geometpdf(",  Token::GeometPdf,  "geometpdf("},
    {"geometcdf(",  Token::GeometCdf,  "geometcdf("},
    // Continuous distributions. χ² also accepts the ASCII alias `chi2`
    // (χ is awkward to type); both display as `χ²…`.
    // Complex numbers (Phase F).
    {"i",       Token::ImagI,    "i"},
    {"conj(",   Token::Conj,     "conj("},
    {"real(",   Token::RealPart, "real("},
    {"imag(",   Token::ImagPart, "imag("},
    {"angle(",  Token::Angle,    "angle("},

    {"tpdf(",     Token::TPdf,   "tpdf("},
    {"tcdf(",     Token::TCdf,   "tcdf("},
    {"χ²pdf(",    Token::ChiPdf, "χ²pdf("},
    {"χ²cdf(",    Token::ChiCdf, "χ²cdf("},
    {"chi2pdf(",  Token::ChiPdf, "χ²pdf("},
    {"chi2cdf(",  Token::ChiCdf, "χ²cdf("},
    {"Fpdf(",     Token::FPdf,   "Fpdf("},
    {"Fcdf(",     Token::FCdf,   "Fcdf("},

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

    // Statement separator (TI-83 `:` chains expressions). Inserted via
    // ALPHA + `.` (the period CalcKey already labels its ALPHA function
    // as `:`).
    {":", Token::Colon, ":"},

    // Comparators / boolean — full set, exposed via the 2ND+MATH
    // "TEST" menu popup. The `<=` / `>=` ASCII aliases exist so the
    // CLI and keyboard typists can enter them without needing the
    // Unicode "≤" / "≥" characters.
    {"=",   Token::Equal,     "="},
    {"≠",   Token::NotEqual,  "≠"},
    {"<",   Token::Less,      "<"},
    {"≤",   Token::LessEq,    "≤"},
    {"<=",  Token::LessEq,    "≤"},
    {">",   Token::Greater,   ">"},
    {"≥",   Token::GreaterEq, "≥"},
    {">=",  Token::GreaterEq, "≥"},
    {"and", Token::And,       "and"},
    {"or",  Token::Or,        "or"},
    {"xor", Token::Xor,       "xor"},
    {"not", Token::Not,       "not"},

    // Matrices
    {"[A]", Token::MatA, "[A]"},
    {"[B]", Token::MatB, "[B]"},
    {"[C]", Token::MatC, "[C]"},
    {"[D]", Token::MatD, "[D]"},
    {"[E]", Token::MatE, "[E]"},

    // Lists (Phase C). `{`/`}` delimit list literals; L1..L6 reference
    // the list registry.
    {"{", Token::LeftBrace, "{"},
    {"}", Token::RightBrace, "}"},
    {"L1", Token::L1, "L1"},
    {"L2", Token::L2, "L2"},
    {"L3", Token::L3, "L3"},
    {"L4", Token::L4, "L4"},
    {"L5", Token::L5, "L5"},
    {"L6", Token::L6, "L6"},
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

namespace {
// Resolve the on-disk state file. Mirrors the crash logger's
// directory choice so admin tools can find everything under one path.
QString resolveStateFilePath() {
  QString stateHome = qEnvironmentVariable("XDG_STATE_HOME");
  QString dir = !stateHome.isEmpty()
                  ? stateHome + "/tux-ti83"
                  : QDir::homePath() + "/.local/state/tux-ti83";
  QDir().mkpath(dir);
  return dir + "/state.json";
}

// Directory holding named save snapshots (a `saves/` subdir alongside
// state.json). Created on demand.
QString resolveSavesDir() {
  QString base = QFileInfo(resolveStateFilePath()).absolutePath();
  QString dir = base + "/saves";
  QDir().mkpath(dir);
  return dir;
}

// Keep only letters/digits/-/_ so a user name maps to a safe filename.
QString sanitizeSaveName(const QString &name) {
  QString s;
  for (QChar c : name.trimmed())
    if (c.isLetterOrNumber() || c == '-' || c == '_') s += c;
  return s;
}

// Schema version. Bump when the JSON layout changes incompatibly so
// older state files are detected and skipped rather than misread.
constexpr int kStateSchemaVersion = 1;
} // namespace

UIController::UIController(QObject *parent) : QObject(parent), m_activeIdx(0) {
  m_functionBuffers.resize(kFunctionCount);
  m_displayStrings.resize(kFunctionCount, "");
  m_functionEnabled.assign(kFunctionCount, true);
  m_functionStyle.assign(kFunctionCount, 0);
  // Note: persisted state is NOT auto-loaded here. The GUI's
  // main.cpp calls loadState() explicitly post-construction so the
  // CLI / REPL / test binaries (which all instantiate a controller
  // too) stay isolated from whatever the user's GUI session left
  // behind on disk. Tests in particular need a deterministic
  // default-zero registry.

  // Y-VARS engine hookup. The math engine knows nothing about which
  // function buffer is which slot; this lambda answers that question.
  // Last-constructed controller wins (the static is process-global),
  // which is fine since production has exactly one and tests run
  // sequentially with one live instance at a time.
  MathStateMachine::yLookup = [this](int idx) -> std::vector<Token> {
    if (idx < 0 || idx >= static_cast<int>(m_functionBuffers.size()))
      return {};
    return m_functionBuffers[idx];
  };
}

QJsonObject UIController::buildStateJson() const {
  QJsonObject root;
  root["version"] = kStateSchemaVersion;

  // Scalars A..Z — 26-element array of doubles.
  QJsonArray scalars;
  for (double v : MathStateMachine::varRegistry)
    scalars.append(v);
  root["scalars"] = scalars;

  // Matrices [A]/[B]/[C] — only persist the ones the user has touched.
  QJsonObject matrices;
  auto persistMatrix = [&matrices](const QString &name, Token tok) {
    auto it = MathStateMachine::matrixRegistry.find(tok);
    if (it == MathStateMachine::matrixRegistry.end()) return;
    const Matrix &m = it->second;
    QJsonObject mo;
    mo["rows"] = m.rows;
    mo["cols"] = m.cols;
    QJsonArray data;
    for (double v : m.data) data.append(v);
    mo["data"] = data;
    matrices[name] = mo;
  };
  persistMatrix("A", Token::MatA);
  persistMatrix("B", Token::MatB);
  persistMatrix("C", Token::MatC);
  persistMatrix("D", Token::MatD);
  persistMatrix("E", Token::MatE);
  root["matrices"] = matrices;

  // Lists L1..L6 (Phase C) — only persist populated slots.
  QJsonObject lists;
  for (int i = 0; i < 6; ++i) {
    const Token tok = static_cast<Token>(static_cast<int>(Token::L1) + i);
    auto it = MathStateMachine::listRegistry.find(tok);
    if (it == MathStateMachine::listRegistry.end())
      continue;
    QJsonArray data;
    for (double v : it->second)
      data.append(v);
    lists[QStringLiteral("L") + QString::number(i + 1)] = data;
  }
  root["lists"] = lists;

  // Function buffers Y1/Y2/Y3 as their display strings. Round-trips
  // through processExpression on load.
  QJsonArray functions;
  for (const auto &str : m_displayStrings) functions.append(str);
  root["functions"] = functions;
  // Y-editor per-slot on/off and line style.
  QJsonArray fnEnabled, fnStyle;
  for (bool b : m_functionEnabled) fnEnabled.append(b);
  for (int s : m_functionStyle) fnStyle.append(s);
  root["fnEnabled"] = fnEnabled;
  root["fnStyle"] = fnStyle;
  // DRAW overlays.
  root["draw"] = QJsonArray::fromVariantList(m_drawObjects);

  // Active function slot index.
  root["activeFunction"] = m_activeIdx;

  // Viewport.
  QJsonObject viewport;
  viewport["xMin"] = m_xMin;
  viewport["xMax"] = m_xMax;
  viewport["yMin"] = m_yMin;
  viewport["yMax"] = m_yMax;
  // ZoomMemory stored window.
  viewport["savedXMin"] = m_savedXMin;
  viewport["savedXMax"] = m_savedXMax;
  viewport["savedYMin"] = m_savedYMin;
  viewport["savedYMax"] = m_savedYMax;
  root["viewport"] = viewport;

  // MODE settings.
  QJsonObject mode;
  mode["angle"]       = static_cast<int>(MathStateMachine::angleMode);
  mode["notation"]    = static_cast<int>(MathStateMachine::notation);
  mode["fixDecimals"] = MathStateMachine::fixDecimals;
  mode["numberBase"]  = static_cast<int>(MathStateMachine::numberBase);
  mode["complexMode"] = static_cast<int>(MathStateMachine::complexMode);
  mode["drawMode"]    = m_drawMode;
  mode["graphMode"]   = m_graphMode;
  mode["statPlotOn"]    = m_statPlotOn;
  mode["statPlotType"]  = m_statPlotType;
  mode["statPlotXList"] = m_statPlotXList;
  mode["statPlotYList"] = m_statPlotYList;
  mode["gridOn"]  = m_gridOn;
  mode["axesOn"]  = m_axesOn;
  mode["coordOn"] = m_coordOn;
  mode["labelOn"] = m_labelOn;
  mode["seqNMax"]  = m_seqNMax;
  mode["seqInitU"] = m_seqInitU;
  mode["seqInitV"] = m_seqInitV;
  mode["seqInitW"] = m_seqInitW;
  root["mode"] = mode;

  // TBLSET (TABLE mode settings — separate object since they're
  // logically distinct from MODE).
  QJsonObject table;
  table["tblStart"] = m_tblStart;
  table["tblStep"]  = m_tblStep;
  root["table"] = table;

  return root;
}

// Write the JSON document to `path`; returns false on open/write failure.
static bool writeJsonFile(const QString &path, const QJsonObject &root) {
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return false;
  f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
  f.close();
  return true;
}

void UIController::saveState() const {
  if (writeJsonFile(resolveStateFilePath(), buildStateJson()))
    CrashLogger::logEvent(QStringLiteral("saveState ok"));
}

void UIController::loadState() {
  QFile f(resolveStateFilePath());
  if (!f.exists() || !f.open(QIODevice::ReadOnly))
    return;
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
  f.close();
  if (err.error != QJsonParseError::NoError || !doc.isObject()) {
    CrashLogger::logEvent(QStringLiteral("loadState skipped: parse error"));
    return;
  }
  applyStateJson(doc.object());
}

void UIController::applyStateJson(const QJsonObject &root) {
  // Version mismatch: skip, leave defaults. Future migrations would
  // branch here.
  if (root.value("version").toInt() != kStateSchemaVersion) {
    CrashLogger::logEvent(QStringLiteral("applyState skipped: version mismatch"));
    return;
  }

  // Scalars.
  QJsonArray scalars = root.value("scalars").toArray();
  for (int i = 0; i < scalars.size() && i < 26; ++i)
    MathStateMachine::varRegistry[static_cast<size_t>(i)] = scalars[i].toDouble();

  // Matrices.
  QJsonObject matrices = root.value("matrices").toObject();
  auto restoreMatrix = [&matrices](const QString &name, Token tok) {
    if (!matrices.contains(name)) return;
    QJsonObject mo = matrices.value(name).toObject();
    Matrix m;
    m.rows = mo.value("rows").toInt();
    m.cols = mo.value("cols").toInt();
    QJsonArray data = mo.value("data").toArray();
    m.data.reserve(static_cast<size_t>(data.size()));
    for (auto v : data) m.data.push_back(v.toDouble());
    MathStateMachine::matrixRegistry[tok] = m;
  };
  restoreMatrix("A", Token::MatA);
  restoreMatrix("B", Token::MatB);
  restoreMatrix("C", Token::MatC);
  restoreMatrix("D", Token::MatD);
  restoreMatrix("E", Token::MatE);

  // Lists L1..L6 (Phase C).
  QJsonObject lists = root.value("lists").toObject();
  for (int i = 0; i < 6; ++i) {
    const QString key = QStringLiteral("L") + QString::number(i + 1);
    if (!lists.contains(key))
      continue;
    QJsonArray data = lists.value(key).toArray();
    std::vector<double> vec;
    vec.reserve(static_cast<size_t>(data.size()));
    for (auto v : data)
      vec.push_back(v.toDouble());
    const Token tok = static_cast<Token>(static_cast<int>(Token::L1) + i);
    MathStateMachine::listRegistry[tok] = vec;
  }

  // MODE — apply before function buffers so any side effects use
  // the right format settings.
  QJsonObject mode = root.value("mode").toObject();
  if (mode.contains("angle"))
    MathStateMachine::angleMode =
        (mode["angle"].toInt() == 1) ? AngleMode::Degree : AngleMode::Radian;
  if (mode.contains("notation")) {
    int n = mode["notation"].toInt();
    MathStateMachine::notation =
        (n == 1) ? NumberNotation::Sci :
        (n == 2) ? NumberNotation::Eng :
                   NumberNotation::Normal;
  }
  if (mode.contains("fixDecimals")) {
    int n = mode["fixDecimals"].toInt(-1);
    MathStateMachine::fixDecimals = (n >= 0 && n <= 9) ? n : -1;
  }
  if (mode.contains("numberBase")) {
    int b = mode["numberBase"].toInt(0);
    MathStateMachine::numberBase =
        (b == 1) ? NumberBase::Hex :
        (b == 2) ? NumberBase::Oct :
        (b == 3) ? NumberBase::Bin :
                   NumberBase::Dec;
  }
  if (mode.contains("complexMode")) {
    int cm = mode["complexMode"].toInt(0);
    MathStateMachine::complexMode =
        (cm == 1) ? ComplexMode::Rect :
        (cm == 2) ? ComplexMode::Polar :
                    ComplexMode::Real;
  }
  if (mode.contains("drawMode"))
    m_drawMode = (mode["drawMode"].toInt() == 1) ? 1 : 0;
  if (mode.contains("graphMode"))
  {
    const int g = mode["graphMode"].toInt();
    m_graphMode = (g >= 1 && g <= 3) ? g : 0;
  }
  if (mode.contains("seqNMax"))  m_seqNMax  = mode["seqNMax"].toDouble(10.0);
  if (mode.contains("seqInitU")) m_seqInitU = mode["seqInitU"].toDouble(1.0);
  if (mode.contains("seqInitV")) m_seqInitV = mode["seqInitV"].toDouble(1.0);
  if (mode.contains("seqInitW")) m_seqInitW = mode["seqInitW"].toDouble(1.0);
  if (mode.contains("statPlotOn"))
    m_statPlotOn = mode["statPlotOn"].toBool();
  if (mode.contains("statPlotType")) {
    int tp = mode["statPlotType"].toInt(0);
    m_statPlotType = (tp >= 0 && tp <= 3) ? tp : 0;
  }
  if (mode.contains("statPlotXList"))
    m_statPlotXList = mode["statPlotXList"].toString();
  if (mode.contains("statPlotYList"))
    m_statPlotYList = mode["statPlotYList"].toString();
  if (mode.contains("gridOn"))  m_gridOn  = mode["gridOn"].toBool();
  if (mode.contains("axesOn"))  m_axesOn  = mode["axesOn"].toBool();
  if (mode.contains("coordOn")) m_coordOn = mode["coordOn"].toBool();
  if (mode.contains("labelOn")) m_labelOn = mode["labelOn"].toBool();

  // TBLSET restore. Step must be non-zero — guard against bad data.
  QJsonObject table = root.value("table").toObject();
  if (table.contains("tblStart"))
    m_tblStart = table["tblStart"].toDouble();
  if (table.contains("tblStep")) {
    double s = table["tblStep"].toDouble(1.0);
    m_tblStep = (s != 0.0) ? s : 1.0;
  }

  // Viewport.
  QJsonObject viewport = root.value("viewport").toObject();
  if (viewport.contains("xMin")) m_xMin = viewport["xMin"].toDouble();
  if (viewport.contains("xMax")) m_xMax = viewport["xMax"].toDouble();
  if (viewport.contains("yMin")) m_yMin = viewport["yMin"].toDouble();
  if (viewport.contains("yMax")) m_yMax = viewport["yMax"].toDouble();
  if (viewport.contains("savedXMin")) m_savedXMin = viewport["savedXMin"].toDouble();
  if (viewport.contains("savedXMax")) m_savedXMax = viewport["savedXMax"].toDouble();
  if (viewport.contains("savedYMin")) m_savedYMin = viewport["savedYMin"].toDouble();
  if (viewport.contains("savedYMax")) m_savedYMax = viewport["savedYMax"].toDouble();

  // Function buffers — replay the display strings through
  // processExpression so the tokeniser handles dispatch. Each slot
  // is processed independently, with the active slot set first so
  // insertions land in the right buffer.
  QJsonArray functions = root.value("functions").toArray();
  const int saved_active = root.value("activeFunction").toInt(0);
  for (int slot = 0; slot < functions.size() && slot < kFunctionCount; ++slot) {
    QString expr = functions[slot].toString();
    if (expr.isEmpty()) continue;
    m_activeIdx = slot;
    processExpression(expr);
  }
  m_activeIdx = (saved_active >= 0 && saved_active < kFunctionCount) ? saved_active : 0;

  // Y-editor per-slot on/off and line style.
  QJsonArray fnEnabled = root.value("fnEnabled").toArray();
  for (int i = 0; i < fnEnabled.size() && i < kFunctionCount; ++i)
    m_functionEnabled[i] = fnEnabled[i].toBool(true);
  QJsonArray fnStyle = root.value("fnStyle").toArray();
  for (int i = 0; i < fnStyle.size() && i < kFunctionCount; ++i) {
    int s = fnStyle[i].toInt(0);
    m_functionStyle[i] = (s >= 0 && s <= 2) ? s : 0;
  }
  m_drawObjects = root.value("draw").toArray().toVariantList();

  // Treat the loaded display content as a "previous result" — next
  // keypress should clear and start fresh (state-machine rule:
  // typing in Evaluated state wipes the active buffer and returns
  // to Inputting). Without this, the loaded Y1 buffer stayed in
  // Inputting state and any new digit appended to the loaded
  // expression — `0` + `52` = displayed as `052`.
  m_displayState = Evaluated;
  m_displayExpression.clear();

  // Fire the change signals so any QML bindings refresh.
  emit angleModeChanged();
  emit notationChanged();
  emit fixDecimalsChanged();
  emit numberBaseChanged();
  emit complexModeChanged();
  emit drawModeChanged();
  emit viewportChanged();
  emit displayChanged();
  emit displayStateChanged();
  emit activeFunctionIndexChanged();
  emit functionsChanged();
  emit drawObjectsChanged();

  CrashLogger::logEvent(QStringLiteral("loadState ok"));
}

bool UIController::exportState(const QString &name) {
  const QString clean = sanitizeSaveName(name);
  if (clean.isEmpty())
    return false;
  const QString path = resolveSavesDir() + "/" + clean + ".t83";
  const bool ok = writeJsonFile(path, buildStateJson());
  CrashLogger::logEvent(QStringLiteral("exportState ") + clean +
                        (ok ? QStringLiteral(" ok") : QStringLiteral(" FAIL")));
  return ok;
}

bool UIController::importState(const QString &name) {
  const QString clean = sanitizeSaveName(name);
  if (clean.isEmpty())
    return false;
  QFile f(resolveSavesDir() + "/" + clean + ".t83");
  if (!f.exists() || !f.open(QIODevice::ReadOnly))
    return false;
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
  f.close();
  if (err.error != QJsonParseError::NoError || !doc.isObject())
    return false;
  applyStateJson(doc.object());
  CrashLogger::logEvent(QStringLiteral("importState ") + clean);
  return true;
}

QStringList UIController::listSaves() const {
  QDir dir(resolveSavesDir());
  QStringList out;
  const auto entries =
      dir.entryInfoList(QStringList{"*.t83"}, QDir::Files, QDir::Name);
  for (const QFileInfo &fi : entries)
    out.append(fi.completeBaseName());
  return out;
}

void UIController::deleteSave(const QString &name) {
  const QString clean = sanitizeSaveName(name);
  if (!clean.isEmpty())
    QFile::remove(resolveSavesDir() + "/" + clean + ".t83");
}

void UIController::resetAll() {
  CrashLogger::logEvent(QStringLiteral("resetAll"));

  // Engine-side statics.
  MathStateMachine::varRegistry.fill(0.0);
  MathStateMachine::matrixRegistry.clear();
  MathStateMachine::listRegistry.clear();
  MathStateMachine::lastResult = {true, 0.0, {}, false, ""};
  MathStateMachine::angleMode    = AngleMode::Radian;
  MathStateMachine::notation     = NumberNotation::Normal;
  MathStateMachine::fixDecimals  = -1;
  MathStateMachine::numberBase   = NumberBase::Dec;
  MathStateMachine::complexMode  = ComplexMode::Real;

  // Controller-owned state.
  for (auto &buf : m_functionBuffers) buf.clear();
  for (auto &s   : m_displayStrings)  s.clear();
  std::fill(m_functionEnabled.begin(), m_functionEnabled.end(), true);
  std::fill(m_functionStyle.begin(), m_functionStyle.end(), 0);
  m_drawObjects.clear();
  m_history.clear();
  m_entryHistory.clear();
  m_recallCycleIdx = -1;
  m_cursorPos = 0;
  m_displayState = Inputting;
  m_displayExpression.clear();
  m_activeIdx = 0;
  m_isGraphMode = false;
  m_isTableMode = false;
  m_drawMode = 0;
  m_graphMode = 0;
  m_statPlotOn = false;
  m_statPlotType = 0;
  m_statPlotXList = QStringLiteral("L1");
  m_statPlotYList = QStringLiteral("L2");
  m_gridOn = true;
  m_axesOn = true;
  m_coordOn = true;
  m_labelOn = true;
  m_seqNMax = 10.0;
  m_seqInitU = 1.0;
  m_seqInitV = 1.0;
  m_seqInitW = 1.0;
  m_insertMode = true;
  m_isTracing = false;
  m_traceX = 0.0;
  m_xMin = -10; m_xMax = 10; m_yMin = -10; m_yMax = 10;
  m_tblStart = 0.0;
  m_tblStep  = 1.0;

  // Remove the on-disk state file so a subsequent restart starts
  // truly clean — otherwise loadState would restore whatever was
  // there on next launch.
  QFile::remove(resolveStateFilePath());

  // Fire every change signal so all QML bindings refresh in one pass.
  emit displayChanged();
  emit historyChanged();
  emit activeFunctionIndexChanged();
  emit viewportChanged();
  emit graphModeChanged();
  emit graphModeSettingChanged();
  emit statPlotChanged();
  emit formatChanged();
  emit tableModeChanged();
  emit tableSettingsChanged();
  emit displayStateChanged();
  emit angleModeChanged();
  emit notationChanged();
  emit fixDecimalsChanged();
  emit numberBaseChanged();
  emit complexModeChanged();
  emit cursorMoved();
  emit insertModeChanged();
  emit drawModeChanged();
  emit traceChanged();
  emit functionsChanged();
  emit drawObjectsChanged();
}

void UIController::clearAllLists() {
  CrashLogger::logEvent(QStringLiteral("clearAllLists"));
  MathStateMachine::listRegistry.clear();
  emit statPlotChanged();  // a stat plot may reference a cleared list
}

void UIController::clearAllMatrices() {
  CrashLogger::logEvent(QStringLiteral("clearAllMatrices"));
  MathStateMachine::matrixRegistry.clear();
}

void UIController::clearAllVars() {
  CrashLogger::logEvent(QStringLiteral("clearAllVars"));
  MathStateMachine::varRegistry.fill(0.0);
}

void UIController::clearEntries() {
  CrashLogger::logEvent(QStringLiteral("clearEntries"));
  m_history.clear();
  m_entryHistory.clear();
  m_recallCycleIdx = -1;
  emit historyChanged();
}

QVariantMap UIController::memInfo() const {
  QVariantMap out;
  int vars = 0;
  for (double v : MathStateMachine::varRegistry)
    if (v != 0.0) ++vars;
  int fns = 0;
  for (const auto &b : m_functionBuffers)
    if (!b.empty()) ++fns;
  out["vars"] = vars;
  out["matrices"] = static_cast<int>(MathStateMachine::matrixRegistry.size());
  out["lists"] = static_cast<int>(MathStateMachine::listRegistry.size());
  out["functions"] = fns;
  out["entries"] = static_cast<int>(m_history.size());
  return out;
}

void UIController::setAngleMode(int m) {
  CrashLogger::logEvent(QStringLiteral("setAngleMode: ") + QString::number(m));
  // Clamp to the two valid values. Anything else becomes Radian — the
  // safer default, matches mathematical convention.
  AngleMode newMode = (m == 1) ? AngleMode::Degree : AngleMode::Radian;
  if (MathStateMachine::angleMode == newMode)
    return;
  MathStateMachine::angleMode = newMode;
  emit angleModeChanged();
}

void UIController::setNotation(int n) {
  CrashLogger::logEvent(QStringLiteral("setNotation: ") + QString::number(n));
  // Clamp to the three valid values. Anything else becomes Normal —
  // the default, matches prior behaviour.
  NumberNotation newNote =
      (n == 1) ? NumberNotation::Sci :
      (n == 2) ? NumberNotation::Eng :
                 NumberNotation::Normal;
  if (MathStateMachine::notation == newNote)
    return;
  MathStateMachine::notation = newNote;
  emit notationChanged();
}

void UIController::setFixDecimals(int n) {
  CrashLogger::logEvent(QStringLiteral("setFixDecimals: ") + QString::number(n));
  // -1 = Float, 0..9 = Fix N. Anything outside that range is clamped
  // to Float so the formatter never sees a nonsensical precision.
  const int clamped = (n >= 0 && n <= 9) ? n : -1;
  if (MathStateMachine::fixDecimals == clamped)
    return;
  MathStateMachine::fixDecimals = clamped;
  emit fixDecimalsChanged();
}

void UIController::setNumberBase(int b) {
  CrashLogger::logEvent(QStringLiteral("setNumberBase: ") + QString::number(b));
  // Clamp to the four valid values. Anything else becomes Dec — the
  // safe default that preserves the historic non-integer behaviour.
  NumberBase newBase =
      (b == 1) ? NumberBase::Hex :
      (b == 2) ? NumberBase::Oct :
      (b == 3) ? NumberBase::Bin :
                 NumberBase::Dec;
  if (MathStateMachine::numberBase == newBase)
    return;
  MathStateMachine::numberBase = newBase;
  emit numberBaseChanged();
}

void UIController::setComplexMode(int m) {
  CrashLogger::logEvent(QStringLiteral("setComplexMode: ") + QString::number(m));
  const ComplexMode nm = (m == 1) ? ComplexMode::Rect
                       : (m == 2) ? ComplexMode::Polar
                                  : ComplexMode::Real;
  if (MathStateMachine::complexMode == nm)
    return;
  MathStateMachine::complexMode = nm;
  emit complexModeChanged();
}

// Format a complex value as `a+bi` (or `a-bi`, `bi`, or just `a` when
// real). Each part routes through formatScalar so Notation/Decimal MODE
// settings apply. In Polar (re^θi) mode a non-real value shows as
// magnitude∠angle instead.
QString UIController::formatComplex(double re, double im) const {
  // Snap floating-point noise from transcendental complex results (e.g.
  // e^(iπ) = -1 + 1e-16 i) so the display reads cleanly. Only reached
  // for complex results, so this can't zero a genuine real value.
  const double eps = 1e-10 * std::max({1.0, std::abs(re), std::abs(im)});
  if (std::abs(im) < eps) im = 0.0;
  if (std::abs(re) < eps) re = 0.0;
  if (im == 0.0)
    return formatScalar(re);
  if (MathStateMachine::complexMode == ComplexMode::Polar) {
    const double mag = std::hypot(re, im);
    double ang = std::atan2(im, re);
    if (MathStateMachine::angleMode == AngleMode::Degree)
      ang = ang * 180.0 / M_PI;
    return formatScalar(mag) + QStringLiteral("∠") + formatScalar(ang);
  }
  // Rectangular a+bi.
  const QString imagPart = (std::abs(im) == 1.0)
                               ? QStringLiteral("i")
                               : (formatScalar(std::abs(im)) + QStringLiteral("i"));
  const QString sign = (im < 0.0) ? QStringLiteral("-") : QStringLiteral("+");
  if (re == 0.0)
    return (im < 0.0 ? QStringLiteral("-") : QString()) + imagPart;
  return formatScalar(re) + sign + imagPart;
}

int UIController::cursorOffset() const {
  const auto &buf = m_functionBuffers[m_activeIdx];
  const auto &rev = tokenToSpec();
  int offset = 0;
  const int limit = std::min(m_cursorPos, static_cast<int>(buf.size()));
  for (int i = 0; i < limit; ++i) {
    auto it = rev.find(static_cast<int>(buf[i]));
    if (it != rev.end())
      offset += QString::fromUtf8(it->second->displayStr).length();
  }
  return offset;
}

void UIController::toggleInsertMode() {
  CrashLogger::logEvent(QStringLiteral("toggleInsertMode"));
  m_insertMode = !m_insertMode;
  emit insertModeChanged();
}

void UIController::setDrawMode(int m) {
  CrashLogger::logEvent(QStringLiteral("setDrawMode: ") + QString::number(m));
  // Clamp to {0, 1}; anything else falls back to Connected (the
  // safer default so a stray write doesn't strand the user with
  // dots-only).
  const int clamped = (m == 1) ? 1 : 0;
  if (m_drawMode == clamped)
    return;
  m_drawMode = clamped;
  emit drawModeChanged();
}

void UIController::setGraphMode(int m) {
  CrashLogger::logEvent(QStringLiteral("setGraphMode: ") + QString::number(m));
  // All four graph modes are implemented: Func (0), Par (1), Pol (2),
  // Seq (3). Out-of-range writes fall back to Func.
  const int clamped = (m >= 1 && m <= 3) ? m : 0;
  if (m_graphMode == clamped)
    return;
  m_graphMode = clamped;
  emit graphModeSettingChanged();
  // The home-screen slot prefix (r/Y) and the graph render both depend
  // on the mode — nudge the display so bindings and the canvas refresh.
  emit displayChanged();
}

void UIController::moveCursorLeft() {
  CrashLogger::logEvent(QStringLiteral("moveCursorLeft"));
  if (m_displayState != Inputting || m_cursorPos <= 0)
    return;
  --m_cursorPos;
  emit cursorMoved();
}

void UIController::moveCursorRight() {
  CrashLogger::logEvent(QStringLiteral("moveCursorRight"));
  if (m_displayState != Inputting)
    return;
  const int maxPos = static_cast<int>(m_functionBuffers[m_activeIdx].size());
  if (m_cursorPos >= maxPos)
    return;
  ++m_cursorPos;
  emit cursorMoved();
}

void UIController::moveCursorHome() {
  CrashLogger::logEvent(QStringLiteral("moveCursorHome"));
  if (m_displayState != Inputting || m_cursorPos == 0)
    return;
  m_cursorPos = 0;
  emit cursorMoved();
}

void UIController::moveCursorEnd() {
  CrashLogger::logEvent(QStringLiteral("moveCursorEnd"));
  if (m_displayState != Inputting)
    return;
  const int endPos = static_cast<int>(m_functionBuffers[m_activeIdx].size());
  if (m_cursorPos == endPos)
    return;
  m_cursorPos = endPos;
  emit cursorMoved();
}

void UIController::recallLastEntry() {
  CrashLogger::logEvent(QStringLiteral("recallLastEntry"));
  if (m_entryHistory.empty())
    return;

  // Advance the cycle. Capped at the oldest available entry so
  // repeated 2ND+ENTER presses stop walking back once we hit the
  // beginning of the ring buffer.
  const int maxIdx = static_cast<int>(m_entryHistory.size()) - 1;
  m_recallCycleIdx = std::min(m_recallCycleIdx + 1, maxIdx);

  const size_t sourceIdx = m_entryHistory.size() - 1 - m_recallCycleIdx;
  auto &currentBuf = m_functionBuffers[m_activeIdx];
  auto &currentStr = m_displayStrings[m_activeIdx];

  // Restore the token buffer verbatim and rebuild the display string
  // via the unified token table — same approach used by backspace so
  // the renderer stays consistent.
  currentBuf = m_entryHistory[sourceIdx];
  currentStr = "";
  const auto &rev = tokenToSpec();
  for (auto t : currentBuf) {
    auto it = rev.find(static_cast<int>(t));
    if (it != rev.end())
      currentStr += QString::fromUtf8(it->second->displayStr);
  }

  // Cursor to end of the recalled expression so the user can append
  // or backspace immediately without an extra End keystroke.
  m_cursorPos = static_cast<int>(currentBuf.size());

  // Return to the editing state so the recalled expression can be
  // modified before the next ENTER.
  m_displayState = Inputting;
  m_displayExpression = "";
  emit displayChanged();
  emit displayStateChanged();
  emit cursorMoved();
}

QString UIController::currentDisplay() const {
  return m_displayStrings[m_activeIdx];
}

QString UIController::formatScalar(double value) {
  const auto notation = MathStateMachine::notation;
  const int fixN = MathStateMachine::fixDecimals;  // -1 = Float

  // Integer-base formatting. Only kicks in when the value is finite,
  // an exact integer, and fits in int64. Everything else falls back
  // through to the Notation/Decimal formatter so floats, overflowing
  // values, NaN, and ±inf still display sensibly.
  const auto base = MathStateMachine::numberBase;
  if (base != NumberBase::Dec && std::isfinite(value) &&
      value == std::floor(value) &&
      value >= -9.2233720368547748e18 &&
      value <=  9.2233720368547748e18) {
    const long long iv = static_cast<long long>(value);
    const unsigned long long mag =
        (iv < 0) ? static_cast<unsigned long long>(-(iv + 1)) + 1ULL
                 : static_cast<unsigned long long>(iv);
    QString digits;
    QString prefix;
    if (base == NumberBase::Hex) {
      digits = QString::number(mag, 16).toUpper();
      prefix = "0x";
    } else if (base == NumberBase::Oct) {
      digits = QString::number(mag, 8);
      prefix = "0o";
    } else {
      digits = QString::number(mag, 2);
      prefix = "0b";
    }
    return (iv < 0 ? QStringLiteral("-") : QString()) + prefix + digits;
  }

  // Normal + Float: the historical default — 'g' at precision 10
  // trims trailing zeros and keeps integer-like values (10!,
  // 3,628,800) out of scientific notation.
  if (notation == NumberNotation::Normal && fixN < 0)
    return QString::number(value, 'g', 10);

  // Normal + Fix N: fixed decimal places, no exponent.
  if (notation == NumberNotation::Normal)
    return QString::number(value, 'f', fixN);

  // Sci mode maps directly onto Qt's 'e' / 'E' formatters. Use 'E'
  // to match TI-83 conventions. Float uses precision 9 (matches the
  // Normal mode's effective digit count in scientific form).
  if (notation == NumberNotation::Sci) {
    const int prec = (fixN >= 0) ? fixN : 9;
    return QString::number(value, 'E', prec);
  }

  // Eng mode: same as Sci but the exponent is always a multiple of 3,
  // so mantissa ∈ [1, 1000). Qt has no built-in engineering formatter
  // so we normalise manually. Zero is a special case — log10(0) is
  // undefined, and the canonical TI-83 display is "0E0".
  if (value == 0.0) {
    const int prec = (fixN >= 0) ? fixN : 0;
    return QString::number(0.0, 'f', prec) + "E0";
  }
  const double absv = std::abs(value);
  const double log10v = std::log10(absv);
  // Round exponent down to the nearest multiple of 3 (works for
  // negatives too via std::floor).
  const int engExp = 3 * static_cast<int>(std::floor(log10v / 3.0));
  const double mantissa = value / std::pow(10.0, engExp);
  const int prec = (fixN >= 0) ? fixN : 6;
  return QString::number(mantissa, 'f', prec) + "E" +
         QString::number(engExp);
}

// ── processInput dispatcher ───────────────────────────────────
//
// Thin switch over the input string. Each branch delegates to a private
// helper that owns one concern. New input categories should be added by
// extending the dispatch table here, not by growing the helpers.
void UIController::processInput(const QString &input) {
  CrashLogger::logEvent(QStringLiteral("processInput: ") + input);

  // Any non-recall input resets the last-entry cycle. recallLastEntry()
  // bypasses processInput, so it's safe to unconditionally reset here.
  m_recallCycleIdx = -1;

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
  m_cursorPos = 0;
  bool stateChanged =
      (m_displayState != Inputting || !m_displayExpression.isEmpty());
  m_displayState = Inputting;
  m_displayExpression = "";
  if (stateChanged)
    emit displayStateChanged();
  emit displayChanged();
  emit cursorMoved();
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
    m_cursorPos = 0;
    m_displayState = Inputting;
    m_displayExpression = "";
    emit displayStateChanged();
    emit displayChanged();
    emit cursorMoved();
    return;
  }
  // In Inputting, backspace removes the token to the LEFT of the cursor
  // (not the last token in the buffer). Cursor at 0 is a no-op.
  if (m_cursorPos <= 0 || currentBuf.empty())
    return;

  currentBuf.erase(currentBuf.begin() + (m_cursorPos - 1));
  --m_cursorPos;

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
  emit cursorMoved();
}

// ENTER — evaluate the buffer, transition to EVALUATED or ERROR.
void UIController::evaluate() {
  auto &currentBuf = m_functionBuffers[m_activeIdx];
  auto &currentStr = m_displayStrings[m_activeIdx];

  CrashLogger::logEvent(QStringLiteral("evaluate: ") + currentStr);

  // Push this entry into the recall ring buffer before any further
  // mutation. Empty buffers (ENTER on an empty line) skip the push —
  // TI-83 doesn't cycle through blank entries. Both successful and
  // failed entries get stored so users can recall a typo to fix it.
  if (!currentBuf.empty()) {
    m_entryHistory.push_back(currentBuf);
    while (m_entryHistory.size() > static_cast<size_t>(kEntryHistoryCap))
      m_entryHistory.pop_front();
  }

  // Snapshot expression for the top "expression =" display line before
  // currentStr is overwritten with the result.
  m_displayExpression = currentStr;
  QString entry =
      functionLabel(m_activeIdx) + ": " + currentStr + " = ";

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
          matStr += formatScalar(result.matrixValue.at(i, j));
          if (j < result.matrixValue.cols - 1)
            matStr += ",";
        }
        if (i < result.matrixValue.rows - 1)
          matStr += "][";
      }
      currentStr = matStr + "]]";
    } else if (result.isList) {
      // Phase C: render a list as {e1,e2,...}. Elements route through
      // formatScalar so they honour the active Notation/Decimal/Base
      // MODE settings, consistent with scalar and matrix display.
      QString listStr = "{";
      for (size_t i = 0; i < result.listValue.size(); ++i) {
        listStr += formatScalar(result.listValue[i]);
        if (i + 1 < result.listValue.size())
          listStr += ",";
      }
      currentStr = listStr + "}";
    } else if (result.imag != 0.0) {
      // Complex result (Phase F) — a+bi (or re^θi in Polar mode).
      currentStr = formatComplex(result.value, result.imag);
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
    else if (msg == "Undefined Matrix" || msg == "Undefined List")
      currentStr = "ERR:UNDEFINED";
    else if (msg == "SINGULAR MAT")
      currentStr = "ERR:SINGULAR MAT";
    else if (msg == "Recursion")
      currentStr = "ERR:RECURSION";
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
  m_history.prepend(functionLabel(m_activeIdx) +
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
  m_history.prepend(functionLabel(m_activeIdx) +
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

  // Defensive clamp: m_cursorPos is a single value across all three
  // function slots, so a slot switch (via setActiveFunction, recall,
  // loadState, etc.) can leave the cursor past the new slot's length.
  // Inserting at a past-end position is undefined for std::vector and
  // crashed on the loadState path (Y1 cursor at 1 → switch to empty
  // Y2 → insert at begin()+1 → SIGSEGV). Clamp keeps the cursor
  // valid; the user-facing effect is "switching slots puts the cursor
  // at the end of the new buffer."
  if (m_cursorPos > static_cast<int>(currentBuf.size()))
    m_cursorPos = static_cast<int>(currentBuf.size());

  // State-machine reset: if a stale EVALUATED/ERROR result is on screen,
  // clear it before appending the new token. Implements the spec rule
  // "next digit/function keypress: clears expr, returns to INPUTTING".
  if (m_displayState != Inputting) {
    currentBuf.clear();
    currentStr = "";
    m_cursorPos = 0;
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
  //
  // "Prior token" is relative to the cursor, not the end of the buffer —
  // cursor-aware editing can insert mid-expression.
  if (tokenToInsert == Token::Sub) {
    bool isUnary = (m_cursorPos == 0);
    if (!isUnary) {
      Token prev = currentBuf[m_cursorPos - 1];
      isUnary = (prev == Token::LeftParen ||
                 prev == Token::Comma ||
                 prev == Token::Add || prev == Token::Sub ||
                 prev == Token::Mul || prev == Token::Div ||
                 prev == Token::Pow || prev == Token::NthRoot ||
                 EOSPrecedence::is_function(prev));
    }
    if (isUnary)
      tokenToInsert = Token::Neg;
  }

  // Cursor-aware insertion. In insert mode (default) splice the new
  // token at m_cursorPos and advance the cursor past it; mid-expression
  // inserts shift everything right. In overwrite mode, if the cursor
  // is on an existing token, replace it; if at the end, fall back to
  // append (matches TI-83 OVR behaviour at the tail).
  const bool atEnd =
      m_cursorPos >= static_cast<int>(currentBuf.size());
  if (m_insertMode || atEnd) {
    currentBuf.insert(currentBuf.begin() + m_cursorPos, tokenToInsert);
  } else {
    currentBuf[m_cursorPos] = tokenToInsert;
  }
  ++m_cursorPos;
  currentStr = "";
  const auto &rev = tokenToSpec();
  for (auto t : currentBuf) {
    auto rit = rev.find(static_cast<int>(t));
    if (rit != rev.end())
      currentStr += QString::fromUtf8(rit->second->displayStr);
  }
  emit displayChanged();
  emit cursorMoved();
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
  CrashLogger::logEvent(QStringLiteral("processExpression: ") + expr);
  QStringList tokens = tokenize(expr);
  if (tokens.isEmpty() && !expr.trimmed().isEmpty())
    return false;
  for (const QString &t : tokens)
    processInput(t);
  return true;
}

QStringList UIController::catalogEntries() const {
  // Walk kTokens, collect insertable display strings, deduplicate (the
  // ASCII aliases like "->" and "<=" share displayStr with their
  // Unicode siblings), and sort alphabetically. Results returned to
  // QML for the CATALOG popup; clicks feed the displayStr back through
  // processExpression so the tokeniser handles dispatch uniformly.
  QStringList out;
  out.reserve(static_cast<qsizetype>(sizeof(kTokens) / sizeof(kTokens[0])));
  for (const auto &spec : kTokens) {
    out.append(QString::fromUtf8(spec.displayStr));
  }
  out.removeDuplicates();
  // Case-insensitive alphabetical sort so `Ans` lands near `abs(`
  // rather than at the top of the list.
  std::sort(out.begin(), out.end(),
            [](const QString &a, const QString &b) {
              return QString::compare(a, b, Qt::CaseInsensitive) < 0;
            });
  return out;
}

// IMP-008: map a matrix name ("[A]".."[J]", or bare "A".."J") to its
// registry Token. Replaces the hardcoded [A]/[B]/[C] if-chain so the
// editor can target any of the engine's ten matrix slots. Returns false
// for anything outside the A..J range.
static bool matrixTokenForName(const QString &name, Token &out) {
  QString s = name;
  s.remove('[').remove(']');
  if (s.size() != 1)
    return false;
  const QChar c = s.at(0).toUpper();
  if (c < QChar('A') || c > QChar('J'))
    return false;
  out = static_cast<Token>(static_cast<int>(Token::MatA) +
                           (c.unicode() - 'A'));
  return true;
}

void UIController::updateMatrix(const QString &name, int rows, int cols,
                                const QVariantList &values) {
  CrashLogger::logEvent(QStringLiteral("updateMatrix: ") + name +
                        QStringLiteral(" ") + QString::number(rows) +
                        QStringLiteral("x") + QString::number(cols));
  Token tok;
  if (!matrixTokenForName(name, tok))
    return;
  Matrix mat;
  mat.rows = rows;
  mat.cols = cols;
  for (const auto &v : values)
    mat.data.push_back(v.toDouble());
  MathStateMachine::matrixRegistry[tok] = mat;
}

// Map a list name ("L1".."L6") to its registry Token. Returns false for
// anything outside L1..L6.
static bool listTokenForName(const QString &name, Token &out) {
  if (name.size() != 2 || name.at(0).toUpper() != QChar('L'))
    return false;
  const QChar d = name.at(1);
  if (d < QChar('1') || d > QChar('6'))
    return false;
  out = static_cast<Token>(static_cast<int>(Token::L1) +
                           (d.unicode() - '1'));
  return true;
}

QVariantList UIController::getList(const QString &name) const {
  QVariantList out;
  Token tok;
  if (listTokenForName(name, tok)) {
    auto it = MathStateMachine::listRegistry.find(tok);
    if (it != MathStateMachine::listRegistry.end())
      for (double v : it->second)
        out.append(v);
  }
  return out;
}

void UIController::updateList(const QString &name,
                             const QVariantList &values) {
  CrashLogger::logEvent(QStringLiteral("updateList: ") + name +
                        QStringLiteral(" n=") +
                        QString::number(values.size()));
  Token tok;
  if (!listTokenForName(name, tok))
    return;
  std::vector<double> data;
  data.reserve(static_cast<size_t>(values.size()));
  for (const auto &v : values)
    data.push_back(v.toDouble());
  MathStateMachine::listRegistry[tok] = data;
}

QVariantMap UIController::oneVarStats(const QString &name) const {
  QVariantMap out;
  Token tok;
  if (!listTokenForName(name, tok)) {
    out["error"] = "UNDEFINED";
    return out;
  }
  auto it = MathStateMachine::listRegistry.find(tok);
  if (it == MathStateMachine::listRegistry.end() || it->second.empty()) {
    out["error"] = "UNDEFINED";
    return out;
  }
  std::vector<double> v = it->second;  // sorted copy below
  const int n = static_cast<int>(v.size());

  double sumX = 0.0, sumX2 = 0.0;
  for (double x : v) { sumX += x; sumX2 += x * x; }
  const double mean = sumX / n;
  double ss = 0.0;
  for (double x : v) ss += (x - mean) * (x - mean);
  const double sigmaX = std::sqrt(ss / n);                      // population
  const double Sx = (n >= 2) ? std::sqrt(ss / (n - 1)) : 0.0;   // sample

  std::sort(v.begin(), v.end());
  // Median of the half-open index range [lo, hi).
  auto med = [&v](int lo, int hi) -> double {
    const int m = hi - lo;
    const int mid = lo + m / 2;
    return (m % 2 == 1) ? v[static_cast<size_t>(mid)]
                        : (v[static_cast<size_t>(mid - 1)] +
                           v[static_cast<size_t>(mid)]) / 2.0;
  };
  const double median = med(0, n);
  // Quartiles (TI-83 rule): split at the median; for odd n the median
  // element is excluded from both halves.
  const int lowHi = n / 2;
  const int upLo  = (n % 2 == 0) ? n / 2 : n / 2 + 1;
  const double Q1 = med(0, lowHi);
  const double Q3 = med(upLo, n);

  out["error"]  = "";
  out["n"]      = n;
  out["mean"]   = mean;
  out["sumX"]   = sumX;
  out["sumX2"]  = sumX2;
  out["Sx"]     = Sx;
  out["sigmaX"] = sigmaX;
  out["minX"]   = v.front();
  out["Q1"]     = Q1;
  out["median"] = median;
  out["Q3"]     = Q3;
  out["maxX"]   = v.back();
  return out;
}

namespace {

// Solve the square linear system A·x = b by Gaussian elimination with
// partial pivoting. Returns false if the matrix is singular.
bool solveLinear(std::vector<std::vector<double>> A, std::vector<double> b,
                 std::vector<double> &x) {
  const int n = static_cast<int>(b.size());
  for (int col = 0; col < n; ++col) {
    int piv = col;
    for (int r = col + 1; r < n; ++r)
      if (std::abs(A[r][col]) > std::abs(A[piv][col])) piv = r;
    if (std::abs(A[piv][col]) < 1e-12) return false;
    std::swap(A[col], A[piv]);
    std::swap(b[col], b[piv]);
    for (int r = 0; r < n; ++r) {
      if (r == col) continue;
      const double f = A[r][col] / A[col][col];
      for (int c = col; c < n; ++c) A[r][c] -= f * A[col][c];
      b[r] -= f * b[col];
    }
  }
  x.assign(static_cast<size_t>(n), 0.0);
  for (int i = 0; i < n; ++i) x[i] = b[i] / A[i][i];
  return true;
}

// Least-squares polynomial fit of the given degree via the normal
// equations. Fills a,b[,c,d] (highest-degree coefficient = a) and R².
QVariantMap polyReg(const std::vector<double> &X, const std::vector<double> &Y,
                    int deg) {
  QVariantMap out;
  const int n = static_cast<int>(X.size());
  if (n < deg + 1) { out["error"] = "DOMAIN"; return out; }
  const int m = deg + 1;
  std::vector<double> powSum(static_cast<size_t>(2 * deg + 1), 0.0);
  for (int k = 0; k <= 2 * deg; ++k) {
    double s = 0.0;
    for (double xv : X) s += std::pow(xv, k);
    powSum[static_cast<size_t>(k)] = s;
  }
  std::vector<std::vector<double>> A(m, std::vector<double>(m));
  std::vector<double> B(static_cast<size_t>(m), 0.0);
  for (int i = 0; i < m; ++i) {
    for (int j = 0; j < m; ++j) A[i][j] = powSum[static_cast<size_t>(i + j)];
    double s = 0.0;
    for (int p = 0; p < n; ++p) s += Y[p] * std::pow(X[p], i);
    B[static_cast<size_t>(i)] = s;
  }
  std::vector<double> coef;
  if (!solveLinear(A, B, coef)) { out["error"] = "DOMAIN"; return out; }

  static const char *kNames[] = {"a", "b", "c", "d", "e"};
  for (int k = 0; k <= deg; ++k) out[kNames[k]] = coef[deg - k];

  double meanY = 0.0;
  for (double yv : Y) meanY += yv;
  meanY /= n;
  double ssTot = 0.0, ssRes = 0.0;
  for (int p = 0; p < n; ++p) {
    double yhat = 0.0;
    for (int k = 0; k <= deg; ++k) yhat += coef[k] * std::pow(X[p], k);
    ssRes += (Y[p] - yhat) * (Y[p] - yhat);
    ssTot += (Y[p] - meanY) * (Y[p] - meanY);
  }
  if (ssTot > 1e-12) out["r2"] = 1.0 - ssRes / ssTot;
  out["error"] = "";
  out["n"] = n;
  return out;
}

// Least-squares straight-line fit; fills slope, intercept, and the
// correlation r. Returns false when x has no spread.
bool linFit(const std::vector<double> &x, const std::vector<double> &y,
            double &slope, double &intercept, double &r) {
  const int n = static_cast<int>(x.size());
  double sx = 0, sy = 0, sxy = 0, sx2 = 0, sy2 = 0;
  for (int i = 0; i < n; ++i) {
    sx += x[i]; sy += y[i]; sxy += x[i] * y[i];
    sx2 += x[i] * x[i]; sy2 += y[i] * y[i];
  }
  const double dx = n * sx2 - sx * sx;
  if (std::abs(dx) < 1e-12) return false;
  slope = (n * sxy - sx * sy) / dx;
  intercept = (sy - slope * sx) / n;
  const double dy = n * sy2 - sy * sy;
  r = (dy > 1e-12) ? (n * sxy - sx * sy) / std::sqrt(dx * dy) : 0.0;
  return true;
}

}  // namespace

QVariantMap UIController::regression(const QString &type, const QString &xName,
                                     const QString &yName) const {
  QVariantMap out;
  Token xt, yt;
  if (!listTokenForName(xName, xt) || !listTokenForName(yName, yt)) {
    out["error"] = "UNDEFINED";
    return out;
  }
  auto xi = MathStateMachine::listRegistry.find(xt);
  auto yi = MathStateMachine::listRegistry.find(yt);
  if (xi == MathStateMachine::listRegistry.end() ||
      yi == MathStateMachine::listRegistry.end() ||
      xi->second.empty() || yi->second.empty()) {
    out["error"] = "UNDEFINED";
    return out;
  }
  const std::vector<double> &X = xi->second;
  const std::vector<double> &Y = yi->second;
  if (X.size() != Y.size()) {
    out["error"] = "DIM";
    return out;
  }
  const int n = static_cast<int>(X.size());
  const QString t = type.toLower();

  if (t == "quad") return polyReg(X, Y, 2);
  if (t == "cubic") return polyReg(X, Y, 3);

  // Linearised models: exp (y=a·bˣ), ln (y=a+b·lnx), pwr (y=a·xᵇ).
  const bool needXpos = (t == "ln" || t == "pwr");
  const bool needYpos = (t == "exp" || t == "pwr");
  if (t != "exp" && t != "ln" && t != "pwr") {
    out["error"] = "UNDEFINED";  // unknown model
    return out;
  }
  for (int i = 0; i < n; ++i) {
    if ((needXpos && X[i] <= 0.0) || (needYpos && Y[i] <= 0.0)) {
      out["error"] = "DOMAIN";
      return out;
    }
  }
  std::vector<double> xt2(n), yt2(n);
  for (int i = 0; i < n; ++i) {
    xt2[i] = needXpos ? std::log(X[i]) : X[i];
    yt2[i] = needYpos ? std::log(Y[i]) : Y[i];
  }
  double slope, intercept, r;
  if (!linFit(xt2, yt2, slope, intercept, r)) {
    out["error"] = "DOMAIN";
    return out;
  }
  double a, b;
  if (t == "exp")      { a = std::exp(intercept); b = std::exp(slope); }
  else if (t == "ln")  { a = intercept;           b = slope;           }
  else /* pwr */       { a = std::exp(intercept); b = slope;           }

  out["error"] = "";
  out["n"]  = n;
  out["a"]  = a;
  out["b"]  = b;
  out["r"]  = r;
  out["r2"] = r * r;
  return out;
}

QVariantMap UIController::getStatPlotData() const {
  QVariantMap out;
  out["on"] = m_statPlotOn;
  out["type"] = m_statPlotType;
  out["error"] = "";
  if (!m_statPlotOn)
    return out;

  Token xt;
  if (!listTokenForName(m_statPlotXList, xt)) { out["error"] = "UNDEFINED"; return out; }
  auto xi = MathStateMachine::listRegistry.find(xt);
  if (xi == MathStateMachine::listRegistry.end() || xi->second.empty()) {
    out["error"] = "UNDEFINED";
    return out;
  }
  const std::vector<double> &X = xi->second;
  const int n = static_cast<int>(X.size());

  if (m_statPlotType == 0 || m_statPlotType == 1) {
    // Scatter / xyLine — pair with Ylist (equal length required).
    Token yt;
    if (!listTokenForName(m_statPlotYList, yt)) { out["error"] = "UNDEFINED"; return out; }
    auto yi = MathStateMachine::listRegistry.find(yt);
    if (yi == MathStateMachine::listRegistry.end() || yi->second.empty()) {
      out["error"] = "UNDEFINED";
      return out;
    }
    const std::vector<double> &Y = yi->second;
    if (X.size() != Y.size()) { out["error"] = "DIM"; return out; }
    std::vector<std::pair<double, double>> pts;
    pts.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) pts.push_back({X[i], Y[i]});
    if (m_statPlotType == 1)  // xyLine: connect in x-order
      std::sort(pts.begin(), pts.end());
    QVariantList points;
    for (auto &p : pts) {
      QVariantMap m;
      m["x"] = p.first;
      m["y"] = p.second;
      points.append(m);
    }
    out["points"] = points;
  } else if (m_statPlotType == 2) {
    // Histogram — auto-binned across [min, max]; frequency on the y-axis.
    double mn = X[0], mx = X[0];
    for (double v : X) { mn = std::min(mn, v); mx = std::max(mx, v); }
    int nbins = std::clamp(static_cast<int>(std::ceil(std::sqrt((double)n))), 1, 20);
    if (mx <= mn) { nbins = 1; mx = mn + 1.0; }  // degenerate — single bin
    const double width = (mx - mn) / nbins;
    std::vector<int> counts(static_cast<size_t>(nbins), 0);
    for (double v : X) {
      int b = static_cast<int>(std::floor((v - mn) / width));
      if (b < 0) b = 0;
      if (b >= nbins) b = nbins - 1;
      counts[static_cast<size_t>(b)]++;
    }
    int maxCount = 0;
    for (int c : counts) maxCount = std::max(maxCount, c);
    QVariantList bins;
    for (int b = 0; b < nbins; ++b) {
      QVariantMap m;
      m["lo"] = mn + b * width;
      m["hi"] = mn + (b + 1) * width;
      m["count"] = counts[static_cast<size_t>(b)];
      bins.append(m);
    }
    out["bins"] = bins;
    out["maxCount"] = maxCount;
  } else {
    // Box plot — five-number summary (TI-83 median-of-halves quartiles).
    std::vector<double> v = X;
    std::sort(v.begin(), v.end());
    auto med = [&v](int lo, int hi) -> double {
      const int m = hi - lo, mid = lo + m / 2;
      return (m % 2 == 1) ? v[static_cast<size_t>(mid)]
                          : (v[static_cast<size_t>(mid - 1)] +
                             v[static_cast<size_t>(mid)]) / 2.0;
    };
    const int lowHi = n / 2;
    const int upLo = (n % 2 == 0) ? n / 2 : n / 2 + 1;
    QVariantMap box;
    box["min"] = v.front();
    box["q1"]  = med(0, lowHi);
    box["med"] = med(0, n);
    box["q3"]  = med(upLo, n);
    box["max"] = v.back();
    out["box"] = box;
  }
  return out;
}

QVariantMap UIController::twoVarStats(const QString &xName,
                                      const QString &yName) const {
  QVariantMap out;
  Token xt, yt;
  if (!listTokenForName(xName, xt) || !listTokenForName(yName, yt)) {
    out["error"] = "UNDEFINED";
    return out;
  }
  auto xi = MathStateMachine::listRegistry.find(xt);
  auto yi = MathStateMachine::listRegistry.find(yt);
  if (xi == MathStateMachine::listRegistry.end() ||
      yi == MathStateMachine::listRegistry.end() ||
      xi->second.empty() || yi->second.empty()) {
    out["error"] = "UNDEFINED";
    return out;
  }
  const std::vector<double> &X = xi->second;
  const std::vector<double> &Y = yi->second;
  if (X.size() != Y.size()) {
    out["error"] = "DIM";
    return out;
  }
  const int n = static_cast<int>(X.size());

  double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0, sumY2 = 0;
  for (int i = 0; i < n; ++i) {
    sumX += X[i]; sumY += Y[i];
    sumXY += X[i] * Y[i];
    sumX2 += X[i] * X[i]; sumY2 += Y[i] * Y[i];
  }
  const double meanX = sumX / n, meanY = sumY / n;
  double ssX = 0, ssY = 0;
  for (int i = 0; i < n; ++i) {
    ssX += (X[i] - meanX) * (X[i] - meanX);
    ssY += (Y[i] - meanY) * (Y[i] - meanY);
  }
  const double Sx = (n >= 2) ? std::sqrt(ssX / (n - 1)) : 0.0;
  const double Sy = (n >= 2) ? std::sqrt(ssY / (n - 1)) : 0.0;

  out["error"]  = "";
  out["n"]      = n;
  out["meanX"]  = meanX;
  out["meanY"]  = meanY;
  out["sumX"]   = sumX;
  out["sumY"]   = sumY;
  out["sumXY"]  = sumXY;
  out["sumX2"]  = sumX2;
  out["sumY2"]  = sumY2;
  out["Sx"]     = Sx;
  out["Sy"]     = Sy;

  // Least-squares linear regression y = ax + b. Skipped (fields left
  // absent → "—" in the results screen) when X has no spread.
  const double denomX = n * sumX2 - sumX * sumX;
  if (std::abs(denomX) > 1e-12) {
    const double a = (n * sumXY - sumX * sumY) / denomX;
    const double b = meanY - a * meanX;
    out["a"] = a;
    out["b"] = b;
    const double denomY = n * sumY2 - sumY * sumY;
    if (denomY > 1e-12) {
      const double r = (n * sumXY - sumX * sumY) / std::sqrt(denomX * denomY);
      out["r"]  = r;
      out["r2"] = r * r;
    }
  }
  return out;
}

QVariantMap UIController::getMatrix(const QString &name) const {
  QVariantMap out;
  QVariantList data;
  Token tok;
  if (matrixTokenForName(name, tok)) {
    auto it = MathStateMachine::matrixRegistry.find(tok);
    if (it != MathStateMachine::matrixRegistry.end()) {
      const Matrix &m = it->second;
      out["rows"] = m.rows;
      out["cols"] = m.cols;
      for (double v : m.data)
        data.append(v);
      out["data"] = data;
      return out;
    }
  }
  // Unknown name or slot never populated — report an empty matrix so the
  // editor falls back to its default blank grid.
  out["rows"] = 0;
  out["cols"] = 0;
  out["data"] = data;
  return out;
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

double UIController::traceY() const {
  // Evaluate the currently-active function at m_traceX. NaN if the
  // active buffer is empty or evaluation fails — QML branches on
  // isFinite to show "Y=undefined" cleanly.
  if (m_activeIdx < 0 ||
      m_activeIdx >= static_cast<int>(m_functionBuffers.size()))
    return std::numeric_limits<double>::quiet_NaN();
  const auto &buf = m_functionBuffers[m_activeIdx];
  if (buf.empty())
    return std::numeric_limits<double>::quiet_NaN();
  MathStateMachine msm;
  CalculationResult res = msm.evaluate(buf, m_traceX);
  if (!res.success || res.isMatrix)
    return std::numeric_limits<double>::quiet_NaN();
  return res.value;
}

void UIController::toggleTrace() {
  CrashLogger::logEvent(QStringLiteral("toggleTrace"));
  m_isTracing = !m_isTracing;
  if (m_isTracing) {
    // Snap the trace to the viewport centre on every entry so the
    // user always starts from a known visible spot, even if they
    // panned away last time.
    m_traceX = (m_xMin + m_xMax) * 0.5;
  }
  emit traceChanged();
}

void UIController::traceLeft() {
  if (!m_isTracing) return;
  // Step by 1/100 of the viewport width — matches real TI-83 sample
  // density and keeps the cursor within the visible curve.
  const double step = (m_xMax - m_xMin) / 100.0;
  m_traceX -= step;
  emit traceChanged();
}

void UIController::traceRight() {
  if (!m_isTracing) return;
  const double step = (m_xMax - m_xMin) / 100.0;
  m_traceX += step;
  emit traceChanged();
}

void UIController::zoomIn() {
  CrashLogger::logEvent(QStringLiteral("zoomIn"));
  // Halve both axes around the viewport centre: curve appears bigger.
  const double cx = (m_xMin + m_xMax) * 0.5;
  const double cy = (m_yMin + m_yMax) * 0.5;
  const double rx = (m_xMax - m_xMin) * 0.5;
  const double ry = (m_yMax - m_yMin) * 0.5;
  m_xMin = cx - rx * 0.5;
  m_xMax = cx + rx * 0.5;
  m_yMin = cy - ry * 0.5;
  m_yMax = cy + ry * 0.5;
  emit viewportChanged();
}

void UIController::zoomOut() {
  CrashLogger::logEvent(QStringLiteral("zoomOut"));
  // Double both axes around the viewport centre: curve appears smaller.
  const double cx = (m_xMin + m_xMax) * 0.5;
  const double cy = (m_yMin + m_yMax) * 0.5;
  const double rx = (m_xMax - m_xMin) * 0.5;
  const double ry = (m_yMax - m_yMin) * 0.5;
  m_xMin = cx - rx * 2.0;
  m_xMax = cx + rx * 2.0;
  m_yMin = cy - ry * 2.0;
  m_yMax = cy + ry * 2.0;
  emit viewportChanged();
}

void UIController::zoomSquare() {
  CrashLogger::logEvent(QStringLiteral("zoomSquare"));
  // Snap y-range to match x-range while keeping the current centre.
  // 1 unit X ≈ 1 unit Y on screen (exact for square canvases).
  const double cy = (m_yMin + m_yMax) * 0.5;
  const double half = (m_xMax - m_xMin) * 0.5;
  m_yMin = cy - half;
  m_yMax = cy + half;
  emit viewportChanged();
}

void UIController::zoomTrig() {
  CrashLogger::logEvent(QStringLiteral("zoomTrig"));
  // TI-83's trig-friendly window. 2.3π chosen on the real device so
  // each pixel maps to π/24 — the exact tick step it uses for sin/cos.
  // We don't have the same fixed pixel grid, but the bounds still
  // give a "natural" trig view.
  m_xMin = -2.3 * M_PI;
  m_xMax =  2.3 * M_PI;
  m_yMin = -4.0;
  m_yMax =  4.0;
  emit viewportChanged();
}

void UIController::zoomDecimal() {
  CrashLogger::logEvent(QStringLiteral("zoomDecimal"));
  // TI-83's ZDecimal: the [-4.7, 4.7] × [-3.1, 3.1] window. On the
  // real device this gives each pixel a coordinate of 0.1 exactly;
  // here it's just a "clean decimal" preset.
  m_xMin = -4.7;
  m_xMax =  4.7;
  m_yMin = -3.1;
  m_yMax =  3.1;
  emit viewportChanged();
}

void UIController::zoomInteger() {
  CrashLogger::logEvent(QStringLiteral("zoomInteger"));
  // Snap each viewport edge to its nearest integer. Keeps the user
  // close to where they were but cleans up the decimals.
  m_xMin = std::floor(m_xMin + 0.5);
  m_xMax = std::floor(m_xMax + 0.5);
  m_yMin = std::floor(m_yMin + 0.5);
  m_yMax = std::floor(m_yMax + 0.5);
  // Guard against degenerate zero-width windows after snapping (e.g.
  // a user who'd already zoomed to a sub-unit range): expand to at
  // least ±1 around centre so the canvas still draws something.
  if (m_xMax - m_xMin < 1.0) {
    const double cx = (m_xMin + m_xMax) * 0.5;
    m_xMin = cx - 1.0; m_xMax = cx + 1.0;
  }
  if (m_yMax - m_yMin < 1.0) {
    const double cy = (m_yMin + m_yMax) * 0.5;
    m_yMin = cy - 1.0; m_yMax = cy + 1.0;
  }
  emit viewportChanged();
}

void UIController::zoomBox(double x1, double y1, double x2, double y2) {
  CrashLogger::logEvent(QStringLiteral("zoomBox"));
  m_zoomBoxArm = false;
  emit zoomBoxArmChanged();
  const double xlo = std::min(x1, x2), xhi = std::max(x1, x2);
  const double ylo = std::min(y1, y2), yhi = std::max(y1, y2);
  // Ignore a degenerate (near-zero-area) box — e.g. a stray click.
  if (xhi - xlo < 1e-9 || yhi - ylo < 1e-9)
    return;
  savePrevViewport();
  m_xMin = xlo; m_xMax = xhi; m_yMin = ylo; m_yMax = yhi;
  emit viewportChanged();
}

void UIController::zoomStat() {
  CrashLogger::logEvent(QStringLiteral("zoomStat"));
  // Fit the viewport to the stat-plot lists: x from Xlist, y from Ylist
  // (scatter/xyLine) or from Xlist otherwise. No-op if Xlist is empty.
  Token xt;
  if (!listTokenForName(m_statPlotXList, xt))
    return;
  auto xi = MathStateMachine::listRegistry.find(xt);
  if (xi == MathStateMachine::listRegistry.end() || xi->second.empty())
    return;
  const std::vector<double> &X = xi->second;

  const std::vector<double> *Y = &X;
  if (m_statPlotType == 0 || m_statPlotType == 1) {
    Token yt;
    if (listTokenForName(m_statPlotYList, yt)) {
      auto yi = MathStateMachine::listRegistry.find(yt);
      if (yi != MathStateMachine::listRegistry.end() && !yi->second.empty())
        Y = &yi->second;
    }
  }

  double xlo = X[0], xhi = X[0];
  for (double v : X) { xlo = std::min(xlo, v); xhi = std::max(xhi, v); }
  double ylo = (*Y)[0], yhi = (*Y)[0];
  for (double v : *Y) { ylo = std::min(ylo, v); yhi = std::max(yhi, v); }

  double xpad = (xhi - xlo) * 0.1; if (xpad <= 0.0) xpad = 1.0;
  double ypad = (yhi - ylo) * 0.1; if (ypad <= 0.0) ypad = 1.0;
  m_xMin = xlo - xpad; m_xMax = xhi + xpad;
  m_yMin = ylo - ypad; m_yMax = yhi + ypad;
  emit viewportChanged();
}

void UIController::drawLine(double x1, double y1, double x2, double y2) {
  QVariantMap o;
  o["type"] = "line"; o["a"] = x1; o["b"] = y1; o["c"] = x2; o["d"] = y2;
  m_drawObjects.append(o);
  emit drawObjectsChanged();
}

void UIController::drawCircle(double x, double y, double r) {
  QVariantMap o;
  o["type"] = "circle"; o["a"] = x; o["b"] = y; o["c"] = r;
  m_drawObjects.append(o);
  emit drawObjectsChanged();
}

void UIController::drawHorizontal(double y) {
  QVariantMap o;
  o["type"] = "hline"; o["a"] = y;
  m_drawObjects.append(o);
  emit drawObjectsChanged();
}

void UIController::drawVertical(double x) {
  QVariantMap o;
  o["type"] = "vline"; o["a"] = x;
  m_drawObjects.append(o);
  emit drawObjectsChanged();
}

void UIController::drawPoint(double x, double y) {
  QVariantMap o;
  o["type"] = "point"; o["a"] = x; o["b"] = y;
  m_drawObjects.append(o);
  emit drawObjectsChanged();
}

void UIController::drawText(double x, double y, const QString &text) {
  QVariantMap o;
  o["type"] = "text"; o["a"] = x; o["b"] = y; o["text"] = text;
  m_drawObjects.append(o);
  emit drawObjectsChanged();
}

void UIController::clrDraw() {
  CrashLogger::logEvent(QStringLiteral("clrDraw"));
  m_drawObjects.clear();
  emit drawObjectsChanged();
}

void UIController::deleteDrawObject(int index) {
  if (index >= 0 && index < m_drawObjects.size()) {
    m_drawObjects.removeAt(index);
    emit drawObjectsChanged();
  }
}

QVariantList UIController::getTableRows(int count, double xStart) {
  // Build `count` rows starting at xStart, stepping by m_tblStep.
  // Each row evaluates Y1/Y2/Y3 at that X; non-scalar / failed
  // results are omitted from the row (QML treats missing keys as
  // "—"). Skipped for empty buffers to keep the table sparse — the
  // user sees blank cells where no expression exists.
  QVariantList rows;
  MathStateMachine msm;
  for (int i = 0; i < count; ++i) {
    const double x = xStart + i * m_tblStep;
    QVariantMap row;
    row["x"] = x;
    for (size_t f = 0; f < m_functionBuffers.size(); ++f) {
      const auto &buf = m_functionBuffers[f];
      if (buf.empty()) continue;
      CalculationResult res = msm.evaluate(buf, x);
      if (!res.success || res.isMatrix) continue;
      row[QString("y%1").arg(f + 1)] = res.value;
    }
    rows.append(row);
  }
  return rows;
}

QVariantList UIController::getMultiGraphPoints(int resolution) {
  // BUG-012 fix: always emit one entry per Y slot (Y1 / Y2 / Y3),
  // using an empty inner list for buffers that have no expression.
  // The QML canvas colours curves by their index in the returned
  // list; before this fix, an empty leading slot compacted the list
  // and shifted later curves into earlier slots' colours (e.g. Y1
  // empty + Y3=X² produced a green-coded parabola rendered in Y1's
  // blue). Preserving slot index → colour mapping is the cleanest
  // fix; QML doesn't need to know about empty slots, it just gets
  // an empty array and skips it.
  QVariantList allFunctions;
  MathStateMachine msm;
  const bool degreeMode = (MathStateMachine::angleMode == AngleMode::Degree);

  // Parametric mode (graphMode == 1): the 10 buffers are read as 5
  // X/Y pairs (X1T,Y1T,X2T,Y2T,...). Sweep the parameter t over a full
  // turn (X stands in for t, like polar's θ) and plot (X_nT(t),Y_nT(t)).
  // Each pair's points land at its even (X) slot so colour-by-index and
  // per-slot style still work; the odd (Y) slot stays empty.
  if (m_graphMode == 1) {
    const double tMax = degreeMode ? 360.0 : 2.0 * M_PI;
    const double tStep = tMax / resolution;
    for (int slot = 0; slot < static_cast<int>(m_functionBuffers.size()); ++slot) {
      QVariantList points;
      const int yslot = slot + 1;
      const bool isXslot = (slot % 2 == 0) &&
                           yslot < static_cast<int>(m_functionBuffers.size());
      const bool ok = isXslot &&
          !m_functionBuffers[slot].empty() && !m_functionBuffers[yslot].empty() &&
          m_functionEnabled[slot] && m_functionEnabled[yslot];
      if (ok) {
        for (int i = 0; i <= resolution; ++i) {
          const double t = i * tStep;
          CalculationResult rx = msm.evaluate(m_functionBuffers[slot], t);
          CalculationResult ry = msm.evaluate(m_functionBuffers[yslot], t);
          if (rx.success && !rx.isMatrix && !rx.isList &&
              ry.success && !ry.isMatrix && !ry.isList) {
            QVariantMap pt;
            pt["x"] = rx.value;
            pt["y"] = ry.value;
            points.append(pt);
          }
        }
      }
      allFunctions.append(QVariant::fromValue(points));
    }
    return allFunctions;
  }

  // Sequence mode (graphMode == 3): slots 0/1/2 are u/v/w. X stands in
  // for n and Ans for the previous term u(n-1). Explicit sequences (no
  // Ans in the buffer) are evaluated directly at each n; recursive ones
  // seed u(nMin) from the initial value and iterate. Points are (n, u).
  if (m_graphMode == 3) {
    const long long nMin = 1;
    long long nMax = static_cast<long long>(std::floor(m_seqNMax));
    if (nMax < nMin) nMax = nMin;
    if (nMax - nMin > 100000) nMax = nMin + 100000;  // runaway cap
    const double inits[3] = {m_seqInitU, m_seqInitV, m_seqInitW};

    const CalculationResult savedAns = MathStateMachine::lastResult;
    for (int slot = 0; slot < static_cast<int>(m_functionBuffers.size()); ++slot) {
      QVariantList points;
      if (slot < 3 && m_functionEnabled[slot] && !m_functionBuffers[slot].empty()) {
        const auto &buf = m_functionBuffers[slot];
        const bool recursive =
            std::find(buf.begin(), buf.end(), Token::Ans) != buf.end();
        double prev = inits[slot];
        for (long long n = nMin; n <= nMax; ++n) {
          double val;
          if (recursive && n == nMin) {
            val = inits[slot];  // seed term u(nMin)
          } else {
            // Feed the previous term in through Ans for recursion.
            MathStateMachine::lastResult = {true, prev, {}, false, ""};
            CalculationResult r = msm.evaluate(buf, static_cast<double>(n));
            if (!r.success || r.isMatrix || r.isList) break;
            val = r.value;
          }
          prev = val;
          QVariantMap pt;
          pt["x"] = static_cast<double>(n);
          pt["y"] = val;
          points.append(pt);
        }
      }
      allFunctions.append(QVariant::fromValue(points));
    }
    MathStateMachine::lastResult = savedAns;  // restore Ans
    return allFunctions;
  }

  // Polar mode (graphMode == 2) sweeps the angle parameter over a full
  // turn and interprets each buffer as r = f(θ), converting (r, θ) to
  // Cartesian for the shared {x, y} point contract the canvas renders.
  // The sweep variable X stands in for θ, so trig inside the expression
  // and the (r,θ)→(x,y) conversion both use the current angle unit and
  // stay consistent. Func mode (0) sweeps x across the viewport.
  const bool polar = (m_graphMode == 2);
  const bool degree = (MathStateMachine::angleMode == AngleMode::Degree);
  const double sweepMin = polar ? 0.0 : m_xMin;
  const double sweepMax =
      polar ? (degree ? 360.0 : 2.0 * M_PI) : m_xMax;
  const double step = (sweepMax - sweepMin) / resolution;

  for (size_t f = 0; f < m_functionBuffers.size(); ++f) {
    QVariantList points;
    // Skip disabled slots (Y-editor on/off) — an empty inner list keeps
    // the slot index → colour mapping stable (BUG-012).
    const bool on = (f < m_functionEnabled.size()) ? m_functionEnabled[f] : true;
    if (on && !m_functionBuffers[f].empty()) {
      for (int i = 0; i <= resolution; ++i) {
        double s = sweepMin + (i * step);
        CalculationResult res = msm.evaluate(m_functionBuffers[f], s);
        if (res.success && !res.isMatrix) {
          QVariantMap pt;
          if (polar) {
            const double thetaRad = degree ? (s * M_PI / 180.0) : s;
            const double r = res.value;
            pt["x"] = r * std::cos(thetaRad);
            pt["y"] = r * std::sin(thetaRad);
          } else {
            pt["x"] = s;
            pt["y"] = res.value;
          }
          points.append(pt);
        }
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

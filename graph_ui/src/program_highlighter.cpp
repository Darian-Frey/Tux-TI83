#include "program_highlighter.hpp"

#include <QQuickTextDocument>
#include <QTextDocument>

namespace tux_ti83 {

namespace {
// Palenight-ish colours — readable on the editor's dark (LCD) background.
const QColor kKeyword("#82aaff");   // blue
const QColor kVariable("#89ddff");  // cyan
const QColor kNumber("#f78c6c");    // orange
const QColor kString("#c3e88d");    // green
const QColor kComment("#676e95");   // muted gray-blue
}  // namespace

ProgramHighlighter::ProgramHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent) {
  // Statements + built-in functions. `\b` word boundaries keep them from
  // matching inside longer identifiers.
  QTextCharFormat kw;
  kw.setForeground(kKeyword);
  static const char *const kKeywords[] = {
      "If",     "Then",     "Else",    "For",    "While",   "Repeat",
      "End",    "break",    "continue","Lbl",    "Goto",    "Pause",
      "Stop",   "Return",   "DelVar",  "Menu",   "prgm",    "Disp",
      "Input",  "Prompt",   "Output",  "ClrHome","ClrDraw", "Line",
      "Circle", "Horizontal","Vertical","Text",  "DispGraph","FnOn",
      "FnOff",  "ZStandard","ZoomFit", "sin",    "cos",     "tan",
      "asin",   "acos",     "atan",    "ln",     "log",     "abs",
      "round",  "int",      "iPart",   "fPart",  "sgn",     "nCr",
      "nPr",    "min",      "max",     "mod",    "sub",     "length",
      "inString","expr",    "getKey",  "not",    "and",     "or",
      "xor",    "seq",      "dim",     "sum",    "augment"};
  QString alt;
  for (const char *k : kKeywords) {
    if (!alt.isEmpty())
      alt += '|';
    alt += QRegularExpression::escape(QString::fromUtf8(k));
  }
  m_rules.push_back({QRegularExpression("\\b(?:" + alt + ")\\b"), kw});

  // Variables: Str1–9, L1–6, Y0–9, window vars, Ans, and single letters A–Z.
  QTextCharFormat var;
  var.setForeground(kVariable);
  m_rules.push_back(
      {QRegularExpression(
           "\\b(?:Str[1-9]|L[1-6]|Y[0-9]|Xmin|Xmax|Ymin|Ymax|Xscl|Yscl|Ans|"
           "[A-Z])\\b"),
       var});

  // Number literals.
  QTextCharFormat num;
  num.setForeground(kNumber);
  m_rules.push_back(
      {QRegularExpression("\\b\\d+\\.?\\d*\\b|\\.\\d+"), num});

  m_stringFormat.setForeground(kString);
  m_commentFormat.setForeground(kComment);
  m_commentFormat.setFontItalic(true);
}

void ProgramHighlighter::setTextDocument(QQuickTextDocument *doc) {
  if (m_quickDoc == doc)
    return;
  m_quickDoc = doc;
  // Re-target the base highlighter onto the QTextDocument (re-highlights).
  setDocument(doc ? doc->textDocument() : nullptr);
  emit textDocumentChanged();
}

void ProgramHighlighter::highlightBlock(const QString &text) {
  // Keywords / variables / numbers first.
  for (const Rule &r : m_rules) {
    auto it = r.pattern.globalMatch(text);
    while (it.hasNext()) {
      const auto m = it.next();
      setFormat(m.capturedStart(), m.capturedLength(), r.format);
    }
  }

  // Find the first top-level `#` (outside a string): everything from there is
  // a comment; up to there, colour "…" string literals (they override the
  // keyword/number formats inside them).
  int commentAt = -1;
  bool inStr = false;
  int stringStart = -1;
  for (int i = 0; i < text.size(); ++i) {
    const QChar c = text[i];
    if (c == '"') {
      if (!inStr) {
        inStr = true;
        stringStart = i;
      } else {
        inStr = false;
        setFormat(stringStart, i - stringStart + 1, m_stringFormat);
      }
    } else if (c == '#' && !inStr) {
      commentAt = i;
      break;
    }
  }
  if (inStr && stringStart >= 0)  // unterminated string → colour to end
    setFormat(stringStart, text.size() - stringStart, m_stringFormat);
  if (commentAt >= 0)
    setFormat(commentAt, text.size() - commentAt, m_commentFormat);
}

}  // namespace tux_ti83

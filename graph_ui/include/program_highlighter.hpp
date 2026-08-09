// Syntax highlighter for the TI-BASIC program editor. A QSyntaxHighlighter
// attached to the editor's QTextDocument (via QQuickTextDocument), colouring
// keywords, variables, strings, numbers, and `#` comments. Registered as a
// QML type so the editor can bind `textDocument: bodyArea.textDocument`.

#pragma once

#include <QColor>
#include <QQuickTextDocument>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QVector>

namespace tux_ti83 {

class ProgramHighlighter : public QSyntaxHighlighter {
  Q_OBJECT
  // The editor's text document (TextArea.textDocument). Setting it attaches
  // the highlighter; QML: `ProgramHighlighter { textDocument: area.textDocument }`.
  Q_PROPERTY(QQuickTextDocument *textDocument READ textDocument WRITE
                 setTextDocument NOTIFY textDocumentChanged)

public:
  explicit ProgramHighlighter(QObject *parent = nullptr);

  QQuickTextDocument *textDocument() const { return m_quickDoc; }
  void setTextDocument(QQuickTextDocument *doc);

signals:
  void textDocumentChanged();

protected:
  void highlightBlock(const QString &text) override;

private:
  struct Rule {
    QRegularExpression pattern;
    QTextCharFormat format;
  };
  QVector<Rule> m_rules;          // keyword / variable / number rules
  QTextCharFormat m_stringFormat;  // "…" literals
  QTextCharFormat m_commentFormat; // # comments
  QQuickTextDocument *m_quickDoc = nullptr;
};

}  // namespace tux_ti83

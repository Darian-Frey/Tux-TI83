#pragma once
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <vector>
#include "capsules/capsule_math.hpp"

namespace tux_ti83 {

class UIController : public QObject {
    Q_OBJECT

public:
    enum DisplayState {
        Inputting = 0,
        Evaluated = 1,
        Error     = 2
    };
    Q_ENUM(DisplayState)

private:
    Q_PROPERTY(double xMin MEMBER m_xMin NOTIFY viewportChanged)
    Q_PROPERTY(double xMax MEMBER m_xMax NOTIFY viewportChanged)
    Q_PROPERTY(double yMin MEMBER m_yMin NOTIFY viewportChanged)
    Q_PROPERTY(double yMax MEMBER m_yMax NOTIFY viewportChanged)
    Q_PROPERTY(QString currentDisplay READ currentDisplay NOTIFY displayChanged)
    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)
    Q_PROPERTY(int activeFunctionIndex READ activeFunctionIndex NOTIFY activeFunctionIndexChanged)
    Q_PROPERTY(bool isGraphMode MEMBER m_isGraphMode NOTIFY graphModeChanged)
    Q_PROPERTY(DisplayState displayState READ displayState NOTIFY displayStateChanged)
    Q_PROPERTY(QString displayExpression READ displayExpression NOTIFY displayStateChanged)

public:
    explicit UIController(QObject* parent = nullptr);

    QString currentDisplay() const;
    QStringList history() const { return m_history; }
    int activeFunctionIndex() const { return m_activeIdx; }
    DisplayState displayState() const { return m_displayState; }
    QString displayExpression() const { return m_displayExpression; }

    Q_INVOKABLE void processInput(const QString& input);
    // Tokenise a free-form expression string ("2+sin(0.5)") into the
    // sequence of input strings the controller's processInput method
    // accepts. Longest-match against the kTokens table plus a small set
    // of control verbs (▶Frac, ▶Dec). Returns an empty list if any
    // character can't be tokenised. Whitespace is ignored.
    static QStringList tokenize(const QString& expr);
    // Tokenise `expr` and feed each token through processInput in order.
    // Returns true on full success, false if tokenisation failed.
    // Note: does not call ENTER — caller decides whether to evaluate.
    Q_INVOKABLE bool processExpression(const QString& expr);
    Q_INVOKABLE void setActiveFunction(int index) { m_activeIdx = index; emit activeFunctionIndexChanged(); }
    Q_INVOKABLE void toggleGraphMode() { m_isGraphMode = !m_isGraphMode; emit graphModeChanged(); }
    Q_INVOKABLE void resetViewport() { m_xMin = -10; m_xMax = 10; m_yMin = -10; m_yMax = 10; emit viewportChanged(); }
    Q_INVOKABLE void zoomFit();
    Q_INVOKABLE void updateMatrix(const QString& name, int rows, int cols, const QVariantList& values);
    Q_INVOKABLE QVariantList getMultiGraphPoints(int resolution);
    Q_INVOKABLE void pan(double dx, double dy, double vw, double vh);
    Q_INVOKABLE void zoom(double f, double mx, double my, double vw, double vh);

signals:
    void displayChanged();
    void historyChanged();
    void activeFunctionIndexChanged();
    void viewportChanged();
    void graphModeChanged();
    void displayStateChanged();

private:
    // processInput dispatches to these. Each handles one concern; the
    // dispatcher itself stays a thin switch over the input string.
    void clearAll();
    void backspace();
    void evaluate();
    void insertToken(const QString& input);
    // Post-hoc display conversions applied to the last result. No-op
    // unless we're in Evaluated state with a scalar result.
    void convertDisplayToFraction();
    void convertDisplayToDecimal();

    std::vector<std::vector<Token>> m_functionBuffers;
    std::vector<QString> m_displayStrings;
    QStringList m_history;
    int m_activeIdx;
    bool m_isGraphMode = false;
    double m_xMin = -10, m_xMax = 10, m_yMin = -10, m_yMax = 10;
    DisplayState m_displayState = Inputting;
    QString m_displayExpression;
};

} // namespace tux_ti83

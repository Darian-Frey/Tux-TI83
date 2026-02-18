#pragma once
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <vector>
#include "capsules/capsule_math.hpp"

namespace tux_ti83 {

class UIController : public QObject {
    Q_OBJECT
    Q_PROPERTY(double xMin MEMBER m_xMin NOTIFY viewportChanged)
    Q_PROPERTY(double xMax MEMBER m_xMax NOTIFY viewportChanged)
    Q_PROPERTY(double yMin MEMBER m_yMin NOTIFY viewportChanged)
    Q_PROPERTY(double yMax MEMBER m_yMax NOTIFY viewportChanged)
    Q_PROPERTY(QString currentDisplay READ currentDisplay NOTIFY displayChanged)
    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)
    Q_PROPERTY(int activeFunctionIndex READ activeFunctionIndex NOTIFY activeFunctionIndexChanged)
    Q_PROPERTY(bool isGraphMode MEMBER m_isGraphMode NOTIFY graphModeChanged)

public:
    explicit UIController(QObject* parent = nullptr);

    QString currentDisplay() const;
    QStringList history() const { return m_history; }
    int activeFunctionIndex() const { return m_activeIdx; }

    Q_INVOKABLE void processInput(const QString& input);
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

private:
    std::vector<std::vector<Token>> m_functionBuffers;
    std::vector<QString> m_displayStrings;
    QStringList m_history;
    int m_activeIdx;
    bool m_isGraphMode = false;
    double m_xMin = -10, m_xMax = 10, m_yMin = -10, m_yMax = 10;
};

} // namespace tux_ti83

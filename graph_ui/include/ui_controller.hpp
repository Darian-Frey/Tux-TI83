#pragma once
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <vector>
#include "capsules/capsule_math.hpp"

namespace tux_ti83 {
class UIController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentDisplay READ currentDisplay NOTIFY displayChanged)
    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)
    Q_PROPERTY(bool isGraphMode READ isGraphMode NOTIFY graphModeChanged)
    Q_PROPERTY(int activeFunctionIndex READ activeFunctionIndex NOTIFY activeFunctionChanged)
    Q_PROPERTY(double xMin READ xMin WRITE setXMin NOTIFY viewportChanged)
    Q_PROPERTY(double xMax READ xMax WRITE setXMax NOTIFY viewportChanged)
    Q_PROPERTY(double yMin READ yMin WRITE setYMin NOTIFY viewportChanged)
    Q_PROPERTY(double yMax READ yMax WRITE setYMax NOTIFY viewportChanged)

public:
    explicit UIController(QObject* parent = nullptr);
    Q_INVOKABLE void processInput(const QString& input); // Changed to QString
    Q_INVOKABLE void restoreFromHistory(int index);
    Q_INVOKABLE QVariantList getMultiGraphPoints(int resolution);
    Q_INVOKABLE void toggleGraphMode() { m_isGraphMode = !m_isGraphMode; emit graphModeChanged(); }
    Q_INVOKABLE void resetViewport() { m_xMin = -10; m_xMax = 10; m_yMin = -10; m_yMax = 10; emit viewportChanged(); }
    Q_INVOKABLE void setActiveFunction(int index) { m_activeIdx = index; emit activeFunctionChanged(); emit displayChanged(); }
    
    Q_INVOKABLE void pan(double dx, double dy, double vw, double vh);
    Q_INVOKABLE void zoom(double f, double mx, double my, double vw, double vh);

    QString currentDisplay() const;
    QStringList history() const { return m_history; }
    bool isGraphMode() const { return m_isGraphMode; }
    int activeFunctionIndex() const { return m_activeIdx; }
    double xMin() const { return m_xMin; }
    double xMax() const { return m_xMax; }
    double yMin() const { return m_yMin; }
    double yMax() const { return m_yMax; }
    void setXMin(double v) { m_xMin = v; emit viewportChanged(); }
    void setXMax(double v) { m_xMax = v; emit viewportChanged(); }
    void setYMin(double v) { m_yMin = v; emit viewportChanged(); }
    void setYMax(double v) { m_yMax = v; emit viewportChanged(); }

signals:
    void displayChanged();
    void historyChanged();
    void graphModeChanged();
    void viewportChanged();
    void activeFunctionChanged();

private:
    QStringList m_history;
    bool m_isGraphMode = false;
    int m_activeIdx = 0;
    double m_xMin = -10.0, m_xMax = 10.0, m_yMin = -10.0, m_yMax = 10.0;
    std::vector<std::vector<Token>> m_functionBuffers;
    std::vector<QString> m_displayStrings;
};
}

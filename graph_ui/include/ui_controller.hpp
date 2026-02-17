#pragma once
#include <QObject>
#include <QString>
#include <QVariant>
#include <vector>
#include "capsules/capsule_math.hpp"

namespace tux_ti83 {
class UIController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentDisplay READ currentDisplay NOTIFY displayChanged)
    Q_PROPERTY(QVariantList graphPoints READ graphPoints NOTIFY graphPointsChanged)
    Q_PROPERTY(double xMin READ xMin NOTIFY viewPortChanged)
    Q_PROPERTY(double xMax READ xMax NOTIFY viewPortChanged)
    Q_PROPERTY(double yMin READ yMin NOTIFY viewPortChanged)
    Q_PROPERTY(double yMax READ yMax NOTIFY viewPortChanged)

public:
    explicit UIController(QObject* parent = nullptr);
    Q_INVOKABLE void processInput(int tokenId);
    Q_INVOKABLE void handlePan(double dx, double dy);

    QString currentDisplay() const { return m_currentDisplay; }
    QVariantList graphPoints() const { return m_graphPoints; }
    double xMin() const { return m_xMin; }
    double xMax() const { return m_xMax; }
    double yMin() const { return m_yMin; }
    double yMax() const { return m_yMax; }

signals:
    void displayChanged();
    void graphPointsChanged();
    void viewPortChanged();

private:
    QString m_currentDisplay;
    QVariantList m_graphPoints;
    std::vector<Token> m_expressionBuffer;
    double m_xMin, m_xMax, m_yMin, m_yMax;
};
}

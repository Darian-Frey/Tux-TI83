#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <vector>
#include "capsules/capsule_math.hpp"

namespace tux_ti83 {
class UIController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentDisplay READ currentDisplay NOTIFY displayChanged)
    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)

public:
    explicit UIController(QObject* parent = nullptr);
    Q_INVOKABLE void processInput(int tokenId);
    Q_INVOKABLE void restoreFromHistory(int index); // New function

    QString currentDisplay() const { return m_currentDisplay; }
    QStringList history() const { return m_history; }

signals:
    void displayChanged();
    void historyChanged();

private:
    QString m_currentDisplay;
    QStringList m_history;
    std::vector<Token> m_expressionBuffer;
};
}

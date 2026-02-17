#include "ui_controller.hpp"
#include <map>

namespace tux_ti83 {

UIController::UIController(QObject* parent)
    : QObject(parent), m_currentDisplay("") {}

void UIController::restoreFromHistory(int index) {
    if (index < 0 || index >= m_history.size()) return;

    QString entry = m_history.at(index);
    QStringList parts = entry.split(" = ");
    
    if (parts.size() > 1) {
        m_currentDisplay = parts.last();
        m_expressionBuffer.clear(); // Clear buffer to prevent mixed logic
        // If it's a number, we should ideally re-tokenize it here for chain calcs
        emit displayChanged();
    }
}

void UIController::processInput(int tokenId) {
    if (tokenId == 16) { // "C"
        m_currentDisplay = "";
        m_expressionBuffer.clear();
        emit displayChanged();
        return;
    }

    if (tokenId == 19) { // "ENTER"
        if (m_expressionBuffer.empty()) return;

        MathStateMachine msm;
        CalculationResult result = msm.evaluate(m_expressionBuffer);
        
        QString entry = m_currentDisplay + " = ";
        if (result.success) {
            QString resVal = QString::number(result.value);
            entry += resVal;
            m_currentDisplay = resVal;
        } else {
            entry += "ERR";
            m_currentDisplay = "SYNTAX ERROR";
        }
        
        m_history.prepend(entry);
        if (m_history.size() > 20) m_history.removeLast();
        
        m_expressionBuffer.clear();
        emit historyChanged();
        emit displayChanged();
        return;
    }

    struct Key { Token t; QString s; };
    static const std::map<int, Key> keyMap = {
        {0, {Token::Num7, "7"}}, {1, {Token::Num8, "8"}}, {2, {Token::Num9, "9"}}, {3, {Token::Div, "/"}},
        {4, {Token::Num4, "4"}}, {5, {Token::Num5, "5"}}, {6, {Token::Num6, "6"}}, {7, {Token::Mul, "*"}},
        {8, {Token::Num1, "1"}}, {9, {Token::Num2, "2"}}, {10, {Token::Num3, "3"}}, {11, {Token::Sub, "-"}},
        {12, {Token::Num0, "0"}}, {13, {Token::Decimal, "."}}, {14, {Token::Pi, "3.14159"}}, {15, {Token::Add, "+"}},
        {17, {Token::LeftParen, "("}}, {18, {Token::RightParen, ")"}}
    };

    if (keyMap.count(tokenId)) {
        auto key = keyMap.at(tokenId);
        m_expressionBuffer.push_back(key.t);
        m_currentDisplay += key.s;
        emit displayChanged();
    }
}

} // namespace tux_ti83

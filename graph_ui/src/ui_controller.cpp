#include "ui_controller.hpp"
#include <map>

namespace tux_ti83 {

UIController::UIController(QObject* parent)
    : QObject(parent), m_currentDisplay(""), m_xMin(-10), m_xMax(10), m_yMin(-10), m_yMax(10) {}

void UIController::processInput(int tokenId) {
    if (tokenId == 16) { // "C"
        m_currentDisplay = "";
        m_expressionBuffer.clear();
        emit displayChanged();
        return;
    }

    if (tokenId == 19) { // "ENTER"
        MathStateMachine msm;
        CalculationResult result = msm.evaluate(m_expressionBuffer);
        
        if (result.success) {
            m_currentDisplay = QString::number(result.value);
            // Optional: push result back to buffer for chain calcs
            m_expressionBuffer.clear(); 
        } else {
            m_currentDisplay = "SYNTAX ERROR";
            m_expressionBuffer.clear();
        }
        emit displayChanged();
        return;
    }

    // Map Grid Index to Token Enum and Display String
    struct Key { Token t; QString s; };
    static const std::map<int, Key> keyMap = {
        {0, {Token::Num7, "7"}}, {1, {Token::Num8, "8"}}, {2, {Token::Num9, "9"}}, {3, {Token::Div, "/"}},
        {4, {Token::Num4, "4"}}, {5, {Token::Num5, "5"}}, {6, {Token::Num6, "6"}}, {7, {Token::Mul, "*"}},
        {8, {Token::Num1, "1"}}, {9, {Token::Num2, "2"}}, {10, {Token::Num3, "3"}}, {11, {Token::Sub, "-"}},
        {12, {Token::Num0, "0"}}, {13, {Token::Decimal, "."}}, {14, {Token::Pi, "π"}}, {15, {Token::Add, "+"}},
        {17, {Token::LeftParen, "("}}, {18, {Token::RightParen, ")"}}
    };

    if (keyMap.count(tokenId)) {
        auto key = keyMap.at(tokenId);
        m_expressionBuffer.push_back(key.t);
        m_currentDisplay += key.s;
        emit displayChanged();
    }
}

void UIController::handlePan(double dx, double dy) {
    double scaleX = (m_xMax - m_xMin) / 900.0;
    double scaleY = (m_yMax - m_yMin) / 600.0;
    m_xMin -= dx * scaleX; m_xMax -= dx * scaleX;
    m_yMin += dy * scaleY; m_yMax += dy * scaleY;
    emit viewPortChanged();
}

} // namespace tux_ti83

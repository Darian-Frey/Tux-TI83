#include "ui_controller.hpp"
#include <map>
#include <cmath>
#include <algorithm>

namespace tux_ti83 {

UIController::UIController(QObject* parent) : QObject(parent) {}

QVariantList UIController::getGraphPoints(int resolution) {
    QVariantList points;
    if (m_expressionBuffer.empty()) return points;

    double step = (m_xMax - m_xMin) / resolution;
    MathStateMachine msm;

    for (int i = 0; i <= resolution; ++i) {
        double xVal = m_xMin + (i * step);
        
        // TOKEN SWAP: Create a temporary copy of the buffer
        std::vector<Token> evalBuffer = m_expressionBuffer;
        
        // We'll use a special evaluation mode or just manually solve for X.
        // For this architecture, we pass the xVal to the evaluator.
        CalculationResult result = msm.evaluate(evalBuffer, xVal);
        
        if (result.success) {
            QVariantMap pt;
            pt["x"] = xVal;
            pt["y"] = result.value;
            points.append(pt);
        }
    }
    return points;
}

void UIController::processInput(int tokenId) {
    if (tokenId == 16) { m_currentDisplay = ""; m_expressionBuffer.clear(); emit displayChanged(); return; }
    if (tokenId == 19) {
        MathStateMachine msm;
        CalculationResult result = msm.evaluate(m_expressionBuffer);
        QString entry = m_currentDisplay + " = ";
        if (result.success) {
            QString resVal = QString::number(result.value);
            entry += resVal; m_currentDisplay = resVal;
        } else {
            entry += "ERR"; m_currentDisplay = "SYNTAX ERROR";
        }
        m_history.prepend(entry);
        m_expressionBuffer.clear();
        emit historyChanged(); emit displayChanged();
        return;
    }

    // Updated Map including X (index 20)
    struct Key { Token t; QString s; };
    static const std::map<int, Key> keyMap = {
        {0, {Token::Num7, "7"}}, {1, {Token::Num8, "8"}}, {2, {Token::Num9, "9"}}, {3, {Token::Div, "/"}},
        {4, {Token::Num4, "4"}}, {5, {Token::Num5, "5"}}, {6, {Token::Num6, "6"}}, {7, {Token::Mul, "*"}},
        {8, {Token::Num1, "1"}}, {9, {Token::Num2, "2"}}, {10, {Token::Num3, "3"}}, {11, {Token::Sub, "-"}},
        {12, {Token::Num0, "0"}}, {13, {Token::Decimal, "."}}, {14, {Token::Pi, "π"}}, {15, {Token::Add, "+"}},
        {17, {Token::LeftParen, "("}}, {18, {Token::RightParen, ")"}}, {20, {Token::VarX, "X"}}
    };

    if (keyMap.count(tokenId)) {
        auto key = keyMap.at(tokenId);
        m_expressionBuffer.push_back(key.t);
        m_currentDisplay += key.s;
        emit displayChanged();
    }
}

void UIController::restoreFromHistory(int index) {
    if (index < 0 || index >= m_history.size()) return;
    QStringList parts = m_history.at(index).split(" = ");
    if (parts.size() > 1) { m_currentDisplay = parts.last(); m_expressionBuffer.clear(); emit displayChanged(); }
}

void UIController::pan(double dx, double dy, double viewWidth, double viewHeight) {
    double rangeX = m_xMax - m_xMin; double rangeY = m_yMax - m_yMin;
    double unitsPerPixelX = rangeX / viewWidth; double unitsPerPixelY = rangeY / viewHeight;
    m_xMin -= dx * unitsPerPixelX; m_xMax -= dx * unitsPerPixelX;
    m_yMin += dy * unitsPerPixelY; m_yMax += dy * unitsPerPixelY;
    emit viewportChanged();
}

void UIController::zoom(double factor, double mouseX, double mouseY, double viewWidth, double viewHeight) {
    double mouseWorldX = m_xMin + (mouseX / viewWidth) * (m_xMax - m_xMin);
    double mouseWorldY = m_yMax - (mouseY / viewHeight) * (m_yMax - m_yMin);
    m_xMin = mouseWorldX + (m_xMin - mouseWorldX) * factor;
    m_xMax = mouseWorldX + (m_xMax - mouseWorldX) * factor;
    m_yMin = mouseWorldY + (m_yMin - mouseWorldY) * factor;
    m_yMax = mouseWorldY + (m_yMax - mouseWorldY) * factor;
    emit viewportChanged();
}

} // namespace tux_ti83

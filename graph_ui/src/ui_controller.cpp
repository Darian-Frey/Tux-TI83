#include "ui_controller.hpp"
#include <map>
#include <cmath>

namespace tux_ti83 {

UIController::UIController(QObject* parent) : QObject(parent), m_activeIdx(0) {
    // Initialize 3 functions: Y1, Y2, Y3
    m_functionBuffers.resize(3);
    m_displayStrings.resize(3, "");
}

QString UIController::currentDisplay() const {
    return m_displayStrings[m_activeIdx];
}

QVariantList UIController::getMultiGraphPoints(int resolution) {
    QVariantList allFunctions;
    double step = (m_xMax - m_xMin) / resolution;
    MathStateMachine msm;

    for (size_t f = 0; f < m_functionBuffers.size(); ++f) {
        if (m_functionBuffers[f].empty()) continue;
        
        QVariantList points;
        for (int i = 0; i <= resolution; ++i) {
            double x = m_xMin + (i * step);
            CalculationResult res = msm.evaluate(m_functionBuffers[f], x);
            if (res.success) {
                QVariantMap pt; pt["x"] = x; pt["y"] = res.value;
                points.append(pt);
            }
        }
        allFunctions.append(QVariant::fromValue(points));
    }
    return allFunctions;
}

void UIController::processInput(int tokenId) {
    auto& currentBuf = m_functionBuffers[m_activeIdx];
    auto& currentStr = m_displayStrings[m_activeIdx];

    if (tokenId == 16) { currentStr = ""; currentBuf.clear(); emit displayChanged(); return; }
    if (tokenId == 19) {
        MathStateMachine msm;
        CalculationResult result = msm.evaluate(currentBuf);
        QString entry = "Y" + QString::number(m_activeIdx + 1) + ": " + currentStr + " = ";
        if (result.success) {
            currentStr = QString::number(result.value);
            entry += currentStr;
        } else {
            entry += "ERR";
        }
        m_history.prepend(entry);
        emit historyChanged(); emit displayChanged();
        return;
    }

    struct Key { Token t; QString s; };
    static const std::map<int, Key> keyMap = {
        {0, {Token::Num7, "7"}}, {1, {Token::Num8, "8"}}, {2, {Token::Num9, "9"}}, {3, {Token::Div, "/"}},
        {4, {Token::Num4, "4"}}, {5, {Token::Num5, "5"}}, {6, {Token::Num6, "6"}}, {7, {Token::Mul, "*"}},
        {8, {Token::Num1, "1"}}, {9, {Token::Num2, "2"}}, {10, {Token::Num3, "3"}}, {11, {Token::Sub, "-"}},
        {12, {Token::Num0, "0"}}, {13, {Token::Decimal, "."}}, {14, {Token::Pi, "π"}}, {15, {Token::Add, "+"}},
        {17, {Token::LeftParen, "("}}, {18, {Token::RightParen, ")"}}, {20, {Token::VarX, "X"}}
    };

    if (keyMap.count(tokenId)) {
        currentBuf.push_back(keyMap.at(tokenId).t);
        currentStr += keyMap.at(tokenId).s;
        emit displayChanged();
    }
}

void UIController::pan(double dx, double dy, double viewWidth, double viewHeight) {
    double rx = m_xMax - m_xMin; double ry = m_yMax - m_yMin;
    m_xMin -= dx * (rx/viewWidth); m_xMax -= dx * (rx/viewWidth);
    m_yMin += dy * (ry/viewHeight); m_yMax += dy * (ry/viewHeight);
    emit viewportChanged();
}

void UIController::zoom(double f, double mx, double my, double vw, double vh) {
    double wx = m_xMin + (mx/vw)* (m_xMax-m_xMin);
    double wy = m_yMax - (my/vh)* (m_yMax-m_yMin);
    m_xMin = wx + (m_xMin-wx)*f; m_xMax = wx + (m_xMax-wx)*f;
    m_yMin = wy + (m_yMin-wy)*f; m_yMax = wy + (m_yMax-wy)*f;
    emit viewportChanged();
}

void UIController::restoreFromHistory(int index) {}

} // namespace tux_ti83

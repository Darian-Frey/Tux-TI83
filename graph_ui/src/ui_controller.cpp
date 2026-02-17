#include "ui_controller.hpp"
#include <map>
#include <cmath>
#include <algorithm>

namespace tux_ti83 {

UIController::UIController(QObject* parent) : QObject(parent), m_activeIdx(0) {
    m_functionBuffers.resize(3);
    m_displayStrings.resize(3, "");
}

QString UIController::currentDisplay() const { return m_displayStrings[m_activeIdx]; }

void UIController::processInput(const QString& input) {
    auto& currentBuf = m_functionBuffers[m_activeIdx];
    auto& currentStr = m_displayStrings[m_activeIdx];

    if (input == "C") { 
        currentStr = ""; 
        currentBuf.clear(); 
        emit displayChanged(); 
        return; 
    }

    if (input == "DEL") {
        if (!currentBuf.empty()) {
            currentBuf.pop_back();
            currentStr = "";
            static std::map<Token, QString> revMap = {
                {Token::Add, "+"}, {Token::Sub, "−"}, {Token::Mul, "×"}, {Token::Div, "÷"},
                {Token::Sin, "sin("}, {Token::Cos, "cos("}, {Token::Tan, "tan("},
                {Token::ASin, "asin("}, {Token::ACos, "acos("}, {Token::ATan, "atan("},
                {Token::Log, "log("}, {Token::Ln, "ln("}, {Token::Sqrt, "√("},
                {Token::Pow, "^"}, {Token::Pi, "π"}, {Token::VarX, "X"},
                {Token::LeftParen, "("}, {Token::RightParen, ")"}, {Token::Decimal, "."}
            };
            for (auto t : currentBuf) {
                int val = static_cast<int>(t);
                if (val >= 0 && val <= 9) currentStr += QString::number(val);
                else if (revMap.count(t)) currentStr += revMap[t];
            }
        }
        emit displayChanged();
        return;
    }
    
    if (input == "ENTER" || input == "▶Frac") {
        MathStateMachine msm;
        CalculationResult result = msm.evaluate(currentBuf);
        QString entry = "Y" + QString::number(m_activeIdx + 1) + ": " + currentStr + " = ";
        if (result.success) {
            std::string fracStr = MathStateMachine::toFraction(result.value);
            QString frac = QString::fromStdString(fracStr);
            currentStr = (frac.isEmpty()) ? QString::number(result.value) : frac;
        } else {
            currentStr = "ERR";
        }
        entry += currentStr;
        m_history.prepend(entry);
        emit historyChanged(); 
        emit displayChanged();
        return;
    }

    static const std::map<QString, Token> tokenMap = {
        {"0", Token::Num0}, {"1", Token::Num1}, {"2", Token::Num2}, {"3", Token::Num3},
        {"4", Token::Num4}, {"5", Token::Num5}, {"6", Token::Num6}, {"7", Token::Num7},
        {"8", Token::Num8}, {"9", Token::Num9}, {"+", Token::Add}, {"−", Token::Sub},
        {"×", Token::Mul}, {"÷", Token::Div}, {"(", Token::LeftParen}, {")", Token::RightParen},
        {".", Token::Decimal}, {"π", Token::Pi}, {"X", Token::VarX}, {"^", Token::Pow},
        {"sin", Token::Sin}, {"cos", Token::Cos}, {"tan", Token::Tan}, {"√", Token::Sqrt},
        {"log", Token::Log}, {"ln", Token::Ln}, {"asin", Token::ASin}, {"acos", Token::ACos}, 
        {"atan", Token::ATan}, {"=", Token::Equal}, {"≠", Token::NotEqual}, {"<", Token::Less}, 
        {">", Token::Greater}, {"and", Token::And}, {"or", Token::Or}, {"not", Token::Not}
    };

    if (tokenMap.count(input)) {
        currentBuf.push_back(tokenMap.at(input));
        bool isFunc = (input == "sin" || input == "cos" || input == "tan" || 
                       input == "√" || input == "log" || input == "ln" ||
                       input == "asin" || input == "acos" || input == "atan" || input == "not");
        if (isFunc) currentStr += input + "(";
        else if (tokenMap.at(input) >= Token::And && tokenMap.at(input) <= Token::Xor) currentStr += " " + input + " ";
        else currentStr += input;
        emit displayChanged();
    }
}

void UIController::zoomFit() {
    double minVal = 1e308; double maxVal = -1e308;
    bool found = false;
    MathStateMachine msm;
    for (const auto& buffer : m_functionBuffers) {
        if (buffer.empty()) continue;
        for (int i = 0; i <= 100; ++i) {
            double x = m_xMin + (i * (m_xMax - m_xMin) / 100.0);
            CalculationResult res = msm.evaluate(buffer, x);
            if (res.success && std::isfinite(res.value)) {
                minVal = std::min(minVal, res.value);
                maxVal = std::max(maxVal, res.value);
                found = true;
            }
        }
    }
    if (found) {
        double margin = (maxVal - minVal) * 0.1;
        if (std::abs(maxVal - minVal) < 1e-9) margin = 1.0;
        m_yMin = minVal - margin; m_yMax = maxVal + margin;
        emit viewportChanged();
    }
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

void UIController::pan(double dx, double dy, double vw, double vh) {
    double rx = m_xMax - m_xMin; double ry = m_yMax - m_yMin;
    m_xMin -= dx * (rx/vw); m_xMax -= dx * (rx/vw);
    m_yMin += dy * (ry/vh); m_yMax += dy * (ry/vh);
    emit viewportChanged();
}

void UIController::zoom(double f, double mx, double my, double vw, double vh) {
    double wx = m_xMin + (mx/vw)* (m_xMax-m_xMin);
    double wy = m_yMax - (my/vh)* (m_yMax-m_yMin);
    m_xMin = wx + (m_xMin-wx)*f; m_xMax = wx + (m_xMax-wx)*f;
    m_yMin = wy + (m_yMin-wy)*f; m_yMax = wy + (m_yMax-wy)*f;
    emit viewportChanged();
}

} // namespace tux_ti83

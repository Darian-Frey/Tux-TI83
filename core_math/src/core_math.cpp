#include "capsules/capsule_math.hpp"
#include <stack>
#include <cmath>
#include <algorithm>
#include <string>

namespace tux_ti83 {

std::map<Token, Matrix> MathStateMachine::matrixRegistry;

int EOSPrecedence::precedence(Token t) {
    switch (t) {
        case Token::Sin: case Token::Cos: case Token::Tan:
        case Token::ASin: case Token::ACos: case Token::ATan:
        case Token::Log: case Token::Ln: case Token::Sqrt: 
        case Token::Not: return 4;
        case Token::Pow: return 3;
        case Token::Mul: case Token::Div: return 2;
        case Token::Add: case Token::Sub: return 1;
        case Token::Equal: case Token::NotEqual: 
        case Token::Less: case Token::LessEq:
        case Token::Greater: case Token::GreaterEq: return -1;
        case Token::And: return -2;
        case Token::Or: case Token::Xor: return -3;
        default: return 0;
    }
}

bool EOSPrecedence::is_left_associative(Token t) { return (t != Token::Pow); }
bool EOSPrecedence::is_operator(Token t) { return precedence(t) != 0 && !is_function(t); }
bool EOSPrecedence::is_function(Token t) { 
    return (t == Token::Sin || t == Token::Cos || t == Token::Tan || 
            t == Token::ASin || t == Token::ACos || t == Token::ATan ||
            t == Token::Log || t == Token::Ln || t == Token::Sqrt || t == Token::Not); 
}

Matrix matrixAdd(const Matrix& a, const Matrix& b) {
    if (a.rows != b.rows || a.cols != b.cols) return {};
    Matrix res = a;
    for (size_t i = 0; i < a.data.size(); ++i) res.data[i] += b.data[i];
    return res;
}

Matrix matrixMul(const Matrix& a, const Matrix& b) {
    if (a.cols != b.rows) return {};
    Matrix res; res.rows = a.rows; res.cols = b.cols;
    res.data.resize(res.rows * res.cols, 0.0);
    for (int i = 0; i < a.rows; ++i)
        for (int j = 0; j < b.cols; ++j)
            for (int k = 0; k < a.cols; ++k)
                res.data[i * b.cols + j] += a.at(i, k) * b.at(k, j);
    return res;
}

CalculationResult MathStateMachine::evaluate(const std::vector<Token>& tokens, double xValue) {
    if (tokens.empty()) return {false, 0.0, {}, false, "Empty"};

    std::vector<double> numericValues;
    std::vector<Token> processedTokens; 
    std::string currentNumStr = "";
    auto flushNum = [&]() {
        if (!currentNumStr.empty()) {
            processedTokens.push_back(Token::Num0);
            numericValues.push_back(std::stod(currentNumStr));
            currentNumStr = "";
        }
    };

    for (auto t : tokens) {
        int val = (int)t;
        if (val >= 0 && val <= 9) currentNumStr += std::to_string(val);
        else if (t == Token::Decimal) currentNumStr += ".";
        else { flushNum(); processedTokens.push_back(t); }
    }
    flushNum();

    std::vector<std::pair<Token, double>> rpn;
    std::stack<Token> opStack;
    int numIdx = 0;
    for (auto t : processedTokens) {
        if (t == Token::Num0) rpn.push_back({t, numericValues[numIdx++]});
        else if ((t >= Token::MatA && t <= Token::MatJ) || t == Token::VarX || t == Token::Pi || t == Token::E) rpn.push_back({t, 0.0});
        else if (EOSPrecedence::is_function(t) || t == Token::LeftParen) opStack.push(t);
        else if (t == Token::RightParen) {
            while (!opStack.empty() && opStack.top() != Token::LeftParen) { rpn.push_back({opStack.top(), 0.0}); opStack.pop(); }
            if (!opStack.empty()) opStack.pop();
            if (!opStack.empty() && EOSPrecedence::is_function(opStack.top())) { rpn.push_back({opStack.top(), 0.0}); opStack.pop(); }
        } else {
            while (!opStack.empty() && opStack.top() != Token::LeftParen && 
                   EOSPrecedence::precedence(opStack.top()) >= EOSPrecedence::precedence(t)) {
                rpn.push_back({opStack.top(), 0.0}); opStack.pop();
            }
            opStack.push(t);
        }
    }
    while (!opStack.empty()) { rpn.push_back({opStack.top(), 0.0}); opStack.pop(); }

    struct Operand { bool isMat; double val; Matrix mat; };
    std::stack<Operand> stack;
    auto toB = [](double v) { return std::abs(v) > 1e-9; };

    for (auto& node : rpn) {
        Token t = node.first;
        if (t == Token::Num0) stack.push({false, node.second, {}});
        else if (t == Token::VarX) stack.push({false, xValue, {}});
        else if (t == Token::Pi) stack.push({false, M_PI, {}});
        else if (t == Token::E) stack.push({false, M_E, {}});
        else if (t >= Token::MatA && t <= Token::MatJ) {
            if (matrixRegistry.count(t)) stack.push({true, 0.0, matrixRegistry[t]});
            else return {false, 0.0, {}, false, "Undefined Matrix"};
        } else if (EOSPrecedence::is_function(t)) {
            if (stack.empty()) return {false, 0.0, {}, false, "Error"};
            Operand a = stack.top(); stack.pop();
            if (a.isMat) return {false, 0.0, {}, false, "Type Error"};
            double v = a.val;
            if (t == Token::Sin) v = std::sin(v);
            else if (t == Token::Cos) v = std::cos(v);
            else if (t == Token::Tan) v = std::tan(v);
            else if (t == Token::Sqrt) v = (v >= 0) ? std::sqrt(v) : 0.0;
            else if (t == Token::Log) v = (v > 0) ? std::log10(v) : -HUGE_VAL;
            else if (t == Token::Ln) v = (v > 0) ? std::log(v) : -HUGE_VAL;
            else if (t == Token::Not) v = toB(v) ? 0.0 : 1.0;
            stack.push({false, v, {}});
        } else {
            if (stack.size() < 2) return {false, 0.0, {}, false, "Error"};
            Operand b = stack.top(); stack.pop();
            Operand a = stack.top(); stack.pop();

            if (t == Token::Add) {
                if (a.isMat && b.isMat) stack.push({true, 0.0, matrixAdd(a.mat, b.mat)});
                else if (!a.isMat && !b.isMat) stack.push({false, a.val + b.val, {}});
                else return {false, 0.0, {}, false, "Type Error"};
            } else if (t == Token::Sub) {
                if (!a.isMat && !b.isMat) stack.push({false, a.val - b.val, {}});
                else return {false, 0.0, {}, false, "Type Error"};
            } else if (t == Token::Mul) {
                if (a.isMat && b.isMat) stack.push({true, 0.0, matrixMul(a.mat, b.mat)});
                else if (a.isMat && !b.isMat) { Matrix m = a.mat; for(auto& v : m.data) v *= b.val; stack.push({true, 0.0, m}); }
                else if (!a.isMat && b.isMat) { Matrix m = b.mat; for(auto& v : m.data) v *= a.val; stack.push({true, 0.0, m}); }
                else stack.push({false, a.val * b.val, {}});
            } else if (t == Token::Div) {
                if (!a.isMat && !b.isMat) stack.push({false, b.val == 0 ? 0.0 : a.val / b.val, {}});
                else return {false, 0.0, {}, false, "Type Error"};
            } else if (t == Token::Pow) {
                if (!a.isMat && !b.isMat) stack.push({false, std::pow(a.val, b.val), {}});
                else return {false, 0.0, {}, false, "Type Error"};
            } else if (t == Token::Equal) stack.push({false, std::abs(a.val - b.val) < 1e-9 ? 1.0 : 0.0, {}});
            else if (t == Token::NotEqual) stack.push({false, std::abs(a.val - b.val) > 1e-9 ? 1.0 : 0.0, {}});
            else if (t == Token::Less) stack.push({false, a.val < b.val ? 1.0 : 0.0, {}});
            else if (t == Token::LessEq) stack.push({false, a.val <= b.val ? 1.0 : 0.0, {}});
            else if (t == Token::Greater) stack.push({false, a.val > b.val ? 1.0 : 0.0, {}});
            else if (t == Token::GreaterEq) stack.push({false, a.val >= b.val ? 1.0 : 0.0, {}});
            else if (t == Token::And) stack.push({false, (toB(a.val) && toB(b.val)) ? 1.0 : 0.0, {}});
            else if (t == Token::Or) stack.push({false, (toB(a.val) || toB(b.val)) ? 1.0 : 0.0, {}});
            else if (t == Token::Xor) stack.push({false, (toB(a.val) ^ toB(b.val)) ? 1.0 : 0.0, {}});
        }
    }

    if (stack.empty()) return {false, 0.0, {}, false, "Error"};
    Operand res = stack.top();
    return {true, res.val, res.mat, res.isMat, ""};
}

std::string MathStateMachine::toFraction(double value, double tolerance) {
    if (std::isinf(value) || std::isnan(value)) return "";
    double x = value; long long n1 = 1, d1 = 0, n2 = 0, d2 = 1; double b = x;
    for (int i = 0; i < 10; ++i) {
        long long a = std::floor(b);
        long long aux_n = n1; n1 = a * n1 + n2; n2 = aux_n;
        long long aux_d = d1; d1 = a * d1 + d2; d2 = aux_d;
        if (std::abs(x - (double)n1 / d1) < tolerance) break;
        if (std::abs(b - a) < 1.0e-12) break;
        b = 1.0 / (b - a);
    }
    if (d1 == 1) return std::to_string(n1);
    return std::to_string(n1) + "/" + std::to_string(d1);
}

} // namespace tux_ti83

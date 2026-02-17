#include "capsules/capsule_math.hpp"
#include <stack>
#include <cmath>
#include <algorithm>
#include <string>

namespace tux_ti83 {

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

bool EOSPrecedence::is_operator(Token t) { 
    int p = precedence(t);
    return p != 0 && !is_function(t);
}

bool EOSPrecedence::is_function(Token t) { 
    return (t == Token::Sin || t == Token::Cos || t == Token::Tan || 
            t == Token::ASin || t == Token::ACos || t == Token::ATan ||
            t == Token::Log || t == Token::Ln || t == Token::Sqrt || t == Token::Not); 
}

CalculationResult MathStateMachine::evaluate(const std::vector<Token>& tokens, double xValue) {
    if (tokens.empty()) return {false, 0, "Empty"};

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
        int val = static_cast<int>(t);
        if (val >= 0 && val <= 9) currentNumStr += std::to_string(val);
        else if (t == Token::Decimal) currentNumStr += ".";
        else { flushNum(); processedTokens.push_back(t); }
    }
    flushNum();

    int balance = 0;
    for (auto t : processedTokens) {
        if (t == Token::LeftParen) balance++;
        if (t == Token::RightParen) balance--;
    }
    while (balance > 0) { processedTokens.push_back(Token::RightParen); balance--; }

    std::vector<std::pair<Token, double>> rpn; 
    std::stack<Token> opStack;
    int numIdx = 0;

    for (auto t : processedTokens) {
        if (t == Token::Num0) rpn.push_back({t, numericValues[numIdx++]});
        else if (t == Token::VarX || t == Token::Pi || t == Token::E) rpn.push_back({t, 0});
        else if (EOSPrecedence::is_function(t) || t == Token::LeftParen) opStack.push(t);
        else if (t == Token::RightParen) {
            while (!opStack.empty() && opStack.top() != Token::LeftParen) {
                rpn.push_back({opStack.top(), 0}); opStack.pop();
            }
            if (!opStack.empty()) opStack.pop();
            if (!opStack.empty() && EOSPrecedence::is_function(opStack.top())) {
                rpn.push_back({opStack.top(), 0}); opStack.pop();
            }
        } else {
            while (!opStack.empty() && opStack.top() != Token::LeftParen &&
                  (std::abs(EOSPrecedence::precedence(opStack.top())) >= std::abs(EOSPrecedence::precedence(t)) && 
                   EOSPrecedence::is_left_associative(t))) {
                rpn.push_back({opStack.top(), 0}); opStack.pop();
            }
            opStack.push(t);
        }
    }
    while (!opStack.empty()) { rpn.push_back({opStack.top(), 0}); opStack.pop(); }

    std::stack<double> valStack;
    auto toBool = [](double v) { return std::abs(v) > 1e-9; };

    for (auto& node : rpn) {
        Token t = node.first;
        if (t == Token::Num0) valStack.push(node.second);
        else if (t == Token::VarX) valStack.push(xValue);
        else if (t == Token::Pi) valStack.push(M_PI);
        else if (t == Token::E) valStack.push(M_E);
        else if (EOSPrecedence::is_function(t)) {
            if (valStack.empty()) return {false, 0, "Error"};
            double a = valStack.top(); valStack.pop();
            if (t == Token::Sin) valStack.push(std::sin(a));
            else if (t == Token::Cos) valStack.push(std::cos(a));
            else if (t == Token::Tan) valStack.push(std::tan(a));
            else if (t == Token::ASin) valStack.push(std::asin(a));
            else if (t == Token::ACos) valStack.push(std::acos(a));
            else if (t == Token::ATan) valStack.push(std::atan(a));
            else if (t == Token::Log) valStack.push(a > 0 ? std::log10(a) : -HUGE_VAL);
            else if (t == Token::Ln) valStack.push(a > 0 ? std::log(a) : -HUGE_VAL);
            else if (t == Token::Sqrt) valStack.push(std::sqrt(a));
            else if (t == Token::Not) valStack.push(toBool(a) ? 0 : 1);
        } else {
            if (valStack.size() < 2) return {false, 0, "Error"};
            double b = valStack.top(); valStack.pop();
            double a = valStack.top(); valStack.pop();
            if (t == Token::Add) valStack.push(a + b);
            else if (t == Token::Sub) valStack.push(a - b);
            else if (t == Token::Mul) valStack.push(a * b);
            else if (t == Token::Div) valStack.push(b == 0 ? 0 : a / b);
            else if (t == Token::Pow) valStack.push(std::pow(a, b));
            else if (t == Token::Equal) valStack.push(std::abs(a - b) < 1e-9 ? 1 : 0);
            else if (t == Token::NotEqual) valStack.push(std::abs(a - b) >= 1e-9 ? 1 : 0);
            else if (t == Token::Less) valStack.push(a < b ? 1 : 0);
            else if (t == Token::LessEq) valStack.push(a <= b ? 1 : 0);
            else if (t == Token::Greater) valStack.push(a > b ? 1 : 0);
            else if (t == Token::GreaterEq) valStack.push(a >= b ? 1 : 0);
            else if (t == Token::And) valStack.push((toBool(a) && toBool(b)) ? 1 : 0);
            else if (t == Token::Or) valStack.push((toBool(a) || toBool(b)) ? 1 : 0);
            else if (t == Token::Xor) valStack.push((toBool(a) ^ toBool(b)) ? 1 : 0);
        }
    }
    return {true, valStack.empty() ? 0 : valStack.top(), ""};
}

std::string MathStateMachine::toFraction(double value, double tolerance) {
    if (std::isinf(value) || std::isnan(value)) return "";
    double x = value;
    long long n1 = 1, d1 = 0;
    long long n2 = 0, d2 = 1;
    double b = x;
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

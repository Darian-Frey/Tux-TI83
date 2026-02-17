#include "capsules/capsule_math.hpp"
#include <stack>
#include <cmath>
#include <algorithm>

namespace tux_ti83 {

int EOSPrecedence::precedence(Token t) {
    switch (t) {
        case Token::Sin: case Token::Cos: case Token::Tan:
        case Token::Log: case Token::Ln: case Token::Sqrt: return 4;
        case Token::Pow: return 3;
        case Token::Mul: case Token::Div: return 2;
        case Token::Add: case Token::Sub: return 1;
        default: return 0;
    }
}

bool EOSPrecedence::is_operator(Token t) { return precedence(t) > 0; }
bool EOSPrecedence::is_function(Token t) { 
    return (t == Token::Sin || t == Token::Cos || t == Token::Tan || 
            t == Token::Log || t == Token::Ln || t == Token::Sqrt); 
}

CalculationResult MathStateMachine::evaluate(const std::vector<Token>& tokens, double xValue) {
    if (tokens.empty()) return {false, 0, "Empty"};

    // --- AUTO-COMPLETE LOGIC ---
    std::vector<Token> balancedTokens = tokens;
    int openCount = 0;
    int closeCount = 0;
    for (auto t : tokens) {
        if (t == Token::LeftParen) openCount++;
        if (t == Token::RightParen) closeCount++;
    }
    // Append missing right parentheses
    while (closeCount < openCount) {
        balancedTokens.push_back(Token::RightParen);
        closeCount++;
    }

    std::vector<Token> rpn;
    std::stack<Token> opStack;

    for (auto t : balancedTokens) {
        if (!EOSPrecedence::is_operator(t) && !EOSPrecedence::is_function(t) && t != Token::LeftParen && t != Token::RightParen) {
            rpn.push_back(t);
        } else if (EOSPrecedence::is_function(t) || t == Token::LeftParen) {
            opStack.push(t);
        } else if (t == Token::RightParen) {
            while (!opStack.empty() && opStack.top() != Token::LeftParen) {
                rpn.push_back(opStack.top()); opStack.pop();
            }
            if (!opStack.empty()) opStack.pop(); 
            if (!opStack.empty() && EOSPrecedence::is_function(opStack.top())) {
                rpn.push_back(opStack.top()); opStack.pop();
            }
        } else {
            while (!opStack.empty() && EOSPrecedence::precedence(opStack.top()) >= EOSPrecedence::precedence(t)) {
                rpn.push_back(opStack.top()); opStack.pop();
            }
            opStack.push(t);
        }
    }
    while (!opStack.empty()) { rpn.push_back(opStack.top()); opStack.pop(); }

    std::stack<double> valStack;
    for (auto t : rpn) {
        if (!EOSPrecedence::is_operator(t) && !EOSPrecedence::is_function(t)) {
            if (t == Token::VarX) valStack.push(xValue);
            else if (t == Token::Pi) valStack.push(M_PI);
            else if (t == Token::E) valStack.push(M_E);
            else {
                int val = static_cast<int>(t);
                if (val >= 0 && val <= 9) valStack.push(static_cast<double>(val));
                else valStack.push(0.0);
            }
        } else if (EOSPrecedence::is_function(t)) {
            if (valStack.empty()) return {false, 0, "Stack Error"};
            double a = valStack.top(); valStack.pop();
            if (t == Token::Sin) valStack.push(std::sin(a));
            else if (t == Token::Cos) valStack.push(std::cos(a));
            else if (t == Token::Tan) valStack.push(std::tan(a));
            else if (t == Token::Log) valStack.push(std::log10(a));
            else if (t == Token::Ln) valStack.push(std::log(a));
            else if (t == Token::Sqrt) valStack.push(std::sqrt(a));
        } else {
            if (valStack.size() < 2) return {false, 0, "Error"};
            double b = valStack.top(); valStack.pop();
            double a = valStack.top(); valStack.pop();
            if (t == Token::Add) valStack.push(a + b);
            else if (t == Token::Sub) valStack.push(a - b);
            else if (t == Token::Mul) valStack.push(a * b);
            else if (t == Token::Div) valStack.push(b == 0 ? 0 : a / b);
            else if (t == Token::Pow) valStack.push(std::pow(a, b));
        }
    }
    return {true, valStack.empty() ? 0 : valStack.top(), ""};
}

} // namespace tux_ti83

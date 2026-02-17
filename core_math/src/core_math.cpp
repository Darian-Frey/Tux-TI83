#include "capsules/capsule_math.hpp"
#include <stack>
#include <cmath>

namespace tux_ti83 {

int EOSPrecedence::precedence(Token t) {
    switch (t) {
        case Token::Add: case Token::Sub: return 1;
        case Token::Mul: case Token::Div: return 2;
        default: return 0;
    }
}

bool EOSPrecedence::is_operator(Token t) { return precedence(t) > 0; }

CalculationResult MathStateMachine::evaluate(const std::vector<Token>& tokens, double xValue) {
    if (tokens.empty()) return {false, 0, "Empty"};

    std::vector<Token> rpn;
    std::stack<Token> opStack;

    // Shunting Yard
    for (auto t : tokens) {
        if (!EOSPrecedence::is_operator(t) && t != Token::LeftParen && t != Token::RightParen) {
            rpn.push_back(t);
        } else if (t == Token::LeftParen) {
            opStack.push(t);
        } else if (t == Token::RightParen) {
            while (!opStack.empty() && opStack.top() != Token::LeftParen) {
                rpn.push_back(opStack.top()); opStack.pop();
            }
            if (!opStack.empty()) opStack.pop();
        } else {
            while (!opStack.empty() && EOSPrecedence::precedence(opStack.top()) >= EOSPrecedence::precedence(t)) {
                rpn.push_back(opStack.top()); opStack.pop();
            }
            opStack.push(t);
        }
    }
    while (!opStack.empty()) { rpn.push_back(opStack.top()); opStack.pop(); }

    // RPN Evaluation
    std::stack<double> valStack;
    for (auto t : rpn) {
        if (!EOSPrecedence::is_operator(t)) {
            if (t == Token::VarX) valStack.push(xValue);
            else if (t == Token::Pi) valStack.push(M_PI);
            else {
                int val = static_cast<int>(t);
                if (val >= 0 && val <= 9) valStack.push(static_cast<double>(val));
                else valStack.push(0.0);
            }
        } else {
            if (valStack.size() < 2) return {false, 0, "Error"};
            double b = valStack.top(); valStack.pop();
            double a = valStack.top(); valStack.pop();
            if (t == Token::Add) valStack.push(a + b);
            else if (t == Token::Sub) valStack.push(a - b);
            else if (t == Token::Mul) valStack.push(a * b);
            else if (t == Token::Div) valStack.push(b == 0 ? 0 : a / b);
        }
    }
    return {true, valStack.empty() ? 0 : valStack.top(), ""};
}

} // namespace tux_ti83

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

bool EOSPrecedence::is_operator(Token t) {
    return precedence(t) > 0;
}

CalculationResult MathStateMachine::evaluate(const std::vector<Token>& tokens) {
    if (tokens.empty()) return {false, 0, "Empty"};

    // 1. Shunting Yard: Infix -> Postfix (RPN)
    std::vector<Token> outputQueue;
    std::stack<Token> opStack;

    for (auto t : tokens) {
        if (!EOSPrecedence::is_operator(t) && t != Token::LeftParen && t != Token::RightParen) {
            outputQueue.push_back(t);
        } else if (t == Token::LeftParen) {
            opStack.push(t);
        } else if (t == Token::RightParen) {
            while (!opStack.empty() && opStack.top() != Token::LeftParen) {
                outputQueue.push_back(opStack.top());
                opStack.pop();
            }
            if (!opStack.empty()) opStack.pop(); // Pop LeftParen
        } else {
            while (!opStack.empty() && EOSPrecedence::precedence(opStack.top()) >= EOSPrecedence::precedence(t)) {
                outputQueue.push_back(opStack.top());
                opStack.pop();
            }
            opStack.push(t);
        }
    }
    while (!opStack.empty()) {
        outputQueue.push_back(opStack.top());
        opStack.pop();
    }

    // 2. RPN Evaluator
    std::stack<double> valStack;
    for (auto t : outputQueue) {
        if (!EOSPrecedence::is_operator(t)) {
            // Mapping tokens to basic values (simplification for this phase)
            if (t == Token::Num0) valStack.push(0);
            else if (t == Token::Num1) valStack.push(1);
            else if (t == Token::Num2) valStack.push(2);
            else if (t == Token::Num3) valStack.push(3);
            else if (t == Token::Num4) valStack.push(4);
            else if (t == Token::Num5) valStack.push(5);
            else if (t == Token::Num6) valStack.push(6);
            else if (t == Token::Num7) valStack.push(7);
            else if (t == Token::Num8) valStack.push(8);
            else if (t == Token::Num9) valStack.push(9);
            else if (t == Token::Pi) valStack.push(M_PI);
        } else {
            if (valStack.size() < 2) return {false, 0, "Stack Underflow"};
            double b = valStack.top(); valStack.pop();
            double a = valStack.top(); valStack.pop();
            
            if (t == Token::Add) valStack.push(a + b);
            else if (t == Token::Sub) valStack.push(a - b);
            else if (t == Token::Mul) valStack.push(a * b);
            else if (t == Token::Div) valStack.push(b == 0 ? 0 : a / b);
        }
    }

    if (valStack.empty()) return {false, 0, "No Result"};
    return {true, valStack.top(), ""};
}

} // namespace tux_ti83

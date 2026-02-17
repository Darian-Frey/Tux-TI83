#pragma once
#include <vector>
#include <map>
#include <optional>
#include <string>

namespace tux_ti83 {

    enum class Token {
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        Decimal, Pi, E, Add, Sub, Mul, Div, Pow, ImplicitMul,
        Sin, Cos, Tan, Log, Ln, Sqrt, Negate, Inverse, Square, Factorial,
        LeftParen, RightParen, Comma, VarX
    };

    struct CalculationResult {
        bool success;
        double value;
        std::string error_message;
    };

    class EOSPrecedence {
    public:
        static int precedence(Token t);
        static bool is_left_associative(Token t);
        static bool is_operator(Token t);
        static bool is_function(Token t);
    };

    class MathStateMachine {
    public:
        CalculationResult evaluate(const std::vector<Token>& graph, double xValue = 0.0);
    };
}

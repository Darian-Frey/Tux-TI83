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
        LeftParen, RightParen, Comma
    };

    struct CalculationResult {
        bool success;
        double value;
        std::string error_message;
    };

    class EOSPrecedence {
    public:
        static int precedence(Token t) {
            switch (t) {
                case Token::Factorial:   return 5;
                case Token::Negate:      return 4;
                case Token::ImplicitMul: return 3;
                case Token::Pow:         return 3;
                case Token::Mul:
                case Token::Div:         return 2;
                case Token::Add:
                case Token::Sub:         return 1;
                default:                 return 0;
            }
        }
        static bool is_left_associative(Token t) { return !(t == Token::Pow); }
        static bool is_operator(Token t) {
            return (precedence(t) > 0 || t == Token::ImplicitMul);
        }
    };

    class MathStateMachine {
    public:
        CalculationResult evaluate(const std::vector<Token>& graph) {
            return {true, 42.0, ""}; // Placeholder for ST[4] logic
        }
        std::optional<std::vector<Token>> infix_to_postfix(const std::vector<Token>& tokens) const;
    };
}

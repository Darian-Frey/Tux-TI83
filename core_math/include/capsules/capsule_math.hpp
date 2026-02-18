#pragma once
#include <vector>
#include <string>
#include <map>

namespace tux_ti83 {

    enum class Token {
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        Decimal, Pi, E, Add, Sub, Mul, Div, Pow, ImplicitMul,
        Sin, Cos, Tan, Log, Ln, Sqrt, ASin, ACos, ATan,
        Equal, NotEqual, Less, LessEq, Greater, GreaterEq,
        And, Or, Xor, Not,
        LeftParen, RightParen, VarX,
        // Matrix Specific Tokens
        OpenBracket, CloseBracket, Comma,
        MatA, MatB, MatC, MatD, MatE, MatF, MatG, MatH, MatI, MatJ
    };

    struct Matrix {
        int rows = 0;
        int cols = 0;
        std::vector<double> data;
        
        double at(int r, int c) const { return data[r * cols + c]; }
        void set(int r, int c, double val) { data[r * cols + c] = val; }
    };

    struct CalculationResult {
        bool success;
        double value;
        Matrix matrixValue; // Supports matrix-to-matrix results
        bool isMatrix = false;
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
        static std::string toFraction(double value, double tolerance = 1.0e-9);
        
        // Matrix Storage
        static std::map<Token, Matrix> matrixRegistry;
    };
}

#pragma once
#include <array>
#include <functional>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace tux_ti83 {

enum class Token {
  Num0,
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,
  Decimal,
  Pi,
  E,
  Add,
  Sub,
  Mul,
  Div,
  Pow,
  // Binary infix nth-root: `n NthRoot x` returns x^(1/n). Same
  // precedence as Pow and right-associative — `2 NthRoot 3 NthRoot 8`
  // groups as `2 NthRoot (3 NthRoot 8)`. DOMAIN error when n is 0;
  // NONREAL ANS for even n on a negative x. Real TI-83 binds 2ND+^
  // to this; `xroot` works as an ASCII alias.
  NthRoot,
  // Implicit multiplication — never typed directly by the user.
  // A preprocessing pass in `evaluate()` synthesises this between
  // juxtaposed value-like tokens (`2π`, `2(3)`, `(3)(4)`, `2sin(x)`,
  // `5X`). Same precedence + behaviour as Mul on the eval side;
  // separate token so the source structure stays inspectable.
  ImplicitMul,
  Sin,
  Cos,
  Tan,
  Log,
  Ln,
  Sqrt,
  ASin,
  ACos,
  ATan,
  Equal,
  NotEqual,
  Less,
  LessEq,
  Greater,
  GreaterEq,
  And,
  Or,
  Xor,
  Not,
  Det,
  // Matrix transpose — unary prefix function that takes a matrix and
  // returns its transpose (rows and columns swapped).
  Transpose,
  // Reduced row-echelon form. Takes a matrix, returns the matrix with
  // row operations applied until each leading coefficient is 1 and is
  // the only non-zero entry in its column (Gauss-Jordan elimination).
  Rref,
  // Unary negation. Semantically distinct from binary Sub so we can
  // honour the TI-83's `(-)` key versus `−` key distinction.
  Neg,
  // Number functions (unary)
  Abs,
  Int,
  IPart,
  FPart,
  // Natural exponential — eˣ as a dedicated token. The 2ND+LN macro
  // already injected "e^(" as three tokens (E, Pow, LeftParen) which
  // computed the same thing via implicit `M_E ^ x`; this token makes
  // it a single first-class entry and matches the TI-83 keytop label.
  Exp,
  // Sign function — returns -1, 0, or +1.
  Sgn,
  // Matrix/List toolkit (Phase F follow-up). All take a built-in `(`.
  //   Identity(n)   — unary scalar → n×n identity matrix.
  //   Dim(arg)      — unary; matrix → {rows,cols} list, list → length scalar.
  //   Ref(M)        — unary matrix → row-echelon form (forward elimination,
  //                   leading 1s; distinct from Rref which back-eliminates).
  //   Augment(a,b)  — binary; matrix‖matrix (equal rows → horizontal concat)
  //                   or list‖list (concatenation).
  //   RandM(r,c)    — binary scalars → r×c matrix of random ints in [-9,9].
  Identity,
  Dim,
  Ref,
  Augment,
  RandM,
  // Number functions (binary). Pop two operands during evaluation;
  // arguments are separated by Comma in the source expression.
  Round,
  Min,
  Max,
  Mod,
  // Combinatorics (binary). Arguments must be non-negative integers
  // with r ≤ n; domain errors otherwise.
  NCr,
  NPr,
  // Factorial — unary postfix. Accepts non-negative integers ≤ 170
  // (beyond that a double would overflow). Returns DOMAIN error for
  // negatives, non-integers, or values > 170.
  Fact,
  // Hyperbolic functions (unary). asinh/atanh accept all reals;
  // acosh requires x ≥ 1 (DOMAIN error otherwise); atanh requires
  // -1 < x < 1 (DOMAIN error otherwise).
  Sinh,
  Cosh,
  Tanh,
  ASinh,
  ACosh,
  ATanh,
  LeftParen,
  RightParen,
  // Scalar variables A..Z (26 contiguous tokens). Backing store is
  // MathStateMachine::varRegistry, indexed by `(int)t - (int)VarA`.
  // VarX is dual-purpose: in calc-mode evaluation the controller passes
  // `varRegistry[X_idx]` as xValue so `5→X` then `X+1` reads the stored
  // value; in graph-mode evaluation the controller passes the sweep x,
  // so plotting Y1=X² walks across the window. All other letters always
  // resolve via the registry.
  VarA,
  VarB,
  VarC,
  VarD,
  VarE,
  VarF,
  VarG,
  VarH,
  VarI,
  VarJ,
  VarK,
  VarL,
  VarM,
  VarN,
  VarO,
  VarP,
  VarQ,
  VarR,
  VarS,
  VarT,
  VarU,
  VarV,
  VarW,
  VarX,
  VarY,
  VarZ,
  // Assignment: <expr> → <var>. Evaluate::preprocess consumes the
  // target VarA..VarZ token and records its index in a sidebar; the
  // evaluator pops the top of the stack, writes it to varRegistry,
  // then pushes the value back so it also appears on the display.
  // Lowest precedence (-10) so the whole LHS expression resolves
  // before the store.
  Sto,
  // Statement separator (real TI-83 syntax: `5→A:A+1→A` chains two
  // expressions). The evaluator splits the token stream at every
  // Colon boundary, evaluates each segment in order, and returns
  // the result of the final non-empty segment. Errors in any segment
  // short-circuit — varRegistry mutations from earlier segments stay
  // (matching TI-83 behaviour: state changes commit per-statement).
  Colon,
  // Matrix Specific Tokens
  OpenBracket,
  CloseBracket,
  Comma,
  MatA,
  MatB,
  MatC,
  MatD,
  MatE,
  MatF,
  MatG,
  MatH,
  MatI,
  MatJ,
  // Last-answer recall. Populated by the controller after any successful
  // ENTER; retrieved by the evaluator when it sees this token and pushed
  // onto the operand stack (scalar or matrix, per lastResult.isMatrix).
  Ans,
  // Y-VARS — references to the user-defined function buffers Y1/Y2/Y3.
  // When evaluated, the engine recursively evaluates the referenced
  // buffer at the current xValue. Bare form only (Y1 alone uses
  // current X); explicit-argument form `Y1(3)` is not supported in
  // v1 — it parses as `Y1 * 3` via the existing implicit-mul rule.
  // Self-reference and cross-Y cycles return "Recursion".
  // Y1..Y9, Y0 — the ten function slots. Kept CONTIGUOUS (Y0 last, the
  // 10th) so the evaluator maps a token to its buffer index as t - Y1
  // (Y0 → 9). Y1Call..Y0Call below mirror this order.
  Y1,
  Y2,
  Y3,
  Y4,
  Y5,
  Y6,
  Y7,
  Y8,
  Y9,
  Y0,
  // Explicit-argument call form for Y-VARS: `Y1(3)` evaluates Y1
  // with X = 3 (the argument), rather than the bare form's
  // implicit-current-X. Synthesised in a preprocessing pass by
  // detecting [Y_n, LeftParen] adjacency and collapsing to a single
  // Y_nCall token. The call form is a unary function with built-in
  // paren — the argument expression flows through the standard
  // function-arg pipeline, gets popped off the operand stack, and
  // overrides xValue for the recursive yLookup eval.
  Y1Call,
  Y2Call,
  Y3Call,
  Y4Call,
  Y5Call,
  Y6Call,
  Y7Call,
  Y8Call,
  Y9Call,
  Y0Call,
  // Deferred-evaluation calculus functions. The user-typed form takes
  // an expression, a bound variable, and one or two bounds:
  //   fnInt(expr, var, lower, upper)        — definite integral
  //   nDeriv(expr, var, point [, h])        — symmetric finite difference
  //   sum(expr, var, start, end)            — Σ from ⌊start⌋ to ⌊end⌋
  //   prod(expr, var, start, end)           — Π from ⌊start⌋ to ⌊end⌋
  // A preprocessing pass in `evaluate()` extracts the unevaluated first
  // argument and the variable letter into a thread-local side table,
  // rewriting the call into a synthetic *Call token whose eager args
  // remain in the token stream (so the existing shunting-yard handles
  // bound expressions identically to any other operand). The synthetic
  // call carries its side-table index as a trailing integer operand,
  // popped by the evaluator alongside the eager arguments.
  FnInt,
  NDeriv,
  Sum,
  Prod,
  FnIntCall,
  NDerivCall,
  SumCall,
  ProdCall,
  // IMP-004: dedicated numeric-literal sentinel. The digit-flush prepass
  // in evaluate() coalesces runs of Num0..Num9/Decimal into a single
  // parsed double and pushes this marker in their place, with the value
  // stored in the parallel `numericValues` array. Distinct from Num0 (the
  // literal digit 0) so post-flush passes can't confuse "the value zero"
  // with "a numeric literal, look up its value". Never produced by the
  // tokenizer — only appears downstream of the flush pass.
  NumLiteral,

  // --- Lists (Phase C) ---
  // L1..L6 registry references (contiguous, like MatA..MatJ) — leaf
  // operands resolved from `listRegistry`.
  L1,
  L2,
  L3,
  L4,
  L5,
  L6,
  // `{` and `}` delimit a list literal. LeftBrace acts as a grouping
  // marker on the operator stack (like LeftParen); RightBrace closes the
  // literal and the shunting-yard emits a MakeList carrying the element
  // count. Commas inside the braces separate elements.
  LeftBrace,
  RightBrace,
  // Synthetic: emitted by the shunting-yard when a `}` closes a list
  // literal. Its RPN node carries the element count in the `.second`
  // field; the evaluator pops that many operands into a List. Never
  // typed by the user.
  MakeList,

  // --- List reduction functions (Phase C Wave 3) ---
  // Unary: take one list operand and return a scalar. Mean/StdDev/
  // Variance are typed directly. ListSum/ListProd are synthetic — the
  // deferred-call rewriter emits them when the calculus `sum(`/`prod(`
  // (Token::Sum/Prod) are called with a single (list) argument instead
  // of the 4-arg (expr,var,start,end) form, overloading the name by
  // arity as IMP-044 anticipated. Min/Max reuse their existing tokens
  // and branch on operand type in the evaluator.
  Mean,
  StdDev,
  Variance,
  ListSum,
  ListProd,
  Median,

  // seq(expr, var, start, end[, step]) — generates a list by evaluating
  // `expr` for `var` stepped from start to end (Phase C Wave 3b). Like
  // the calculus sum/prod it captures an unevaluated first argument, so
  // it uses the deferred-call framework: `Seq` is the surface token
  // (rewritten out before the shunting-yard) and `SeqCall` is the
  // synthetic form the evaluator sees. Unlike sum/prod it returns a
  // list, not a scalar.
  Seq,
  SeqCall,

  // --- Random functions (Phase C Wave 5) ---
  // `Rand` is a bare leaf value in [0,1). The others are functions:
  // the 2-arg scalar forms (RandInt/RandNorm/RandBin) and the 3-arg
  // list forms (…List), which a preprocessing pass selects by counting
  // the arguments (a 3rd `count` argument → the list variant).
  Rand,
  RandInt,
  RandNorm,
  RandBin,
  RandIntList,
  RandNormList,
  RandBinList,

  // --- Distributions (Phase C follow-on) ---
  // Normal family. μ/σ are optional (default 0/1); a preprocessing pass
  // pads the missing arguments so the evaluator sees a fixed arity —
  // normalpdf/invNorm take 3 (x/area, μ, σ), normalcdf takes 4
  // (lower, upper, μ, σ).
  NormalPdf,
  NormalCdf,
  InvNorm,

  // Discrete distributions. binom takes 3 args (scalar P/cumulative at x)
  // or 2 (the …List variant — the whole distribution over x=0..n, chosen
  // by an arg-counting pass). Poisson and geometric are 2-arg scalars.
  BinomPdf,
  BinomCdf,
  BinomPdfList,
  BinomCdfList,
  PoissonPdf,
  PoissonCdf,
  GeometPdf,
  GeometCdf,

  // Continuous distributions. tpdf/χ²pdf take 2 args, tcdf/χ²cdf and
  // Fpdf take 3, Fcdf takes 4 — all fixed arity. CDFs use the
  // regularized incomplete gamma (χ²) and beta (t, F) functions.
  TPdf,
  TCdf,
  ChiPdf,
  ChiCdf,
  FPdf,
  FCdf,

  // Complex numbers (Phase F). ImagI is the imaginary unit `i` (a leaf,
  // like Pi/E). The rest are unary functions over a complex value.
  ImagI,
  Conj,
  RealPart,
  ImagPart,
  Angle
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
  // Phase C lists. Appended after the original five fields so existing
  // positional initializers ({success, value, mat, isMatrix, err}) keep
  // working — isList defaults false, listValue empty.
  bool isList = false;
  std::vector<double> listValue;
  // Complex numbers (Phase F). `value` holds the real part; `imag` the
  // imaginary part. Appended last so existing positional initializers
  // keep working (imag defaults 0 → a real result).
  double imag = 0.0;
};

class EOSPrecedence {
public:
  static int precedence(Token t);
  static bool is_left_associative(Token t);
  static bool is_operator(Token t);
  static bool is_function(Token t);
  // True for functions that pop two operands during evaluation
  // (round, min, max, mod). Arguments are comma-separated in the source.
  static bool is_binary_function(Token t);
  // True for functions whose kTokens input string ends in `(` (so the
  // user types e.g. `abs(` as one keystroke and never types a separate
  // `(`). The shunting-yard pushes a synthetic LeftParen alongside
  // these so the matching `)` and any inner commas have a clear scope
  // marker. Functions whose input is bare (sin, cos, ..., neg) instead
  // expect the user to type `(` separately.
  static bool has_built_in_paren(Token t);
};

// Trig-function angle interpretation. Default is Radian (matches
// mathematical convention and preserves prior behaviour). When set to
// Degree, sin/cos/tan convert their input from degrees, and
// asin/acos/atan return degrees. Hyperbolic functions ignore this —
// hyperbolic arguments are dimensionless.
enum class AngleMode { Radian, Degree };

// Number-format notation for scalar results. Normal is the default
// "smart" format ('g' precision trims trailing zeros); Sci forces
// scientific notation (`1.234E5`); Eng forces engineering notation —
// same as Sci but the exponent is always a multiple of 3
// (`123.4E3` rather than `1.234E5`). Interpretation lives in
// UIController::formatScalar so the engine stays format-agnostic.
enum class NumberNotation { Normal, Sci, Eng };

// Integer display base for scalar results. Dec is the default and the
// only mode that displays non-integers; Hex/Oct/Bin truncate to int64
// and render the magnitude in their respective bases with a sign
// prefix (`0xFF`, `0o77`, `0b1010`, `-0xFF`). Non-integer values, NaN,
// infinities, and magnitudes outside int64 range fall back to the
// active Notation/Decimal formatter regardless of base. Interpretation
// lives in UIController::formatScalar.
enum class NumberBase { Dec, Hex, Oct, Bin };

// Complex-result mode (MODE → Complex). Real: operations that would
// produce a non-real result from real inputs (√ of a negative, etc.)
// error with NONREAL ANS. Rect (a+bi) and Polar (re^θi) allow complex
// results; they differ only in how the result is displayed.
enum class ComplexMode { Real, Rect, Polar };

class MathStateMachine {
public:
  CalculationResult evaluate(const std::vector<Token> &graph,
                             double xValue = 0.0);
  static std::string toFraction(double value, double tolerance = 1.0e-9);

  // Matrix Storage
  static std::map<Token, Matrix> matrixRegistry;

  // List storage L1..L6 (Phase C). Keyed by the L1..L6 token. A slot
  // absent from the map is an undefined list (ERR:UNDEFINED on read).
  static std::map<Token, std::vector<double>> listRegistry;

  // Shared PRNG for the random functions (Phase C Wave 5). Seeded from
  // std::random_device at startup; seedRandom() forces a deterministic
  // sequence (used by the test suite).
  static std::mt19937 rng;
  static void seedRandom(unsigned int seed);

  // Scalar variable registry A..Z. Zero-initialised on program start;
  // mutated via Sto. Errors don't overwrite — the evaluator writes to
  // the registry only after a successful arithmetic result.
  static std::array<double, 26> varRegistry;

  // Current angle mode. Process-global static so CLI/REPL/GUI share
  // one setting; reflected in the trig-function evaluation below.
  static AngleMode angleMode;

  // Number-format controls. Both are read by UIController::formatScalar
  // when turning a double into a display string. `fixDecimals` == -1 is
  // the Float mode (default); 0..9 fixes that many decimal places.
  static NumberNotation notation;
  static int fixDecimals;

  // Integer display base. Read by UIController::formatScalar; Dec
  // preserves the historic Notation/Decimal behaviour, Hex/Oct/Bin
  // switch integer-valued scalars to base 16/8/2 with sign + prefix.
  static NumberBase numberBase;
  static ComplexMode complexMode;

  // Last successful evaluation result. Updated by the UI controller
  // after every successful ENTER and recalled via Token::Ans. Matches
  // a TI-83's `Ans` behaviour — errors don't overwrite it.
  static CalculationResult lastResult;

  // Y-VARS lookup. The UI controller sets this on construction to a
  // lambda that returns m_functionBuffers[idx] (idx ∈ {0,1,2} for
  // Y1/Y2/Y3). Stays null in headless contexts (CLI/REPL/tests) —
  // the evaluator then treats every Y_n as 0, matching the TI-83
  // behaviour of an empty function slot.
  static std::function<std::vector<Token>(int)> yLookup;
};
} // namespace tux_ti83

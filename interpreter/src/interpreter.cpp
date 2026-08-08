// TI-BASIC interpreter — P0 scaffolding. See interpreter.hpp / docs/TIBASIC.md.

#include "interpreter.hpp"

#include <algorithm>
#include <cmath>

namespace tux_ti83 {

namespace {

bool isIdentChar(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9');
}

// Find the string-function call (length/inString/sub/expr) with the greatest
// start index — guaranteed innermost, so its arguments contain no further such
// call. Returns false if none. Sets name / nameStart / closeParen.
bool findInnermostStrCall(const std::string &s, std::string &name,
                          std::size_t &nameStart, std::size_t &closeParen) {
  static const char *const kNames[] = {"length", "inString", "sub", "expr"};
  bool found = false;
  bool inStr = false;
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '"') {
      inStr = !inStr;
      continue;
    }
    if (inStr)
      continue;
    if (i > 0 && isIdentChar(s[i - 1]))
      continue;  // not the start of a word
    for (const char *nm : kNames) {
      const std::size_t len = std::string(nm).size();
      if (i + len < s.size() && s.compare(i, len, nm) == 0 &&
          s[i + len] == '(') {
        // Match the closing paren (respecting nested () and strings).
        int depth = 0;
        bool q = false;
        std::size_t close = std::string::npos;
        for (std::size_t j = i + len; j < s.size(); ++j) {
          const char d = s[j];
          if (d == '"') {
            q = !q;
            continue;
          }
          if (q)
            continue;
          if (d == '(')
            ++depth;
          else if (d == ')' && --depth == 0) {
            close = j;
            break;
          }
        }
        if (close != std::string::npos && (!found || i > nameStart)) {
          found = true;
          name = nm;
          nameStart = i;
          closeParen = close;
        }
      }
    }
  }
  return found;
}

// Trim ASCII whitespace from both ends. Multi-byte UTF-8 characters (→, π,
// …) never contain a 0x20/0x09/etc. byte, so byte-level trimming is safe.
std::string trim(const std::string &s) {
  const char *ws = " \t\r\n\f\v";
  const auto begin = s.find_first_not_of(ws);
  if (begin == std::string::npos)
    return {};
  const auto end = s.find_last_not_of(ws);
  return s.substr(begin, end - begin + 1);
}

// Does `stmt` begin with keyword `kw` as a whole word — i.e. followed by a
// space, end of statement, or an opening delimiter (so "Disp X", "Disp",
// and "Disp(…)" match, but "Disparate" would not)?
// True if `stmt` has a top-level store arrow (`->` or `→`), outside any
// string — i.e. it's an assignment. TI-83 assignments run silently in a
// program (only a bare expression with no store echoes its value).
bool isAssignment(const std::string &stmt) {
  bool inStr = false;
  for (std::size_t i = 0; i < stmt.size(); ++i) {
    const char c = stmt[i];
    if (c == '"') {
      inStr = !inStr;
      continue;
    }
    if (inStr)
      continue;
    if (c == '-' && i + 1 < stmt.size() && stmt[i + 1] == '>')
      return true;
    if (static_cast<unsigned char>(c) == 0xE2 && i + 2 < stmt.size() &&
        static_cast<unsigned char>(stmt[i + 1]) == 0x86 &&
        static_cast<unsigned char>(stmt[i + 2]) == 0x92)  // → (U+2192)
      return true;
  }
  return false;
}

bool matchKeyword(const std::string &stmt, const std::string &kw) {
  if (stmt.size() < kw.size() || stmt.compare(0, kw.size(), kw) != 0)
    return false;
  if (stmt.size() == kw.size())
    return true;
  const char next = stmt[kw.size()];
  return next == ' ' || next == '\t' || next == '(' || next == '"';
}

}  // namespace

std::vector<std::string> Interpreter::splitStatements(const std::string &line) {
  std::vector<std::string> out;
  std::string current;
  bool inStr = false;
  for (char c : line) {
    if (c == '"')
      inStr = !inStr;
    if (c == ':' && !inStr) {
      const std::string t = trim(current);
      if (!t.empty())
        out.push_back(t);
      current.clear();
    } else {
      current.push_back(c);
    }
  }
  const std::string t = trim(current);
  if (!t.empty())
    out.push_back(t);
  return out;
}

void Interpreter::loadStatements(const std::vector<std::string> &lines) {
  m_statements.clear();
  m_statementSrcLine.clear();
  for (int lineNo = 0; lineNo < static_cast<int>(lines.size()); ++lineNo) {
    auto stmts = splitStatements(lines[static_cast<size_t>(lineNo)]);
    for (auto &s : stmts) {
      m_statements.push_back(std::move(s));
      m_statementSrcLine.push_back(lineNo);  // map each statement to its line
    }
  }
  buildControlTables();
}

void Interpreter::load(const std::vector<std::string> &lines,
                       const std::string &name) {
  m_currentProgram = name;
  loadStatements(lines);
  reset();
}

int Interpreter::errorSourceLine() const {
  if (m_errorLine < 0 ||
      m_errorLine >= static_cast<int>(m_statementSrcLine.size()))
    return -1;
  return m_statementSrcLine[static_cast<size_t>(m_errorLine)];
}

bool Interpreter::returnFromCall() {
  if (m_callStack.empty())
    return false;
  CallFrame f = std::move(m_callStack.back());
  m_callStack.pop_back();
  m_statements = std::move(f.statements);
  m_statementSrcLine = std::move(f.srcLine);
  m_currentProgram = std::move(f.program);
  m_pc = f.pc;
  m_openerToEnd = std::move(f.openerToEnd);
  m_thenToElse = std::move(f.thenToElse);
  m_elseToEnd = std::move(f.elseToEnd);
  m_endToOpener = std::move(f.endToOpener);
  m_labels = std::move(f.labels);
  m_forStack = std::move(f.forStack);
  return true;
}

void Interpreter::buildControlTables() {
  const int n = static_cast<int>(m_statements.size());
  m_openerToEnd.assign(static_cast<size_t>(n), -1);
  m_thenToElse.assign(static_cast<size_t>(n), -1);
  m_elseToEnd.assign(static_cast<size_t>(n), -1);
  m_endToOpener.assign(static_cast<size_t>(n), -1);
  m_labels.clear();
  std::vector<int> stack;  // open Then/For/While/Repeat indices
  for (int i = 0; i < n; ++i) {
    const std::string &s = m_statements[static_cast<size_t>(i)];
    if (matchKeyword(s, "Lbl")) {
      m_labels[trim(s.substr(3))] = i;
    } else if (matchKeyword(s, "Then") || matchKeyword(s, "For") ||
               matchKeyword(s, "While") || matchKeyword(s, "Repeat")) {
      stack.push_back(i);
    } else if (matchKeyword(s, "Else")) {
      if (!stack.empty() &&
          matchKeyword(m_statements[static_cast<size_t>(stack.back())], "Then"))
        m_thenToElse[static_cast<size_t>(stack.back())] = i;
    } else if (matchKeyword(s, "End")) {
      if (!stack.empty()) {
        const int opener = stack.back();
        stack.pop_back();
        m_openerToEnd[static_cast<size_t>(opener)] = i;
        m_endToOpener[static_cast<size_t>(i)] = opener;
        // If this block was an If-Then with an Else, map that Else → End.
        const int elseIdx = m_thenToElse[static_cast<size_t>(opener)];
        if (elseIdx >= 0)
          m_elseToEnd[static_cast<size_t>(elseIdx)] = i;
      }
    }
  }
}

std::vector<std::string> Interpreter::splitArgs(const std::string &s) {
  std::vector<std::string> out;
  std::string cur;
  int depth = 0;  // nesting depth across () [] {}
  bool inStr = false;
  for (char c : s) {
    if (c == '"')
      inStr = !inStr;
    if (!inStr) {
      if (c == '(' || c == '[' || c == '{')
        ++depth;
      else if (c == ')' || c == ']' || c == '}')
        --depth;
    }
    if (c == ',' && depth <= 0 && !inStr) {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

int Interpreter::strVarIndex(const std::string &s) {
  if (s.size() == 4 && s.compare(0, 3, "Str") == 0 && s[3] >= '1' &&
      s[3] <= '9')
    return s[3] - '0';
  return 0;
}

std::vector<std::string> Interpreter::splitPlus(const std::string &s) {
  std::vector<std::string> out;
  std::string cur;
  int depth = 0;
  bool inStr = false;
  for (char c : s) {
    if (c == '"')
      inStr = !inStr;
    if (!inStr) {
      if (c == '(' || c == '[' || c == '{')
        ++depth;
      else if (c == ')' || c == ']' || c == '}')
        --depth;
    }
    if (c == '+' && depth <= 0 && !inStr) {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

int Interpreter::stringStoreTarget(const std::string &stmt, std::string &lhs) {
  // Find the last top-level store arrow ("->" or "→", not inside a string).
  std::size_t arrowPos = std::string::npos;
  std::size_t arrowLen = 0;
  bool inStr = false;
  for (std::size_t i = 0; i < stmt.size(); ++i) {
    const char c = stmt[i];
    if (c == '"') {
      inStr = !inStr;
      continue;
    }
    if (inStr)
      continue;
    if (c == '-' && i + 1 < stmt.size() && stmt[i + 1] == '>') {
      arrowPos = i;
      arrowLen = 2;
    } else if (static_cast<unsigned char>(c) == 0xE2 && i + 2 < stmt.size() &&
               static_cast<unsigned char>(stmt[i + 1]) == 0x86 &&
               static_cast<unsigned char>(stmt[i + 2]) == 0x92) {  // → UTF-8
      arrowPos = i;
      arrowLen = 3;
    }
  }
  if (arrowPos == std::string::npos)
    return 0;
  const int n = strVarIndex(trim(stmt.substr(arrowPos + arrowLen)));
  if (n == 0)
    return 0;
  lhs = trim(stmt.substr(0, arrowPos));
  return n;
}

Interpreter::StrEval Interpreter::evalStringExpr(const std::string &src,
                                                 std::string &out) {
  // Resolve string functions first (sub( → a "…" literal), then evaluate the
  // resulting '+'-chain. A function error surfaces via m_strFuncError.
  const std::string resolved = resolveStrFuncs(src);
  if (!m_strFuncError.empty())
    return StrEval::TypeError;
  return evalStringChain(resolved, out);
}

Interpreter::StrEval Interpreter::evalStringChain(const std::string &src,
                                                  std::string &out) const {
  const auto terms = splitPlus(src);
  const std::string first = terms.empty() ? "" : trim(terms.front());
  const bool firstIsStr =
      (!first.empty() && first.front() == '"') || strVarIndex(first) != 0;
  if (!firstIsStr)
    return StrEval::NotString;  // numeric — let the engine handle it

  std::string result;
  for (const auto &t : terms) {
    const std::string term = trim(t);
    if (!term.empty() && term.front() == '"') {
      std::string inner = term.substr(1);
      if (!inner.empty() && inner.back() == '"')
        inner.pop_back();
      result += inner;
    } else if (const int n = strVarIndex(term)) {
      auto it = m_strVars.find(n);
      if (it != m_strVars.end())
        result += it->second;
    } else {
      return StrEval::TypeError;  // a non-string term in a string chain
    }
  }
  out = result;
  return StrEval::Ok;
}

std::string Interpreter::resolveStrFuncs(std::string expr) {
  m_strFuncError.clear();
  // Repeatedly resolve the innermost string-function call. Innermost-first
  // means each call's arguments are already free of these functions.
  for (int guard = 0; guard < 1000; ++guard) {
    std::string name;
    std::size_t st = 0, cl = 0;
    if (!findInnermostStrCall(expr, name, st, cl))
      break;
    const std::size_t argStart = st + name.size() + 1;  // past "name("
    const auto args = splitArgs(expr.substr(argStart, cl - argStart));

    // Evaluate a string argument (literals / StrN only, no funcs left here).
    auto strArg = [&](const std::string &a, std::string &s) -> bool {
      return evalStringChain(trim(a), s) == StrEval::Ok;
    };
    std::string repl;
    if (name == "length") {
      std::string s;
      if (args.size() != 1 || !strArg(args[0], s)) {
        m_strFuncError = "ERR:DATA TYPE";
        break;
      }
      repl = std::to_string(s.size());
    } else if (name == "sub") {
      std::string s;
      if (args.size() != 3 || !strArg(args[0], s)) {
        m_strFuncError = "ERR:DATA TYPE";
        break;
      }
      const EvalResult b = m_eval(trim(args[1]));
      const EvalResult c = m_eval(trim(args[2]));
      if (!b.ok || !c.ok) {
        m_strFuncError = "ERR:DATA TYPE";
        break;
      }
      const long begin = std::lround(b.value);
      const long count = std::lround(c.value);
      if (begin < 1 || count < 0 ||
          begin - 1 + count > static_cast<long>(s.size())) {
        m_strFuncError = "ERR:DOMAIN";
        break;
      }
      repl = "\"" + s.substr(static_cast<std::size_t>(begin - 1),
                             static_cast<std::size_t>(count)) +
             "\"";  // string result → a literal, so it stays a string
    } else if (name == "inString") {
      std::string hay, needle;
      if (args.size() < 2 || args.size() > 3 || !strArg(args[0], hay) ||
          !strArg(args[1], needle)) {
        m_strFuncError = "ERR:DATA TYPE";
        break;
      }
      long start = 1;
      if (args.size() == 3) {
        const EvalResult s3 = m_eval(trim(args[2]));
        if (!s3.ok) {
          m_strFuncError = "ERR:DATA TYPE";
          break;
        }
        start = std::lround(s3.value);
      }
      if (start < 1)
        start = 1;
      long pos = 0;
      if (static_cast<std::size_t>(start - 1) <= hay.size()) {
        const auto f = hay.find(needle, static_cast<std::size_t>(start - 1));
        pos = (f == std::string::npos) ? 0 : static_cast<long>(f) + 1;
      }
      repl = std::to_string(pos);
    } else {  // expr — splice the string's content as a sub-expression
      std::string s;
      if (args.size() != 1 || !strArg(args[0], s)) {
        m_strFuncError = "ERR:DATA TYPE";
        break;
      }
      repl = "(" + s + ")";
    }
    expr = expr.substr(0, st) + repl + expr.substr(cl + 1);
  }
  return expr;
}

EvalResult Interpreter::mEval(const std::string &expr) {
  const std::string resolved = resolveStrFuncs(expr);
  if (!m_strFuncError.empty()) {
    EvalResult r;
    r.ok = false;
    r.error = m_strFuncError;
    return r;
  }
  return m_eval(resolved);
}

void Interpreter::reset() {
  m_pc = 0;
  m_output.clear();
  m_errorLine = -1;
  m_errorMessage.clear();
  m_stopRequested = false;
  m_forStack.clear();
  m_callStack.clear();
  m_status = RunStatus::Running;
}

bool Interpreter::evalCond(const std::string &expr, bool &ok) {
  const EvalResult r = mEval(expr);
  ok = r.ok;
  if (!ok) {
    m_errorLine = static_cast<int>(m_pc);
    m_errorMessage = r.error;
    return false;
  }
  return r.value != 0.0;
}

RunStatus Interpreter::fail(const std::string &label) {
  m_errorLine = static_cast<int>(m_pc);
  m_errorMessage = label;
  return RunStatus::Error;
}

void Interpreter::placeOutput(int row, int col, const std::string &text) {
  // Grow the buffer so row (1-based) exists; earlier rows stay blank.
  while (static_cast<int>(m_output.size()) < row)
    m_output.push_back("");
  // Clip so the text can't run past the 16-column screen edge.
  std::string clipped = text;
  const int maxLen = 16 - (col - 1);
  if (maxLen >= 0 && static_cast<int>(clipped.size()) > maxLen)
    clipped.resize(static_cast<std::size_t>(maxLen));
  std::string &line = m_output[static_cast<std::size_t>(row - 1)];
  if (static_cast<int>(line.size()) < col - 1)
    line.resize(static_cast<std::size_t>(col - 1), ' ');  // pad to the column
  for (std::size_t i = 0; i < clipped.size(); ++i) {
    const std::size_t pos = static_cast<std::size_t>(col - 1) + i;
    if (pos < line.size())
      line[pos] = clipped[i];
    else
      line.push_back(clipped[i]);
  }
}

RunStatus Interpreter::execStatement(const std::string &stmt) {
  // Without an evaluator, no-op (P0 behaviour) but still advance so a
  // program without one runs to Done.
  if (!m_eval) {
    ++m_pc;
    return RunStatus::Running;
  }

  const std::size_t here = m_pc;
  const std::size_t N = m_statements.size();

  // ─────────────────────── Control flow ───────────────────────
  if (matchKeyword(stmt, "If")) {
    bool ok;
    const bool truthy = evalCond(trim(stmt.substr(2)), ok);
    if (!ok)
      return RunStatus::Error;
    const std::size_t next = here + 1;
    const bool blockForm = next < N && matchKeyword(m_statements[next], "Then");
    if (blockForm) {
      const std::size_t thenIdx = next;
      if (truthy) {
        m_pc = thenIdx + 1;  // enter the Then body
      } else {
        const int elseIdx = m_thenToElse[thenIdx];
        if (elseIdx >= 0) {
          m_pc = static_cast<std::size_t>(elseIdx) + 1;  // enter Else body
        } else {
          const int endIdx = m_openerToEnd[thenIdx];
          m_pc = (endIdx >= 0) ? static_cast<std::size_t>(endIdx) + 1 : N;
        }
      }
    } else {
      // Single-statement If: the next statement runs iff the condition holds.
      m_pc = truthy ? here + 1 : here + 2;
    }
    return RunStatus::Running;
  }

  if (matchKeyword(stmt, "Then") || matchKeyword(stmt, "Lbl")) {
    ++m_pc;  // structural markers — no-op
    return RunStatus::Running;
  }

  if (matchKeyword(stmt, "Else")) {
    // Reached only after the true branch fell through — skip past the End.
    const int endIdx = m_elseToEnd[here];
    m_pc = (endIdx >= 0) ? static_cast<std::size_t>(endIdx) + 1 : N;
    return RunStatus::Running;
  }

  if (matchKeyword(stmt, "Goto")) {
    auto it = m_labels.find(trim(stmt.substr(4)));
    if (it == m_labels.end())
      return fail("ERR:LABEL");
    m_pc = static_cast<std::size_t>(it->second);
    return RunStatus::Running;
  }

  if (matchKeyword(stmt, "While")) {
    bool ok;
    const bool truthy = evalCond(trim(stmt.substr(5)), ok);
    if (!ok)
      return RunStatus::Error;
    if (truthy) {
      m_pc = here + 1;  // enter body
    } else {
      const int endIdx = m_openerToEnd[here];
      m_pc = (endIdx >= 0) ? static_cast<std::size_t>(endIdx) + 1 : N;
    }
    return RunStatus::Running;
  }

  if (matchKeyword(stmt, "Repeat")) {
    // Repeat always runs its body once; the condition is checked at End.
    m_pc = here + 1;
    return RunStatus::Running;
  }

  if (matchKeyword(stmt, "For")) {
    // For(var, start, end [, step])
    const auto lp = stmt.find('(');
    const auto rp = stmt.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp)
      return fail("ERR:SYNTAX");
    const auto args = splitArgs(stmt.substr(lp + 1, rp - lp - 1));
    if (args.size() < 3)
      return fail("ERR:SYNTAX");
    const std::string var = trim(args[0]);
    const std::string startSrc = trim(args[1]);
    const std::string endSrc = trim(args[2]);
    const std::string stepSrc = (args.size() >= 4) ? trim(args[3]) : "1";
    // Initialise the loop variable; capture end/step once (TI-style).
    if (!mEval("(" + startSrc + ")->" + var).ok)
      return fail("ERR:SYNTAX");
    const EvalResult er = mEval(endSrc);
    if (!er.ok)
      return fail(er.error);
    const EvalResult sr = mEval(stepSrc);
    if (!sr.ok)
      return fail(sr.error);
    const double cur = mEval(var).value;
    const bool inRange = (sr.value >= 0) ? (cur <= er.value + 1e-9)
                                         : (cur >= er.value - 1e-9);
    const int endIdx = m_openerToEnd[here];
    if (inRange) {
      m_forStack.push_back({var, er.value, sr.value, stepSrc, here + 1});
      m_pc = here + 1;  // enter body
    } else {
      m_pc = (endIdx >= 0) ? static_cast<std::size_t>(endIdx) + 1 : N;
    }
    return RunStatus::Running;
  }

  if (matchKeyword(stmt, "End")) {
    const int opener = m_endToOpener[here];
    if (opener < 0) {  // stray End — treat as no-op
      ++m_pc;
      return RunStatus::Running;
    }
    const std::string &op = m_statements[static_cast<std::size_t>(opener)];
    if (matchKeyword(op, "For")) {
      if (m_forStack.empty()) {
        ++m_pc;
        return RunStatus::Running;
      }
      ForFrame &f = m_forStack.back();
      if (!mEval(f.var + "+(" + f.stepSrc + ")->" + f.var).ok)
        return fail("ERR:SYNTAX");
      const double cur = mEval(f.var).value;
      const bool inRange = (f.stepVal >= 0) ? (cur <= f.endVal + 1e-9)
                                            : (cur >= f.endVal - 1e-9);
      if (inRange) {
        m_pc = f.bodyStart;  // loop
      } else {
        m_forStack.pop_back();
        ++m_pc;  // exit
      }
    } else if (matchKeyword(op, "While")) {
      bool ok;
      const bool truthy = evalCond(trim(op.substr(5)), ok);
      if (!ok)
        return RunStatus::Error;
      m_pc = truthy ? static_cast<std::size_t>(opener) + 1 : here + 1;
    } else if (matchKeyword(op, "Repeat")) {
      bool ok;
      const bool truthy = evalCond(trim(op.substr(6)), ok);
      if (!ok)
        return RunStatus::Error;
      // Repeat loops *until* the condition becomes true.
      m_pc = truthy ? here + 1 : static_cast<std::size_t>(opener) + 1;
    } else {
      ++m_pc;  // If-Then block — just continue
    }
    return RunStatus::Running;
  }

  // ── prgmNAME: call another program as a sub-routine (P5) ──
  // "prgm" is followed directly by the program name (no delimiter), so this
  // is a plain prefix check rather than matchKeyword.
  if (stmt.size() > 4 && stmt.compare(0, 4, "prgm") == 0) {
    const std::string name = trim(stmt.substr(4));
    if (!m_progLoader)
      return fail("ERR:UNDEFINED");
    if (m_callStack.size() >= 128)
      return fail("ERR:MEMORY");  // recursion / nesting too deep
    const auto subLines = m_progLoader(name);
    if (!subLines)
      return fail("ERR:UNDEFINED");  // no such program
    // Suspend the caller, to resume at the statement AFTER this call.
    m_callStack.push_back({m_statements, m_statementSrcLine, m_currentProgram,
                           here + 1, m_openerToEnd, m_thenToElse, m_elseToEnd,
                           m_endToOpener, m_labels, m_forStack});
    loadStatements(*subLines);  // sub's statements + fresh control tables
    m_currentProgram = name;    // errors now refer to the sub-program
    m_forStack.clear();
    m_pc = 0;
    return RunStatus::Running;
  }

  if (matchKeyword(stmt, "Return")) {
    // Return to the caller; in the main program it ends the run.
    if (!returnFromCall())
      m_stopRequested = true;
    return RunStatus::Running;
  }

  // ─────────────────────── P2 statements ───────────────────────
  if (matchKeyword(stmt, "ClrHome")) {
    m_output.clear();
    ++m_pc;
    return RunStatus::Running;
  }
  if (matchKeyword(stmt, "DelVar")) {
    const std::string v = trim(stmt.substr(6));
    if (v.empty())
      return fail("ERR:SYNTAX");
    if (const int n = strVarIndex(v))
      m_strVars.erase(n);  // clear a string variable
    else
      mEval("0->" + v);  // scalar: reset to 0 (our "deleted" state)
    ++m_pc;
    return RunStatus::Running;
  }
  if (matchKeyword(stmt, "Stop")) {
    m_stopRequested = true;
    ++m_pc;
    return RunStatus::Running;
  }
  if (matchKeyword(stmt, "Input")) {
    // Input VAR  |  Input "prompt",VAR  — pause for a value into VAR.
    const std::string rest = trim(stmt.substr(5));
    if (rest.empty())
      return fail("ERR:SYNTAX");
    if (rest.front() == '"') {
      const auto args = splitArgs(rest);
      if (args.size() < 2)
        return fail("ERR:SYNTAX");
      std::string p = trim(args[0]);
      if (p.size() >= 2 && p.front() == '"' && p.back() == '"')
        p = p.substr(1, p.size() - 2);
      m_inputPrompt = p;
      m_inputVar = trim(args[1]);
    } else {
      m_inputPrompt = "?";
      m_inputVar = rest;
    }
    m_inputIsString = (strVarIndex(m_inputVar) != 0);
    return RunStatus::NeedInput;  // pc stays; provideInput() advances
  }

  if (matchKeyword(stmt, "Prompt")) {
    // Prompt VAR — auto-labelled "VAR=?" (single variable for now).
    const std::string var = trim(stmt.substr(6));
    if (var.empty())
      return fail("ERR:SYNTAX");
    m_inputVar = var;
    m_inputPrompt = var + "=?";
    m_inputIsString = (strVarIndex(m_inputVar) != 0);
    return RunStatus::NeedInput;
  }

  if (matchKeyword(stmt, "Pause")) {
    // Optional argument is displayed, then the run waits for a keypress.
    const std::string rest = trim(stmt.substr(5));
    if (!rest.empty()) {
      if (rest.front() == '"') {
        std::string inner = rest.substr(1);
        if (!inner.empty() && inner.back() == '"')
          inner.pop_back();
        m_output.push_back(inner);
      } else {
        const EvalResult r = mEval(rest);
        if (!r.ok)
          return fail(r.error);
        m_output.push_back(r.display);
      }
    }
    return RunStatus::NeedKey;  // pc stays; resumeFromPause() advances
  }
  if (matchKeyword(stmt, "Disp")) {
    const std::string rest = trim(stmt.substr(4));
    if (rest.empty()) {
      m_output.push_back("");
      ++m_pc;
      return RunStatus::Running;
    }
    for (const std::string &raw : splitArgs(rest)) {
      const std::string arg = trim(raw);
      if (arg.empty()) {
        m_output.push_back("");
        continue;
      }
      // String expression (literal / StrN / concat) prints its text;
      // otherwise evaluate numerically.
      std::string sout;
      const StrEval se = evalStringExpr(arg, sout);
      if (se == StrEval::Ok) {
        m_output.push_back(sout);
        continue;
      }
      if (se == StrEval::TypeError)
        return fail(m_strFuncError.empty() ? "ERR:DATA TYPE" : m_strFuncError);
      const EvalResult r = mEval(arg);
      if (!r.ok)
        return fail(r.error);
      m_output.push_back(r.display);
    }
    ++m_pc;
    return RunStatus::Running;
  }

  if (matchKeyword(stmt, "Output")) {
    // Output(row, col, value) — positioned text on the home-screen grid
    // (1-based, rows 1..8, cols 1..16).
    const auto lp = stmt.find('(');
    const auto rp = stmt.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp)
      return fail("ERR:SYNTAX");
    const auto args = splitArgs(stmt.substr(lp + 1, rp - lp - 1));
    if (args.size() != 3)
      return fail("ERR:ARGUMENT");
    const EvalResult rr = mEval(trim(args[0]));
    if (!rr.ok)
      return fail(rr.error);
    const EvalResult cc = mEval(trim(args[1]));
    if (!cc.ok)
      return fail(cc.error);
    const long row = std::lround(rr.value);
    const long col = std::lround(cc.value);
    if (row < 1 || row > 8 || col < 1 || col > 16)
      return fail("ERR:DOMAIN");
    // The value prints as a string (literal / StrN / sub) or a number.
    std::string text, sout;
    const StrEval se = evalStringExpr(trim(args[2]), sout);
    if (se == StrEval::Ok) {
      text = sout;
    } else if (se == StrEval::TypeError) {
      return fail(m_strFuncError.empty() ? "ERR:DATA TYPE" : m_strFuncError);
    } else {
      const EvalResult v = mEval(trim(args[2]));
      if (!v.ok)
        return fail(v.error);
      text = v.display;
    }
    placeOutput(static_cast<int>(row), static_cast<int>(col), text);
    ++m_pc;
    return RunStatus::Running;
  }

  // ── String store: <str expr>→StrN ──
  {
    std::string lhs;
    if (const int n = stringStoreTarget(stmt, lhs)) {
      std::string sout;
      const StrEval se = evalStringExpr(lhs, sout);
      if (se != StrEval::Ok)  // can't store a non-string into StrN
        return fail(m_strFuncError.empty() ? "ERR:DATA TYPE" : m_strFuncError);
      m_strVars[n] = sout;
      // A store runs silently (TI-style) — no echo.
      ++m_pc;
      return RunStatus::Running;
    }
  }

  // ── Bare string expression (e.g. Str1) ── echo its text. ──
  {
    std::string sout;
    const StrEval se = evalStringExpr(stmt, sout);
    if (se == StrEval::Ok) {
      m_output.push_back(sout);
      ++m_pc;
      return RunStatus::Running;
    }
    if (se == StrEval::TypeError)
      return fail(m_strFuncError.empty() ? "ERR:DATA TYPE" : m_strFuncError);
  }

  // ── Bare numeric expression / Sto ──
  // Evaluate for its value + side effects. A bare expression echoes its
  // result (like the home screen); an assignment (`…→var`) is silent, as on
  // the TI-83 — otherwise a `getKey→K` poll loop floods the screen.
  const EvalResult r = mEval(stmt);
  if (!r.ok)
    return fail(r.error);
  if (!isAssignment(stmt))
    m_output.push_back(r.display);
  ++m_pc;
  return RunStatus::Running;
}

RunStatus Interpreter::step() {
  if (m_status != RunStatus::Running)
    return m_status;
  // Past the end of the current (sub)program: return to the caller if there
  // is one (loop, in case the caller was also at its end), else finish.
  while (m_pc >= m_statements.size()) {
    if (!returnFromCall()) {
      m_status = RunStatus::Done;
      return m_status;
    }
  }

  const RunStatus st = execStatement(m_statements[m_pc]);
  if (st == RunStatus::Error) {
    m_status = RunStatus::Error;
    return m_status;
  }
  if (st == RunStatus::NeedInput || st == RunStatus::NeedKey) {
    m_status = st;  // pause for interaction (pc unchanged)
    return m_status;
  }
  if (m_stopRequested)  // Stop, or Return in the main program: end everything
    m_status = RunStatus::Done;
  return m_status;  // next step() handles a natural end-of-program
}

void Interpreter::provideInput(const std::string &valueSource) {
  if (m_status != RunStatus::NeedInput)
    return;
  if (m_inputIsString) {
    // Input into a StrN stores the raw typed text (no evaluation).
    m_strVars[strVarIndex(m_inputVar)] = valueSource;
  } else {
    const EvalResult r = mEval("(" + valueSource + ")->" + m_inputVar);
    if (!r.ok)
      return;  // invalid value — stay NeedInput so the caller re-prompts
  }
  ++m_pc;  // past the Input/Prompt statement
  m_status = RunStatus::Running;
}

void Interpreter::resumeFromPause() {
  if (m_status != RunStatus::NeedKey)
    return;
  ++m_pc;
  m_status = RunStatus::Running;
}

namespace {
// Lifetime step ceiling — a backstop against a runaway loop hanging a
// headless caller (CLI / tests). It lives in run() only; the GUI drives
// runSlice() (which is deliberately unguarded) and relies on the user's
// STOP button instead, so an interactive getKey loop isn't cut off.
constexpr long kMaxSteps = 5'000'000;
}  // namespace

RunStatus Interpreter::runSlice(long maxSteps) {
  // Bounded, unguarded stepping — the caller yields between slices and owns
  // the stop policy (see UIController::stepProgramToPause).
  while (m_status == RunStatus::Running && maxSteps-- > 0)
    step();
  return m_status;
}

RunStatus Interpreter::run() {
  // Blocking run to completion/pause, with a runaway-loop backstop for
  // headless callers (CLI / tests) that have no interactive break.
  long guard = 0;
  while (m_status == RunStatus::Running) {
    if (++guard > kMaxSteps) {
      m_errorLine = static_cast<int>(m_pc);
      m_errorMessage = "ERR:BREAK";
      m_status = RunStatus::Error;
      break;
    }
    step();
  }
  return m_status;
}

void Interpreter::interrupt() {
  if (m_status == RunStatus::Running) {
    m_errorLine = static_cast<int>(m_pc);
    m_errorMessage = "ERR:BREAK";
    m_status = RunStatus::Error;
  }
}

// ── ProgramStore ──────────────────────────────────────────────────────

void ProgramStore::put(const std::string &name,
                       const std::vector<std::string> &lines) {
  m_programs[name] = lines;
}

bool ProgramStore::has(const std::string &name) const {
  return m_programs.find(name) != m_programs.end();
}

const std::vector<std::string> *ProgramStore::get(
    const std::string &name) const {
  auto it = m_programs.find(name);
  return (it == m_programs.end()) ? nullptr : &it->second;
}

bool ProgramStore::remove(const std::string &name) {
  return m_programs.erase(name) > 0;
}

std::vector<std::string> ProgramStore::names() const {
  std::vector<std::string> out;
  out.reserve(m_programs.size());
  for (const auto &kv : m_programs)
    out.push_back(kv.first);
  std::sort(out.begin(), out.end());  // std::map is already ordered, but be explicit
  return out;
}

}  // namespace tux_ti83

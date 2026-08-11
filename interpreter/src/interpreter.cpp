// TI-BASIC interpreter — P0 scaffolding. See interpreter.hpp / docs/TIBASIC.md.

#include "interpreter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

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
  static const char *const kNames[] = {"length", "inString", "sub", "expr",
                                       "toString"};
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

// Format a double as a source literal that round-trips exactly (for restoring
// a saved Local value through the string-based evaluator).
std::string numLiteral(double v) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.17g", v);
  return buf;
}

// Drop a trailing `#…` comment from a source line (everything from the first
// top-level '#' to end of line), leaving any '#' inside a "…" string alone.
std::string stripComment(const std::string &line) {
  bool inStr = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    if (line[i] == '"')
      inStr = !inStr;
    else if (line[i] == '#' && !inStr)
      return line.substr(0, i);
  }
  return line;
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
    // Strip a `#` comment first (per line), then split into statements.
    auto stmts = splitStatements(stripComment(lines[static_cast<size_t>(lineNo)]));
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

void Interpreter::restoreLocals() {
  // Write each Local back to its saved value (reverse order handles a var
  // declared Local more than once), then clear.
  for (auto it = m_locals.rbegin(); it != m_locals.rend(); ++it)
    if (m_eval)
      m_eval("(" + numLiteral(it->second) + ")->" + it->first);
  m_locals.clear();
}

bool Interpreter::returnFromCall() {
  if (m_callStack.empty())
    return false;
  restoreLocals();  // this sub-program's locals go back to their saved values
  CallFrame f = std::move(m_callStack.back());
  m_callStack.pop_back();
  // Discard any Try blocks that belonged to the returning sub-program.
  while (!m_tryStack.empty() && m_tryStack.back().callDepth > m_callStack.size())
    m_tryStack.pop_back();
  m_locals = std::move(f.locals);  // caller's locals
  m_statements = std::move(f.statements);
  m_statementSrcLine = std::move(f.srcLine);
  m_currentProgram = std::move(f.program);
  m_pc = f.pc;
  m_openerToEnd = std::move(f.openerToEnd);
  m_thenToElse = std::move(f.thenToElse);
  m_elseToEnd = std::move(f.elseToEnd);
  m_endToOpener = std::move(f.endToOpener);
  m_enclosingLoop = std::move(f.enclosingLoop);
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
  m_enclosingLoop.assign(static_cast<size_t>(n), -1);
  m_labels.clear();
  std::vector<int> stack;      // open Then/For/While/Repeat indices
  std::vector<int> loopStack;  // open For/While/Repeat indices (for break/continue)
  auto isLoop = [&](int idx) {
    const std::string &o = m_statements[static_cast<size_t>(idx)];
    return matchKeyword(o, "For") || matchKeyword(o, "While") ||
           matchKeyword(o, "Repeat");
  };
  for (int i = 0; i < n; ++i) {
    // Innermost enclosing loop for this statement (before it opens/closes one).
    m_enclosingLoop[static_cast<size_t>(i)] =
        loopStack.empty() ? -1 : loopStack.back();
    const std::string &s = m_statements[static_cast<size_t>(i)];
    if (matchKeyword(s, "Lbl")) {
      m_labels[trim(s.substr(3))] = i;
    } else if (matchKeyword(s, "For") || matchKeyword(s, "While") ||
               matchKeyword(s, "Repeat")) {
      stack.push_back(i);
      loopStack.push_back(i);
    } else if (matchKeyword(s, "Then") || matchKeyword(s, "Define") ||
               matchKeyword(s, "Try")) {
      stack.push_back(i);  // non-loop opener (matched to its End)
    } else if (matchKeyword(s, "Else")) {
      if (!stack.empty() &&
          (matchKeyword(m_statements[static_cast<size_t>(stack.back())],
                        "Then") ||
           matchKeyword(m_statements[static_cast<size_t>(stack.back())],
                        "Try")))
        m_thenToElse[static_cast<size_t>(stack.back())] = i;  // opener → Else
    } else if (matchKeyword(s, "End")) {
      if (!stack.empty()) {
        const int opener = stack.back();
        stack.pop_back();
        if (isLoop(opener) && !loopStack.empty())
          loopStack.pop_back();
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

std::string Interpreter::storeTargetName(const std::string &stmt,
                                         std::string &lhs) {
  // Last top-level store arrow ("->"/"→", not inside a string) → target name.
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
    return {};
  lhs = trim(stmt.substr(0, arrowPos));
  return trim(stmt.substr(arrowPos + arrowLen));
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
    } else if (name == "toString") {
      // toString(number) → its display text as a "…" literal (P7).
      if (args.size() != 1) {
        m_strFuncError = "ERR:ARGUMENT";
        break;
      }
      const EvalResult r = m_eval(trim(args[0]));
      if (!r.ok) {
        m_strFuncError = r.error.empty() ? "ERR:DATA TYPE" : r.error;
        break;
      }
      repl = "\"" + r.display + "\"";
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
  m_locals.clear();
  m_tryStack.clear();
  m_returnValue = 0.0;
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

  if (matchKeyword(stmt, "Try")) {
    // Begin a protected block: on an error inside it, execution jumps to the
    // Else handler (or past End if none) instead of halting (P7).
    const int endIdx = m_openerToEnd[here];
    if (endIdx < 0)
      return fail("ERR:SYNTAX");
    TryFrame tf;
    tf.elseIdx = m_thenToElse[here];
    tf.endIdx = endIdx;
    tf.forDepth = m_forStack.size();
    tf.localDepth = m_locals.size();
    tf.callDepth = m_callStack.size();
    m_tryStack.push_back(tf);
    ++m_pc;  // enter the try body
    return RunStatus::Running;
  }

  if (matchKeyword(stmt, "Else")) {
    // Reached by falling through (If's true branch, or a Try body that
    // finished without error) — skip past the End. A Try that completes
    // cleanly pops its frame here so its handler is skipped.
    if (!m_tryStack.empty() && m_tryStack.back().elseIdx == static_cast<int>(here))
      m_tryStack.pop_back();
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

  // break / continue — jump out of / to the end of the innermost loop.
  if (matchKeyword(stmt, "break") || matchKeyword(stmt, "continue")) {
    const int opener =
        (here < m_enclosingLoop.size()) ? m_enclosingLoop[here] : -1;
    if (opener < 0)
      return fail("ERR:SYNTAX");  // not inside a loop
    const int endIdx = m_openerToEnd[static_cast<std::size_t>(opener)];
    if (endIdx < 0)
      return fail("ERR:SYNTAX");
    if (matchKeyword(stmt, "continue")) {
      m_pc = static_cast<std::size_t>(endIdx);  // End re-tests / increments
    } else {  // break — leave the loop
      if (matchKeyword(m_statements[static_cast<std::size_t>(opener)], "For") &&
          !m_forStack.empty())
        m_forStack.pop_back();  // discard this For's frame
      m_pc = static_cast<std::size_t>(endIdx) + 1;
    }
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
      // If-Then or Try block — just continue. A Try with no Else that
      // finished cleanly pops its frame here.
      if (matchKeyword(op, "Try") && !m_tryStack.empty() &&
          m_tryStack.back().endIdx == static_cast<int>(here))
        m_tryStack.pop_back();
      ++m_pc;
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
                           m_endToOpener, m_enclosingLoop, m_labels, m_forStack,
                           m_locals});
    loadStatements(*subLines);  // sub's statements + fresh control tables
    m_currentProgram = name;    // errors now refer to the sub-program
    m_forStack.clear();
    m_locals.clear();           // the sub starts with its own locals
    m_pc = 0;
    return RunStatus::Running;
  }

  if (matchKeyword(stmt, "Return")) {
    // `Return expr` sets the function's return value (P7-B3); bare `Return`
    // just returns. Either way: to the caller, or ends the run in the main.
    const std::string rest = trim(stmt.substr(6));
    if (!rest.empty()) {
      const EvalResult r = mEval(rest);
      if (!r.ok)
        return fail(r.error);
      m_returnValue = r.value;
    }
    if (!returnFromCall())
      m_stopRequested = true;
    return RunStatus::Running;
  }

  if (matchKeyword(stmt, "Define")) {
    // Define name(params) … End — register the function body, then skip it
    // (the body only runs when the function is called). P7-B3.
    const int endIdx = m_openerToEnd[here];
    if (endIdx < 0)
      return fail("ERR:SYNTAX");
    const std::string rest = trim(stmt.substr(6));
    const auto lp = rest.find('(');
    const auto rp = rest.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp)
      return fail("ERR:SYNTAX");
    const std::string name = trim(rest.substr(0, lp));
    std::vector<std::string> params;
    const std::string paramStr = trim(rest.substr(lp + 1, rp - lp - 1));
    if (!paramStr.empty())
      for (const std::string &p : splitArgs(paramStr))
        params.push_back(trim(p));
    std::vector<std::string> body(
        m_statements.begin() + static_cast<long>(here) + 1,
        m_statements.begin() + endIdx);
    if (m_defineSink)
      m_defineSink(name, params, body);
    m_pc = static_cast<std::size_t>(endIdx) + 1;  // skip the body
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
  if (matchKeyword(stmt, "Local")) {
    // Local A,B,… — save each scalar's value (restored when the frame exits)
    // and start it fresh at 0, so a sub-program can't clobber the caller (P7).
    const std::string rest = trim(stmt.substr(5));
    if (rest.empty())
      return fail("ERR:SYNTAX");
    for (const std::string &raw : splitArgs(rest)) {
      const std::string v = trim(raw);
      if (v.empty() || strVarIndex(v) != 0)
        return fail("ERR:SYNTAX");  // scalar variables only
      m_locals.push_back({v, mEval(v).value});
      mEval("0->" + v);
    }
    ++m_pc;
    return RunStatus::Running;
  }
  if (matchKeyword(stmt, "Stop")) {
    m_stopRequested = true;
    ++m_pc;
    return RunStatus::Running;
  }
  if (matchKeyword(stmt, "SortA") || matchKeyword(stmt, "SortD")) {
    // Sort a list in place (the controller mutates the registry) — a command,
    // so no echo (P7-A1).
    const EvalResult r = mEval(stmt);
    if (!r.ok)
      return fail(r.error);
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

  if (matchKeyword(stmt, "Menu")) {
    // Menu("title","opt1",Lbl1,"opt2",Lbl2,…) — show a menu, pause for a
    // choice, then jump to the chosen option's Lbl (a Goto, not a call).
    const auto lp = stmt.find('(');
    const auto rp = stmt.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp)
      return fail("ERR:SYNTAX");
    const auto args = splitArgs(stmt.substr(lp + 1, rp - lp - 1));
    // Title + N (option, label) pairs → an odd count of at least 3.
    if (args.size() < 3 || args.size() % 2 == 0)
      return fail("ERR:ARGUMENT");
    // Resolve a display arg as a string (literal / StrN / sub) or a number.
    auto asText = [&](const std::string &a, std::string &out) -> bool {
      std::string s;
      const StrEval se = evalStringExpr(trim(a), s);
      if (se == StrEval::Ok) {
        out = s;
        return true;
      }
      if (se == StrEval::TypeError)
        return false;
      const EvalResult v = mEval(trim(a));
      if (!v.ok)
        return false;
      out = v.display;
      return true;
    };
    std::string title;
    if (!asText(args[0], title))
      return fail(m_strFuncError.empty() ? "ERR:DATA TYPE" : m_strFuncError);
    m_menuTitle = title;
    m_menuOptions.clear();
    m_menuLabels.clear();
    for (std::size_t i = 1; i + 1 < args.size(); i += 2) {
      std::string opt;
      if (!asText(args[i], opt))
        return fail(m_strFuncError.empty() ? "ERR:DATA TYPE" : m_strFuncError);
      m_menuOptions.push_back(opt);
      m_menuLabels.push_back(trim(args[i + 1]));  // Lbl name, as written
    }
    return RunStatus::NeedMenu;  // pc stays; provideMenuChoice() jumps
  }

  // ── Graph statements (P6): dispatched to the injected graph sink ──
  if (matchKeyword(stmt, "DispGraph")) {
    GraphCmd c;
    c.kind = GraphCmd::Kind::DispGraph;
    if (!m_graphSink || !m_graphSink(c))
      return fail("ERR:UNDEFINED");
    ++m_pc;
    return RunStatus::Running;
  }
  if (matchKeyword(stmt, "ZStandard") || matchKeyword(stmt, "ZoomFit")) {
    GraphCmd c;
    c.kind = GraphCmd::Kind::Zoom;
    c.arg = matchKeyword(stmt, "ZStandard") ? "Standard" : "Fit";
    if (!m_graphSink || !m_graphSink(c))
      return fail("ERR:UNDEFINED");
    ++m_pc;
    return RunStatus::Running;
  }
  if (matchKeyword(stmt, "FnOn") || matchKeyword(stmt, "FnOff")) {
    const bool on = matchKeyword(stmt, "FnOn");
    const std::string rest =
        trim(stmt.substr(on ? 4 : 5));  // args after FnOn / FnOff
    if (!m_graphSink)
      return fail("ERR:UNDEFINED");
    if (rest.empty()) {  // no args → all functions
      GraphCmd c;
      c.kind = on ? GraphCmd::Kind::FnOn : GraphCmd::Kind::FnOff;
      c.slot = -1;
      if (!m_graphSink(c))
        return fail("ERR:UNDEFINED");
    } else {
      for (const std::string &a : splitArgs(rest)) {  // FnOn 1,2,3
        const EvalResult r = mEval(trim(a));
        if (!r.ok)
          return fail(r.error);
        const int num = static_cast<int>(std::lround(r.value));
        const int slot = (num == 0) ? 9 : (num - 1);  // Y1..Y9→0..8, Y0→9
        if (slot < 0 || slot > 9)
          return fail("ERR:DOMAIN");
        GraphCmd c;
        c.kind = on ? GraphCmd::Kind::FnOn : GraphCmd::Kind::FnOff;
        c.slot = slot;
        if (!m_graphSink(c))
          return fail("ERR:UNDEFINED");
      }
    }
    ++m_pc;
    return RunStatus::Running;
  }

  // ── Draw overlay (P6-2): graphics commands routed to the graph sink ──
  if (matchKeyword(stmt, "ClrDraw")) {
    GraphCmd c;
    c.kind = GraphCmd::Kind::ClrDraw;
    if (!m_graphSink || !m_graphSink(c))
      return fail("ERR:UNDEFINED");
    ++m_pc;
    return RunStatus::Running;
  }
  // Name(args…) with `n` numeric args → a draw command.
  auto drawCall = [&](GraphCmd::Kind kind, std::size_t n) -> RunStatus {
    const auto lp = stmt.find('(');
    const auto rp = stmt.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp)
      return fail("ERR:SYNTAX");
    const auto args = splitArgs(stmt.substr(lp + 1, rp - lp - 1));
    if (args.size() != n)
      return fail("ERR:ARGUMENT");
    GraphCmd c;
    c.kind = kind;
    for (const auto &a : args) {
      const EvalResult r = mEval(trim(a));
      if (!r.ok)
        return fail(r.error);
      c.nums.push_back(r.value);
    }
    if (!m_graphSink || !m_graphSink(c))
      return fail("ERR:UNDEFINED");
    ++m_pc;
    return RunStatus::Running;
  };
  if (matchKeyword(stmt, "Line"))
    return drawCall(GraphCmd::Kind::DrawLine, 4);
  if (matchKeyword(stmt, "Circle"))
    return drawCall(GraphCmd::Kind::DrawCircle, 3);
  if (matchKeyword(stmt, "Pt-On"))
    return drawCall(GraphCmd::Kind::DrawPoint, 2);
  if (matchKeyword(stmt, "Pt-Off"))
    return drawCall(GraphCmd::Kind::PtOff, 2);
  if (matchKeyword(stmt, "Pt-Change"))
    return drawCall(GraphCmd::Kind::PtChange, 2);
  if (matchKeyword(stmt, "Pxl-On"))
    return drawCall(GraphCmd::Kind::PxlOn, 2);
  if (matchKeyword(stmt, "Pxl-Off"))
    return drawCall(GraphCmd::Kind::PxlOff, 2);
  if (matchKeyword(stmt, "Horizontal") || matchKeyword(stmt, "Vertical")) {
    const bool horiz = matchKeyword(stmt, "Horizontal");
    const std::string rest = trim(stmt.substr(horiz ? 10 : 8));
    if (rest.empty())
      return fail("ERR:ARGUMENT");
    const EvalResult r = mEval(rest);
    if (!r.ok)
      return fail(r.error);
    GraphCmd c;
    c.kind = horiz ? GraphCmd::Kind::DrawHorizontal : GraphCmd::Kind::DrawVertical;
    c.nums.push_back(r.value);
    if (!m_graphSink || !m_graphSink(c))
      return fail("ERR:UNDEFINED");
    ++m_pc;
    return RunStatus::Running;
  }
  if (matchKeyword(stmt, "Text")) {
    // Text(x, y, value…) — position (graph coords) then text (string/number).
    const auto lp = stmt.find('(');
    const auto rp = stmt.rfind(')');
    if (lp == std::string::npos || rp == std::string::npos || rp <= lp)
      return fail("ERR:SYNTAX");
    const auto args = splitArgs(stmt.substr(lp + 1, rp - lp - 1));
    if (args.size() < 3)
      return fail("ERR:ARGUMENT");
    GraphCmd c;
    c.kind = GraphCmd::Kind::DrawText;
    for (int i = 0; i < 2; ++i) {
      const EvalResult r = mEval(trim(args[static_cast<std::size_t>(i)]));
      if (!r.ok)
        return fail(r.error);
      c.nums.push_back(r.value);
    }
    std::string text;
    for (std::size_t i = 2; i < args.size(); ++i) {
      std::string sout;
      const StrEval se = evalStringExpr(trim(args[i]), sout);
      if (se == StrEval::Ok)
        text += sout;
      else if (se == StrEval::TypeError)
        return fail(m_strFuncError.empty() ? "ERR:DATA TYPE" : m_strFuncError);
      else {
        const EvalResult r = mEval(trim(args[i]));
        if (!r.ok)
          return fail(r.error);
        text += r.display;
      }
    }
    c.arg = text;
    if (!m_graphSink || !m_graphSink(c))
      return fail("ERR:UNDEFINED");
    ++m_pc;
    return RunStatus::Running;
  }

  // ── Graph stores (P6): <expr>→Yn  and  <value>→<window var> ──
  {
    std::string lhs;
    const std::string tgt = storeTargetName(stmt, lhs);
    if (tgt.size() == 2 && tgt[0] == 'Y' &&
        ((tgt[1] >= '1' && tgt[1] <= '9') || tgt[1] == '0')) {
      const int slot = (tgt[1] == '0') ? 9 : (tgt[1] - '1');
      // Accept both `"X²"→Y1` (TI-style, quoted) and `X²→Y1` (bare): strip a
      // surrounding pair of quotes so the controller sees the expression.
      std::string fexpr = lhs;
      if (fexpr.size() >= 2 && fexpr.front() == '"' && fexpr.back() == '"')
        fexpr = fexpr.substr(1, fexpr.size() - 2);
      GraphCmd c;
      c.kind = GraphCmd::Kind::SetFunc;
      c.slot = slot;
      c.arg = fexpr;  // the controller tokenises + stores it
      if (!m_graphSink || !m_graphSink(c))
        return fail(m_graphSink ? "ERR:SYNTAX" : "ERR:UNDEFINED");
      ++m_pc;
      return RunStatus::Running;
    }
    static const char *const kWindowVars[] = {"Xmin", "Xmax", "Ymin",
                                              "Ymax", "Xscl", "Yscl"};
    for (const char *w : kWindowVars) {
      if (tgt == w) {
        const EvalResult v = mEval(lhs);
        if (!v.ok)
          return fail(v.error);
        GraphCmd c;
        c.kind = GraphCmd::Kind::SetWindow;
        c.arg = tgt;
        c.value = v.value;
        if (!m_graphSink || !m_graphSink(c))
          return fail("ERR:UNDEFINED");
        ++m_pc;
        return RunStatus::Running;
      }
    }
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
      restoreLocals();  // main program's Local variables
      m_status = RunStatus::Done;
      return m_status;
    }
  }

  const RunStatus st = execStatement(m_statements[m_pc]);
  if (st == RunStatus::Error) {
    // A `Try` block catches the error: unwind back to it and run its handler
    // instead of halting (P7). Nearest Try = top of the stack.
    if (!m_tryStack.empty()) {
      const TryFrame tf = m_tryStack.back();
      m_tryStack.pop_back();
      while (m_callStack.size() > tf.callDepth)
        returnFromCall();  // unwind sub-program frames
      while (m_forStack.size() > tf.forDepth)
        m_forStack.pop_back();
      while (m_locals.size() > tf.localDepth) {  // restore locals opened in Try
        if (m_eval)
          m_eval("(" + numLiteral(m_locals.back().second) + ")->" +
                 m_locals.back().first);
        m_locals.pop_back();
      }
      m_errorLine = -1;  // swallow the error
      m_errorMessage.clear();
      m_pc = (tf.elseIdx >= 0) ? static_cast<std::size_t>(tf.elseIdx) + 1
                               : static_cast<std::size_t>(tf.endIdx) + 1;
      return m_status;  // still Running
    }
    m_status = RunStatus::Error;
    return m_status;
  }
  if (st == RunStatus::NeedInput || st == RunStatus::NeedKey ||
      st == RunStatus::NeedMenu) {
    m_status = st;  // pause for interaction (pc unchanged)
    return m_status;
  }
  if (m_stopRequested) {  // Stop, or Return in the main program: end everything
    restoreLocals();
    m_status = RunStatus::Done;
  }
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

void Interpreter::provideMenuChoice(int index) {
  if (m_status != RunStatus::NeedMenu)
    return;
  if (index < 0 || index >= static_cast<int>(m_menuLabels.size()))
    return;  // out-of-range pick — stay in the menu
  const auto it = m_labels.find(m_menuLabels[static_cast<std::size_t>(index)]);
  if (it == m_labels.end()) {
    m_errorLine = static_cast<int>(m_pc);
    m_errorMessage = "ERR:LABEL";
    m_status = RunStatus::Error;
    return;
  }
  m_pc = static_cast<std::size_t>(it->second);  // jump to the option's Lbl
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

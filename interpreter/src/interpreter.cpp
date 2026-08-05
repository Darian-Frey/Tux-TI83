// TI-BASIC interpreter — P0 scaffolding. See interpreter.hpp / docs/TIBASIC.md.

#include "interpreter.hpp"

#include <algorithm>

namespace tux_ti83 {

namespace {

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
  for (char c : line) {
    if (c == ':') {
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

void Interpreter::load(const std::vector<std::string> &lines) {
  m_statements.clear();
  for (const auto &line : lines) {
    auto stmts = splitStatements(line);
    m_statements.insert(m_statements.end(), stmts.begin(), stmts.end());
  }
  reset();
}

std::vector<std::string> Interpreter::splitArgs(const std::string &s) {
  std::vector<std::string> out;
  std::string cur;
  int depth = 0;  // nesting depth across () [] {}
  for (char c : s) {
    if (c == '(' || c == '[' || c == '{')
      ++depth;
    else if (c == ')' || c == ']' || c == '}')
      --depth;
    if (c == ',' && depth <= 0) {
      out.push_back(cur);
      cur.clear();
    } else {
      cur.push_back(c);
    }
  }
  out.push_back(cur);
  return out;
}

void Interpreter::reset() {
  m_pc = 0;
  m_output.clear();
  m_errorLine = -1;
  m_errorMessage.clear();
  m_stopRequested = false;
  m_status = RunStatus::Running;
}

RunStatus Interpreter::execStatement(const std::string &stmt) {
  // Without an evaluator the interpreter can't compute — treat every
  // statement as a no-op (the P0 behaviour; keeps structural tests valid).
  if (!m_eval)
    return RunStatus::Running;

  // ── Keyword statements ──
  if (matchKeyword(stmt, "ClrHome")) {
    m_output.clear();
    return RunStatus::Running;
  }
  if (matchKeyword(stmt, "Stop")) {
    m_stopRequested = true;
    return RunStatus::Running;
  }
  if (matchKeyword(stmt, "Pause")) {
    // P2: no-op. Real Pause (display + wait for a keypress) lands with the
    // interaction phase (P4/P5), which shares the resumable-input UI.
    return RunStatus::Running;
  }
  if (matchKeyword(stmt, "Disp")) {
    // Everything after "Disp", split on top-level commas → one line each.
    const std::string rest = trim(stmt.substr(4));
    if (rest.empty()) {
      m_output.push_back("");
      return RunStatus::Running;
    }
    for (const std::string &raw : splitArgs(rest)) {
      const std::string arg = trim(raw);
      if (arg.empty()) {
        m_output.push_back("");
        continue;
      }
      // Mini-string support: a quoted literal is printed verbatim (the full
      // string type — Str1..Str9, concat, sub( — arrives in P4).
      if (arg.front() == '"') {
        std::string inner = arg.substr(1);
        if (!inner.empty() && inner.back() == '"')
          inner.pop_back();
        m_output.push_back(inner);
        continue;
      }
      const EvalResult r = m_eval(arg);
      if (!r.ok) {
        m_errorLine = static_cast<int>(m_pc);
        m_errorMessage = r.error;
        return RunStatus::Error;
      }
      m_output.push_back(r.display);
    }
    return RunStatus::Running;
  }

  // ── Bare expression / Sto ──
  // Evaluate; Sto's side effects happen inside the evaluator. Like the
  // TI-83, an expression statement echoes its result to the output.
  const EvalResult r = m_eval(stmt);
  if (!r.ok) {
    m_errorLine = static_cast<int>(m_pc);
    m_errorMessage = r.error;
    return RunStatus::Error;
  }
  m_output.push_back(r.display);
  return RunStatus::Running;
}

RunStatus Interpreter::step() {
  if (m_status != RunStatus::Running)
    return m_status;
  if (m_pc >= m_statements.size()) {
    m_status = RunStatus::Done;
    return m_status;
  }

  const RunStatus st = execStatement(m_statements[m_pc]);
  if (st == RunStatus::Error) {
    m_status = RunStatus::Error;
    return m_status;
  }

  ++m_pc;
  if (m_stopRequested || m_pc >= m_statements.size())
    m_status = RunStatus::Done;
  return m_status;
}

RunStatus Interpreter::run() {
  while (m_status == RunStatus::Running)
    step();
  return m_status;
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

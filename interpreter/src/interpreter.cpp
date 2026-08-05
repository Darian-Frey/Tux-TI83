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

void Interpreter::reset() {
  m_pc = 0;
  m_output.clear();
  m_errorLine = -1;
  m_status = RunStatus::Running;
}

RunStatus Interpreter::step() {
  if (m_status != RunStatus::Running)
    return m_status;
  if (m_pc >= m_statements.size()) {
    m_status = RunStatus::Done;
    return m_status;
  }

  // P0: statements are not executed yet — advance the program counter as a
  // no-op. Statement dispatch (expressions / Sto / Disp / control flow /
  // I/O) is added in P2 and later.
  ++m_pc;

  if (m_pc >= m_statements.size())
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

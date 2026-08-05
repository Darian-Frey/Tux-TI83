// TI-BASIC interpreter — a statement-level execution layer above the
// single-expression evaluator (MathStateMachine). See docs/TIBASIC.md for
// the full design and phase plan.
//
// P0 (scaffolding): the program model, the statement splitter, the
// resumable RunStatus step loop, and a named program store. No statements
// are executed yet — running a program simply advances the program counter
// through its statements to `Done`. Real statement dispatch (expressions,
// Sto, Disp, control flow, I/O) arrives in later phases (P2+).
//
// Pure C++ (no Qt) so the CLI/REPL can run programs headless and the test
// suite can exercise it directly.

#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace tux_ti83 {

// Status returned after each execution step. The GUI can't block on input,
// so the interpreter yields one of these at every pause point; the caller
// (UIController, or the CLI) reacts and resumes. In P0 only Running / Done
// / Error are reachable; the I/O statuses are wired up in P4/P5.
enum class RunStatus {
  Running,    // more to do — call step() again
  Output,     // text was appended to the output buffer; render it
  NeedInput,  // Input / Prompt — waiting for a value or string
  NeedKey,    // Pause / getKey / Menu — waiting for a keypress / choice
  Done,       // program finished normally
  Error       // a runtime error occurred (see errorLine())
};

// Executes one program. Holds all execution state (program counter, and —
// in later phases — the block/loop stack, label table, and call stack) in
// the object rather than on the C++ call stack, so execution can suspend at
// an I/O point and resume later.
class Interpreter {
public:
  // Load a program from its source lines. Each line is split on top-level
  // ':' separators into individual statements; blank statements are
  // dropped. Resets execution state so the program is ready to run.
  void load(const std::vector<std::string> &lines);

  // Rewind to the first statement and clear output / error state.
  void reset();

  // Execute a single statement and advance. Returns the new status.
  RunStatus step();

  // Step repeatedly until the program pauses (I/O), finishes, or errors.
  RunStatus run();

  RunStatus status() const { return m_status; }
  const std::vector<std::string> &output() const { return m_output; }
  int errorLine() const { return m_errorLine; }
  std::size_t statementCount() const { return m_statements.size(); }
  std::size_t programCounter() const { return m_pc; }
  const std::vector<std::string> &statements() const { return m_statements; }

  // Split one program line on top-level ':' into trimmed, non-empty
  // statements. Exposed for testing. (Strings can contain ':'; that's
  // handled when the string type lands in P4.)
  static std::vector<std::string> splitStatements(const std::string &line);

private:
  std::vector<std::string> m_statements;  // flattened program
  std::vector<std::string> m_output;      // one entry per Disp/Output line
  std::size_t m_pc = 0;                    // program counter (statement idx)
  RunStatus m_status = RunStatus::Done;
  int m_errorLine = -1;
};

// Named program storage: program name → source lines. Persistence (state
// JSON / named saves) is layered on in the controller when the editor
// lands in P1; this core store is pure C++ so it's independently testable.
class ProgramStore {
public:
  // Create or replace a program.
  void put(const std::string &name, const std::vector<std::string> &lines);

  bool has(const std::string &name) const;
  // Returns nullptr if absent.
  const std::vector<std::string> *get(const std::string &name) const;
  bool remove(const std::string &name);

  // Program names, sorted.
  std::vector<std::string> names() const;
  std::size_t size() const { return m_programs.size(); }
  void clear() { m_programs.clear(); }

private:
  std::map<std::string, std::vector<std::string>> m_programs;
};

}  // namespace tux_ti83

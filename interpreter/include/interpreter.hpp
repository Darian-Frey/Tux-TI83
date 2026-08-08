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
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace tux_ti83 {

// Result of evaluating one source expression, returned by the injected
// Evaluator. The interpreter is pure C++ and can't tokenise/evaluate on its
// own, so the controller (which owns the tokeniser + MathStateMachine +
// formatter) supplies this. `display` is the formatted result string (for
// Disp / echo); `value` is the scalar value (used by conditions/loops in
// P3+). Side effects (Sto writing a variable) happen inside the evaluator.
struct EvalResult {
  bool ok = false;
  double value = 0.0;
  std::string display;  // formatted result (e.g. "25", "[[1,2][3,4]]")
  std::string error;    // "ERR:…" label when !ok
};

// Evaluate a source expression string → EvalResult. Injected by the caller.
using Evaluator = std::function<EvalResult(const std::string &)>;

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
  // Inject the expression evaluator (see Evaluator above). Without one, the
  // interpreter runs every statement as a no-op (the P0 behaviour) — it
  // can't compute anything on its own.
  void setEvaluator(Evaluator e) { m_eval = std::move(e); }

  // Inject a loader that returns a named program's source lines, or
  // std::nullopt if no such program exists (→ ERR:UNDEFINED at the call
  // site). Used by `prgmNAME` sub-program calls (P5).
  void setProgramLoader(
      std::function<std::optional<std::vector<std::string>>(const std::string &)> l) {
    m_progLoader = std::move(l);
  }

  // Load a program from its source lines. Each line is split on top-level
  // ':' separators into individual statements; blank statements are
  // dropped. `name` labels the program for error reporting (jump-to-line).
  // Resets execution state so the program is ready to run.
  void load(const std::vector<std::string> &lines, const std::string &name = "");

  // Rewind to the first statement and clear output / error state.
  void reset();

  // Execute a single statement and advance. Returns the new status.
  RunStatus step();

  // Step repeatedly until the program pauses (I/O), finishes, or errors.
  RunStatus run();

  // Run at most `maxSteps` statements, then return — Running if the budget
  // ran out mid-program (call again to continue), otherwise the terminal
  // or pause status. A lifetime runaway-guard applies across the whole run
  // regardless of slice size. The GUI executes in slices so the UI stays
  // responsive and a user break can interrupt a tight loop (P5b).
  RunStatus runSlice(long maxSteps);

  // Force a Running program to stop now with ERR:BREAK (the user pressed a
  // STOP / ON key). No-op if the program isn't currently Running.
  void interrupt();

  RunStatus status() const { return m_status; }
  const std::vector<std::string> &output() const { return m_output; }
  int errorLine() const { return m_errorLine; }
  const std::string &errorMessage() const { return m_errorMessage; }
  // 0-based source line of the erroring statement in the program that
  // errored (statements are flattened from ':'-chained lines, so this maps
  // back to the editor line). -1 if there's no error. See currentProgram().
  int errorSourceLine() const;
  // Name of the program currently loaded/executing — the one an error line
  // refers to (a sub-program during a prgm call). Empty if unnamed.
  const std::string &currentProgram() const { return m_currentProgram; }

  // ── Resumable interaction (P4) ──
  // When status() is NeedInput, the run paused on Input/Prompt; inputPrompt()
  // is the label to show. Feed the user's entered value (a source expression)
  // to resume: on success it stores into the target variable and advances;
  // on a bad value the run stays NeedInput (re-prompt). When status() is
  // NeedKey (Pause), call resumeFromPause() to continue. After either, call
  // run() again to keep stepping.
  const std::string &inputPrompt() const { return m_inputPrompt; }
  void provideInput(const std::string &valueSource);
  void resumeFromPause();
  std::size_t statementCount() const { return m_statements.size(); }
  std::size_t programCounter() const { return m_pc; }
  const std::vector<std::string> &statements() const { return m_statements; }

  // Split one program line on top-level ':' into trimmed, non-empty
  // statements. Exposed for testing. (Strings can contain ':'; that's
  // handled when the string type lands in P4.)
  static std::vector<std::string> splitStatements(const std::string &line);

  // Split a Disp/argument list on top-level ',' (ignoring commas nested in
  // (), [], {}, and inside "..." strings). Exposed for testing.
  static std::vector<std::string> splitArgs(const std::string &s);

  // "Str1".."Str9" → 1..9; anything else → 0. Exposed for testing.
  static int strVarIndex(const std::string &s);

private:
  // Execute the statement at m_pc. Advances m_pc itself (normal statements
  // ++; control statements jump), so step() never increments. Returns
  // Running, or Error (with m_errorLine / m_errorMessage set). No-op when no
  // evaluator is set.
  RunStatus execStatement(const std::string &stmt);

  // Fill m_statements from source lines (split each on top-level ':') and
  // build the control tables. Shared by load() and prgm sub-calls; unlike
  // load() it does NOT touch output / status / the call stack.
  void loadStatements(const std::vector<std::string> &lines);

  // Pre-pass over the flattened program (structural, no eval): match block
  // openers (Then / For( / While / Repeat) to their Else/End, and collect
  // Lbl targets. Built once in load().
  void buildControlTables();

  // Pop one sub-program call frame, restoring the caller's statements / PC /
  // control tables / For stack. Returns false if the call stack is empty
  // (we're in the main program). See prgmNAME (P5).
  bool returnFromCall();
  // Evaluate a condition expression → {ok, truthy}. Sets error state on
  // failure. Non-zero is true (relational/boolean ops return 1/0).
  bool evalCond(const std::string &expr, bool &ok);
  // Report a runtime error at the current statement and return Error.
  RunStatus fail(const std::string &label);

  // ── Strings (P4b, interpreter-level; the engine stays numeric) ──
  // Try to evaluate a source expression as a string: a '+'-joined chain of
  // "…" literals and Str1..Str9 references. NotString → it's numeric (use
  // the engine); Ok → `out` holds the result; TypeError → mixed str/number.
  enum class StrEval { NotString, Ok, TypeError };
  StrEval evalStringExpr(const std::string &src, std::string &out) const;
  // Split on top-level '+' (respecting quotes and brackets).
  static std::vector<std::string> splitPlus(const std::string &s);
  // If a statement is `<expr>→StrN` / `<expr>->StrN`, return N (1..9) and
  // set `lhs`; else 0.
  static int stringStoreTarget(const std::string &stmt, std::string &lhs);

  // Per-For loop state (endVal/step captured at loop entry, TI-style).
  struct ForFrame {
    std::string var;      // loop variable (e.g. "A")
    double endVal = 0.0;
    double stepVal = 1.0;
    std::string stepSrc;  // original step source, for in-engine increment
    std::size_t bodyStart = 0;
  };

  // One suspended caller, saved when a `prgmNAME` sub-call begins (P5).
  // Globals (variables, lists, strings) are shared across programs the
  // TI-BASIC way, so only the per-program execution state is saved here.
  struct CallFrame {
    std::vector<std::string> statements;
    std::vector<int> srcLine;
    std::string program;
    std::size_t pc = 0;
    std::vector<int> openerToEnd, thenToElse, elseToEnd, endToOpener;
    std::map<std::string, int> labels;
    std::vector<ForFrame> forStack;
  };

  Evaluator m_eval;
  std::function<std::optional<std::vector<std::string>>(const std::string &)>
      m_progLoader;
  std::vector<CallFrame> m_callStack;     // suspended callers (prgmNAME)
  std::vector<std::string> m_statements;  // flattened program
  std::vector<int> m_statementSrcLine;    // source line (0-based) per statement
  std::string m_currentProgram;           // name of the loaded program
  std::vector<std::string> m_output;      // one entry per Disp/Output line
  std::size_t m_pc = 0;                    // program counter (statement idx)
  RunStatus m_status = RunStatus::Done;
  bool m_stopRequested = false;
  int m_errorLine = -1;
  std::string m_errorMessage;
  // Pending Input/Prompt target + prompt label (valid while NeedInput).
  std::string m_inputVar;
  std::string m_inputPrompt;
  bool m_inputIsString = false;              // pending Input targets a StrN
  std::map<int, std::string> m_strVars;      // Str1..Str9 (persist across runs)

  // Control-flow tables (indices into m_statements; -1 = none).
  std::vector<int> m_openerToEnd;   // Then/For/While/Repeat → matching End
  std::vector<int> m_thenToElse;    // Then → its Else (or -1)
  std::vector<int> m_elseToEnd;     // Else → its End (or -1)
  std::vector<int> m_endToOpener;   // End → its opener (or -1)
  std::map<std::string, int> m_labels;  // Lbl name → Lbl statement index
  std::vector<ForFrame> m_forStack;     // active For loops
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

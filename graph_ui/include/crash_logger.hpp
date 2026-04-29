#pragma once
#include <QString>

namespace tux_ti83 {

// Append-only session logger that's also a crash trap. The intent is
// "if the calculator crashes, we want to know exactly what the user did
// in the seconds leading up to it" — so every UIController entry point
// calls `logEvent()`, every event is fsync'd to disk before the call
// returns, and signal/terminate handlers append a final crash marker
// plus backtrace.
//
// Log location: $XDG_STATE_HOME/tux-ti83/session.log (falls back to
// ~/.local/state/tux-ti83/session.log on most Linux desktops). Sessions
// append; delimited by `=== Session start: <iso8601> ===` headers.
//
// All public methods are safe to call from any thread; signal handlers
// use only async-signal-safe APIs (write, backtrace_symbols_fd).
class CrashLogger {
public:
    // Open the log file, write a session-start header, and install
    // signal + std::terminate handlers. Idempotent.
    static void init();

    // Append `event` to the log with a millisecond timestamp prefix,
    // then fsync the fd so the line survives a subsequent crash.
    static void logEvent(const QString& event);

    // Write a clean-exit marker and close the fd. Call from main()
    // just before returning.
    static void shutdown();

    // Diagnostic: returns the absolute path of the active log file,
    // or an empty string if init() hasn't run.
    static QString currentLogPath();
};

} // namespace tux_ti83

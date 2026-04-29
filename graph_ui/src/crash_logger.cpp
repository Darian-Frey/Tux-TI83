#include "crash_logger.hpp"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <exception>

#include <execinfo.h>
#include <fcntl.h>
#include <unistd.h>

namespace tux_ti83 {

namespace {

// File descriptor for the open session log. -1 until init() runs.
// `std::atomic<int>` so signal handlers can read it without UB; only
// the main thread writes it.
std::atomic<int> g_logFd{-1};

// Resolved absolute path of the active log. Stored alongside the fd
// so currentLogPath() doesn't have to recompute it.
QString g_logPath;

// Once-flag so init() is idempotent.
std::atomic<bool> g_initialised{false};

// Async-signal-safe constant write helper. Writes a NUL-terminated
// string in full, retrying on EINTR. We deliberately ignore short
// writes after a retry to keep the handler simple — the caller is
// already abnormally terminating.
void writeAsync(int fd, const char* msg) {
    if (fd < 0 || msg == nullptr) return;
    size_t total = std::strlen(msg);
    size_t done = 0;
    while (done < total) {
        ssize_t n = ::write(fd, msg + done, total - done);
        if (n < 0) {
            if (errno == EINTR) continue;
            return;
        }
        done += static_cast<size_t>(n);
    }
}

// Async-signal-safe int → decimal text. Writes into `buf` (assumed
// large enough) and returns the number of chars written. Handles 0,
// negatives, and INT_MIN.
int intToBuf(int value, char* buf) {
    if (value == 0) { buf[0] = '0'; buf[1] = '\0'; return 1; }
    bool negative = value < 0;
    // Avoid overflow on -INT_MIN by working with unsigned.
    unsigned int abs = negative ? (unsigned int)(-(long long)value)
                                : (unsigned int)value;
    char tmp[16];
    int idx = 0;
    while (abs > 0) {
        tmp[idx++] = '0' + (abs % 10);
        abs /= 10;
    }
    int out = 0;
    if (negative) buf[out++] = '-';
    for (int i = idx - 1; i >= 0; --i) buf[out++] = tmp[i];
    buf[out] = '\0';
    return out;
}

// Signal handler. Async-signal-safe only — no Qt, no std::cout, no
// dynamic allocation. Writes a CRASH marker + backtrace to the log
// fd, then re-raises the signal with the default handler so a core
// file is still produced.
extern "C" void crashSignalHandler(int sig) {
    int fd = g_logFd.load();
    writeAsync(fd, "\n=== CRASH: signal ");
    char numbuf[16];
    intToBuf(sig, numbuf);
    writeAsync(fd, numbuf);
    writeAsync(fd, " ===\n");

    // Backtrace via libc. backtrace_symbols_fd is documented as
    // async-signal-safe; backtrace_symbols (heap-allocating) is not.
    void* frames[64];
    int count = ::backtrace(frames, 64);
    ::backtrace_symbols_fd(frames, count, fd);

    if (fd >= 0) ::fsync(fd);

    // Restore default handler and re-raise so the OS produces a core
    // dump (subject to ulimit) and exits with the conventional status.
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

// std::terminate handler. Not in a signal context — Qt and the C++
// runtime are usable. Captures the exception's what() string before
// aborting.
void crashTerminateHandler() {
    int fd = g_logFd.load();
    writeAsync(fd, "\n=== TERMINATE: ");
    try {
        auto eptr = std::current_exception();
        if (eptr) {
            std::rethrow_exception(eptr);
        } else {
            writeAsync(fd, "no active exception");
        }
    } catch (const std::exception& e) {
        writeAsync(fd, e.what());
    } catch (...) {
        writeAsync(fd, "unknown exception type");
    }
    writeAsync(fd, " ===\n");

    void* frames[64];
    int count = ::backtrace(frames, 64);
    ::backtrace_symbols_fd(frames, count, fd);

    if (fd >= 0) ::fsync(fd);
    std::abort();
}

} // namespace

void CrashLogger::init() {
    bool expected = false;
    if (!g_initialised.compare_exchange_strong(expected, true))
        return;

    // Resolve log directory: prefer XDG_STATE_HOME, fall back to
    // QStandardPaths::CacheLocation which on Linux maps to
    // ~/.cache/<app>. The app name is taken from
    // QCoreApplication::applicationName() if set; fall back to a
    // hard-coded default so init() still works before Qt knows about
    // the app metadata.
    QString stateHome = qEnvironmentVariable("XDG_STATE_HOME");
    QString dir;
    if (!stateHome.isEmpty()) {
        dir = stateHome + "/tux-ti83";
    } else {
        QString home = QDir::homePath();
        dir = home + "/.local/state/tux-ti83";
    }
    QDir().mkpath(dir);
    g_logPath = dir + "/session.log";

    int fd = ::open(g_logPath.toLocal8Bit().constData(),
                    O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) {
        // Best-effort: if we can't open the file, there's nothing
        // useful we can do — leave the logger inert.
        g_initialised.store(false);
        return;
    }
    g_logFd.store(fd);

    logEvent(QString("=== Session start: %1 ===")
             .arg(QDateTime::currentDateTime().toString(Qt::ISODate)));

    // Install crash handlers. SIGSEGV / SIGABRT / SIGFPE / SIGILL /
    // SIGBUS cover the usual abnormal-termination cases. We let
    // SIGINT / SIGTERM use their default behaviour so Ctrl-C still
    // exits cleanly through the shutdown path.
    std::signal(SIGSEGV, crashSignalHandler);
    std::signal(SIGABRT, crashSignalHandler);
    std::signal(SIGFPE,  crashSignalHandler);
    std::signal(SIGILL,  crashSignalHandler);
    std::signal(SIGBUS,  crashSignalHandler);
    std::set_terminate(crashTerminateHandler);
}

void CrashLogger::logEvent(const QString& event) {
    int fd = g_logFd.load();
    if (fd < 0) return;
    QString line = QString("[%1] %2\n")
                     .arg(QDateTime::currentMSecsSinceEpoch())
                     .arg(event);
    QByteArray bytes = line.toUtf8();
    ::write(fd, bytes.constData(), bytes.size());
    ::fsync(fd);
}

void CrashLogger::shutdown() {
    if (!g_initialised.load()) return;
    logEvent(QStringLiteral("=== Session end (clean) ==="));
    int fd = g_logFd.exchange(-1);
    if (fd >= 0) ::close(fd);
    g_initialised.store(false);
}

QString CrashLogger::currentLogPath() {
    return g_logFd.load() >= 0 ? g_logPath : QString();
}

} // namespace tux_ti83

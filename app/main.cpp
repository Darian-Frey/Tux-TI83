#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QFileInfo>
#include <QLockFile>
#include <QTimer>
#include <QtQml>
#include "ui_controller.hpp"
#include "crash_logger.hpp"
#include "program_highlighter.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("tux-ti83"));

    // Single-instance guard (BUG-024). Two instances sharing one state.json
    // race on save (last write wins), silently dropping saved programs —
    // most visibly after a rebuild if the old process is still running.
    // The lock sits beside state.json; QLockFile auto-reclaims a stale lock
    // left by a crashed process (dead PID), so this doesn't wedge on kill.
    const QString lockPath =
        QFileInfo(tux_ti83::UIController::stateFilePath()).absolutePath() +
        QStringLiteral("/tux-ti83.lock");
    QLockFile instanceLock(lockPath);
    if (!instanceLock.tryLock(200)) {
        qWarning() << "Another Tux-TI83 instance is already running — "
                      "exiting to protect saved state.";
        return 0;
    }

    // Crash logger comes up before anything else so it captures the
    // entire session — see graph_ui/include/crash_logger.hpp.
    tux_ti83::CrashLogger::init();

    // Create the engine first
    QQmlApplicationEngine engine;

    // Create the controller
    static tux_ti83::UIController uiController;

    // Restore previous session's variables / matrices / Y= buffers /
    // MODE settings / viewport. Bails silently if no state file
    // exists (first run) or it fails to parse — leaves the controller
    // in default state in that case.
    uiController.loadState();

    // Periodic save — protects against crashes that skip the
    // post-exec saveState(). Fires every 30s; saveState() is
    // idempotent and cheap (single ~1 KB JSON file). Tests / CLI /
    // REPL don't run a Qt event loop so no timer for them.
    QTimer autoSaveTimer;
    autoSaveTimer.setInterval(30 * 1000);
    QObject::connect(&autoSaveTimer, &QTimer::timeout,
                     &uiController, &tux_ti83::UIController::saveState);
    autoSaveTimer.start();

    // EXPLICIT LINK: Register the controller BEFORE loading the file
    engine.rootContext()->setContextProperty("uiController", &uiController);

    // Syntax highlighter for the PRGM editor — usable from QML as
    // `import Tux 1.0; ProgramHighlighter { textDocument: area.textDocument }`.
    qmlRegisterType<tux_ti83::ProgramHighlighter>("Tux", 1, 0, "ProgramHighlighter");

    const QUrl url("qrc:/App/Main.qml");

    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const auto &error : warnings) {
            qDebug() << "QML Error:" << error.toString();
        }
    });

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        tux_ti83::CrashLogger::shutdown();
        return -1;
    }

    int rc = app.exec();
    // Persist before the crash logger shuts down so a saveState
    // failure still gets logged.
    uiController.saveState();
    tux_ti83::CrashLogger::shutdown();
    return rc;
}

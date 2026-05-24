#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QTimer>
#include "ui_controller.hpp"
#include "crash_logger.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("tux-ti83"));

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

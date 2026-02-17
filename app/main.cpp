#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include "ui_controller.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    
    // Create the engine first
    QQmlApplicationEngine engine;
    
    // Create the controller
    static tux_ti83::UIController uiController;
    
    // EXPLICIT LINK: Register the controller BEFORE loading the file
    engine.rootContext()->setContextProperty("uiController", &uiController);
    
    const QUrl url("qrc:/graph_ui/qml/Main.qml");
    
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, [](const QList<QQmlError> &warnings) {
        for (const auto &error : warnings) {
            qDebug() << "QML Error:" << error.toString();
        }
    });

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}

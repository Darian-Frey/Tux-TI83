#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ui_controller.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    tux_ti83::UIController uiController;
    engine.rootContext()->setContextProperty("uiController", &uiController);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    return app.exec();
}

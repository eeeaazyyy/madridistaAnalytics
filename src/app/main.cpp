#include <QApplication>
#include <QMainWindow>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("eeeaazyyy"));
    QApplication::setApplicationName(QStringLiteral("Madridista Analytics"));

    QMainWindow window;
    window.resize(1200, 800);
    window.show();

    return app.exec();
}
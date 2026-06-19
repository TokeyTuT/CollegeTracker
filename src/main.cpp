#include "mainwindow.h"
#include "LoginDialog.h"

#include <QApplication>
#include <QEventLoop>
#include <QLocale>
#include <QTranslator>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>

#include"DatabaseMannager.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);

    // 强制使用浅色模式，避免在 macOS / Windows 深色模式下被系统颜色影响。
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(244, 241, 234));
    lightPalette.setColor(QPalette::WindowText, QColor(23, 32, 29));
    lightPalette.setColor(QPalette::Base, QColor(255, 254, 250));
    lightPalette.setColor(QPalette::AlternateBase, QColor(250, 248, 243));
    lightPalette.setColor(QPalette::ToolTipBase, QColor(32, 63, 56));
    lightPalette.setColor(QPalette::ToolTipText, QColor(255, 249, 241));
    lightPalette.setColor(QPalette::Text, QColor(23, 32, 29));
    lightPalette.setColor(QPalette::Button, QColor(255, 254, 250));
    lightPalette.setColor(QPalette::ButtonText, QColor(23, 32, 29));
    lightPalette.setColor(QPalette::Highlight, QColor(31, 107, 91));
    lightPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    a.setPalette(lightPalette);


    DatabaseManager &db = DatabaseManager::getInstance();
    db.initDatabase();


    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "CollegeTracker_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    while (true) {
        LoginDialog login;
        if (login.exec() != QDialog::Accepted)
            return 0;

        MainWindow window;
        QEventLoop windowLoop;
        bool loggingOut = false;

        QObject::connect(&window, &MainWindow::logoutRequested,
                         &windowLoop, [&]() {
            loggingOut = true;
            window.close();
            windowLoop.quit();
        });
        QObject::connect(&a, &QGuiApplication::lastWindowClosed,
                         &windowLoop, &QEventLoop::quit);

        window.show();
        windowLoop.exec();

        if (!loggingOut)
            return 0;
    }


}

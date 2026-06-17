#include "mainwindow.h"
#include "LoginDialog.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>

#include"DatabaseMannager.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 强制使用浅色模式，避免在 macOS / Windows 深色模式下被系统颜色影响。
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(244, 247, 251));
    lightPalette.setColor(QPalette::WindowText, QColor(15, 23, 42));
    lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::AlternateBase, Q
                                                       Color(248, 250, 252));
    lightPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::ToolTipText, QColor(15, 23, 42));
    lightPalette.setColor(QPalette::Text, QColor(15, 23, 42));
    lightPalette.setColor(QPalette::Button, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::ButtonText, QColor(15, 23, 42));
    lightPalette.setColor(QPalette::Highlight, QColor(37, 99, 235));
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

    LoginDialog login;
    if (login.exec() != QDialog::Accepted) {
        return 0;
    }

    MainWindow w;
    w.show();
    return QCoreApplication::exec();


}

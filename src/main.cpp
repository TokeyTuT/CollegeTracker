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

    // 暖象牙白美学：强制浅色 Fusion 风格，全局暖色调色板。
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QPalette warmPalette;
    warmPalette.setColor(QPalette::Window, QColor(254, 250, 243));       // #FEFAF3
    warmPalette.setColor(QPalette::WindowText, QColor(61, 46, 31));      // #3D2E1F
    warmPalette.setColor(QPalette::Base, QColor(255, 254, 249));          // #FFFEF9
    warmPalette.setColor(QPalette::AlternateBase, QColor(253, 248, 238)); // #FDF8EE
    warmPalette.setColor(QPalette::ToolTipBase, QColor(255, 254, 249));
    warmPalette.setColor(QPalette::ToolTipText, QColor(61, 46, 31));
    warmPalette.setColor(QPalette::Text, QColor(61, 46, 31));
    warmPalette.setColor(QPalette::Button, QColor(255, 254, 249));
    warmPalette.setColor(QPalette::ButtonText, QColor(61, 46, 31));
    warmPalette.setColor(QPalette::Highlight, QColor(200, 148, 62));      // #C8943E
    warmPalette.setColor(QPalette::HighlightedText, QColor(255, 254, 249));
    a.setPalette(warmPalette);


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

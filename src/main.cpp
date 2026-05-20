#include "mainwindow.h"
#include "LoginDialog.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include"DatabaseMannager.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


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

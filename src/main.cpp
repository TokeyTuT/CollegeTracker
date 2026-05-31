#include "mainwindow.h"
#include "LoginDialog.h"
#include "User.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include "DatabaseMannager.h"

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

    while (true) {
        LoginDialog login;
        if (login.exec() != QDialog::Accepted) {
            return 0;  // 用户关闭登录窗口 → 退出应用
        }

        MainWindow w;
        w.show();

        int ret = a.exec();  // 进入事件循环，最后一个窗口关闭时返回

        if (!User::getInstance().isLoggedIn()) {
            // 用户点击了"退出登入" → 回到登录界面
            continue;
        }
        // 用户正常关闭窗口（点X）→ 退出应用
        return ret;
    }
}

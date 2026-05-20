#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>
#include <QVariantMap>
#include <QCoreApplication>

class DatabaseManager {
public:
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    static DatabaseManager& getInstance() {
        static DatabaseManager instance;
        return instance;
    }

    bool initDatabase();

    // 统计（按用户过滤）
    QVariantMap getTotalStats(int userId);

    // 用户管理
    bool registerUser(const QString &username, const QString &password,
                      const QString &grade, const QString &gender, const QString &major);
    int loginUser(const QString &username, const QString &password); // 返回 user id，-1 表示失败
    QVariantMap getUserInfo(int userId);

private:
    DatabaseManager();
    ~DatabaseManager();

    QSqlDatabase m_db;

    bool createTables();
    //后序维护，给每个表添加了外键
    void migrateTables();
};

#endif // DATABASEMANAGER_H
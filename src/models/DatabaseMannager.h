#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>
#include<QCoreApplication>

class DatabaseManager {
public:
    // 单例模式
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // 获取唯一实例的入口
    static DatabaseManager& getInstance() {
        static DatabaseManager instance;
        return instance;
    }

    // 初始化数据库
    bool initDatabase();
    //更新状态
    QVariantMap getTotalStats();

private:
    DatabaseManager(); // 构造函数私有化, 不能被外部初始化
    ~DatabaseManager();

    QSqlDatabase m_db;

    // 辅助函数：创建初始表结构
    bool createTables();
};

#endif // DATABASEMANAGER_H
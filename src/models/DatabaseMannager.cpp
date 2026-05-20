#include "DatabaseMannager.h"

DatabaseManager::DatabaseManager() {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = "college_tracker.db";
    m_db.setDatabaseName(dbPath);
    qDebug() << "数据库创建成功";
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::initDatabase() {
    if (!m_db.open()) {
        qDebug() << "错误：数据库连接失败" << m_db.lastError().text();
        return false;
    }
    qDebug() << "数据库连接成功";
    if (!createTables()) return false;
    migrateTables();
    return true;
}

bool DatabaseManager::createTables() {
    QSqlQuery query;
    bool success = true;

    QString sqlUsers = "CREATE TABLE IF NOT EXISTS users ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "username TEXT UNIQUE NOT NULL, "
                       "password TEXT NOT NULL, "
                       "grade TEXT DEFAULT '', "
                       "gender TEXT DEFAULT '', "
                       "major TEXT DEFAULT ''"
                       ");";
    if (!query.exec(sqlUsers)) success = false;

    QString sqlCourses = "CREATE TABLE IF NOT EXISTS courses ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "user_id INTEGER NOT NULL, "
                         "name TEXT NOT NULL, "
                         "credit REAL, "
                         "score REAL, "
                         "semester TEXT"
                         ");";
    if (!query.exec(sqlCourses)) success = false;

    QString sqlExp = "CREATE TABLE IF NOT EXISTS experiences ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "user_id INTEGER NOT NULL, "
                     "title TEXT NOT NULL, "
                     "type TEXT NOT NULL, "
                     "date TEXT, "
                     "content TEXT"
                     ");";
    if (!query.exec(sqlExp)) success = false;

    QString sqlAwards = "CREATE TABLE IF NOT EXISTS awards ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "user_id INTEGER NOT NULL, "
                        "name TEXT NOT NULL, "
                        "level TEXT, "
                        "date TEXT"
                        ");";
    if (!query.exec(sqlAwards)) success = false;

    if (!success) {
        qDebug() << "部分表创建失败：" << query.lastError().text();
    }
    qDebug() << "所有表创建成功！";
    return success;
}

void DatabaseManager::migrateTables() {
    QSqlQuery query;

    // 为已有的 users 表添加新列（如果不存在则忽略错误）
    query.exec("ALTER TABLE users ADD COLUMN grade TEXT DEFAULT ''");
    query.exec("ALTER TABLE users ADD COLUMN gender TEXT DEFAULT ''");
    query.exec("ALTER TABLE users ADD COLUMN major TEXT DEFAULT ''");

    // 为已有的数据表添加 user_id 列
    query.exec("ALTER TABLE courses ADD COLUMN user_id INTEGER NOT NULL DEFAULT 1");
    query.exec("ALTER TABLE experiences ADD COLUMN user_id INTEGER NOT NULL DEFAULT 1");
    query.exec("ALTER TABLE awards ADD COLUMN user_id INTEGER NOT NULL DEFAULT 1");

    qDebug() << "数据库迁移完成";
}

QVariantMap DatabaseManager::getTotalStats(int userId) {
    QVariantMap stats;
    QSqlQuery query;

    QString sql = "SELECT "
                  "COUNT(*), "
                  "AVG(score), "
                  "SUM(score * credit), "
                  "SUM(credit) "
                  "FROM courses WHERE user_id = :uid";
    query.prepare(sql);
    query.bindValue(":uid", userId);

    if (query.exec() && query.next()) {
        int count = query.value(0).toInt();
        double avgScore = query.value(1).toDouble();
        double sumWeighted = query.value(2).toDouble();
        double sumCredits = query.value(3).toDouble();

        double gpa = (sumCredits > 0) ? (sumWeighted / sumCredits) : 0.0;

        stats["count"] = count;
        stats["avg"] = avgScore;
        stats["gpa"] = gpa;
        stats["totalCredits"] = sumCredits;
    }
    return stats;
}

bool DatabaseManager::registerUser(const QString &username, const QString &password,
                                    const QString &grade, const QString &gender, const QString &major) {
    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password, grade, gender, major) "
                  "VALUES (:username, :password, :grade, :gender, :major)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.bindValue(":grade", grade);
    query.bindValue(":gender", gender);
    query.bindValue(":major", major);
    return query.exec();
}

int DatabaseManager::loginUser(const QString &username, const QString &password) {
    QSqlQuery query;
    query.prepare("SELECT id FROM users WHERE username = :username AND password = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", password);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

QVariantMap DatabaseManager::getUserInfo(int userId) {
    QVariantMap info;
    QSqlQuery query;
    query.prepare("SELECT id, username, grade, gender, major FROM users WHERE id = :id");
    query.bindValue(":id", userId);

    if (query.exec() && query.next()) {
        info["id"] = query.value(0).toInt();
        info["username"] = query.value(1).toString();
        info["grade"] = query.value(2).toString();
        info["gender"] = query.value(3).toString();
        info["major"] = query.value(4).toString();
    }
    return info;
}

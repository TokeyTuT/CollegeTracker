#include "DatabaseMannager.h"

DatabaseManager::DatabaseManager() {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = QCoreApplication::applicationDirPath() + "/college_tracker.db";
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
                       "major TEXT DEFAULT '', "
                       "school TEXT DEFAULT ''"
                       ");";
    if (!query.exec(sqlUsers)) success = false;

    QString sqlCourses = "CREATE TABLE IF NOT EXISTS courses ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "user_id INTEGER NOT NULL, "
                         "name TEXT NOT NULL, "
                         "credit REAL, "
                         "score REAL, "
                         "semester TEXT, "
                         "gpa REAL DEFAULT 0, "
                         "semester_order INTEGER DEFAULT 0"
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
                        "date TEXT, "
                        "amount REAL DEFAULT 0"
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
    query.exec("ALTER TABLE users ADD COLUMN school TEXT DEFAULT ''");

    // 为已有的数据表添加 user_id 列
    query.exec("ALTER TABLE courses ADD COLUMN user_id INTEGER NOT NULL DEFAULT 1");
    query.exec("ALTER TABLE courses ADD COLUMN gpa REAL DEFAULT 0");
    query.exec("UPDATE courses SET gpa = CASE "
               "WHEN score >= 90 THEN 4.0 "
               "WHEN score >= 85 THEN 3.7 "
               "WHEN score >= 82 THEN 3.3 "
               "WHEN score >= 78 THEN 3.0 "
               "WHEN score >= 75 THEN 2.7 "
               "WHEN score >= 72 THEN 2.3 "
               "WHEN score >= 68 THEN 2.0 "
               "WHEN score >= 64 THEN 1.5 "
               "WHEN score >= 60 THEN 1.0 "
               "ELSE 0 END WHERE score IS NOT NULL");
    query.exec("ALTER TABLE experiences ADD COLUMN user_id INTEGER NOT NULL DEFAULT 1");
    query.exec("ALTER TABLE awards ADD COLUMN user_id INTEGER NOT NULL DEFAULT 1");
    query.exec("ALTER TABLE awards ADD COLUMN amount REAL DEFAULT 0");

    // 为 courses 表添加 semester_order 列，用于按学期时间排序
    query.exec("ALTER TABLE courses ADD COLUMN semester_order INTEGER DEFAULT 0");
    query.exec("UPDATE courses SET semester_order = CASE "
               "WHEN semester = '大一上' THEN 0 "
               "WHEN semester = '大一下' THEN 1 "
               "WHEN semester = '大二上' THEN 2 "
               "WHEN semester = '大二下' THEN 3 "
               "WHEN semester = '大三上' THEN 4 "
               "WHEN semester = '大三下' THEN 5 "
               "WHEN semester = '大四上' THEN 6 "
               "WHEN semester = '大四下' THEN 7 "
               "ELSE 0 END");

    qDebug() << "数据库迁移完成";
}


bool DatabaseManager::updateUserInfo(int userId, const QString &grade, const QString &gender, const QString &major, const QString &school) {
    QSqlQuery query;
    query.prepare("UPDATE users SET grade = :grade, gender = :gender, major = :major, school = :school WHERE id = :id");
    query.bindValue(":grade", grade);
    query.bindValue(":gender", gender);
    query.bindValue(":major", major);
    query.bindValue(":school", school);
    query.bindValue(":id", userId);
    return query.exec();
}

double DatabaseManager::scoreToGpa(double score) {
    if (score >= 90.0) return 4.0;
    if (score >= 85.0) return 3.7;
    if (score >= 82.0) return 3.3;
    if (score >= 78.0) return 3.0;
    if (score >= 75.0) return 2.7;
    if (score >= 72.0) return 2.3;
    if (score >= 68.0) return 2.0;
    if (score >= 64.0) return 1.5;
    if (score >= 60.0) return 1.0;
    return 0.0;
}

QVariantMap DatabaseManager::getTotalStats(int userId) {
    QVariantMap stats;
    QSqlQuery query;

    query.prepare("SELECT score, credit FROM courses WHERE user_id = :uid");
    query.bindValue(":uid", userId);

    int count = 0;
    double scoreSum = 0.0;
    double gpaCreditSum = 0.0;
    double totalCredits = 0.0;

    if (query.exec()) {
        while (query.next()) {
            double score = query.value(0).toDouble();
            double credit = query.value(1).toDouble();
            double gpa = scoreToGpa(score);

            ++count;
            scoreSum += score;
            if (credit > 0.0) {
                gpaCreditSum += gpa * credit;
                totalCredits += credit;
            }
        }
    }

    stats["count"] = count;
    stats["avg"] = (count > 0) ? (scoreSum / count) : 0.0;
    stats["gpa"] = (totalCredits > 0.0) ? (gpaCreditSum / totalCredits) : 0.0;
    stats["totalCredits"] = totalCredits;
    return stats;
}

bool DatabaseManager::registerUser(const QString &username, const QString &password,
                                    const QString &grade, const QString &gender, const QString &major, const QString &school) {
    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password, grade, gender, major, school) "
                  "VALUES (:username, :password, :grade, :gender, :major, :school)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.bindValue(":grade", grade);
    query.bindValue(":gender", gender);
    query.bindValue(":major", major);
    query.bindValue(":school", school);
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
    query.prepare("SELECT id, username, grade, gender, major, school FROM users WHERE id = :id");
    query.bindValue(":id", userId);

    if (query.exec() && query.next()) {
        info["id"] = query.value(0).toInt();
        info["username"] = query.value(1).toString();
        info["grade"] = query.value(2).toString();
        info["gender"] = query.value(3).toString();
        info["major"] = query.value(4).toString();
        info["school"] = query.value(5).toString();
    }
    return info;
}

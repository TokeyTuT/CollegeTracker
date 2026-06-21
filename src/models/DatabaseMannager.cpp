#include "DatabaseMannager.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QSqlRecord>

namespace {

QVariantMap queryRowToMap(const QSqlQuery &query) {
    QVariantMap row;
    const QSqlRecord record = query.record();
    for (int i = 0; i < record.count(); ++i)
        row.insert(record.fieldName(i), query.value(i));
    return row;
}

QVariantList queryToList(QSqlQuery &query) {
    QVariantList rows;
    while (query.next())
        rows.append(queryRowToMap(query));
    return rows;
}

QVariant valueOrDefault(const QVariantMap &data, const QString &key,
                        const QVariant &defaultValue = QVariant()) {
    return data.contains(key) ? data.value(key) : defaultValue;
}

} // namespace

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
    QSqlQuery pragmaQuery(m_db);
    if (!pragmaQuery.exec("PRAGMA foreign_keys = ON"))
        qWarning() << "启用 SQLite 外键失败：" << pragmaQuery.lastError().text();
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
                         "semester_order INTEGER DEFAULT 0, "
                         "is_core INTEGER NOT NULL DEFAULT 0 "
                         "CHECK (is_core IN (0, 1)), "
                         "FOREIGN KEY (user_id) REFERENCES users(id) "
                         "ON DELETE CASCADE"
                         ");";
    if (!query.exec(sqlCourses)) success = false;

    QString sqlExp = "CREATE TABLE IF NOT EXISTS experiences ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "user_id INTEGER NOT NULL, "
                     "title TEXT NOT NULL, "
                     "type TEXT NOT NULL, "
                     "date TEXT, "
                     "content TEXT, "
                     "organization TEXT DEFAULT '', "
                     "role TEXT DEFAULT '', "
                     "sort_order INTEGER NOT NULL DEFAULT 0, "
                     "is_visible INTEGER NOT NULL DEFAULT 1 "
                     "CHECK (is_visible IN (0, 1)), "
                     "FOREIGN KEY (user_id) REFERENCES users(id) "
                     "ON DELETE CASCADE"
                     ");";
    if (!query.exec(sqlExp)) success = false;

    QString sqlAwards = "CREATE TABLE IF NOT EXISTS awards ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "user_id INTEGER NOT NULL, "
                        "name TEXT NOT NULL, "
                        "level TEXT, "
                        "date TEXT, "
                        "amount REAL DEFAULT 0, "
                        "description TEXT DEFAULT '', "
                        "sort_order INTEGER NOT NULL DEFAULT 0, "
                        "is_visible INTEGER NOT NULL DEFAULT 1 "
                        "CHECK (is_visible IN (0, 1)), "
                        "FOREIGN KEY (user_id) REFERENCES users(id) "
                        "ON DELETE CASCADE"
                        ");";
    if (!query.exec(sqlAwards)) success = false;

    QString sqlResumeProfiles =
        "CREATE TABLE IF NOT EXISTS resume_profiles ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL UNIQUE, "
        "full_name TEXT DEFAULT '', "
        "phone TEXT DEFAULT '', "
        "email TEXT DEFAULT '', "
        "job_target TEXT DEFAULT '', "
        "github_url TEXT DEFAULT '', "
        "website_url TEXT DEFAULT '', "
        "summary TEXT DEFAULT '', "
        "skills TEXT DEFAULT '', "
        "photo_path TEXT DEFAULT '', "
        "template_id TEXT NOT NULL DEFAULT 'classic', "
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");";
    if (!query.exec(sqlResumeProfiles)) success = false;

    QString sqlEducation =
        "CREATE TABLE IF NOT EXISTS education_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "school TEXT NOT NULL, "
        "major TEXT DEFAULT '', "
        "degree TEXT DEFAULT '', "
        "start_date TEXT DEFAULT '', "
        "end_date TEXT DEFAULT '', "
        "description TEXT DEFAULT '', "
        "sort_order INTEGER NOT NULL DEFAULT 0, "
        "is_visible INTEGER NOT NULL DEFAULT 1 CHECK (is_visible IN (0, 1)), "
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");";
    if (!query.exec(sqlEducation)) success = false;

    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_education_user_sort "
                    "ON education_records(user_id, sort_order, id)"))
        success = false;

    if (!success) {
        qDebug() << "部分表创建失败：" << query.lastError().text();
    }
    qDebug() << "所有表创建成功！";
    return success;
}

bool DatabaseManager::ensureColumn(const QString &tableName,
                                   const QString &columnName,
                                   const QString &columnDefinition) {
    QSqlQuery tableInfo(m_db);
    if (!tableInfo.exec(QString("PRAGMA table_info(%1)").arg(tableName))) {
        qWarning() << "读取表结构失败：" << tableName
                   << tableInfo.lastError().text();
        return false;
    }

    while (tableInfo.next()) {
        if (tableInfo.value(1).toString() == columnName)
            return true;
    }

    QSqlQuery alterQuery(m_db);
    const QString sql = QString("ALTER TABLE %1 ADD COLUMN %2 %3")
                            .arg(tableName, columnName, columnDefinition);
    if (!alterQuery.exec(sql)) {
        qWarning() << "添加字段失败：" << tableName << columnName
                   << alterQuery.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::hasColumn(const QString &tableName,
                                const QString &columnName) const {
    QSqlQuery tableInfo(m_db);
    if (!tableInfo.exec(QString("PRAGMA table_info(%1)").arg(tableName)))
        return false;

    while (tableInfo.next()) {
        if (tableInfo.value(1).toString() == columnName)
            return true;
    }
    return false;
}

bool DatabaseManager::removeLegacyUserResumeColumns() {
    const QStringList legacyColumns = {
        "phone", "email", "job_target", "website"
    };

    bool hasLegacyColumn = false;
    for (const QString &column : legacyColumns) {
        if (hasColumn("users", column)) {
            hasLegacyColumn = true;
            break;
        }
    }
    if (!hasLegacyColumn)
        return true;

    if (!m_db.transaction()) {
        qWarning() << "无法开始 users 遗留字段删除事务："
                   << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    for (const QString &column : legacyColumns) {
        if (!hasColumn("users", column))
            continue;

        if (!query.exec(QString("ALTER TABLE users DROP COLUMN %1")
                            .arg(column))) {
            qWarning() << "删除 users." << column << "失败："
                       << query.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        qWarning() << "提交 users 遗留字段删除事务失败："
                   << m_db.lastError().text();
        m_db.rollback();
        return false;
    }
    return true;
}

void DatabaseManager::migrateTables() {
    ensureColumn("users", "grade", "TEXT DEFAULT ''");
    ensureColumn("users", "gender", "TEXT DEFAULT ''");
    ensureColumn("users", "major", "TEXT DEFAULT ''");
    ensureColumn("users", "school", "TEXT DEFAULT ''");
    ensureColumn("users", "password_salt", "TEXT DEFAULT ''");

    ensureColumn("courses", "user_id", "INTEGER NOT NULL DEFAULT 1");
    ensureColumn("courses", "gpa", "REAL DEFAULT 0");
    ensureColumn("courses", "semester_order", "INTEGER DEFAULT 0");
    ensureColumn("courses", "is_core", "INTEGER NOT NULL DEFAULT 0");

    ensureColumn("experiences", "user_id", "INTEGER NOT NULL DEFAULT 1");
    ensureColumn("experiences", "organization", "TEXT DEFAULT ''");
    ensureColumn("experiences", "role", "TEXT DEFAULT ''");
    ensureColumn("experiences", "sort_order", "INTEGER NOT NULL DEFAULT 0");
    ensureColumn("experiences", "is_visible",
                 "INTEGER NOT NULL DEFAULT 1");

    ensureColumn("awards", "user_id", "INTEGER NOT NULL DEFAULT 1");
    ensureColumn("awards", "amount", "REAL DEFAULT 0");
    ensureColumn("awards", "description", "TEXT DEFAULT ''");
    ensureColumn("awards", "sort_order", "INTEGER NOT NULL DEFAULT 0");
    ensureColumn("awards", "is_visible", "INTEGER NOT NULL DEFAULT 1");
    ensureColumn("resume_profiles", "template_id",
                 "TEXT NOT NULL DEFAULT 'classic'");

    QSqlQuery query(m_db);
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

    query.exec("CREATE INDEX IF NOT EXISTS idx_experiences_user_resume "
               "ON experiences(user_id, is_visible, sort_order, id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_awards_user_resume "
               "ON awards(user_id, is_visible, sort_order, id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_courses_user_core "
               "ON courses(user_id, is_core, semester_order, id)");

    // 为旧用户建立默认简历资料；用户名只作为真实姓名的初始值。
    query.exec(
        "INSERT OR IGNORE INTO resume_profiles (user_id, full_name) "
        "SELECT id, username FROM users");

    // 兼容曾被误加到 users 表中的简历字段。仅填补 resume_profiles
    // 中的空值，避免覆盖用户已经在简历页维护的新数据。
    struct LegacyResumeField {
        const char *userColumn;
        const char *resumeColumn;
    };
    const LegacyResumeField legacyFields[] = {
        {"phone", "phone"},
        {"email", "email"},
        {"job_target", "job_target"},
        {"website", "website_url"}
    };
    for (const LegacyResumeField &field : legacyFields) {
        if (!hasColumn("users", field.userColumn))
            continue;

        const QString migrateSql = QString(
            "UPDATE resume_profiles "
            "SET %1 = COALESCE(("
            "    SELECT u.%2 FROM users u "
            "    WHERE u.id = resume_profiles.user_id"
            "), ''), "
            "updated_at = CURRENT_TIMESTAMP "
            "WHERE TRIM(COALESCE(%1, '')) = '' "
            "AND EXISTS ("
            "    SELECT 1 FROM users u "
            "    WHERE u.id = resume_profiles.user_id "
            "    AND TRIM(COALESCE(u.%2, '')) <> ''"
            ")")
            .arg(field.resumeColumn, field.userColumn);
        if (!query.exec(migrateSql)) {
            qWarning() << "迁移 users." << field.userColumn
                       << "到 resume_profiles." << field.resumeColumn
                       << "失败：" << query.lastError().text();
            continue;
        }

    }

    // 重复数据已经迁入 resume_profiles，正式移除 users 中的遗留字段。
    // 四列在同一事务中删除，防止数据库停留在部分迁移状态。
    removeLegacyUserResumeColumns();

    // 旧版 users 中已有学校信息时，初始化一条教育经历且避免重复。
    query.exec(
        "INSERT INTO education_records (user_id, school, major, degree, "
        "start_date, end_date, description, sort_order, is_visible) "
        "SELECT u.id, u.school, u.major, '', '', '', '', 0, 1 "
        "FROM users u "
        "WHERE TRIM(COALESCE(u.school, '')) <> '' "
        "AND NOT EXISTS (SELECT 1 FROM education_records e "
        "                WHERE e.user_id = u.id)");

    // 修复旧版本造成的学校/专业不同步。只有一条教育经历时可以明确认定其为
    // 主教育经历；多条记录留给 updateUserInfo 按旧学校精确匹配，避免误改。
    query.exec(
        "UPDATE education_records "
        "SET school = (SELECT u.school FROM users u "
        "              WHERE u.id = education_records.user_id), "
        "    major = (SELECT u.major FROM users u "
        "             WHERE u.id = education_records.user_id) "
        "WHERE user_id IN ("
        "    SELECT user_id FROM education_records "
        "    GROUP BY user_id HAVING COUNT(*) = 1"
        ") "
        "AND EXISTS ("
        "    SELECT 1 FROM users u "
        "    WHERE u.id = education_records.user_id "
        "    AND TRIM(COALESCE(u.school, '')) <> ''"
        ")");

    // 迁移已有明文密码：为空 salt 的用户生成随机盐值并哈希其密码
    query.prepare("SELECT id, password FROM users "
                  "WHERE (password_salt IS NULL OR password_salt = '') "
                  "AND password IS NOT NULL AND password != ''");
    if (query.exec()) {
        while (query.next()) {
            const int userId = query.value(0).toInt();
            const QString plainPassword = query.value(1).toString();

            QByteArray saltBytes(16, Qt::Uninitialized);
            QRandomGenerator::global()->fillRange(
                reinterpret_cast<quint32 *>(saltBytes.data()),
                saltBytes.size() / sizeof(quint32));
            const QString salt = QString::fromLatin1(saltBytes.toHex());
            const QString hashedPassword = hashPassword(plainPassword, salt);

            QSqlQuery updateQuery(m_db);
            updateQuery.prepare("UPDATE users SET password = :hash, "
                                "password_salt = :salt WHERE id = :id");
            updateQuery.bindValue(":hash", hashedPassword);
            updateQuery.bindValue(":salt", salt);
            updateQuery.bindValue(":id", userId);
            if (!updateQuery.exec())
                qWarning() << "密码迁移失败 user_id=" << userId
                           << updateQuery.lastError().text();
        }
    }

    qDebug() << "数据库迁移完成";
}


bool DatabaseManager::updateUserInfo(int userId, const QString &grade, const QString &gender,
                                     const QString &major, const QString &school,
                                     const QString &startYear, const QString &endYear,
                                     const QString &phone, const QString &email,
                                     const QString &jobTarget, const QString &website) {
    if (!m_db.transaction())
        return false;

    QSqlQuery query(m_db);
    QString previousSchool;
    QString previousMajor;
    query.prepare("SELECT school, major FROM users WHERE id = :id");
    query.bindValue(":id", userId);
    if (!query.exec() || !query.next()) {
        m_db.rollback();
        return false;
    }
    previousSchool = query.value(0).toString();
    previousMajor = query.value(1).toString();

    query.prepare("UPDATE users SET grade = :grade, gender = :gender, "
                  "major = :major, school = :school WHERE id = :id");
    query.bindValue(":grade", grade);
    query.bindValue(":gender", gender);
    query.bindValue(":major", major);
    query.bindValue(":school", school);
    query.bindValue(":id", userId);
    if (!query.exec() || query.numRowsAffected() <= 0) {
        m_db.rollback();
        return false;
    }

    // users 中的学校和专业代表侧边栏使用的主教育信息。
    // 简历则读取 education_records，因此这里同步最匹配的主教育经历。
    query.prepare(
        "SELECT id FROM education_records WHERE user_id = :uid "
        "ORDER BY "
        "CASE "
        "  WHEN school = :previous_school AND major = :previous_major THEN 0 "
        "  WHEN school = :previous_school THEN 1 "
        "  ELSE 2 "
        "END, sort_order ASC, id ASC "
        "LIMIT 1");
    query.bindValue(":uid", userId);
    query.bindValue(":previous_school", previousSchool);
    query.bindValue(":previous_major", previousMajor);
    if (!query.exec()) {
        m_db.rollback();
        return false;
    }

    if (query.next()) {
        const int educationId = query.value(0).toInt();
        query.prepare(
            "UPDATE education_records SET school = :school, major = :major, "
            "start_date = :start_date, end_date = :end_date "
            "WHERE id = :id AND user_id = :uid");
        query.bindValue(":school", school);
        query.bindValue(":major", major);
        query.bindValue(":start_date", startYear);
        query.bindValue(":end_date", endYear);
        query.bindValue(":id", educationId);
        query.bindValue(":uid", userId);
        if (!query.exec() || query.numRowsAffected() <= 0) {
            m_db.rollback();
            return false;
        }
    } else {
        query.prepare(
            "INSERT INTO education_records "
            "(user_id, school, major, start_date, end_date, "
            "sort_order, is_visible) "
            "VALUES (:uid, :school, :major, :start_date, :end_date, 0, 1)");
        query.bindValue(":uid", userId);
        query.bindValue(":school", school);
        query.bindValue(":major", major);
        query.bindValue(":start_date", startYear);
        query.bindValue(":end_date", endYear);
        if (!query.exec()) {
            m_db.rollback();
            return false;
        }
    }

    QVariantMap profile = getResumeProfile(userId);
    profile["phone"] = phone;
    profile["email"] = email;
    profile["job_target"] = jobTarget;
    profile["website_url"] = website;
    if (!updateResumeProfile(userId, profile)) {
        m_db.rollback();
        return false;
    }

    return m_db.commit();
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

QString DatabaseManager::hashPassword(const QString &password, const QString &salt) {
    QByteArray data = (salt + password).toUtf8();
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
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
    if (!m_db.transaction())
        return false;

    // 生成随机盐值
    QByteArray saltBytes(16, Qt::Uninitialized);
    QRandomGenerator::global()->fillRange(reinterpret_cast<quint32 *>(saltBytes.data()),
                                          saltBytes.size() / sizeof(quint32));
    const QString salt = QString::fromLatin1(saltBytes.toHex());
    const QString hashedPassword = hashPassword(password, salt);

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (username, password, password_salt, grade, gender, major, school) "
                  "VALUES (:username, :password, :password_salt, :grade, :gender, :major, :school)");
    query.bindValue(":username", username);
    query.bindValue(":password", hashedPassword);
    query.bindValue(":password_salt", salt);
    query.bindValue(":grade", grade);
    query.bindValue(":gender", gender);
    query.bindValue(":major", major);
    query.bindValue(":school", school);
    if (!query.exec()) {
        m_db.rollback();
        return false;
    }

    const int userId = query.lastInsertId().toInt();
    query.prepare("INSERT INTO resume_profiles (user_id, full_name) "
                  "VALUES (:uid, :full_name)");
    query.bindValue(":uid", userId);
    query.bindValue(":full_name", username);
    if (!query.exec()) {
        m_db.rollback();
        return false;
    }

    if (!school.trimmed().isEmpty()) {
        query.prepare(
            "INSERT INTO education_records "
            "(user_id, school, major, sort_order, is_visible) "
            "VALUES (:uid, :school, :major, 0, 1)");
        query.bindValue(":uid", userId);
        query.bindValue(":school", school.trimmed());
        query.bindValue(":major", major.trimmed());
        if (!query.exec()) {
            m_db.rollback();
            return false;
        }
    }

    return m_db.commit();
}

int DatabaseManager::loginUser(const QString &username, const QString &password) {
    QSqlQuery query;
    query.prepare("SELECT id, password, password_salt FROM users WHERE username = :username");
    query.bindValue(":username", username);

    if (query.exec() && query.next()) {
        const QString storedHash = query.value(1).toString();
        const QString salt = query.value(2).toString();
        const QString inputHash = hashPassword(password, salt);
        if (inputHash == storedHash)
            return query.value(0).toInt();
    }
    return -1;
}

QVariantMap DatabaseManager::getUserInfo(int userId) {
    QVariantMap info;
    QSqlQuery query(m_db);
    query.prepare("SELECT u.id, u.username, u.grade, u.gender, u.major, "
                  "u.school, r.phone, r.email, r.job_target, r.website_url "
                  "FROM users u "
                  "LEFT JOIN resume_profiles r ON r.user_id = u.id "
                  "WHERE u.id = :id");
    query.bindValue(":id", userId);

    if (query.exec() && query.next()) {
        info["id"] = query.value(0).toInt();
        info["username"] = query.value(1).toString();
        info["grade"] = query.value(2).toString();
        info["gender"] = query.value(3).toString();
        info["major"] = query.value(4).toString();
        info["school"] = query.value(5).toString();
        info["phone"] = query.value(6).toString();
        info["email"] = query.value(7).toString();
        info["job_target"] = query.value(8).toString();
        info["website"] = query.value(9).toString();
    }
    return info;
}

QVariantMap DatabaseManager::getResumeProfile(int userId) {
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT id, user_id, full_name, phone, email, job_target, github_url, "
        "website_url, summary, skills, photo_path, template_id, "
        "created_at, updated_at "
        "FROM resume_profiles WHERE user_id = :uid");
    query.bindValue(":uid", userId);
    if (query.exec() && query.next())
        return queryRowToMap(query);
    return {};
}

bool DatabaseManager::updateResumeProfile(int userId,
                                          const QVariantMap &profile) {
    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE resume_profiles SET full_name = :full_name, phone = :phone, "
        "email = :email, job_target = :job_target, "
        "github_url = :github_url, website_url = :website_url, "
        "summary = :summary, skills = :skills, photo_path = :photo_path, "
        "template_id = :template_id, "
        "updated_at = CURRENT_TIMESTAMP WHERE user_id = :uid");
    query.bindValue(":full_name", valueOrDefault(profile, "full_name", ""));
    query.bindValue(":phone", valueOrDefault(profile, "phone", ""));
    query.bindValue(":email", valueOrDefault(profile, "email", ""));
    query.bindValue(":job_target", valueOrDefault(profile, "job_target", ""));
    query.bindValue(":github_url", valueOrDefault(profile, "github_url", ""));
    query.bindValue(":website_url", valueOrDefault(profile, "website_url", ""));
    query.bindValue(":summary", valueOrDefault(profile, "summary", ""));
    query.bindValue(":skills", valueOrDefault(profile, "skills", ""));
    query.bindValue(":photo_path", valueOrDefault(profile, "photo_path", ""));
    query.bindValue(":template_id",
                    valueOrDefault(profile, "template_id", "classic"));
    query.bindValue(":uid", userId);
    if (!query.exec())
        return false;
    if (query.numRowsAffected() > 0)
        return true;

    query.prepare(
        "INSERT INTO resume_profiles "
        "(user_id, full_name, phone, email, job_target, github_url, "
        "website_url, summary, skills, photo_path, template_id) "
        "VALUES (:uid, :full_name, :phone, :email, :job_target, :github_url, "
        ":website_url, :summary, :skills, :photo_path, :template_id)");
    query.bindValue(":uid", userId);
    query.bindValue(":full_name", valueOrDefault(profile, "full_name", ""));
    query.bindValue(":phone", valueOrDefault(profile, "phone", ""));
    query.bindValue(":email", valueOrDefault(profile, "email", ""));
    query.bindValue(":job_target", valueOrDefault(profile, "job_target", ""));
    query.bindValue(":github_url", valueOrDefault(profile, "github_url", ""));
    query.bindValue(":website_url", valueOrDefault(profile, "website_url", ""));
    query.bindValue(":summary", valueOrDefault(profile, "summary", ""));
    query.bindValue(":skills", valueOrDefault(profile, "skills", ""));
    query.bindValue(":photo_path", valueOrDefault(profile, "photo_path", ""));
    query.bindValue(":template_id",
                    valueOrDefault(profile, "template_id", "classic"));
    return query.exec();
}

QVariantList DatabaseManager::getEducationRecords(int userId,
                                                   bool visibleOnly) {
    QSqlQuery query(m_db);
    QString sql =
        "SELECT id, user_id, school, major, degree, start_date, end_date, "
        "description, sort_order, is_visible "
        "FROM education_records WHERE user_id = :uid";
    if (visibleOnly)
        sql += " AND is_visible = 1";
    sql += " ORDER BY sort_order ASC, id DESC";
    query.prepare(sql);
    query.bindValue(":uid", userId);
    if (!query.exec())
        return {};
    return queryToList(query);
}

int DatabaseManager::addEducationRecord(int userId,
                                        const QVariantMap &education) {
    const QString school = education.value("school").toString().trimmed();
    if (school.isEmpty())
        return -1;

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO education_records "
        "(user_id, school, major, degree, start_date, end_date, description, "
        "sort_order, is_visible) "
        "VALUES (:uid, :school, :major, :degree, :start_date, :end_date, "
        ":description, :sort_order, :is_visible)");
    query.bindValue(":uid", userId);
    query.bindValue(":school", school);
    query.bindValue(":major", valueOrDefault(education, "major", ""));
    query.bindValue(":degree", valueOrDefault(education, "degree", ""));
    query.bindValue(":start_date",
                    valueOrDefault(education, "start_date", ""));
    query.bindValue(":end_date", valueOrDefault(education, "end_date", ""));
    query.bindValue(":description",
                    valueOrDefault(education, "description", ""));
    query.bindValue(":sort_order",
                    valueOrDefault(education, "sort_order", 0));
    query.bindValue(":is_visible",
                    valueOrDefault(education, "is_visible", true).toBool()
                        ? 1
                        : 0);
    if (!query.exec())
        return -1;
    return query.lastInsertId().toInt();
}

bool DatabaseManager::updateEducationRecord(
    int userId, int educationId, const QVariantMap &education) {
    const QString school = education.value("school").toString().trimmed();
    if (school.isEmpty())
        return false;

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE education_records SET school = :school, major = :major, "
        "degree = :degree, start_date = :start_date, end_date = :end_date, "
        "description = :description, sort_order = :sort_order, "
        "is_visible = :is_visible "
        "WHERE id = :id AND user_id = :uid");
    query.bindValue(":school", school);
    query.bindValue(":major", valueOrDefault(education, "major", ""));
    query.bindValue(":degree", valueOrDefault(education, "degree", ""));
    query.bindValue(":start_date",
                    valueOrDefault(education, "start_date", ""));
    query.bindValue(":end_date", valueOrDefault(education, "end_date", ""));
    query.bindValue(":description",
                    valueOrDefault(education, "description", ""));
    query.bindValue(":sort_order",
                    valueOrDefault(education, "sort_order", 0));
    query.bindValue(":is_visible",
                    valueOrDefault(education, "is_visible", true).toBool()
                        ? 1
                        : 0);
    query.bindValue(":id", educationId);
    query.bindValue(":uid", userId);
    return query.exec() && query.numRowsAffected() > 0;
}

bool DatabaseManager::deleteEducationRecord(int userId, int educationId) {
    QSqlQuery query(m_db);
    query.prepare(
        "DELETE FROM education_records WHERE id = :id AND user_id = :uid");
    query.bindValue(":id", educationId);
    query.bindValue(":uid", userId);
    return query.exec() && query.numRowsAffected() > 0;
}

QVariantList DatabaseManager::getResumeExperiences(int userId,
                                                   bool visibleOnly) {
    QSqlQuery query(m_db);
    QString sql =
        "SELECT id, user_id, title, type, date, content, organization, role, "
        "sort_order, is_visible FROM experiences WHERE user_id = :uid";
    if (visibleOnly)
        sql += " AND is_visible = 1";
    sql += " ORDER BY sort_order ASC, id DESC";
    query.prepare(sql);
    query.bindValue(":uid", userId);
    if (!query.exec())
        return {};
    return queryToList(query);
}

QVariantList DatabaseManager::getResumeAwards(int userId, bool visibleOnly) {
    QSqlQuery query(m_db);
    QString sql =
        "SELECT id, user_id, name, level, date, amount, description, "
        "sort_order, is_visible FROM awards WHERE user_id = :uid";
    if (visibleOnly)
        sql += " AND is_visible = 1";
    sql += " ORDER BY sort_order ASC, id DESC";
    query.prepare(sql);
    query.bindValue(":uid", userId);
    if (!query.exec())
        return {};
    return queryToList(query);
}

QVariantList DatabaseManager::getResumeCoreCourses(int userId) {
    QSqlQuery query(m_db);
    query.prepare(
        "SELECT id, user_id, name, credit, score, semester, gpa, "
        "semester_order, is_core FROM courses "
        "WHERE user_id = :uid AND is_core = 1 "
        "ORDER BY semester_order ASC, score DESC, id ASC");
    query.bindValue(":uid", userId);
    if (!query.exec())
        return {};
    return queryToList(query);
}

bool DatabaseManager::updateExperienceResumeFields(
    int userId, int experienceId, const QString &organization,
    const QString &role, int sortOrder, bool isVisible) {
    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE experiences SET organization = :organization, role = :role, "
        "sort_order = :sort_order, is_visible = :is_visible "
        "WHERE id = :id AND user_id = :uid");
    query.bindValue(":organization", organization.trimmed());
    query.bindValue(":role", role.trimmed());
    query.bindValue(":sort_order", sortOrder);
    query.bindValue(":is_visible", isVisible ? 1 : 0);
    query.bindValue(":id", experienceId);
    query.bindValue(":uid", userId);
    return query.exec() && query.numRowsAffected() > 0;
}

bool DatabaseManager::updateAwardResumeFields(
    int userId, int awardId, const QString &description, int sortOrder,
    bool isVisible) {
    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE awards SET description = :description, "
        "sort_order = :sort_order, is_visible = :is_visible "
        "WHERE id = :id AND user_id = :uid");
    query.bindValue(":description", description.trimmed());
    query.bindValue(":sort_order", sortOrder);
    query.bindValue(":is_visible", isVisible ? 1 : 0);
    query.bindValue(":id", awardId);
    query.bindValue(":uid", userId);
    return query.exec() && query.numRowsAffected() > 0;
}

bool DatabaseManager::setCourseCore(int userId, int courseId, bool isCore) {
    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE courses SET is_core = :is_core "
        "WHERE id = :id AND user_id = :uid");
    query.bindValue(":is_core", isCore ? 1 : 0);
    query.bindValue(":id", courseId);
    query.bindValue(":uid", userId);
    return query.exec() && query.numRowsAffected() > 0;
}

QVariantMap DatabaseManager::getResumeData(int userId) {
    QVariantMap data;
    data["user"] = getUserInfo(userId);
    data["profile"] = getResumeProfile(userId);
    data["education"] = getEducationRecords(userId, true);
    data["experiences"] = getResumeExperiences(userId, true);
    data["awards"] = getResumeAwards(userId, true);
    data["core_courses"] = getResumeCoreCourses(userId);
    data["course_stats"] = getTotalStats(userId);
    return data;
}

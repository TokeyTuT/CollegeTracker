#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QString>
#include <QVariantList>
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
                      const QString &grade, const QString &gender, const QString &major, const QString &school);
    int loginUser(const QString &username, const QString &password); // 返回 user id，-1 表示失败
    QVariantMap getUserInfo(int userId);
    bool updateUserInfo(int userId, const QString &grade, const QString &gender, const QString &major, const QString &school);

    // 简历基本资料（每个用户一份，updateResumeProfile 使用 upsert）
    QVariantMap getResumeProfile(int userId);
    bool updateResumeProfile(int userId, const QVariantMap &profile);

    // 教育经历
    QVariantList getEducationRecords(int userId, bool visibleOnly = false);
    int addEducationRecord(int userId, const QVariantMap &education);
    bool updateEducationRecord(int userId, int educationId,
                               const QVariantMap &education);
    bool deleteEducationRecord(int userId, int educationId);

    // 简历中复用的课程、经历和荣誉
    QVariantList getResumeExperiences(int userId, bool visibleOnly = true);
    QVariantList getResumeAwards(int userId, bool visibleOnly = true);
    QVariantList getResumeCoreCourses(int userId);
    bool updateExperienceResumeFields(int userId, int experienceId,
                                      const QString &organization,
                                      const QString &role, int sortOrder,
                                      bool isVisible);
    bool updateAwardResumeFields(int userId, int awardId,
                                 const QString &description, int sortOrder,
                                 bool isVisible);
    bool setCourseCore(int userId, int courseId, bool isCore);

    // 一次性取得 HTML 模板所需的全部数据
    QVariantMap getResumeData(int userId);

    static double scoreToGpa(double score);

private:
    DatabaseManager();
    ~DatabaseManager();

    QSqlDatabase m_db;

    bool createTables();
    // 后续维护：兼容已有数据库并补齐简历相关字段
    void migrateTables();
    bool ensureColumn(const QString &tableName, const QString &columnName,
                      const QString &columnDefinition);
};

#endif // DATABASEMANAGER_H

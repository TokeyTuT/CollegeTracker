#include"DatabaseMannager.h"
// DatabaseManager::DatabaseManager(){
//     //指定sqlite驱动
//     m_db = QSqlDatabase::addDatabase("QSQLITE");



//     m_db.setDatabaseName("college_tracker.db");
// }
DatabaseManager::DatabaseManager() {
    m_db = QSqlDatabase::addDatabase("QSQLITE");

    //修改数据库的位置
    QString dbPath = "college_tracker.db";
    m_db.setDatabaseName(dbPath);

    qDebug() << "数据库创建成功";
}

DatabaseManager::~DatabaseManager(){
    //按时关闭数据库
    if(m_db.isOpen()){
        m_db.close();
    }
}

bool DatabaseManager::initDatabase(){
    if(!m_db.open()){
        qDebug() << "错误：数据库连接失败"<<m_db.lastError().text();
        return false;
    }

    qDebug()<<"数据库连接成功";
    return createTables();
}

QVariantMap DatabaseManager::getTotalStats() {
    QVariantMap stats;
    QSqlQuery query;

    //计算总数、算术平均分、加权总分、总学分
    QString sql = "SELECT "
                  "COUNT(*), "
                  "AVG(score), "
                  "SUM(score * credit), "
                  "SUM(credit) "
                  "FROM courses";

    if (query.exec(sql) && query.next()) {
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

//建表
bool DatabaseManager::createTables(){
    QSqlQuery query;
    bool success = true;
    // 1. 课程与成绩表 (Courses)
    QString sqlCourses = "CREATE TABLE IF NOT EXISTS courses ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "name TEXT NOT NULL, "
                         "credit REAL, "     // 学分
                         "score REAL, "      // 成绩
                         "semester TEXT"     // 学期
                         ");";
    if (!query.exec(sqlCourses)) success = false;

    // 2. 课外经历表 (Experiences)
    QString sqlExp = "CREATE TABLE IF NOT EXISTS experiences ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "title TEXT NOT NULL, " // 项目/实习名称
                     "type TEXT NOT NULL,"
                     "date TEXT, "           // 时间点
                     "content TEXT"          // 详细内容
                     ");";
    if (!query.exec(sqlExp)) success = false;

    // 3. 个人荣誉/成就表 (Awards)
    QString sqlAwards = "CREATE TABLE IF NOT EXISTS awards ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "name TEXT NOT NULL, "  // 奖项名称
                        "level TEXT, "          // 级别 (如：校级、国家级)
                        "date TEXT"             // 获奖时间
                        ");";
    if (!query.exec(sqlAwards)) success = false;

    if (!success) {
        qDebug() << "部分表创建失败：" << query.lastError().text();
    }
    qDebug()<<"所有表创建成功！";
    return success;
}
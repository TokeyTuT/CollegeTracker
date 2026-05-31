#include "User.h"
#include "DatabaseMannager.h"

User::User() = default;
User::~User() = default;

void User::login(int userId) {
    QVariantMap info = DatabaseManager::getInstance().getUserInfo(userId);
    if (info.isEmpty()) return;

    m_id = info["id"].toInt();
    m_username = info["username"].toString();
    m_grade = info["grade"].toString();
    m_gender = info["gender"].toString();
    m_major = info["major"].toString();
    m_school = info["school"].toString();
}


void User::refresh() {
    if (m_id == -1) return;
    QVariantMap info = DatabaseManager::getInstance().getUserInfo(m_id);
    if (info.isEmpty()) return;

    m_username = info["username"].toString();
    m_grade = info["grade"].toString();
    m_gender = info["gender"].toString();
    m_major = info["major"].toString();
    m_school = info["school"].toString();
}

void User::logout() {
    m_id = -1;
    m_username.clear();
    m_grade.clear();
    m_gender.clear();
    m_major.clear();
    m_school.clear();
}

bool User::isLoggedIn() const {
    return m_id != -1;
}

int User::getId() const { return m_id; }
QString User::getUsername() const { return m_username; }
QString User::getGrade() const { return m_grade; }
QString User::getGender() const { return m_gender; }
QString User::getMajor() const { return m_major; }
QString User::getSchool() const { return m_school; }

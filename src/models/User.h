#ifndef USER_H
#define USER_H

#include <QString>

class User
{
public:
    //设置单例模式
    User(const User&) = delete;
    User& operator=(const User&) = delete;

    static User& getInstance() {
        static User user;
        return user;
    }

    bool isLoggedIn() const;

    void login(int userId);
    void logout();

    int getId() const;
    QString getUsername() const;
    QString getGrade() const;
    QString getGender() const;
    QString getMajor() const;

private:
    int m_id = -1;
    QString m_username;
    QString m_grade;
    QString m_gender;
    QString m_major;

    User();
    ~User();
};

#endif // USER_H

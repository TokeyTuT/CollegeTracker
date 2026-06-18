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
    void refresh();

    int getId() const;
    QString getUsername() const;
    QString getGrade() const;
    QString getGender() const;
    QString getMajor() const;
    QString getSchool() const;
    QString getPhone() const;
    QString getEmail() const;
    QString getJobTarget() const;
    QString getWebsite() const;

private:
    int m_id = -1;
    QString m_username;
    QString m_grade;
    QString m_gender;
    QString m_major;
    QString m_school;
    QString m_phone;
    QString m_email;
    QString m_jobTarget;
    QString m_website;

    User();
    ~User();
};

#endif // USER_H

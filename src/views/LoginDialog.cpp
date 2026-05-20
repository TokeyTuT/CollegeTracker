#include "LoginDialog.h"
#include "../models/DatabaseMannager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("登入");
    setModal(true);
    setFixedSize(300, 200);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *userLabel = new QLabel("用户名:", this);
    m_usernameEdit = new QLineEdit(this);
    layout->addWidget(userLabel);
    layout->addWidget(m_usernameEdit);

    QLabel *passLabel = new QLabel("密码:", this);
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(passLabel);
    layout->addWidget(m_passwordEdit);

    m_messageLabel = new QLabel("", this);
    m_messageLabel->setStyleSheet("color: red;");
    layout->addWidget(m_messageLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_loginBtn = new QPushButton("登入", this);
    m_registerBtn = new QPushButton("注册", this);
    buttonLayout->addWidget(m_loginBtn);
    buttonLayout->addWidget(m_registerBtn);
    layout->addLayout(buttonLayout);

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
}

QString LoginDialog::getUsername() const {
    return m_usernameEdit->text();
}

void LoginDialog::onLoginClicked() {
    QString username = m_usernameEdit->text();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_messageLabel->setText("用户名或密码不能为空");
        return;
    }

    DatabaseManager &db = DatabaseManager::getInstance();
    if (db.loginUser(username, password)) {
        accept();
    } else {
        m_messageLabel->setText("用户名或密码错误");
        m_passwordEdit->clear();
    }
}

void LoginDialog::onRegisterClicked() {
    QString username = m_usernameEdit->text();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_messageLabel->setText("用户名或密码不能为空");
        return;
    }

    DatabaseManager &db = DatabaseManager::getInstance();
    if (db.registerUser(username, password)) {
        m_messageLabel->setText("注册成功，请登入");
        m_passwordEdit->clear();
    } else {
        m_messageLabel->setText("用户名已存在");
    }
}

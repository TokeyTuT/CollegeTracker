#include "LoginDialog.h"
#include "RegisterDialog.h"
#include "DatabaseMannager.h"
#include "User.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("College Tracker - 登入");
    setModal(true);
    setFixedSize(400, 480);

    setStyleSheet(
        "QDialog {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #f0f4f8, stop:1 #d9e2ec);"
        "}"
    );

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(40, 40, 40, 40);
    outerLayout->setSpacing(0);

    // 标题
    QLabel *titleLabel = new QLabel("College Tracker", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 24px; font-weight: bold; color: #2b4c7e;"
        "margin-bottom: 6px;"
    );
    outerLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel("登入您的账号", this);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        "font-size: 14px; color: #627d98; margin-bottom: 30px;"
    );
    outerLayout->addWidget(subtitleLabel);

    auto inputStyle = QStringLiteral(
        "QLineEdit {"
        "  border: 2px solid #bccde0; border-radius: 8px;"
        "  padding: 0 14px; font-size: 14px;"
        "  background: white; color: #334e68;"
        "}"
        "QLineEdit:focus {"
        "  border-color: #3b71ca;"
        "}"
    );

    auto labelStyle = QStringLiteral(
        "font-size: 13px; color: #486581; font-weight: bold; margin-bottom: 4px;"
    );

    // 用户名
    QLabel *userLabel = new QLabel("用户名", this);
    userLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(userLabel);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("请输入用户名");
    m_usernameEdit->setFixedHeight(42);
    m_usernameEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_usernameEdit);

    outerLayout->addSpacing(16);

    // 密码
    QLabel *passLabel = new QLabel("密码", this);
    passLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(passLabel);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_passwordEdit->setFixedHeight(42);
    m_passwordEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_passwordEdit);

    outerLayout->addSpacing(8);

    // 提示信息
    m_messageLabel = new QLabel("", this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setFixedHeight(24);
    m_messageLabel->setStyleSheet("color: #d93232; font-size: 13px;");
    outerLayout->addWidget(m_messageLabel);

    outerLayout->addSpacing(12);

    // 登入按钮
    m_loginBtn = new QPushButton("登 入", this);
    m_loginBtn->setFixedHeight(44);
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    m_loginBtn->setStyleSheet(
        "QPushButton {"
        "  background: #3b71ca; color: white;"
        "  border: none; border-radius: 8px;"
        "  font-size: 15px; font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #2c5fb5;"
        "}"
        "QPushButton:pressed {"
        "  background: #1e4d9c;"
        "}"
    );
    outerLayout->addWidget(m_loginBtn);

    outerLayout->addSpacing(10);

    // 注册按钮
    m_registerBtn = new QPushButton("注册新账号", this);
    m_registerBtn->setFixedHeight(40);
    m_registerBtn->setCursor(Qt::PointingHandCursor);
    m_registerBtn->setStyleSheet(
        "QPushButton {"
        "  background: transparent; color: #3b71ca;"
        "  border: 2px solid #3b71ca; border-radius: 8px;"
        "  font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(59, 113, 202, 0.08);"
        "}"
        "QPushButton:pressed {"
        "  background: rgba(59, 113, 202, 0.16);"
        "}"
    );
    outerLayout->addWidget(m_registerBtn);

    outerLayout->addStretch();

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginDialog::onOpenRegister);
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

    int userId = DatabaseManager::getInstance().loginUser(username, password);
    if (userId != -1) {
        User::getInstance().login(userId);
        accept();
    } else {
        m_messageLabel->setText("用户名或密码错误");
        m_passwordEdit->clear();
    }
}

void LoginDialog::onOpenRegister() {
    RegisterDialog reg(this);
    if (reg.exec() == QDialog::Accepted) {
        m_messageLabel->setStyleSheet("color: #2e7d32; font-size: 13px;");
        m_messageLabel->setText("注册成功，请登入");
    }
}

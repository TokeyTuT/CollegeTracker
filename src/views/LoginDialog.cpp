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
    setFixedSize(420, 520);

    // 暖象牙白渐变底
    setStyleSheet(
        "QDialog {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #FEFAF3, stop:1 #F9F0DE);"
        "}"
    );

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(48, 48, 48, 48);
    outerLayout->setSpacing(0);

    // 标题 — 极端细字重
    QLabel *titleLabel = new QLabel("College Tracker", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 28px; font-weight: 300; color: #3D2E1F;"
        "letter-spacing: 4px; margin-bottom: 4px;"
    );
    outerLayout->addWidget(titleLabel);

    // 副标题
    QLabel *subtitleLabel = new QLabel("登入您的账号", this);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        "font-size: 14px; color: #A89880; font-weight: 400; margin-bottom: 36px;"
    );
    outerLayout->addWidget(subtitleLabel);

    auto inputStyle = QStringLiteral(
        "QLineEdit {"
        "  border: 1.5px solid #DDD0BE; border-radius: 10px;"
        "  padding: 0 18px; font-size: 14px;"
        "  background: #FFFEF9; color: #3D2E1F;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #C8943E;"
        "  background: #FFFDF7;"
        "}"
    );

    auto labelStyle = QStringLiteral(
        "font-size: 13px; color: #8B7355; font-weight: 600; margin-bottom: 6px;"
    );

    // 用户名
    QLabel *userLabel = new QLabel("用户名", this);
    userLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(userLabel);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("请输入用户名");
    m_usernameEdit->setFixedHeight(46);
    m_usernameEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_usernameEdit);

    outerLayout->addSpacing(20);

    // 密码
    QLabel *passLabel = new QLabel("密码", this);
    passLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(passLabel);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_passwordEdit->setFixedHeight(46);
    m_passwordEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_passwordEdit);

    outerLayout->addSpacing(10);

    // 提示信息
    m_messageLabel = new QLabel("", this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setFixedHeight(24);
    m_messageLabel->setStyleSheet("color: #C0392B; font-size: 13px; font-weight: 500;");
    outerLayout->addWidget(m_messageLabel);

    outerLayout->addSpacing(14);

    // 登入按钮 — 暖金色
    m_loginBtn = new QPushButton("登 入", this);
    m_loginBtn->setFixedHeight(48);
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    m_loginBtn->setStyleSheet(
        "QPushButton {"
        "  background: #C8943E; color: #FFFEF9;"
        "  border: none; border-radius: 10px;"
        "  font-size: 16px; font-weight: 700;"
        "  letter-spacing: 2px;"
        "}"
        "QPushButton:hover {"
        "  background: #B8860B;"
        "}"
        "QPushButton:pressed {"
        "  background: #8B6914;"
        "}"
    );
    outerLayout->addWidget(m_loginBtn);

    outerLayout->addSpacing(14);

    // 注册按钮 — 暖金描边
    m_registerBtn = new QPushButton("注册新账号", this);
    m_registerBtn->setFixedHeight(42);
    m_registerBtn->setCursor(Qt::PointingHandCursor);
    m_registerBtn->setStyleSheet(
        "QPushButton {"
        "  background: transparent; color: #A0764A;"
        "  border: 1.5px solid rgba(200, 148, 62, 0.55); border-radius: 10px;"
        "  font-size: 14px; font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(200, 148, 62, 0.08);"
        "  border: 1.5px solid #C8943E;"
        "  color: #8B6914;"
        "}"
        "QPushButton:pressed {"
        "  background: rgba(200, 148, 62, 0.16);"
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
        m_messageLabel->setStyleSheet("color: #5B8C5A; font-size: 13px; font-weight: 500;");
        m_messageLabel->setText("注册成功，请登入");
    }
}

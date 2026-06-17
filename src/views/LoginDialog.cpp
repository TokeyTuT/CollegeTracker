#include "LoginDialog.h"
#include "RegisterDialog.h"
#include "DatabaseMannager.h"
#include "Theme.h"
#include "User.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent) {
    using namespace Theme;

    setWindowTitle("College Tracker - 登入");
    setModal(true);
    setFixedSize(400, 500);

    setStyleSheet(Color::surfaceVariant
                  ? QString("QDialog { background: %1; }").arg(Color::surfaceVariant)
                  : QString());

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(40, 44, 40, 40);
    outerLayout->setSpacing(0);

    // 标题
    QLabel *titleLabel = new QLabel("College Tracker", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        QString("font-size: %1px; font-weight: %2; color: %3; margin-bottom: 6px;")
            .arg(TypeScale::display).arg(FontWeight::heavy).arg(Color::onSurface));
    outerLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel("登入您的账号", this);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        QString("font-size: %1px; color: %2; margin-bottom: 32px;")
            .arg(TypeScale::body).arg(Color::onSurfaceVar));
    outerLayout->addWidget(subtitleLabel);

    auto inputStyle = QString(
        "QLineEdit {"
        "  border: 2px solid %1; border-radius: %2px;"
        "  padding: 0 14px; min-height: 44px;"
        "  font-size: %3px; background: #FFFFFF; color: %4;"
        "}"
        "QLineEdit:focus { border-color: %5; }"
    ).arg(Color::outline).arg(Radius::md)
     .arg(TypeScale::body).arg(Color::onSurface).arg(Color::primary);

    auto labelStyle = QString("font-size: %1px; color: %2; font-weight: %3; margin-bottom: 4px;")
                          .arg(TypeScale::caption).arg(Color::onSurfaceVar).arg(FontWeight::bold);

    // 用户名
    QLabel *userLabel = new QLabel("用户名", this);
    userLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(userLabel);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("请输入用户名");
    m_usernameEdit->setFixedHeight(44);
    m_usernameEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_usernameEdit);

    outerLayout->addSpacing(18);

    // 密码
    QLabel *passLabel = new QLabel("密码", this);
    passLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(passLabel);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_passwordEdit->setFixedHeight(44);
    m_passwordEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_passwordEdit);

    outerLayout->addSpacing(8);

    // 提示信息
    m_messageLabel = new QLabel("", this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setFixedHeight(28);
    m_messageLabel->setStyleSheet(
        QString("color: %1; font-size: %2px;").arg(Color::error).arg(TypeScale::caption));
    outerLayout->addWidget(m_messageLabel);

    outerLayout->addSpacing(14);

    // 登入按钮 — 使用 Theme primary style
    m_loginBtn = new QPushButton("登 入", this);
    m_loginBtn->setFixedHeight(44);
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    m_loginBtn->setStyleSheet(
        QString("QPushButton { background: %1; color: #FFFFFF; border: none;"
                "  border-radius: %2px; font-size: %3px; font-weight: %4; }"
                "QPushButton:hover { background: %5; }"
                "QPushButton:pressed { background: #0B5E57; }")
            .arg(Color::primary).arg(Radius::md)
            .arg(TypeScale::body).arg(FontWeight::bold).arg(Color::primaryHover));
    outerLayout->addWidget(m_loginBtn);

    outerLayout->addSpacing(12);

    // 注册按钮
    m_registerBtn = new QPushButton("注册新账号", this);
    m_registerBtn->setFixedHeight(44);
    m_registerBtn->setCursor(Qt::PointingHandCursor);
    m_registerBtn->setStyleSheet(
        QString("QPushButton { background: transparent; color: %1;"
                "  border: 2px solid %1; border-radius: %2px;"
                "  font-size: %3px; font-weight: %4; }"
                "QPushButton:hover { background: %5; }"
                "QPushButton:pressed { background: rgba(13,148,136,0.14); }")
            .arg(Color::primary).arg(Radius::md)
            .arg(TypeScale::body).arg(FontWeight::medium).arg(Color::primaryBg));
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

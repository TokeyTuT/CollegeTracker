#include "LoginDialog.h"

#include "DatabaseMannager.h"
#include "RegisterDialog.h"

#include "User.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("College Tracker · 登录");
    setModal(true);
    setFixedSize(460, 590);
    setStyleSheet(QStringLiteral(R"QSS(
        QDialog {
            background: #F4F1EA;
            font-family: "Avenir Next", "PingFang SC", sans-serif;
        }
        QLabel#brandMark {
            background: #D97745; color: #FFF9F1; border-radius: 12px;
            font-size: 15px; font-weight: 900; letter-spacing: 1px;
        }
        QLabel#eyebrow {
            color: #1F6B5B; font-size: 11px; font-weight: 800;
            letter-spacing: 2px;
        }
        QLabel#title {
            color: #17201D; font-size: 30px; font-weight: 850;
        }
        QLabel#subtitle {
            color: #68716D; font-size: 14px; font-weight: 500;
        }
        QLabel#fieldLabel {
            color: #46524E; font-size: 12px; font-weight: 750;
        }
        QLineEdit {
            min-height: 46px; background: #FFFEFA; color: #17201D;
            border: 1px solid #D6D0C4; border-radius: 10px;
            padding: 0 14px; font-size: 16px;
            selection-background-color: #BFD6CD;
        }
        QLineEdit:hover { border-color: #9DABA5; }
        QLineEdit:focus { border: 2px solid #1F6B5B; }
        QPushButton {
            min-height: 46px; border-radius: 10px;
            font-size: 15px; font-weight: 750;
        }
        QPushButton#loginButton {
            background: #1F6B5B; color: #FFF; border: 1px solid #1F6B5B;
        }
        QPushButton#loginButton:hover {
            background: #174F44; border-color: #174F44;
        }
        QPushButton#registerButton {
            background: transparent; color: #38564F;
            border: 1px solid #B8C4BE;
        }
        QPushButton#registerButton:hover {
            background: #E6ECE8; border-color: #7F958C;
        }
        QLabel#messageLabel {
            color: #B94B45; font-size: 12px; font-weight: 650;
        }
        QLabel#footer {
            color: #969C98; font-size: 10px; letter-spacing: 0.5px;
        }
    )QSS"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(46, 40, 46, 32);
    layout->setSpacing(0);

    auto *brandRow = new QHBoxLayout;
    brandRow->setSpacing(12);
    auto *brandMark = new QLabel("CT", this);
    brandMark->setObjectName("brandMark");
    brandMark->setFixedSize(44, 44);
    brandMark->setAlignment(Qt::AlignCenter);
    auto *eyebrow = new QLabel("COLLEGE TRACKER", this);
    eyebrow->setObjectName("eyebrow");
    brandRow->addWidget(brandMark);
    brandRow->addWidget(eyebrow);
    brandRow->addStretch();
    layout->addLayout(brandRow);

    layout->addSpacing(38);
    auto *title = new QLabel("欢迎回来", this);
    title->setObjectName("title");
    layout->addWidget(title);

    layout->addSpacing(7);
    auto *subtitle = new QLabel("继续整理你的课程、经历与成果。", this);
    subtitle->setObjectName("subtitle");
    layout->addWidget(subtitle);

    layout->addSpacing(32);
    auto addField = [this, layout](const QString &labelText,
                                   const QString &placeholder,
                                   QLineEdit **field) {
        auto *label = new QLabel(labelText, this);
        label->setObjectName("fieldLabel");
        layout->addWidget(label);
        layout->addSpacing(7);
        *field = new QLineEdit(this);
        (*field)->setPlaceholderText(placeholder);
        layout->addWidget(*field);
    };

    addField("用户名", "例如：洞穴学霸007", &m_usernameEdit);
    layout->addSpacing(18);
    addField("密码", "输入你的秘密咒语", &m_passwordEdit);
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    layout->addSpacing(8);
    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName("messageLabel");
    m_messageLabel->setFixedHeight(24);
    layout->addWidget(m_messageLabel);

    layout->addSpacing(10);
    m_loginBtn = new QPushButton("登录", this);
    m_loginBtn->setObjectName("loginButton");
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    m_loginBtn->setDefault(true);
    layout->addWidget(m_loginBtn);

    layout->addSpacing(10);
    m_registerBtn = new QPushButton("创建新账号", this);
    m_registerBtn->setObjectName("registerButton");
    m_registerBtn->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_registerBtn);

    layout->addStretch();
    auto *footer = new QLabel("你的数据仅保存在本机 SQLite 数据库中", this);
    footer->setObjectName("footer");
    footer->setAlignment(Qt::AlignCenter);
    layout->addWidget(footer);

    connect(m_loginBtn, &QPushButton::clicked, this,
            &LoginDialog::onLoginClicked);
    connect(m_registerBtn, &QPushButton::clicked, this,
            &LoginDialog::onOpenRegister);
}

QString LoginDialog::getUsername() const { return m_usernameEdit->text(); }

void LoginDialog::onLoginClicked() {
    const QString username = m_usernameEdit->text().trimmed();
    const QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        m_messageLabel->setText("请输入用户名和密码");
        return;
    }

    const int userId =
        DatabaseManager::getInstance().loginUser(username, password);
    if (userId != -1) {
        User::getInstance().login(userId);
        accept();
    } else {
        m_messageLabel->setText("用户名或密码不正确");
        m_passwordEdit->clear();
        m_passwordEdit->setFocus();
    }
}

void LoginDialog::onOpenRegister() {
    RegisterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        m_messageLabel->setStyleSheet(
            "color:#43846F; font-size:12px; font-weight:650;");
        m_messageLabel->setText("账号创建成功，现在可以登录了");
    }
}

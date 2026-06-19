#include "RegisterDialog.h"

#include "DatabaseMannager.h"

#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("College Tracker · 创建账号");
    setModal(true);
    setFixedSize(520, 650);
    setStyleSheet(QStringLiteral(R"QSS(
        QDialog {
            background: #F4F1EA;
            font-family: "Avenir Next", "PingFang SC", sans-serif;
        }
        QLabel#eyebrow {
            color: #D97745; font-size: 11px; font-weight: 850;
            letter-spacing: 2px;
        }
        QLabel#title { color: #17201D; font-size: 28px; font-weight: 850; }
        QLabel#subtitle { color: #68716D; font-size: 13px; }
        QLabel#fieldLabel {
            color: #46524E; font-size: 12px; font-weight: 750;
        }
        QLineEdit, QComboBox {
            min-height: 43px; background: #FFFEFA; color: #17201D;
            border: 1px solid #D6D0C4; border-radius: 9px;
            padding: 0 12px; font-size: 15px;
            selection-background-color: #BFD6CD;
        }
        QLineEdit:hover, QComboBox:hover { border-color: #9DABA5; }
        QLineEdit:focus, QComboBox:focus { border: 2px solid #1F6B5B; }
        QComboBox::drop-down { border: none; width: 30px; }
        QPushButton {
            min-height: 44px; border-radius: 10px;
            font-size: 14px; font-weight: 750;
        }
        QPushButton#registerButton {
            background: #1F6B5B; color: #FFF; border: 1px solid #1F6B5B;
        }
        QPushButton#registerButton:hover {
            background: #174F44; border-color: #174F44;
        }
        QPushButton#backButton {
            background: transparent; color: #38564F;
            border: 1px solid #B8C4BE;
        }
        QPushButton#backButton:hover { background: #E6ECE8; }
        QLabel#messageLabel {
            color: #B94B45; font-size: 12px; font-weight: 650;
        }
    )QSS"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(42, 34, 42, 30);
    layout->setSpacing(0);

    auto *eyebrow = new QLabel("NEW PROFILE", this);
    eyebrow->setObjectName("eyebrow");
    layout->addWidget(eyebrow);
    layout->addSpacing(8);

    auto *title = new QLabel("创建你的学业档案", this);
    title->setObjectName("title");
    layout->addWidget(title);
    layout->addSpacing(6);

    auto *subtitle = new QLabel("先填写基础信息，之后都可以在应用内修改。", this);
    subtitle->setObjectName("subtitle");
    layout->addWidget(subtitle);
    layout->addSpacing(26);

    auto *form = new QGridLayout;
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(8);

    auto addField = [this, form](int row, int column, const QString &text,
                                 QWidget *field) {
        auto *label = new QLabel(text, this);
        label->setObjectName("fieldLabel");
        form->addWidget(label, row * 2, column);
        form->addWidget(field, row * 2 + 1, column);
    };

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("用于登录");
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setPlaceholderText("至少 6 位");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    addField(0, 0, "用户名", m_usernameEdit);
    addField(0, 1, "密码", m_passwordEdit);

    m_confirmEdit = new QLineEdit(this);
    m_confirmEdit->setPlaceholderText("再次输入密码");
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    m_schoolEdit = new QLineEdit(this);
    m_schoolEdit->setPlaceholderText("例如：浙江大学");
    addField(1, 0, "确认密码", m_confirmEdit);
    addField(1, 1, "学校", m_schoolEdit);

    m_gradeCombo = new QComboBox(this);
    m_gradeCombo->addItems({"大一", "大二", "大三", "大四"});
    m_genderCombo = new QComboBox(this);
    m_genderCombo->addItems({"男", "女", "其他", "不显示"});
    addField(2, 0, "年级", m_gradeCombo);
    addField(2, 1, "性别", m_genderCombo);

    m_majorEdit = new QLineEdit(this);
    m_majorEdit->setPlaceholderText("例如：计算机科学与技术");
    addField(3, 0, "专业", m_majorEdit);
    form->addWidget(m_majorEdit, 7, 0, 1, 2);

    layout->addLayout(form);
    layout->addSpacing(12);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName("messageLabel");
    m_messageLabel->setFixedHeight(24);
    layout->addWidget(m_messageLabel);

    layout->addStretch();
    m_registerBtn = new QPushButton("创建账号", this);
    m_registerBtn->setObjectName("registerButton");
    m_registerBtn->setCursor(Qt::PointingHandCursor);
    m_registerBtn->setDefault(true);
    layout->addWidget(m_registerBtn);
    layout->addSpacing(10);

    m_backBtn = new QPushButton("返回登录", this);
    m_backBtn->setObjectName("backButton");
    m_backBtn->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_backBtn);

    connect(m_registerBtn, &QPushButton::clicked,
            this, &RegisterDialog::onRegisterClicked);
    connect(m_backBtn, &QPushButton::clicked,
            this, &RegisterDialog::onBackClicked);
}

void RegisterDialog::onRegisterClicked() {
    const QString username = m_usernameEdit->text().trimmed();
    const QString password = m_passwordEdit->text();
    const QString confirm = m_confirmEdit->text();

    if (username.isEmpty() || password.isEmpty() || confirm.isEmpty()) {
        showMessage("请完整填写账号信息", true);
        return;
    }
    if (password.length() < 6) {
        showMessage("密码长度不能少于 6 位", true);
        return;
    }
    if (password != confirm) {
        showMessage("两次输入的密码不一致", true);
        return;
    }

    const QString grade = m_gradeCombo->currentText();
    QString gender = m_genderCombo->currentText();
    if (gender == "不显示")
        gender.clear();
    const QString major = m_majorEdit->text().trimmed();
    const QString school = m_schoolEdit->text().trimmed();

    if (school.isEmpty() || major.isEmpty()) {
        showMessage("请填写学校和专业", true);
        return;
    }

    if (DatabaseManager::getInstance().registerUser(
            username, password, grade, gender, major, school)) {
        accept();
    } else {
        showMessage("这个用户名已经被使用", true);
    }
}

void RegisterDialog::onBackClicked() {
    reject();
}

void RegisterDialog::showMessage(const QString &msg, bool isError) {
    m_messageLabel->setText(msg);
    m_messageLabel->setStyleSheet(
        isError
            ? "color:#B94B45; font-size:12px; font-weight:650;"
            : "color:#43846F; font-size:12px; font-weight:650;");
}

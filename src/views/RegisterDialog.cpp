#include "RegisterDialog.h"
#include "DatabaseMannager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("College Tracker - 注册");
    setModal(true);
    setFixedSize(400, 620);

    setStyleSheet(
        "QDialog {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #f0f4f8, stop:1 #d9e2ec);"
        "}"
    );

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(40, 40, 40, 40);
    outerLayout->setSpacing(0);

    QLabel *titleLabel = new QLabel("创建账号", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 24px; font-weight: bold; color: #2b4c7e;"
        "margin-bottom: 6px;"
    );
    outerLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel("注册一个新的账号", this);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        "font-size: 14px; color: #627d98; margin-bottom: 20px;"
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

    auto comboStyle = QStringLiteral(
        "QComboBox {"
        "  border: 2px solid #bccde0; border-radius: 8px;"
        "  padding: 0 10px; font-size: 14px;"
        "  background: white; color: #334e68;"
        "}"
        "QComboBox:focus {"
        "  border-color: #3b71ca;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "}"
    );

    auto labelStyle = QStringLiteral(
        "font-size: 13px; color: #486581; font-weight: bold; margin-bottom: 2px;"
    );

    // 用户名
    QLabel *userLabel = new QLabel("用户名", this);
    userLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(userLabel);

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("请输入用户名");
    m_usernameEdit->setFixedHeight(38);
    m_usernameEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_usernameEdit);

    outerLayout->addSpacing(10);

    // 密码
    QLabel *passLabel = new QLabel("密码", this);
    passLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(passLabel);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("请输入密码（至少6位）");
    m_passwordEdit->setFixedHeight(38);
    m_passwordEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_passwordEdit);

    outerLayout->addSpacing(10);

    // 确认密码
    QLabel *confirmLabel = new QLabel("确认密码", this);
    confirmLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(confirmLabel);

    m_confirmEdit = new QLineEdit(this);
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    m_confirmEdit->setPlaceholderText("请再次输入密码");
    m_confirmEdit->setFixedHeight(38);
    m_confirmEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_confirmEdit);

    outerLayout->addSpacing(10);

    // 年级
    QLabel *gradeLabel = new QLabel("年级", this);
    gradeLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(gradeLabel);

    m_gradeCombo = new QComboBox(this);
    m_gradeCombo->addItems({"大一", "大二", "大三", "大四"});
    m_gradeCombo->setFixedHeight(38);
    m_gradeCombo->setStyleSheet(comboStyle);
    outerLayout->addWidget(m_gradeCombo);

    outerLayout->addSpacing(10);

    // 性别
    QLabel *genderLabel = new QLabel("性别", this);
    genderLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(genderLabel);

    m_genderCombo = new QComboBox(this);
    m_genderCombo->addItems({"男", "女"});
    m_genderCombo->setFixedHeight(38);
    m_genderCombo->setStyleSheet(comboStyle);
    outerLayout->addWidget(m_genderCombo);

    outerLayout->addSpacing(10);

    // 专业
    QLabel *majorLabel = new QLabel("专业", this);
    majorLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(majorLabel);

    m_majorEdit = new QLineEdit(this);
    m_majorEdit->setPlaceholderText("请输入专业");
    m_majorEdit->setFixedHeight(38);
    m_majorEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_majorEdit);

    outerLayout->addSpacing(6);

    // 提示信息
    m_messageLabel = new QLabel("", this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setFixedHeight(24);
    m_messageLabel->setStyleSheet("color: #d93232; font-size: 13px;");
    outerLayout->addWidget(m_messageLabel);

    outerLayout->addSpacing(8);

    // 注册按钮
    m_registerBtn = new QPushButton("注 册", this);
    m_registerBtn->setFixedHeight(40);
    m_registerBtn->setCursor(Qt::PointingHandCursor);
    m_registerBtn->setStyleSheet(
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
    outerLayout->addWidget(m_registerBtn);

    outerLayout->addSpacing(8);

    // 返回按钮
    m_backBtn = new QPushButton("返回登入", this);
    m_backBtn->setFixedHeight(36);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(
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
    outerLayout->addWidget(m_backBtn);

    outerLayout->addStretch();

    connect(m_registerBtn, &QPushButton::clicked, this, &RegisterDialog::onRegisterClicked);
    connect(m_backBtn, &QPushButton::clicked, this, &RegisterDialog::onBackClicked);
}

void RegisterDialog::onRegisterClicked() {
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();
    QString confirm = m_confirmEdit->text();

    if (username.isEmpty() || password.isEmpty() || confirm.isEmpty()) {
        showMessage("所有字段均不能为空", true);
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

    QString grade = m_gradeCombo->currentText();
    QString gender = m_genderCombo->currentText();
    QString major = m_majorEdit->text().trimmed();

    DatabaseManager &db = DatabaseManager::getInstance();
    if (db.registerUser(username, password, grade, gender, major)) {
        accept();
    } else {
        showMessage("用户名已存在", true);
    }
}

void RegisterDialog::onBackClicked() {
    reject();
}

void RegisterDialog::showMessage(const QString &msg, bool isError) {
    m_messageLabel->setText(msg);
    m_messageLabel->setStyleSheet(
        isError
            ? "color: #d93232; font-size: 13px;"
            : "color: #2e7d32; font-size: 13px;"
    );
}

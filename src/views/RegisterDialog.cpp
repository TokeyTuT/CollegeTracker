#include "RegisterDialog.h"
#include "DatabaseMannager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("College Tracker - 注册");
    setModal(true);
    setFixedSize(420, 720);

    // 暖象牙白渐变底
    setStyleSheet(
        "QDialog {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #FEFAF3, stop:1 #F9F0DE);"
        "}"
    );

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(48, 42, 48, 42);
    outerLayout->setSpacing(0);

    // 标题 — 极端细字重
    QLabel *titleLabel = new QLabel("创建账号", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 28px; font-weight: 300; color: #3D2E1F;"
        "letter-spacing: 4px; margin-bottom: 4px;"
    );
    outerLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel("注册一个新的账号", this);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        "font-size: 14px; color: #A89880; font-weight: 400; margin-bottom: 24px;"
    );
    outerLayout->addWidget(subtitleLabel);

    auto inputStyle = QStringLiteral(
        "QLineEdit {"
        "  border: 1.5px solid #DDD0BE; border-radius: 10px;"
        "  padding: 0 16px; font-size: 14px;"
        "  background: #FFFEF9; color: #3D2E1F;"
        "}"
        "QLineEdit:focus {"
        "  border: 2px solid #C8943E;"
        "  background: #FFFDF7;"
        "}"
    );

    auto comboStyle = QStringLiteral(
        "QComboBox {"
        "  border: 1.5px solid #DDD0BE; border-radius: 10px;"
        "  padding: 0 14px; font-size: 14px;"
        "  background: #FFFEF9; color: #3D2E1F;"
        "}"
        "QComboBox:focus {"
        "  border-color: #C8943E;"
        "}"
        "QComboBox::drop-down {"
        "  border: none; width: 28px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #FFFEF9; border: 1px solid #DDD0BE;"
        "  selection-background-color: #C8943E;"
        "  selection-color: #FFFEF9;"
        "}"
    );

    auto labelStyle = QStringLiteral(
        "font-size: 13px; color: #8B7355; font-weight: 600; margin-bottom: 2px;"
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

    outerLayout->addSpacing(12);

    // 密码
    QLabel *passLabel = new QLabel("密码", this);
    passLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(passLabel);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("请输入密码（至少6位）");
    m_passwordEdit->setFixedHeight(42);
    m_passwordEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_passwordEdit);

    outerLayout->addSpacing(12);

    // 确认密码
    QLabel *confirmLabel = new QLabel("确认密码", this);
    confirmLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(confirmLabel);

    m_confirmEdit = new QLineEdit(this);
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    m_confirmEdit->setPlaceholderText("请再次输入密码");
    m_confirmEdit->setFixedHeight(42);
    m_confirmEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_confirmEdit);

    outerLayout->addSpacing(12);

    // 学校
    QLabel *schoolLabel = new QLabel("学校", this);
    schoolLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(schoolLabel);

    m_schoolEdit = new QLineEdit(this);
    m_schoolEdit->setPlaceholderText("请输入学校");
    m_schoolEdit->setFixedHeight(42);
    m_schoolEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_schoolEdit);

    outerLayout->addSpacing(12);

    // 年级
    QLabel *gradeLabel = new QLabel("年级", this);
    gradeLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(gradeLabel);

    m_gradeCombo = new QComboBox(this);
    m_gradeCombo->addItems({"大一", "大二", "大三", "大四"});
    m_gradeCombo->setFixedHeight(42);
    m_gradeCombo->setStyleSheet(comboStyle);
    outerLayout->addWidget(m_gradeCombo);

    outerLayout->addSpacing(12);

    // 性别
    QLabel *genderLabel = new QLabel("性别", this);
    genderLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(genderLabel);

    m_genderCombo = new QComboBox(this);
    m_genderCombo->addItems({"男", "女"});
    m_genderCombo->setFixedHeight(42);
    m_genderCombo->setStyleSheet(comboStyle);
    outerLayout->addWidget(m_genderCombo);

    outerLayout->addSpacing(12);

    // 专业
    QLabel *majorLabel = new QLabel("专业", this);
    majorLabel->setStyleSheet(labelStyle);
    outerLayout->addWidget(majorLabel);

    m_majorEdit = new QLineEdit(this);
    m_majorEdit->setPlaceholderText("请输入专业");
    m_majorEdit->setFixedHeight(42);
    m_majorEdit->setStyleSheet(inputStyle);
    outerLayout->addWidget(m_majorEdit);

    outerLayout->addSpacing(10);

    // 提示信息
    m_messageLabel = new QLabel("", this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setFixedHeight(24);
    m_messageLabel->setStyleSheet("color: #C0392B; font-size: 13px; font-weight: 500;");
    outerLayout->addWidget(m_messageLabel);

    outerLayout->addSpacing(10);

    // 注册按钮 — 暖金色
    m_registerBtn = new QPushButton("注 册", this);
    m_registerBtn->setFixedHeight(44);
    m_registerBtn->setCursor(Qt::PointingHandCursor);
    m_registerBtn->setStyleSheet(
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
    outerLayout->addWidget(m_registerBtn);

    outerLayout->addSpacing(10);

    // 返回按钮 — 暖金描边
    m_backBtn = new QPushButton("返回登入", this);
    m_backBtn->setFixedHeight(38);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(
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
    QString school = m_schoolEdit->text().trimmed();

    if (school.isEmpty() || major.isEmpty()) {
        showMessage("学校和专业不能为空", true);
        return;
    }

    DatabaseManager &db = DatabaseManager::getInstance();
    if (db.registerUser(username, password, grade, gender, major, school)) {
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
            ? "color: #C0392B; font-size: 13px; font-weight: 500;"
            : "color: #5B8C5A; font-size: 13px; font-weight: 500;"
    );
}

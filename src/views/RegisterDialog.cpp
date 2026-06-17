#include "RegisterDialog.h"
#include "DatabaseMannager.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent) {
    using namespace Theme;

    setWindowTitle("College Tracker - 注册");
    setModal(true);
    setFixedSize(420, 680);

    setStyleSheet(QString("QDialog { background: %1; }").arg(Color::surfaceVariant));

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(40, 36, 40, 36);
    outerLayout->setSpacing(0);

    QLabel *titleLabel = new QLabel("创建账号", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        QString("font-size: %1px; font-weight: %2; color: %3; margin-bottom: 6px;")
            .arg(TypeScale::display).arg(FontWeight::heavy).arg(Color::onSurface));
    outerLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel("注册一个新的账号", this);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        QString("font-size: %1px; color: %2; margin-bottom: 24px;")
            .arg(TypeScale::body).arg(Color::onSurfaceVar));
    outerLayout->addWidget(subtitleLabel);

    auto inputStyle = QString(
        "QLineEdit {"
        "  border: 2px solid %1; border-radius: %2px;"
        "  padding: 0 14px; min-height: 40px;"
        "  font-size: %3px; background: #FFFFFF; color: %4;"
        "}"
        "QLineEdit:focus { border-color: %5; }"
    ).arg(Color::outline).arg(Radius::md)
     .arg(TypeScale::body).arg(Color::onSurface).arg(Color::primary);

    auto comboStyle = QString(
        "QComboBox {"
        "  border: 2px solid %1; border-radius: %2px;"
        "  padding: 0 10px; min-height: 40px;"
        "  font-size: %3px; background: #FFFFFF; color: %4;"
        "}"
        "QComboBox:focus { border-color: %5; }"
        "QComboBox::drop-down { border: none; width: 28px; }"
    ).arg(Color::outline).arg(Radius::md)
     .arg(TypeScale::body).arg(Color::onSurface).arg(Color::primary);

    auto labelStyle = QString("font-size: %1px; color: %2; font-weight: %3; margin-bottom: 2px;")
                          .arg(TypeScale::caption).arg(Color::onSurfaceVar).arg(FontWeight::bold);

    // 辅助宏：快速添加 Label + Widget
    auto addField = [&](const QString &text, QWidget *widget) {
        QLabel *label = new QLabel(text, this);
        label->setStyleSheet(labelStyle);
        outerLayout->addWidget(label);
        outerLayout->addWidget(widget);
        outerLayout->addSpacing(12);
    };

    // 用户名
    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("请输入用户名");
    m_usernameEdit->setFixedHeight(40);
    m_usernameEdit->setStyleSheet(inputStyle);
    addField("用户名", m_usernameEdit);

    // 密码
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("请输入密码（至少6位）");
    m_passwordEdit->setFixedHeight(40);
    m_passwordEdit->setStyleSheet(inputStyle);
    addField("密码", m_passwordEdit);

    // 确认密码
    m_confirmEdit = new QLineEdit(this);
    m_confirmEdit->setEchoMode(QLineEdit::Password);
    m_confirmEdit->setPlaceholderText("请再次输入密码");
    m_confirmEdit->setFixedHeight(40);
    m_confirmEdit->setStyleSheet(inputStyle);
    addField("确认密码", m_confirmEdit);

    // 学校
    m_schoolEdit = new QLineEdit(this);
    m_schoolEdit->setPlaceholderText("请输入学校");
    m_schoolEdit->setFixedHeight(40);
    m_schoolEdit->setStyleSheet(inputStyle);
    addField("学校", m_schoolEdit);

    // 年级
    m_gradeCombo = new QComboBox(this);
    m_gradeCombo->addItems({"大一", "大二", "大三", "大四"});
    m_gradeCombo->setFixedHeight(40);
    m_gradeCombo->setStyleSheet(comboStyle);
    addField("年级", m_gradeCombo);

    // 性别
    m_genderCombo = new QComboBox(this);
    m_genderCombo->addItems({"男", "女"});
    m_genderCombo->setFixedHeight(40);
    m_genderCombo->setStyleSheet(comboStyle);
    addField("性别", m_genderCombo);

    // 专业
    m_majorEdit = new QLineEdit(this);
    m_majorEdit->setPlaceholderText("请输入专业");
    m_majorEdit->setFixedHeight(40);
    m_majorEdit->setStyleSheet(inputStyle);
    addField("专业", m_majorEdit);

    // 提示信息
    m_messageLabel = new QLabel("", this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setFixedHeight(24);
    m_messageLabel->setStyleSheet(
        QString("color: %1; font-size: %2px;").arg(Color::error).arg(TypeScale::caption));
    outerLayout->addWidget(m_messageLabel);

    outerLayout->addSpacing(8);

    // 注册按钮
    m_registerBtn = new QPushButton("注 册", this);
    m_registerBtn->setFixedHeight(44);
    m_registerBtn->setCursor(Qt::PointingHandCursor);
    m_registerBtn->setStyleSheet(
        QString("QPushButton { background: %1; color: #FFFFFF; border: none;"
                "  border-radius: %2px; font-size: %3px; font-weight: %4; }"
                "QPushButton:hover { background: %5; }"
                "QPushButton:pressed { background: #0B5E57; }")
            .arg(Color::primary).arg(Radius::md)
            .arg(TypeScale::body).arg(FontWeight::bold).arg(Color::primaryHover));
    outerLayout->addWidget(m_registerBtn);

    outerLayout->addSpacing(10);

    // 返回按钮
    m_backBtn = new QPushButton("返回登入", this);
    m_backBtn->setFixedHeight(40);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(
        QString("QPushButton { background: transparent; color: %1;"
                "  border: 2px solid %1; border-radius: %2px;"
                "  font-size: %3px; font-weight: %4; }"
                "QPushButton:hover { background: %5; }"
                "QPushButton:pressed { background: rgba(13,148,136,0.14); }")
            .arg(Color::primary).arg(Radius::md)
            .arg(TypeScale::body).arg(FontWeight::medium).arg(Color::primaryBg));
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
            ? "color: #d93232; font-size: 13px;"
            : "color: #2e7d32; font-size: 13px;"
    );
}

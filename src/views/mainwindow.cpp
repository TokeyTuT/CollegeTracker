#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "CoursePage.h"
#include "AvatarUtils.h"
#include "DatabaseMannager.h"
#include "ExperiencePage.h"
#include "HomePage.h"
#include "ResumePage.h"
#include "User.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>

namespace {

QString educationYear(const QString &date) {
    const QRegularExpressionMatch match =
        QRegularExpression(QStringLiteral("(?:19|20)\\d{2}")).match(date);
    return match.hasMatch() ? match.captured(0) : QString();
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    applyModernStyle();
    createPages();
    initFrame();
    updateSidebarUserInfo();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::createPages() {
    m_homePage = new HomePage(ui->stackedWidget);
    m_coursePage = new CoursePage(ui->stackedWidget);
    m_experiencePage = new ExperiencePage(ui->stackedWidget);
    m_resumePage = new ResumePage(ui->stackedWidget);
    ui->stackedWidget->addWidget(m_homePage);
    ui->stackedWidget->addWidget(m_coursePage);
    ui->stackedWidget->addWidget(m_experiencePage);
    ui->stackedWidget->addWidget(m_resumePage);

    connect(m_coursePage, &CoursePage::dataChanged, m_homePage,
            &HomePage::refresh);
    connect(m_coursePage, &CoursePage::dataChanged, m_resumePage,
            &ResumePage::refresh);
    connect(m_experiencePage, &ExperiencePage::dataChanged, m_homePage,
            &HomePage::refresh);
    connect(m_experiencePage, &ExperiencePage::dataChanged, m_resumePage,
            &ResumePage::refresh);
    connect(m_homePage, &HomePage::allDataChanged, m_coursePage,
            &CoursePage::refresh);
    connect(m_homePage, &HomePage::allDataChanged, m_experiencePage,
            &ExperiencePage::refresh);
    connect(m_homePage, &HomePage::allDataChanged, m_resumePage,
            &ResumePage::refresh);
    connect(m_resumePage, &ResumePage::photoChanged, this,
            [this]() { updateSidebarAvatar(); });
}

void MainWindow::applyModernStyle() {
    setWindowTitle(QStringLiteral("College Tracker"));
    resize(1240, 780);
    setMinimumSize(1080, 690);
    ui->menubar->hide();
    ui->statusbar->hide();
    ui->mainContentFrame->setFrameShape(QFrame::NoFrame);
    ui->sidebarFrame->setFrameShape(QFrame::NoFrame);

    QFile styleFile(QStringLiteral(":/styles/mainwindow.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
        setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    else
        qWarning() << "无法加载主窗口样式：" << styleFile.errorString();
}

void MainWindow::initFrame() {
    ui->sidebarFrame->setFixedWidth(252);
    ui->userProfileWidget->setMinimumHeight(190);
    ui->userNameLbl->setMinimumHeight(30);
    ui->detailLbl->setMaximumWidth(112);
    ui->majorLbl->setMinimumHeight(42);
    ui->userNameLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->detailLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->majorLbl->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ui->majorLbl->setWordWrap(true);

    auto *identityRow = new QWidget(ui->userProfileWidget);
    auto *identityLayout = new QHBoxLayout(identityRow);
    identityLayout->setContentsMargins(0, 0, 0, 0);
    identityLayout->setSpacing(12);
    m_sidebarAvatarLabel = new QLabel(identityRow);
    m_sidebarAvatarLabel->setObjectName(QStringLiteral("sidebarAvatarLbl"));
    m_sidebarAvatarLabel->setFixedSize(60, 60);
    m_sidebarAvatarLabel->setAlignment(Qt::AlignCenter);
    identityLayout->addWidget(m_sidebarAvatarLabel);
    auto *identityTextLayout = new QVBoxLayout;
    identityTextLayout->setContentsMargins(0, 2, 0, 2);
    identityTextLayout->setSpacing(5);
    identityTextLayout->addWidget(ui->userNameLbl);
    identityTextLayout->addWidget(ui->detailLbl, 0, Qt::AlignLeft);
    identityTextLayout->addStretch();
    identityLayout->addLayout(identityTextLayout, 1);
    ui->verticalLayout->insertWidget(0, identityRow);

    m_editProfileButton =
        new QPushButton(QStringLiteral("编辑资料"), ui->userProfileWidget);
    m_editProfileButton->setObjectName(QStringLiteral("editProfileBtn"));
    m_editProfileButton->setFlat(true);
    ui->verticalLayout->addWidget(m_editProfileButton);
    connect(m_editProfileButton, &QPushButton::clicked, this,
            &MainWindow::openEditProfileDialog);

    m_logoutButton =
        new QPushButton(QStringLiteral("退出登录"), ui->sidebarFrame);
    m_logoutButton->setObjectName(QStringLiteral("logoutBtn"));
    m_logoutButton->setFixedHeight(42);
    m_logoutButton->setToolTip(QStringLiteral("退出当前账号并返回登录页"));
    ui->verticalLayout_3->addWidget(m_logoutButton);
    connect(m_logoutButton, &QPushButton::clicked, this,
            &MainWindow::logout);

    const QList<QPushButton *> navigationButtons = {
        ui->navHomeBtn, ui->navCourseBtn, ui->navExpBtn, ui->navExportBtn};
    for (QPushButton *button : navigationButtons) {
        button->setFixedHeight(48);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setFlat(true);
        button->setAutoDefault(false);
        button->setDefault(false);
        button->setAttribute(Qt::WA_StyledBackground, true);
    }

    connect(ui->navHomeBtn, &QPushButton::clicked, this, [this]() {
        setCurrentPage(0, QStringLiteral("首页总览"),
                       QStringLiteral("把每一段成长，整理成清晰的轨迹。"),
                       ui->navHomeBtn);
        m_homePage->refresh();
    });
    connect(ui->navCourseBtn, &QPushButton::clicked, this, [this]() {
        setCurrentPage(
            1, QStringLiteral("课程与成绩"),
            QStringLiteral("记录课程、学分与绩点，随时看见学习节奏。"),
            ui->navCourseBtn);
    });
    connect(ui->navExpBtn, &QPushButton::clicked, this, [this]() {
        setCurrentPage(
            2, QStringLiteral("经历与荣誉"),
            QStringLiteral("把课堂之外的投入，沉淀成可复用的履历。"),
            ui->navExpBtn);
    });
    connect(ui->navExportBtn, &QPushButton::clicked, this, [this]() {
        setCurrentPage(
            3, QStringLiteral("简历导出"),
            QStringLiteral("整理关键信息，生成一份真正属于你的简历。"),
            ui->navExportBtn);
        m_resumePage->refresh();
    });

    for (QPushButton *button : findChildren<QPushButton *>())
        button->setCursor(Qt::PointingHandCursor);

    setCurrentPage(0, QStringLiteral("首页总览"),
                   QStringLiteral("把每一段成长，整理成清晰的轨迹。"),
                   ui->navHomeBtn);
}

void MainWindow::setCurrentPage(int index, const QString &title,
                                const QString &subtitle,
                                QPushButton *activeButton) {
    m_resumePage->hideTemplatePreview();
    ui->stackedWidget->setCurrentIndex(index);
    ui->currentPageLbl->setText(title);
    ui->headerSubtitleLbl->setText(subtitle);
    const QList<QPushButton *> navigationButtons = {
        ui->navHomeBtn, ui->navCourseBtn, ui->navExpBtn, ui->navExportBtn};
    for (QPushButton *button : navigationButtons) {
        button->setProperty("active", button == activeButton);
        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }
}

void MainWindow::updateSidebarUserInfo() {
    const User &user = User::getInstance();
    if (!user.isLoggedIn())
        return;
    ui->userNameLbl->setText(user.getUsername());
    ui->majorLbl->setText(
        user.getSchool().isEmpty()
            ? user.getMajor()
            : user.getSchool() + QStringLiteral("\n") + user.getMajor());
    ui->detailLbl->setText(
        user.getGender().isEmpty()
            ? user.getGrade()
            : user.getGrade() + QStringLiteral(" | ") + user.getGender());
    updateSidebarAvatar();
}

void MainWindow::updateSidebarAvatar() {
    if (!m_sidebarAvatarLabel)
        return;
    const QString photoPath =
        m_resumePage ? m_resumePage->photoPath() : QString();
    m_sidebarAvatarLabel->setPixmap(
        AvatarUtils::circularAvatar(photoPath, 60));
    m_sidebarAvatarLabel->setText(QString());
    m_sidebarAvatarLabel->setToolTip(
        photoPath.isEmpty() ? QStringLiteral("默认哥布林头像")
                            : QStringLiteral("个人头像"));
}

void MainWindow::openEditProfileDialog() {
    User &user = User::getInstance();
    if (!user.isLoggedIn())
        return;

    QVariantMap primaryEducation;
    const QVariantList educationRecords =
        DatabaseManager::getInstance().getEducationRecords(user.getId());
    for (const QVariant &value : educationRecords) {
        const QVariantMap record = value.toMap();
        if (record.value(QStringLiteral("school")).toString() ==
                user.getSchool() &&
            record.value(QStringLiteral("major")).toString() ==
                user.getMajor()) {
            primaryEducation = record;
            break;
        }
    }
    if (primaryEducation.isEmpty() && !educationRecords.isEmpty())
        primaryEducation = educationRecords.first().toMap();

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("修改个人信息"));
    dialog.setMinimumWidth(480);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 24, 26, 22);
    layout->setSpacing(16);
    auto *title = new QLabel(QStringLiteral("修改个人信息"));
    title->setStyleSheet(
        QStringLiteral("font-size:22px;font-weight:900;color:#0F172A;"));
    layout->addWidget(title);

    auto *photoSection = new QFrame(&dialog);
    photoSection->setObjectName(QStringLiteral("profilePhotoSection"));
    photoSection->setStyleSheet(QStringLiteral(
        "QFrame#profilePhotoSection{background:#FFFEFA;"
        "border:1px solid #DED8CC;border-radius:12px;}"));
    auto *photoLayout = new QHBoxLayout(photoSection);
    auto *avatarPreview = new QLabel(photoSection);
    avatarPreview->setFixedSize(72, 72);
    avatarPreview->setAlignment(Qt::AlignCenter);
    avatarPreview->setStyleSheet(QStringLiteral(
        "background:#E5EEE9;color:#315C53;border:2px solid #8AA89F;"
        "border-radius:36px;font-size:22px;font-weight:850;"));
    auto *photoText = new QLabel(
        QStringLiteral("未上传照片时，由这位沉思哥布林代班。头像也会用于简历导出。"),
        photoSection);
    photoText->setWordWrap(true);
    auto *photoButtons = new QVBoxLayout;
    auto *changePhotoButton =
        new QPushButton(QStringLiteral("更换照片"), photoSection);
    auto *removePhotoButton =
        new QPushButton(QStringLiteral("移除照片"), photoSection);
    photoButtons->addWidget(changePhotoButton);
    photoButtons->addWidget(removePhotoButton);
    photoLayout->addWidget(avatarPreview);
    photoLayout->addWidget(photoText, 1);
    photoLayout->addLayout(photoButtons);
    layout->addWidget(photoSection);

    auto refreshPhotoPreview = [this, avatarPreview]() {
        const QString path = m_resumePage->photoPath();
        avatarPreview->setPixmap(AvatarUtils::circularAvatar(path, 72));
        avatarPreview->setText(QString());
    };
    refreshPhotoPreview();
    connect(changePhotoButton, &QPushButton::clicked, &dialog,
            [this, &dialog, refreshPhotoPreview]() {
                m_resumePage->choosePhoto(&dialog);
                refreshPhotoPreview();
            });
    connect(removePhotoButton, &QPushButton::clicked, &dialog,
            [this, &dialog, refreshPhotoPreview]() {
                if (m_resumePage->photoPath().isEmpty())
                    return;
                if (QMessageBox::question(
                        &dialog, QStringLiteral("移除照片"),
                        QStringLiteral("确定移除当前个人照片吗？")) !=
                    QMessageBox::Yes)
                    return;
                m_resumePage->removePhoto();
                refreshPhotoPreview();
            });

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(14);
    auto *gradeBox = new QComboBox(&dialog);
    gradeBox->addItems({QStringLiteral("大一"), QStringLiteral("大二"),
                        QStringLiteral("大三"), QStringLiteral("大四"),
                        QStringLiteral("已毕业"), QStringLiteral("其他")});
    gradeBox->setCurrentText(user.getGrade());
    auto *genderBox = new QComboBox(&dialog);
    genderBox->addItems({QStringLiteral("男"), QStringLiteral("女"),
                         QStringLiteral("其他"), QStringLiteral("不显示")});
    genderBox->setCurrentText(user.getGender().isEmpty()
                                  ? QStringLiteral("不显示")
                                  : user.getGender());
    auto *schoolEdit = new QLineEdit(user.getSchool(), &dialog);
    schoolEdit->setPlaceholderText(QStringLiteral("例如：首都哥布林大学"));
    auto *majorEdit = new QLineEdit(user.getMajor(), &dialog);
    majorEdit->setPlaceholderText(QStringLiteral("例如：地下城数据科学"));

    auto *yearsWidget = new QWidget(&dialog);
    auto *yearsLayout = new QHBoxLayout(yearsWidget);
    yearsLayout->setContentsMargins(0, 0, 0, 0);
    auto *startYearEdit = new QLineEdit(
        educationYear(
            primaryEducation.value(QStringLiteral("start_date")).toString()),
        yearsWidget);
    auto *endYearEdit = new QLineEdit(
        educationYear(
            primaryEducation.value(QStringLiteral("end_date")).toString()),
        yearsWidget);
    const QRegularExpression yearPattern(
        QStringLiteral("(?:19|20)\\d{2}"));
    startYearEdit->setValidator(
        new QRegularExpressionValidator(yearPattern, startYearEdit));
    endYearEdit->setValidator(
        new QRegularExpressionValidator(yearPattern, endYearEdit));
    startYearEdit->setMaximumWidth(90);
    endYearEdit->setMaximumWidth(90);
    yearsLayout->addWidget(startYearEdit);
    yearsLayout->addWidget(new QLabel(QStringLiteral("年 ～"), yearsWidget));
    yearsLayout->addWidget(endYearEdit);
    yearsLayout->addWidget(new QLabel(QStringLiteral("年"), yearsWidget));
    yearsLayout->addStretch();

    auto *phoneEdit = new QLineEdit(user.getPhone(), &dialog);
    phoneEdit->setPlaceholderText(QStringLiteral("例如：114-514-1919"));
    auto *emailEdit = new QLineEdit(user.getEmail(), &dialog);
    emailEdit->setPlaceholderText(
        QStringLiteral("例如：chief@goblin.edu"));
    auto *jobTargetEdit = new QLineEdit(user.getJobTarget(), &dialog);
    jobTargetEdit->setPlaceholderText(
        QStringLiteral("例如：地下城数据分析师"));
    auto *websiteEdit = new QLineEdit(user.getWebsite(), &dialog);
    websiteEdit->setPlaceholderText(
        QStringLiteral("例如：https://goblin.example/cave"));
    form->addRow(QStringLiteral("学校："), schoolEdit);
    form->addRow(QStringLiteral("年级："), gradeBox);
    form->addRow(QStringLiteral("性别："), genderBox);
    form->addRow(QStringLiteral("专业："), majorEdit);
    form->addRow(QStringLiteral("就读时间："), yearsWidget);
    form->addRow(QStringLiteral("电话："), phoneEdit);
    form->addRow(QStringLiteral("邮箱："), emailEdit);
    form->addRow(QStringLiteral("求职方向："), jobTargetEdit);
    form->addRow(QStringLiteral("个人网站："), websiteEdit);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)
        ->setText(QStringLiteral("保存修改"));
    buttons->button(QDialogButtonBox::Cancel)
        ->setText(QStringLiteral("取消"));
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QString gender = genderBox->currentText();
    if (gender == QStringLiteral("不显示"))
        gender.clear();
    const QString school = schoolEdit->text().trimmed();
    const QString major = majorEdit->text().trimmed();
    const QString startYear = startYearEdit->text().trimmed();
    const QString endYear = endYearEdit->text().trimmed();
    if (school.isEmpty() || major.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("学校和专业不能为空"));
        return;
    }
    if (startYear.isEmpty() != endYear.isEmpty() ||
        (!startYear.isEmpty() && startYear.toInt() > endYear.toInt())) {
        QMessageBox::warning(this, QStringLiteral("提示"),
                             QStringLiteral("请检查入学和毕业年份"));
        return;
    }

    if (!DatabaseManager::getInstance().updateUserInfo(
            user.getId(), gradeBox->currentText(), gender, major, school,
            startYear, endYear, phoneEdit->text().trimmed(),
            emailEdit->text().trimmed(), jobTargetEdit->text().trimmed(),
            websiteEdit->text().trimmed())) {
        QMessageBox::critical(this, QStringLiteral("修改失败"),
                              QStringLiteral("数据库写入失败，请稍后再试"));
        return;
    }
    user.refresh();
    updateSidebarUserInfo();
    m_resumePage->refresh();
    QMessageBox::information(this, QStringLiteral("修改成功"),
                             QStringLiteral("个人信息已经更新"));
}

void MainWindow::logout() {
    if (QMessageBox::question(
            this, QStringLiteral("退出登录"),
            QStringLiteral("确定要退出当前账号并返回登录页面吗？"),
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Cancel) != QMessageBox::Yes)
        return;
    User::getInstance().logout();
    emit logoutRequested();
}

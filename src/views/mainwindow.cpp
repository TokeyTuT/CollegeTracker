#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "DatabaseMannager.h"
#include "User.h"
#include "AddCourseDialog.h"

#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QCursor>
#include <QStyle>
#include <QSizePolicy>
#include <QPainter>
#include <QPainterPath>
#include <QSqlQuery>
#include <QGridLayout>
#include <QSpacerItem>
#include <QtMath>
#include <QVector>
#include <QFont>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QComboBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    applyModernStyle();
    buildHomePage();
    buildExportPage();

    InitFrame();
    updateSidebarUserInfo();
    InitCoursePage();
    InitExpPage();
    InitAwardPage();
    updateHomePageStats();
}

MainWindow::~MainWindow()
{
    delete courseModel;
    delete expModel;
    delete awardModel;
    delete ui;
}


void MainWindow::applyModernStyle() {
    setWindowTitle("College Tracker");
    resize(1200, 800);
    setMinimumSize(1080, 700);

    ui->menubar->hide();
    ui->statusbar->hide();

    ui->horizontalLayout->setContentsMargins(0, 0, 0, 0);
    ui->horizontalLayout->setSpacing(0);
    ui->verticalLayout_3->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout_3->setSpacing(0);

    ui->mainContentFrame->setFrameShape(QFrame::NoFrame);
    ui->sidebarFrame->setFrameShape(QFrame::NoFrame);
    ui->sidebarFrame->setMinimumWidth(280);
    ui->sidebarFrame->setMaximumWidth(280);

    // 更大留白，更宽松的布局
    ui->currentPageLbl->setGeometry(48, 36, 820, 52);
    ui->stackedWidget->setGeometry(48, 108, 830, 590);

    ui->courseTableView->setGeometry(32, 110, 766, 360);
    ui->addCourseBtn->setGeometry(528, 42, 138, 44);
    ui->deleteCourseBtn->setGeometry(680, 42, 140, 44);
    ui->labelStats->setGeometry(32, 488, 798, 52);

    ui->expTableView->setGeometry(32, 20, 766, 240);
    ui->addExpFrame->setGeometry(32, 304, 766, 230);
    ui->addExpTitle->setGeometry(44, 268, 200, 30);

    ui->awardTableView->setGeometry(32, 20, 766, 240);
    ui->addAwardFrame->setGeometry(32, 304, 766, 215);

    ui->addExpTypeLbl->setGeometry(40, 32, 76, 40);
    ui->addExpTypeCBox->setGeometry(126, 32, 200, 40);
    ui->addExpTitleLbl->setGeometry(368, 32, 76, 40);
    ui->addExpTitleLine->setGeometry(454, 32, 276, 40);
    ui->addExpDateLbl->setGeometry(40, 94, 76, 40);
    ui->addExpDateLine->setGeometry(126, 94, 200, 40);
    ui->addExpDescLbl->setGeometry(368, 94, 76, 40);
    ui->addExpDescLine->setGeometry(454, 94, 276, 40);
    ui->addExpBtn->setGeometry(454, 164, 128, 42);
    ui->DelExpBtn->setGeometry(598, 164, 132, 42);

    ui->addAwardTitleLbl->setGeometry(40, 38, 92, 40);
    ui->addAwardLine->setGeometry(142, 38, 266, 40);
    ui->addAwardDateLbl->setGeometry(436, 38, 68, 40);
    ui->addAwardDateLine->setGeometry(514, 38, 196, 40);
    ui->addAwardLevelLbl->setGeometry(40, 102, 92, 40);
    ui->addAwardLevelCBox->setGeometry(142, 102, 196, 40);
    ui->addAwardBtn->setGeometry(454, 156, 128, 42);
    ui->delAwardBtn->setGeometry(598, 156, 132, 42);

    // 只兼容浅色模式：不用系统深色调色板，所有控件颜色都明确写死。
    // ── 暖象牙白美学样式表 ──
    const QString qss = R"(
        * {
            font-family: "PingFang SC", "Microsoft YaHei", "Noto Serif SC", "Helvetica Neue", Arial;
            font-size: 14px;
            color: #3D2E1F;
        }
        QMainWindow, QWidget#centralwidget {
            background: #FEFAF3;
        }

        /* ── 侧边栏：暖奶油渐变 ── */
        QFrame#sidebarFrame {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #FEF9F0,
                        stop:0.45 #FCF5E8,
                        stop:1 #F9F0DE);
            border-right: 1px solid rgba(180, 160, 130, 0.18);
        }

        /* ── 用户信息卡片：暖白玻璃质感 ── */
        QWidget#userProfileWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 rgba(255,254,249,0.92),
                        stop:0.5 rgba(254,250,242,0.84),
                        stop:1 rgba(252,245,232,0.76));
            border: 1px solid rgba(200, 180, 150, 0.48);
            border-radius: 24px;
        }
        QLabel#userNameLbl {
            color: #3D2E1F;
            font-size: 24px;
            font-weight: 900;
            letter-spacing: 0.8px;
            qproperty-alignment: AlignCenter;
            background: transparent;
            border: none;
        }
        QLabel#detailLbl {
            color: #C8943E;
            font-size: 13px;
            font-weight: 700;
            qproperty-alignment: AlignCenter;
            background: rgba(200, 148, 62, 0.10);
            border: 1px solid rgba(200, 148, 62, 0.28);
            border-radius: 14px;
            padding: 4px 14px;
        }
        QLabel#majorLbl {
            color: #8B7355;
            font-size: 12px;
            font-weight: 500;
            qproperty-alignment: AlignCenter;
            background: transparent;
            border: none;
            padding: 0px 10px;
            line-height: 20px;
        }

        /* ── 编辑资料按钮 ── */
        QPushButton#editProfileBtn {
            min-height: 30px;
            max-height: 30px;
            border-radius: 15px;
            border: 1px solid rgba(200, 148, 62, 0.32);
            background: rgba(255, 254, 249, 0.70);
            color: #A0764A;
            font-size: 12px;
            font-weight: 700;
            text-align: center;
            padding: 0 14px;
        }
        QPushButton#editProfileBtn:hover {
            background: rgba(200, 148, 62, 0.10);
            border: 1px solid rgba(200, 148, 62, 0.55);
            color: #8B6914;
        }

        /* ── 主内容区：纯暖象牙白 ── */
        QFrame#mainContentFrame {
            background: #FDF5E8;
            border: none;
            border-left: 1px solid rgba(180, 160, 130, 0.22);
        }

        /* ── 页面标题：极大留白 + 左侧暖金装饰线 ── */
        QLabel#currentPageLbl {
            color: #3D2E1F;
            font-size: 30px;
            font-weight: 300;
            letter-spacing: 1.2px;
            qproperty-alignment: AlignLeft | AlignVCenter;
            background: transparent;
            border-left: 6px solid #C8943E;
            padding-left: 20px;
        }

        /* ── 导航按钮：大胶囊 + 柔和边框 ── */
        #sidebarFrame QPushButton {
            min-height: 64px;
            max-height: 64px;
            border: 1px solid rgba(190, 170, 140, 0.42);
            border-radius: 32px;
            padding-left: 28px;
            padding-right: 20px;
            text-align: left;
            color: #6B5B45;
            background: rgba(255, 254, 249, 0.52);
            font-size: 15px;
            font-weight: 500;
            letter-spacing: 0.5px;
        }
        #sidebarFrame QPushButton:hover {
            background: rgba(255, 254, 249, 0.84);
            border: 1px solid rgba(200, 148, 62, 0.38);
            color: #3D2E1F;
        }
        #sidebarFrame QPushButton:pressed {
            background: rgba(245, 238, 222, 0.82);
            border: 1px solid rgba(200, 148, 62, 0.52);
        }
        #sidebarFrame QPushButton[active="true"] {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                        stop:0 #C8943E,
                        stop:0.5 #D4A843,
                        stop:1 #B8860B);
            color: #FFFEF9;
            border: 1px solid rgba(255, 254, 249, 0.64);
            border-radius: 32px;
            font-weight: 700;
            letter-spacing: 0.6px;
        }

        /* ── 退出登录：柔和暖色 ── */
        QPushButton#logoutBtn {
            min-height: 38px;
            max-height: 38px;
            border: none;
            background: transparent;
            color: #A89880;
            font-size: 13px;
            font-weight: 500;
            text-align: center;
        }
        QPushButton#logoutBtn:hover {
            color: #C0392B;
            background: rgba(192, 57, 43, 0.06);
        }

        /* ── 通用按钮 ── */
        QPushButton {
            min-height: 40px;
            border: 1px solid #B8860B;
            border-radius: 8px;
            padding: 0 24px;
            color: #FFFEF9;
            background: #C8943E;
            font-weight: 700;
            letter-spacing: 0.3px;
        }
        QPushButton:hover {
            background: #B8860B;
            border-color: #A0764A;
        }
        QPushButton:pressed { background: #8B6914; }
        QPushButton:disabled { background: #E5D9C6; border-color: #DDD0BE; color: #A89880; }

        /* 删除按钮：暖锈红色 */
        QPushButton#deleteCourseBtn, QPushButton#DelExpBtn, QPushButton#delAwardBtn {
            background: #FFFEF9;
            color: #C0392B;
            border: 1px solid #C0392B;
        }
        QPushButton#deleteCourseBtn:hover, QPushButton#DelExpBtn:hover, QPushButton#delAwardBtn:hover {
            background: #C0392B;
            color: #FFFEF9;
        }

        QStackedWidget, QWidget#homePage, QWidget#coursePage, QWidget#expPage, QWidget#awardPage, QWidget#profilePage {
            background: transparent;
        }

        /* ── 添加记录卡片 ── */
        QFrame#addExpFrame, QFrame#addAwardFrame {
            background: #FFFEF9;
            border: 1px solid #E5D9C6;
            border-top: 4px solid #C8943E;
            border-radius: 12px;
        }
        QLabel#addExpTitle {
            color: #3D2E1F;
            font-size: 22px;
            font-weight: 300;
            background: transparent;
            qproperty-alignment: AlignLeft | AlignVCenter;
        }

        /* ── 标签文字：中等字重 ── */
        QLabel {
            color: #5C4A35;
            background: transparent;
            font-weight: 500;
        }

        /* ── 输入控件 ── */
        QLineEdit, QComboBox, QDateEdit {
            min-height: 40px;
            border: 1px solid #DDD0BE;
            border-radius: 8px;
            padding: 0 16px;
            background: #FFFEF9;
            color: #3D2E1F;
            selection-background-color: #C8943E;
            selection-color: #FFFEF9;
        }
        QLineEdit:hover, QComboBox:hover, QDateEdit:hover {
            border: 1px solid #C8943E;
            background: #FFFDF7;
        }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus {
            border: 2px solid #C8943E;
            background: #FFFEF9;
        }
        QComboBox::drop-down, QDateEdit::drop-down {
            border: none;
            width: 30px;
        }
        QComboBox QAbstractItemView {
            background: #FFFEF9;
            border: 1px solid #DDD0BE;
            selection-background-color: #C8943E;
            selection-color: #FFFEF9;
            outline: none;
        }

        /* ── 表格视图 ── */
        QTableView {
            background: #FFFEF9;
            alternate-background-color: #FDF8EE;
            border: 1px solid #E5D9C6;
            border-top: 4px solid #C8943E;
            border-radius: 12px;
            gridline-color: transparent;
            selection-background-color: #FDF0D5;
            selection-color: #3D2E1F;
            padding: 8px;
            outline: none;
        }
        QTableView::item {
            min-height: 40px;
            padding: 10px;
            border-bottom: 1px solid #F0E8D8;
        }

        /* ── 表头：暖深褐底 + 金色字 ── */
        QHeaderView::section {
            background: #3D2E1F;
            color: #F5E6D0;
            border: none;
            border-right: 1px solid #5C4A35;
            padding: 14px 12px;
            font-size: 13px;
            font-weight: 700;
            letter-spacing: 0.5px;
        }
        QTableCornerButton::section {
            background: #3D2E1F;
            border: none;
        }

        /* ── 滚动条 ── */
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 8px 2px 8px 0px;
        }
        QScrollBar::handle:vertical {
            background: #C8B898;
            border-radius: 4px;
            min-height: 32px;
        }
        QScrollBar::handle:vertical:hover { background: #C8943E; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }

        /* ── 首页卡片 ── */
        QFrame#homeChartCard, QFrame#homeStatCard {
            background: #FFFEF9;
            border: 1px solid rgba(200, 180, 150, 0.55);
            border-radius: 20px;
        }
        QFrame#homeChartCard {
            border-left: 5px solid #C8943E;
        }
        QLabel#homeCardTitle {
            color: #3D2E1F;
            font-size: 22px;
            font-weight: 300;
            background: transparent;
            letter-spacing: 0.8px;
        }
        QLabel#homeChartLabel {
            background: transparent;
            border: none;
        }
        QLabel#homeStatTitle {
            color: #8B7355;
            font-size: 13px;
            font-weight: 500;
            background: transparent;
            letter-spacing: 0.3px;
        }
        QLabel#homeStatValue {
            color: #3D2E1F;
            font-size: 34px;
            font-weight: 900;
            background: transparent;
            letter-spacing: 0.5px;
        }
        QLabel#homeStatSub {
            color: #A89880;
            font-size: 12px;
            font-weight: 400;
            background: transparent;
        }
        QLabel#labelStats {
            background: #FFFEF9;
            border: 1px solid #E5D9C6;
            border-left: 5px solid #C8943E;
            border-radius: 8px;
            color: #3D2E1F;
            font-size: 16px;
            font-weight: 700;
            qproperty-alignment: AlignCenter;
        }
        QMessageBox, QDialog {
            background: #FFFEF9;
        }
    )";
    setStyleSheet(qss);

    QList<QPushButton*> buttons = findChildren<QPushButton*>();
    for (QPushButton *button : buttons) {
        button->setCursor(Qt::PointingHandCursor);
    }
}

void MainWindow::setupTableView(QTableView *tableView) {
    tableView->setAlternatingRowColors(true);
    tableView->setShowGrid(false);
    tableView->verticalHeader()->setVisible(false);
    tableView->verticalHeader()->setDefaultSectionSize(42);
    tableView->horizontalHeader()->setMinimumHeight(44);
    tableView->horizontalHeader()->setStretchLastSection(true);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void MainWindow::buildHomePage() {
    if (ui->homePage->layout() != nullptr) return;

    auto *mainLayout = new QVBoxLayout(ui->homePage);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(16);

    auto makeCard = [](const QString &title, const QString &objectName) {
        auto *card = new QFrame;
        card->setObjectName(objectName);
        card->setFrameShape(QFrame::NoFrame);
        auto *shadow = new QGraphicsDropShadowEffect(card);
        shadow->setBlurRadius(42);
        shadow->setOffset(0, 12);
        shadow->setColor(QColor(60, 35, 15, 16));
        card->setGraphicsEffect(shadow);

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(28, 22, 28, 22);
        layout->setSpacing(12);
        auto *titleLbl = new QLabel(title);
        titleLbl->setObjectName("homeCardTitle");
        layout->addWidget(titleLbl);
        return card;
    };

    QFrame *chartCard = makeCard("学期平均 GPA 走势", "homeChartCard");
    chartCard->setMinimumHeight(350);
    homeChartLabel = new QLabel;
    homeChartLabel->setObjectName("homeChartLabel");
    homeChartLabel->setMinimumHeight(260);
    homeChartLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chartCard->layout()->addWidget(homeChartLabel);

    auto *bottomLayout = new QGridLayout;
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setHorizontalSpacing(14);
    bottomLayout->setVerticalSpacing(14);

    auto makeStatCard = [](const QString &title, const QString &subTitle, QLabel **valueLabel, const QString &objectName) {
        auto *card = new QFrame;
        card->setObjectName(objectName);
        card->setFrameShape(QFrame::NoFrame);
        card->setMinimumHeight(124);
        // 暖棕阴影
        auto *shadow = new QGraphicsDropShadowEffect(card);
        shadow->setBlurRadius(32);
        shadow->setOffset(0, 8);
        shadow->setColor(QColor(60, 35, 15, 13));
        card->setGraphicsEffect(shadow);

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(22, 18, 22, 18);
        layout->setSpacing(6);
        auto *titleLbl = new QLabel(title);
        titleLbl->setObjectName("homeStatTitle");
        *valueLabel = new QLabel("0");
        (*valueLabel)->setObjectName("homeStatValue");
        auto *subLbl = new QLabel(subTitle);
        subLbl->setObjectName("homeStatSub");
        layout->addWidget(titleLbl);
        layout->addWidget(*valueLabel);
        layout->addWidget(subLbl);
        return card;
    };

    bottomLayout->addWidget(makeStatCard("课程数量", "已录入课程", &homeCourseCountLbl, "homeStatCard"), 0, 0);
    bottomLayout->addWidget(makeStatCard("平均 GPA", "按学分加权", &homeGpaLbl, "homeStatCard"), 0, 1);
    bottomLayout->addWidget(makeStatCard("竞赛经历", "课外活动统计", &homeCompetitionCountLbl, "homeStatCard"), 0, 2);
    bottomLayout->addWidget(makeStatCard("实习经历", "实践经历统计", &homeInternshipCountLbl, "homeStatCard"), 0, 3);
    bottomLayout->addWidget(makeStatCard("项目经历", "项目/科研统计", &homeProjectCountLbl, "homeStatCard"), 1, 0);
    bottomLayout->addWidget(makeStatCard("个人荣誉", "奖项证书统计", &homeAwardCountLbl, "homeStatCard"), 1, 1);
    bottomLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 2, 1, 2);

    mainLayout->addWidget(chartCard);
    mainLayout->addLayout(bottomLayout);
    mainLayout->addStretch();
}


double MainWindow::scoreToGpa(double score) const {
    return DatabaseManager::scoreToGpa(score);
}

void MainWindow::updateHomePageStats() {
    if (homeChartLabel == nullptr) return;

    const int userId = User::getInstance().getId();
    const QStringList semesters = {"大一上", "大一下", "大二上", "大二下", "大三上", "大三下", "大四上", "大四下"};
    QVector<double> gpas(semesters.size(), -1.0);

    QVector<double> semesterGpaCreditSums(semesters.size(), 0.0);
    QVector<double> semesterCreditSums(semesters.size(), 0.0);

    QSqlQuery courseQuery;
    courseQuery.prepare("SELECT semester, score, credit FROM courses WHERE user_id = :uid");
    courseQuery.bindValue(":uid", userId);
    int totalCourses = 0;
    double totalGpaCreditSum = 0.0;
    double totalCredits = 0.0;

    if (courseQuery.exec()) {
        while (courseQuery.next()) {
            QString semester = courseQuery.value(0).toString().trimmed();
            double score = courseQuery.value(1).toDouble();
            double credit = courseQuery.value(2).toDouble();
            double gpa = scoreToGpa(score);

            ++totalCourses;
            if (credit <= 0.0) continue;

            totalGpaCreditSum += gpa * credit;
            totalCredits += credit;

            int index = semesters.indexOf(semester);
            if (index < 0) {
                for (int i = 0; i < semesters.size(); ++i) {
                    if (semester.contains(semesters[i])) { index = i; break; }
                }
            }
            if (index >= 0) {
                semesterGpaCreditSums[index] += gpa * credit;
                semesterCreditSums[index] += credit;
            }
        }
    }

    for (int i = 0; i < semesters.size(); ++i) {
        if (semesterCreditSums[i] > 0.0) {
            gpas[i] = semesterGpaCreditSums[i] / semesterCreditSums[i];
        }
    }

    double avgGpa = totalCredits > 0.0 ? (totalGpaCreditSum / totalCredits) : 0.0;

    int internshipCount = 0;
    int competitionCount = 0;
    int projectCount = 0;
    QSqlQuery expQuery;
    expQuery.prepare("SELECT type, COUNT(*) FROM experiences WHERE user_id = :uid GROUP BY type");
    expQuery.bindValue(":uid", userId);
    if (expQuery.exec()) {
        while (expQuery.next()) {
            QString type = expQuery.value(0).toString();
            int count = expQuery.value(1).toInt();
            if (type.contains("实习")) internshipCount += count;
            else if (type.contains("竞赛")) competitionCount += count;
            else if (type.contains("项目")) projectCount += count;
        }
    }

    int awardCount = 0;
    QSqlQuery awardQuery;
    awardQuery.prepare("SELECT COUNT(*) FROM awards WHERE user_id = :uid");
    awardQuery.bindValue(":uid", userId);
    if (awardQuery.exec() && awardQuery.next()) awardCount = awardQuery.value(0).toInt();

    homeCourseCountLbl->setText(QString::number(totalCourses));
    homeGpaLbl->setText(QString::number(avgGpa, 'f', 2));
    homeCompetitionCountLbl->setText(QString::number(competitionCount));
    homeInternshipCountLbl->setText(QString::number(internshipCount));
    homeProjectCountLbl->setText(QString::number(projectCount));
    homeAwardCountLbl->setText(QString::number(awardCount));

    const int width = qMax(720, homeChartLabel->width() > 10 ? homeChartLabel->width() : 760);
    const int height = qMax(250, homeChartLabel->height() > 10 ? homeChartLabel->height() : 260);
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRectF plot(56, 22, width - 92, height - 70);
    // 暖奶油色网格线
    QPen gridPen(QColor(235, 222, 200));
    gridPen.setWidth(1);
    painter.setPen(gridPen);
    painter.setFont(QFont("PingFang SC", 9, QFont::Light));
    painter.setBrush(Qt::NoBrush);

    for (int i = 0; i <= 4; ++i) {
        double y = plot.bottom() - plot.height() * i / 4.0;
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.setPen(QColor(139, 115, 85));
        painter.drawText(QRectF(10, y - 10, 40, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(i, 'f', 1));
        painter.setPen(gridPen);
    }

    // 暖调坐标轴
    painter.setPen(QPen(QColor(200, 180, 150), 1));
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.drawLine(plot.bottomLeft(), plot.topLeft());

    QVector<QPointF> points;
    const double step = plot.width() / (semesters.size() - 1);
    painter.setPen(QColor(92, 74, 53));
    for (int i = 0; i < semesters.size(); ++i) {
        double x = plot.left() + i * step;
        painter.drawText(QRectF(x - 28, plot.bottom() + 10, 56, 28), Qt::AlignCenter, semesters[i]);
        if (gpas[i] >= 0.0) {
            double y = plot.bottom() - plot.height() * qBound(0.0, gpas[i], 4.0) / 4.0;
            points.append(QPointF(x, y));
        }
    }

    if (points.size() >= 2) {
        QPainterPath path(points.first());
        for (int i = 1; i < points.size(); ++i) path.lineTo(points[i]);
        // 暖金折线
        QPen linePen(QColor(200, 148, 62), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(linePen);
        painter.drawPath(path);
    }

    // 暖金数据点
    for (const QPointF &point : points) {
        painter.setPen(QPen(QColor(255, 254, 249), 5));
        painter.setBrush(QColor(200, 148, 62));
        painter.drawEllipse(point, 7, 7);
    }

    if (points.isEmpty()) {
        painter.setPen(QColor(168, 152, 128));
        painter.setFont(QFont("PingFang SC", 15, QFont::Light));
        painter.drawText(plot, Qt::AlignCenter, "暂无课程 GPA 数据\n录入课程后这里会自动生成折线图");
    }

    painter.setPen(QColor(139, 115, 85));
    painter.setFont(QFont("PingFang SC", 10, QFont::Light));
    painter.drawText(QRectF(plot.left(), 0, plot.width(), 20), Qt::AlignLeft | Qt::AlignVCenter, "横轴：学期    纵轴：平均 GPA");

    homeChartLabel->setPixmap(pixmap);
}

void MainWindow::buildExportPage() {
    if (ui->profilePage->layout() != nullptr) return;

    // 简历导出页面正文先保持空白，后续按需求再添加导出内容。
    auto *layout = new QVBoxLayout(ui->profilePage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

void MainWindow::InitFrame() {
    ui->sidebarFrame->setFixedWidth(280);
    ui->userProfileWidget->setFixedHeight(218);
    ui->userNameLbl->setMinimumHeight(46);
    ui->detailLbl->setMinimumHeight(30);
    ui->majorLbl->setMinimumHeight(52);
    ui->userNameLbl->setWordWrap(false);
    ui->detailLbl->setWordWrap(false);
    ui->majorLbl->setWordWrap(true);

    if (editProfileBtn == nullptr) {
        editProfileBtn = new QPushButton("编辑资料", ui->userProfileWidget);
        editProfileBtn->setObjectName("editProfileBtn");
        editProfileBtn->setCursor(Qt::PointingHandCursor);
        editProfileBtn->setFlat(true);
        ui->verticalLayout->addWidget(editProfileBtn);
        connect(editProfileBtn, &QPushButton::clicked, this, &MainWindow::openEditProfileDialog);
    }

    ui->verticalLayout->setContentsMargins(22, 20, 22, 20);
    ui->verticalLayout->setSpacing(10);
    ui->verticalLayout_3->setContentsMargins(16, 22, 16, 22);
    ui->verticalLayout_3->setSpacing(22);
    ui->verticalLayout_2->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout_2->setSpacing(16);
    ui->verticalSpacer_2->changeSize(20, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);

    const QList<QPushButton*> navButtonsForSize = {
        ui->navHomeBtn, ui->navCourseBtn, ui->navExpBtn, ui->navAwardBtn, ui->navExportBtn
    };
    // 用户卡片阴影：柔和深邃暖棕调
    auto *profileShadow = new QGraphicsDropShadowEffect(ui->userProfileWidget);
    profileShadow->setBlurRadius(48);
    profileShadow->setOffset(0, 12);
    profileShadow->setColor(QColor(60, 35, 15, 20));
    ui->userProfileWidget->setGraphicsEffect(profileShadow);

    for (QPushButton *button : navButtonsForSize) {
        button->setFixedHeight(64);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setFlat(true);
        button->setAutoDefault(false);
        button->setDefault(false);
        button->setAttribute(Qt::WA_StyledBackground, true);

        // 导航按钮阴影：轻柔暖棕
        auto *shadow = new QGraphicsDropShadowEffect(button);
        shadow->setBlurRadius(24);
        shadow->setOffset(0, 6);
        shadow->setColor(QColor(50, 30, 10, 12));
        button->setGraphicsEffect(shadow);
    }

    auto setActiveNav = [this](QPushButton *activeButton) {
        const QList<QPushButton*> navButtons = {
            ui->navHomeBtn, ui->navCourseBtn, ui->navExpBtn, ui->navAwardBtn, ui->navExportBtn
        };
        for (QPushButton *button : navButtons) {
            button->setProperty("active", button == activeButton);
            button->style()->unpolish(button);
            button->style()->polish(button);
            button->update();
        }
    };

    ui->navHomeBtn->setText("01  首页总览");
    ui->navCourseBtn->setText("02  课程与成绩");
    ui->navExpBtn->setText("03  课外活动");
    ui->navAwardBtn->setText("04  个人荣誉");
    ui->navExportBtn->setText("05  简历导出");

    ui->stackedWidget->setCurrentIndex(0);
    ui->currentPageLbl->setText("首页总览");
    setActiveNav(ui->navHomeBtn);

    connect(ui->navHomeBtn, &QPushButton::clicked, this, [=] {
        ui->stackedWidget->setCurrentIndex(0);
        ui->currentPageLbl->setText("首页总览");
        setActiveNav(ui->navHomeBtn);
        updateHomePageStats();
    });
    connect(ui->navCourseBtn, &QPushButton::clicked, this, [=] {
        ui->stackedWidget->setCurrentIndex(1);
        ui->currentPageLbl->setText("课程与成绩");
        setActiveNav(ui->navCourseBtn);
    });
    connect(ui->navExpBtn, &QPushButton::clicked, this, [=] {
        ui->stackedWidget->setCurrentIndex(2);
        ui->currentPageLbl->setText("课外活动");
        setActiveNav(ui->navExpBtn);
    });
    connect(ui->navAwardBtn, &QPushButton::clicked, this, [=] {
        ui->stackedWidget->setCurrentIndex(3);
        ui->currentPageLbl->setText("个人荣誉");
        setActiveNav(ui->navAwardBtn);
    });
    connect(ui->navExportBtn, &QPushButton::clicked, this, [=] {
        ui->stackedWidget->setCurrentIndex(4);
        ui->currentPageLbl->setText("简历导出");
        setActiveNav(ui->navExportBtn);
    });
}

void MainWindow::updateSidebarUserInfo() {
    User &user = User::getInstance();
    if (user.isLoggedIn()) {
        ui->userNameLbl->setText(user.getUsername());
        QString schoolMajor = user.getMajor();
        if (!user.getSchool().isEmpty()) {
            // 学校和专业分两行显示，避免长学校名把左侧信息卡片挤出边界。
            schoolMajor = user.getSchool() + "\n" + user.getMajor();
        }
        ui->majorLbl->setText(schoolMajor);
        QString detail = user.getGrade();
        if (!user.getGender().isEmpty()) {
            detail = user.getGrade() + " | " + user.getGender();
        }
        ui->detailLbl->setText(detail);
    }
}

void MainWindow::openEditProfileDialog() {
    User &user = User::getInstance();
    if (!user.isLoggedIn()) return;

    QDialog dialog(this);
    dialog.setWindowTitle("修改个人信息");
    dialog.setMinimumWidth(440);
    dialog.setStyleSheet("QDialog { background: #FFFEF9; }");

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(32, 28, 32, 26);
    layout->setSpacing(18);

    auto *title = new QLabel("修改个人信息");
    title->setStyleSheet("font-size:24px; font-weight:300; color:#3D2E1F; letter-spacing:1px;");
    layout->addWidget(title);

    auto *hint = new QLabel("用户名暂不修改，学校、年级、性别和专业可以随时更新。保存后左上角信息会立即刷新。");
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#A89880; font-size:13px; font-weight:400;");
    layout->addWidget(hint);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignTop);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(14);

    auto *gradeBox = new QComboBox(&dialog);
    gradeBox->addItems({"大一", "大二", "大三", "大四", "已毕业", "其他"});
    int gradeIndex = gradeBox->findText(user.getGrade());
    if (gradeIndex >= 0) gradeBox->setCurrentIndex(gradeIndex);

    auto *genderBox = new QComboBox(&dialog);
    genderBox->addItems({"男", "女", "其他", "不显示"});
    int genderIndex = genderBox->findText(user.getGender());
    if (genderIndex >= 0) genderBox->setCurrentIndex(genderIndex);

    auto *schoolEdit = new QLineEdit(user.getSchool(), &dialog);
    schoolEdit->setPlaceholderText("例如：对外经济贸易大学");

    auto *majorEdit = new QLineEdit(user.getMajor(), &dialog);
    majorEdit->setPlaceholderText("例如：数据科学与大数据技术");

    form->addRow("学校：", schoolEdit);
    form->addRow("年级：", gradeBox);
    form->addRow("性别：", genderBox);
    form->addRow("专业：", majorEdit);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText("保存修改");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;

    QString grade = gradeBox->currentText();
    QString gender = genderBox->currentText();
    if (gender == "不显示") gender.clear();
    QString major = majorEdit->text().trimmed();
    QString school = schoolEdit->text().trimmed();

    if (school.isEmpty() || major.isEmpty()) {
        QMessageBox::warning(this, "提示", "学校和专业不能为空");
        return;
    }

    if (DatabaseManager::getInstance().updateUserInfo(user.getId(), grade, gender, major, school)) {
        user.refresh();
        updateSidebarUserInfo();
        QMessageBox::information(this, "修改成功", "个人信息已经更新");
    } else {
        QMessageBox::critical(this, "修改失败", "数据库写入失败，请稍后再试");
    }
}

// ==================== 课程页 ====================

void MainWindow::InitCoursePage() {
    int userId = User::getInstance().getId();

    ui->courseTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->courseTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    setupTableView(ui->courseTableView);

    courseModel = new QSqlTableModel(this);
    courseModel->setTable("courses");
    courseModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    courseModel->setFilter(QString("user_id = %1").arg(userId));

    courseModel->setHeaderData(2, Qt::Horizontal, "课程名称");
    courseModel->setHeaderData(3, Qt::Horizontal, "学分");
    courseModel->setHeaderData(4, Qt::Horizontal, "成绩");
    courseModel->setHeaderData(5, Qt::Horizontal, "学期");
    courseModel->setHeaderData(6, Qt::Horizontal, "绩点");

    // 按学期时间排序（semester_order 列：大一上=0 ... 大四下=7）
    courseModel->setSort(7, Qt::AscendingOrder);

    ui->courseTableView->setModel(courseModel);
    ui->courseTableView->setColumnHidden(0, true);
    ui->courseTableView->setColumnHidden(1, true);
    ui->courseTableView->setColumnHidden(7, true);  // 隐藏 semester_order 排序列

    courseModel->select();
    updateTotalStats();
}

void MainWindow::on_addCourseBtn_clicked() {
    int userId = User::getInstance().getId();
    AddCourseDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.getName();
        double credit = dialog.getCredit();
        double score = dialog.getScore();
        QString semester = dialog.getSemester();
        int semesterOrder = dialog.getSemesterOrder();

        if (name.isEmpty()) return;

        int row = courseModel->rowCount();
        if (courseModel->insertRow(row)) {
            courseModel->setData(courseModel->index(row, 1), userId);
            courseModel->setData(courseModel->index(row, 2), name);
            courseModel->setData(courseModel->index(row, 3), credit);
            courseModel->setData(courseModel->index(row, 4), score);
            courseModel->setData(courseModel->index(row, 5), semester);
            courseModel->setData(courseModel->index(row, 6), scoreToGpa(score));
            courseModel->setData(courseModel->index(row, 7), semesterOrder);

            if (courseModel->submitAll()) {
                ui->courseTableView->selectRow(row);
                qDebug() << "成功录入：" << name << " | " << semester;
                updateTotalStats();
                updateHomePageStats();
            } else {
                courseModel->revertAll();
                qDebug() << "数据库写入失败";
            }
        }
    }
}

void MainWindow::on_deleteCourseBtn_clicked() {
    QModelIndex currentIndex = ui->courseTableView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "提示", "请先在表格中点击选择一行内容");
        return;
    }

    auto result = QMessageBox::question(this, "确认删除", "确定要删除该门课程吗？");
    if (result != QMessageBox::Yes) return;

    int row = currentIndex.row();
    courseModel->removeRow(row);

    if (courseModel->submitAll()) {
        qDebug() << "行号" << row << "已成功从数据库抹除";
        updateTotalStats();
        updateHomePageStats();
    } else {
        courseModel->revertAll();
        qDebug() << "提交失败";
        QMessageBox::critical(this, "错误", "数据库写入失败！");
    }
}

void MainWindow::updateTotalStats() {
    int userId = User::getInstance().getId();
    QVariantMap stats = DatabaseManager::getInstance().getTotalStats(userId);

    if (stats["count"].toInt() == 0) {
        ui->labelStats->setText("暂无数据");
        return;
    }

    ui->labelStats->setText(QString("总课程: %1 | 算术平均: %2 | 加权 GPA: %3 | 总学分: %4")
                                .arg(stats["count"].toInt())
                                .arg(stats["avg"].toDouble(), 0, 'f', 2)
                                .arg(stats["gpa"].toDouble(), 0, 'f', 2)
                                .arg(stats["totalCredits"].toDouble(), 0, 'f', 1));
}

// ==================== 课外经历页 ====================

void MainWindow::InitExpPage() {
    int userId = User::getInstance().getId();

    ui->addExpTypeCBox->addItem("实习");
    ui->addExpTypeCBox->addItem("竞赛");
    ui->addExpTypeCBox->addItem("项目");
    ui->addExpTypeCBox->addItem("其他");
    ui->addExpTypeCBox->setCurrentIndex(0);

    ui->addExpDateLine->setCalendarPopup(true);
    ui->addExpDateLine->setDate(QDate::currentDate());
    ui->addExpDateLine->setDisplayFormat("yyyy-MM-dd");

    ui->addExpTitleLine->setPlaceholderText("示例：校园项目负责人");
    ui->addExpDescLine->setPlaceholderText("示例：负责数据整理与展示");

    ui->expTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->expTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    setupTableView(ui->expTableView);

    expModel = new QSqlTableModel(this);
    expModel->setTable("experiences");
    expModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    expModel->setFilter(QString("user_id = %1").arg(userId));

    expModel->setHeaderData(2, Qt::Horizontal, "标题");
    expModel->setHeaderData(3, Qt::Horizontal, "类型");
    expModel->setHeaderData(4, Qt::Horizontal, "时间");
    expModel->setHeaderData(5, Qt::Horizontal, "描述");

    // 按经历时间排序（date 列为 yyyy-MM-dd 格式，字典序 = 时间序）
    expModel->setSort(4, Qt::AscendingOrder);

    ui->expTableView->setModel(expModel);
    ui->expTableView->setColumnHidden(0, true);
    ui->expTableView->setColumnHidden(1, true);

    expModel->select();
}

void MainWindow::on_addExpBtn_clicked() {
    int userId = User::getInstance().getId();
    QString title = ui->addExpTitleLine->text();
    QString type = ui->addExpTypeCBox->currentText();
    QDate temp_date = ui->addExpDateLine->date();

    if (temp_date > QDate::currentDate()) {
        QMessageBox::warning(this, "提示", "不能选择比今天还大的日期！");
    } else {
        QString desc = ui->addExpDescLine->text();
        QString date = temp_date.toString("yyyy-MM-dd");

        if (type.isEmpty() || title.isEmpty()) {
            QMessageBox::warning(this, "提示", "标题不能为空！");
        } else {
            int row = expModel->rowCount();
            if (expModel->insertRow(row)) {
                expModel->setData(expModel->index(row, 1), userId);
                expModel->setData(expModel->index(row, 2), title);
                expModel->setData(expModel->index(row, 3), type);
                expModel->setData(expModel->index(row, 4), date);
                expModel->setData(expModel->index(row, 5), desc);

                if (expModel->submitAll()) {
                    ui->expTableView->selectRow(row);
                    qDebug() << "写入成功";
                    updateHomePageStats();
                } else {
                    qDebug() << "数据库写入失败";
                }
            }
        }
    }

    ui->addExpTitleLine->clear();
    ui->addExpDateLine->setDate(QDate::currentDate());
    ui->addExpTypeCBox->setCurrentIndex(0);
    ui->addExpDescLine->clear();
}

void MainWindow::on_DelExpBtn_clicked() {
    QModelIndex currentIndex = ui->expTableView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "提示", "请先在表格中点击选择一行内容");
        return;
    }
    auto result = QMessageBox::question(this, "确认删除", "确定要删除该经历吗?");
    if (result != QMessageBox::Yes) {
        return;
    }

    int row = currentIndex.row();
    expModel->removeRow(row);

    if (expModel->submitAll()) {
        qDebug() << "行号" << row << "已从数据库中抹除";
        updateHomePageStats();
    } else {
        expModel->revertAll();
        qDebug() << "提交失败";
        QMessageBox::critical(this, "错误", "数据库写入失败！");
    }
}

// ==================== 个人荣誉页 ====================

void MainWindow::InitAwardPage() {
    int userId = User::getInstance().getId();

    ui->addAwardDateLine->setCalendarPopup(true);
    ui->addAwardDateLine->setDate(QDate::currentDate());
    ui->addAwardDateLine->setDisplayFormat("yyyy-MM-dd");

    ui->addAwardLine->setPlaceholderText("示例: 全国大学生数学建模一等奖");

    ui->awardTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->awardTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    setupTableView(ui->awardTableView);

    awardModel = new QSqlTableModel(this);
    awardModel->setTable("awards");
    awardModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    awardModel->setFilter(QString("user_id = %1").arg(userId));

    awardModel->setHeaderData(2, Qt::Horizontal, "奖项名称");
    awardModel->setHeaderData(3, Qt::Horizontal, "奖项等级");
    awardModel->setHeaderData(4, Qt::Horizontal, "获奖时间");

    ui->awardTableView->setModel(awardModel);
    ui->awardTableView->setColumnHidden(0, true);
    ui->awardTableView->setColumnHidden(1, true);

    awardModel->select();
}

void MainWindow::on_addAwardBtn_clicked() {
    int userId = User::getInstance().getId();
    QString name = ui->addAwardLine->text();
    QString level = ui->addAwardLevelCBox->currentText();
    QDate temp_date = ui->addAwardDateLine->date();

    if (temp_date > QDate::currentDate()) {
        QMessageBox::warning(this, "提示", "不能选择比今天还大的日期！");
        return;
    }

    if (name.isEmpty()) {
        QMessageBox::warning(this, "提示", "奖项名称不能为空！");
        return;
    }

    QString date = temp_date.toString("yyyy-MM-dd");

    int row = awardModel->rowCount();
    if (awardModel->insertRow(row)) {
        awardModel->setData(awardModel->index(row, 1), userId);
        awardModel->setData(awardModel->index(row, 2), name);
        awardModel->setData(awardModel->index(row, 3), level);
        awardModel->setData(awardModel->index(row, 4), date);

        if (awardModel->submitAll()) {
            ui->awardTableView->selectRow(row);
            qDebug() << "荣誉写入成功";
            updateHomePageStats();
        } else {
            qDebug() << "数据库写入失败";
        }
    }

    ui->addAwardLine->clear();
    ui->addAwardDateLine->setDate(QDate::currentDate());
}

void MainWindow::on_delAwardBtn_clicked() {
    QModelIndex currentIndex = ui->awardTableView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "提示", "请先在表格中点击选择一行内容");
        return;
    }
    auto result = QMessageBox::question(this, "确认删除", "确定要删除该荣誉吗?");
    if (result != QMessageBox::Yes) {
        return;
    }

    int row = currentIndex.row();
    awardModel->removeRow(row);

    if (awardModel->submitAll()) {
        qDebug() << "行号" << row << "已从数据库中抹除";
        updateHomePageStats();
    } else {
        awardModel->revertAll();
        qDebug() << "提交失败";
        QMessageBox::critical(this, "错误", "数据库写入失败！");
    }
}

// ==================== 导航栏按钮 ====================

void MainWindow::on_navHomeBtn_clicked() {
    ui->currentPageLbl->setText(ui->navHomeBtn->text());
}
void MainWindow::on_navCourseBtn_clicked() {
    ui->currentPageLbl->setText(ui->navCourseBtn->text());
}
void MainWindow::on_navExpBtn_clicked() {
    ui->currentPageLbl->setText(ui->navExpBtn->text());
}
void MainWindow::on_navExportBtn_clicked() {
    ui->currentPageLbl->setText(ui->navExportBtn->text());
}
void MainWindow::on_navAwardBtn_clicked() {
    ui->currentPageLbl->setText(ui->navAwardBtn->text());
}

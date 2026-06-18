#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "AddCourseDialog.h"
#include "DatabaseMannager.h"
#include "Theme.h"
#include "User.h"

#include <QComboBox>
#include <QCursor>
#include <QDate>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QSqlQuery>
#include <QStyle>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>
#include <QtMath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    applyModernStyle();
    buildHomePage();
    buildExportPage();

    InitFrame();
    updateSidebarUserInfo();
    InitCoursePage();
    InitExpPage();
    updateHomePageStats();
}

MainWindow::~MainWindow() {
    delete courseModel;
    delete expModel;
    delete awardModel;
    delete ui;
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);

    // 等窗口完成首次布局后再绘制首页图表。构造阶段控件尺寸尚未稳定，
    // 会导致登录后首次显示的折线图仍使用初始化时的临时尺寸。
    QTimer::singleShot(0, this, [this]() {
        if (ui->stackedWidget->currentIndex() == 0)
            updateHomePageStats();
    });
}

void MainWindow::applyModernStyle() {
    setWindowTitle("College Tracker");
    resize(1120, 720);
    setMinimumSize(1040, 660);

    ui->menubar->hide();
    ui->statusbar->hide();

    ui->horizontalLayout->setContentsMargins(0, 0, 0, 0);
    ui->horizontalLayout->setSpacing(0);
    ui->verticalLayout_3->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout_3->setSpacing(0);

    ui->mainContentFrame->setFrameShape(QFrame::NoFrame);
    ui->sidebarFrame->setFrameShape(QFrame::NoFrame);
    ui->sidebarFrame->setMinimumWidth(238);
    ui->sidebarFrame->setMaximumWidth(238);
    ui->sidebarFrame->setProperty("lightOnly", true);

    // 固定为浅色视觉：白色侧栏 + 浅灰工作区 + 卡片式内容。
    ui->currentPageLbl->setGeometry(42, 30, 780, 46);
    ui->stackedWidget->setGeometry(42, 96, 810, 545);

    ui->courseTableView->setGeometry(28, 102, 754, 350);
    ui->addCourseBtn->setGeometry(500, 38, 132, 40);
    ui->deleteCourseBtn->setGeometry(646, 38, 136, 40);
    ui->labelStats->setGeometry(28, 466, 754, 48);

    ui->expTableView->setGeometry(28, 60, 754, 200);
    ui->addExpFrame->setGeometry(28, 288, 754, 220);
    ui->addExpTitle->setGeometry(38, 256, 180, 28);

    ui->awardTableView->setGeometry(28, 60, 754, 200);
    ui->addAwardFrame->setGeometry(28, 288, 754, 280);

    ui->addExpTypeLbl->setGeometry(36, 30, 72, 36);
    ui->addExpTypeCBox->setGeometry(118, 30, 190, 36);
    ui->addExpTitleLbl->setGeometry(354, 30, 72, 36);
    ui->addExpTitleLine->setGeometry(436, 30, 260, 36);
    ui->addExpDateLbl->setGeometry(36, 88, 72, 36);
    ui->addExpDateLine->setGeometry(118, 88, 190, 36);
    ui->addExpDescLbl->setGeometry(354, 88, 72, 36);
    ui->addExpDescLine->setGeometry(436, 88, 260, 36);
    ui->addExpBtn->setGeometry(436, 154, 116, 38);
    ui->DelExpBtn->setGeometry(570, 154, 126, 38);

    ui->addAwardTitleLbl->setGeometry(36, 34, 88, 36);
    ui->addAwardLine->setGeometry(134, 34, 250, 36);
    ui->addAwardDateLbl->setGeometry(416, 34, 64, 36);
    ui->addAwardDateLine->setGeometry(490, 34, 180, 36);
    ui->addAwardLevelLbl->setGeometry(36, 96, 88, 36);
    ui->addAwardLevelCBox->setGeometry(134, 96, 180, 36);
    ui->addAwardBtn->setGeometry(436, 220, 116, 38);
    ui->delAwardBtn->setGeometry(570, 220, 126, 38);

    // CSV 导入导出按钮
    importCourseCsvBtn = new QPushButton("导入CSV", ui->coursePage);
    importCourseCsvBtn->setObjectName("importCourseCsvBtn");
    importCourseCsvBtn->setGeometry(28, 38, 120, 40);
    importCourseCsvBtn->setCursor(Qt::PointingHandCursor);

    exportCourseCsvBtn = new QPushButton("导出CSV", ui->coursePage);
    exportCourseCsvBtn->setObjectName("exportCourseCsvBtn");
    exportCourseCsvBtn->setGeometry(158, 38, 120, 40);
    exportCourseCsvBtn->setCursor(Qt::PointingHandCursor);

    importExpCsvBtn = new QPushButton("导入CSV", ui->expPage);
    importExpCsvBtn->setObjectName("importExpCsvBtn");
    importExpCsvBtn->setGeometry(310, 14, 120, 38);
    importExpCsvBtn->setCursor(Qt::PointingHandCursor);

    exportExpCsvBtn = new QPushButton("导出CSV", ui->expPage);
    exportExpCsvBtn->setObjectName("exportExpCsvBtn");
    exportExpCsvBtn->setGeometry(440, 14, 120, 38);
    exportExpCsvBtn->setCursor(Qt::PointingHandCursor);

    // CSV 格式帮助按钮（小圆形 ? 按钮）
    csvHelpCourseBtn = new QPushButton("?", ui->coursePage);
    csvHelpCourseBtn->setObjectName("csvHelpCourseBtn");
    csvHelpCourseBtn->setGeometry(284, 38, 40, 40);
    csvHelpCourseBtn->setCursor(Qt::PointingHandCursor);
    csvHelpCourseBtn->setToolTip("查看课程 CSV 格式说明");

    csvHelpExpBtn = new QPushButton("?", ui->expPage);
    csvHelpExpBtn->setObjectName("csvHelpExpBtn");
    csvHelpExpBtn->setGeometry(566, 14, 40, 38);
    csvHelpExpBtn->setCursor(Qt::PointingHandCursor);
    csvHelpExpBtn->setToolTip("查看 CSV 格式说明");

    // ======================================================================
    // 统一 QSS 样式表 — 基于 Theme Token 的语义化设计系统
    // 主色：Teal (#0D9488) → 专业教育感；强调色：Cyan (#06B6D4) → 交互反馈
    // 排版基准 16px (Major Third 1.25)
    // ======================================================================
    using namespace Theme;

    const QString qss = QStringLiteral(R"(
        * {
            font-family: "Microsoft YaHei", "PingFang SC", "Helvetica Neue", Arial, sans-serif;
            font-size: %1px;
            color: %2;
        }
        QMainWindow, QWidget#centralwidget { background: %3; }

        /* ===== 侧边栏 ===== */
        QFrame#sidebarFrame {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #F8FCFD, stop:0.48 #F1F8FA, stop:1 #EAF4F6);
            border-right: 1px solid rgba(148,163,184,0.22);
        }

        /* ===== 用户信息卡片 ===== */
        QWidget#userProfileWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 rgba(255,255,255,0.90),
                        stop:0.54 rgba(243,250,249,0.82),
                        stop:1 rgba(235,245,243,0.72));
            border: 1px solid rgba(153,200,188,0.52);
            border-radius: %4px;
        }
        QLabel#userNameLbl {
            color: #0F172A;
            font-size: %5px;
            font-weight: %6;
            letter-spacing: 0.3px;
            qproperty-alignment: AlignCenter;
            background: transparent;
            border: none;
        }
        QLabel#detailLbl {
            color: %7;
            font-size: %8px;
            font-weight: %9;
            qproperty-alignment: AlignCenter;
            background: %10;
            border: 1px solid rgba(13,148,136,0.22);
            border-radius: %11px;
            padding: 3px 12px;
        }
        QLabel#majorLbl {
            color: %12;
            font-size: %13px;
            font-weight: %14;
            qproperty-alignment: AlignCenter;
            background: transparent;
            border: none;
            padding: 0px 8px;
            line-height: 18px;
        }
        QPushButton#editProfileBtn {
            min-height: 30px; max-height: 30px;
            border-radius: %11px;
            border: 1px solid rgba(13,148,136,0.34);
            background: rgba(255,255,255,0.70);
            color: %7;
            font-size: %13px;
            font-weight: %9;
            text-align: center;
            padding: 0 12px;
        }
        QPushButton#editProfileBtn:hover {
            background: %10;
            border: 1px solid rgba(13,148,136,0.60);
            color: #0B5E57;
        }

        /* ===== 主内容区 ===== */
        QFrame#mainContentFrame {
            background: %15;
            border: none;
            border-left: 1px solid rgba(148,163,184,0.22);
        }
        QLabel#currentPageLbl {
            color: %2;
            font-size: %5px;
            font-weight: %6;
            letter-spacing: 0.35px;
            qproperty-alignment: AlignLeft | AlignVCenter;
            background: transparent;
            border-left: 6px solid %7;
            padding-left: 14px;
        }

        /* ===== 侧边栏导航按钮 ===== */
        #sidebarFrame QPushButton {
            min-height: 56px; max-height: 56px;
            border: 1px solid rgba(148,163,184,0.42);
            border-radius: %11px;
            padding-left: 22px; padding-right: 16px;
            text-align: left;
            color: %12;
            background: rgba(255,255,255,0.50);
            font-size: %16px;
            font-weight: %9;
            letter-spacing: 0.2px;
        }
        #sidebarFrame QPushButton:hover {
            background: rgba(255,255,255,0.80);
            border: 1px solid rgba(6,182,212,0.50);
            color: %2;
        }
        #sidebarFrame QPushButton:pressed {
            background: rgba(226,232,240,0.78);
            border: 1px solid rgba(13,148,136,0.42);
        }
        #sidebarFrame QPushButton[active="true"] {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                        stop:0 #0D9488, stop:0.56 #0EA5E9, stop:1 #06B6D4);
            color: #FFFFFF;
            border: 1px solid rgba(255,255,255,0.70);
            border-radius: %11px;
        }

        /* ===== 通用按钮 ===== */
        QPushButton {
            min-height: 36px;
            border: 1px solid %17;
            border-radius: %18px;
            padding: 0 20px;
            color: #FFFFFF;
            background: %17;
            font-weight: %9;
            letter-spacing: 0.2px;
        }
        QPushButton:hover {
            background: #252B34;
            border-color: %19;
        }
        QPushButton:pressed { background: #05070A; }
        QPushButton:disabled { background: #CBD0D7; border-color: #CBD0D7; color: #FFFFFF; }

        /* CSV 帮助按钮 */
        QPushButton#csvHelpCourseBtn, QPushButton#csvHelpExpBtn, QPushButton#homeCsvHelpBtn {
            min-height: 38px; max-height: 40px;
            min-width: 38px; max-width: 40px;
            border-radius: 20px;
            background: %20;
            color: %12;
            border: 1px solid %21;
            font-size: 18px;
            font-weight: %9;
            padding: 0px;
        }
        QPushButton#csvHelpCourseBtn:hover, QPushButton#csvHelpExpBtn:hover, QPushButton#homeCsvHelpBtn:hover {
            background: %10;
            color: %7;
            border-color: %7;
        }

        /* 首页一键导入导出 */
        QPushButton#homeImportAllBtn, QPushButton#homeExportAllBtn {
            min-height: 42px; max-height: 42px;
            min-width: 172px;
            font-size: %16px;
            font-weight: %9;
            border-radius: %18px;
        }
        QPushButton#homeImportAllBtn {
            background: %20; color: %17; border: 1px solid %17;
        }
        QPushButton#homeImportAllBtn:hover {
            background: %17; color: #FFFFFF;
        }
        QPushButton#homeExportAllBtn {
            background: %7; color: #FFFFFF; border: 1px solid %7;
        }
        QPushButton#homeExportAllBtn:hover {
            background: #0B5E57; border-color: #0B5E57;
        }

        /* 删除按钮 */
        QPushButton#deleteCourseBtn, QPushButton#DelExpBtn, QPushButton#delAwardBtn {
            background: %20; color: %22; border: 1px solid %22;
        }
        QPushButton#deleteCourseBtn:hover, QPushButton#DelExpBtn:hover, QPushButton#delAwardBtn:hover {
            background: %22; color: #FFFFFF;
        }

        /* Stacked 页面透明 */
        QStackedWidget, QWidget#homePage, QWidget#coursePage, QWidget#expPage, QWidget#awardPage, QWidget#profilePage {
            background: transparent;
        }

        /* ===== 表单区域 Card ===== */
        QFrame#addExpFrame, QFrame#addAwardFrame {
            background: %20;
            border: 1px solid %21;
            border-top: 4px solid %7;
            border-radius: %18px;
        }
        QLabel#addExpTitle {
            color: %2;
            font-size: %36px;
            font-weight: %6;
            background: transparent;
            qproperty-alignment: AlignLeft | AlignVCenter;
        }

        /* ===== 标签 ===== */
        QLabel {
            color: %23;
            background: transparent;
            font-weight: %14;
        }

        /* ===== 输入控件 ===== */
        QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox {
            min-height: 36px;
            border: 1px solid %21;
            border-radius: %18px;
            padding: 0 12px;
            background: %20;
            color: %2;
            selection-background-color: %7;
            selection-color: #FFFFFF;
        }
        QLineEdit:hover, QComboBox:hover, QDateEdit:hover, QDoubleSpinBox:hover {
            border: 1px solid %19;
            background: #FBFCFD;
        }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QDoubleSpinBox:focus {
            border: 2px solid %7;
            background: %20;
        }
        QComboBox::drop-down, QDateEdit::drop-down { border: none; width: 28px; }
        QComboBox QAbstractItemView {
            background: %20;
            border: 1px solid %21;
            selection-background-color: %17;
            selection-color: #FFFFFF;
            outline: none;
        }

        /* ===== 表格 ===== */
        QTableView {
            background: %20;
            alternate-background-color: %24;
            border: 1px solid %21;
            border-top: 4px solid %17;
            border-radius: %18px;
            gridline-color: transparent;
            selection-background-color: %25;
            selection-color: %2;
            padding: 4px;
            outline: none;
        }
        QTableView::item {
            min-height: 38px;
            padding: 8px;
            border-bottom: 1px solid #E8ECF1;
        }
        QHeaderView::section {
            background: %17;
            color: #FFFFFF;
            border: none;
            border-right: 1px solid #343B46;
            padding: 12px 10px;
            font-size: %13px;
            font-weight: %9;
            letter-spacing: 0.25px;
        }
        QTableCornerButton::section {
            background: %17; border: none;
        }

        /* ===== 滚动条 ===== */
        QScrollBar:vertical {
            background: transparent; width: 10px;
            margin: 6px 2px 6px 0px;
        }
        QScrollBar::handle:vertical {
            background: #9AA4B2; border-radius: 3px; min-height: 30px;
        }
        QScrollBar::handle:vertical:hover { background: %19; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }

        /* ===== 首页卡片 ===== */
        QFrame#homeChartCard, QFrame#homeStatCard {
            background: rgba(255,255,255,0.92);
            border: 1px solid rgba(203,213,225,0.88);
            border-radius: %26px;
        }
        QFrame#homeChartCard { border-left: 5px solid %19; }
        QLabel#homeCardTitle {
            color: %2; font-size: %36px; font-weight: %6;
            background: transparent;
        }
        QLabel#homeChartLabel { background: transparent; border: none; }
        QLabel#homeStatTitle {
            color: %12; font-size: %13px; font-weight: %9;
            background: transparent;
        }
        QLabel#homeStatValue {
            color: %2; font-size: %27px; font-weight: %6;
            background: transparent;
        }
        QLabel#homeStatSub {
            color: %28; font-size: %13px; font-weight: %14;
            background: transparent;
        }

        /* ===== 统计标签 ===== */
        QLabel#labelStats {
            background: %20;
            border: 1px solid %21;
            border-left: 5px solid %7;
            border-radius: %18px;
            color: %2;
            font-size: %16px;
            font-weight: %9;
            qproperty-alignment: AlignCenter;
        }

        /* ===== 对话框 ===== */
        QMessageBox, QDialog { background: %20; }
    )"
    ).arg(TypeScale::body)          // %1
     .arg(Color::onSurface)         // %2
     .arg(Color::background)        // %3
     .arg(Radius::xl)               // %4
     .arg(TypeScale::h1)            // %5
     .arg(FontWeight::heavy)        // %6
     .arg(Color::primary)           // %7
     .arg(TypeScale::caption)       // %8
     .arg(FontWeight::bold)         // %9
     .arg(Color::primaryBg)         // %10
     .arg(Radius::lg)               // %11
     .arg(Color::onSurfaceVar)      // %12
     .arg(TypeScale::caption)       // %13
     .arg(FontWeight::medium)       // %14
     .arg(Color::surfaceVariant)    // %15
     .arg(TypeScale::body)          // %16 (repeated)
     .arg(Color::headerBg)          // %17
     .arg(Radius::sm)               // %18
     .arg(Color::accent)            // %19
     .arg(Color::surface)           // %20
     .arg(Color::outline)           // %21
     .arg(Color::error)             // %22
     .arg(Color::onSurfaceVar)      // %23 (repeated)
     .arg(Color::background)        // %24 (alternate bg)
     .arg(Color::accentLight)       // %25
     .arg(Radius::lg)               // %26 (repeated for cards)
     .arg(TypeScale::display)       // %27
     .arg(Color::onSurfaceMuted);   // %28

    setStyleSheet(qss);

    QList<QPushButton *> buttons = findChildren<QPushButton *>();
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
    tableView->setEditTriggers(QAbstractItemView::DoubleClicked);
}

void MainWindow::buildHomePage() {
    if (ui->homePage->layout() != nullptr)
        return;

    auto *mainLayout = new QVBoxLayout(ui->homePage);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(14);

    auto makeCard = [](const QString &title, const QString &objectName) {
        auto *card = new QFrame;
        card->setObjectName(objectName);
        card->setFrameShape(QFrame::NoFrame);
        auto *shadow = new QGraphicsDropShadowEffect(card);
        shadow->setBlurRadius(24);
        shadow->setOffset(0, 8);
        shadow->setColor(QColor(40, 62, 84, 16));
        card->setGraphicsEffect(shadow);

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(22, 16, 22, 18);
        layout->setSpacing(8);
        auto *titleLbl = new QLabel(title);
        titleLbl->setObjectName("homeCardTitle");
        layout->addWidget(titleLbl);
        return card;
    };

    QFrame *chartCard = makeCard("学期平均 GPA 走势", "homeChartCard");
    chartCard->setMinimumHeight(292);
    chartCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    homeChartLabel = new QLabel;
    homeChartLabel->setObjectName("homeChartLabel");
    homeChartLabel->setMinimumSize(520, 220);
    homeChartLabel->setAlignment(Qt::AlignCenter);
    homeChartLabel->setSizePolicy(QSizePolicy::Expanding,
                                  QSizePolicy::Expanding);
    chartCard->layout()->addWidget(homeChartLabel);

    auto *metricsPanel = new QFrame;
    metricsPanel->setObjectName("homeMetricsPanel");
    metricsPanel->setFixedHeight(126);
    auto *metricsLayout = new QVBoxLayout(metricsPanel);
    metricsLayout->setContentsMargins(0, 0, 0, 0);
    metricsLayout->setSpacing(7);

    auto *metricsTitle = new QLabel("学习档案快照");
    metricsTitle->setObjectName("homeMetricsTitle");
    auto *metricsSub = new QLabel("课程、实践与成果概览");
    metricsSub->setObjectName("homeMetricsSub");
    auto *metricsHeader = new QHBoxLayout;
    metricsHeader->setContentsMargins(2, 0, 2, 0);
    metricsHeader->setSpacing(10);
    metricsHeader->addWidget(metricsTitle);
    metricsHeader->addWidget(metricsSub);
    metricsHeader->addStretch();
    metricsLayout->addLayout(metricsHeader);

    auto *statsLayout = new QGridLayout;
    statsLayout->setContentsMargins(0, 0, 0, 0);
    statsLayout->setHorizontalSpacing(8);
    statsLayout->setVerticalSpacing(0);

    auto makeStatCard = [](const QString &title, const QString &subTitle,
                           QLabel **valueLabel, const QString &tone) {
        auto *card = new QFrame;
        card->setObjectName("homeStatCard");
        card->setProperty("tone", tone);
        card->setFrameShape(QFrame::NoFrame);
        card->setMinimumHeight(92);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(12, 8, 10, 7);
        layout->setSpacing(1);
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

    statsLayout->addWidget(makeStatCard("已修课程", "修读课程总数",
                                        &homeCourseCountLbl, "teal"), 0, 0);
    statsLayout->addWidget(makeStatCard("GPA", "加权 GPA",
                                        &homeGpaLbl, "cyan"), 0, 1);
    statsLayout->addWidget(makeStatCard("竞赛", "课外活动",
                                        &homeCompetitionCountLbl, "amber"), 0, 2);
    statsLayout->addWidget(makeStatCard("实习", "实践经历",
                                        &homeInternshipCountLbl, "blue"), 0, 3);
    statsLayout->addWidget(makeStatCard("项目", "项目 / 科研",
                                        &homeProjectCountLbl, "violet"), 0, 4);
    statsLayout->addWidget(makeStatCard("荣誉", "奖项成果",
                                        &homeAwardCountLbl, "rose"), 0, 5);
    for (int column = 0; column < 6; ++column)
        statsLayout->setColumnStretch(column, 1);
    metricsLayout->addLayout(statsLayout);

    mainLayout->addWidget(chartCard, 1);
    mainLayout->addWidget(metricsPanel);

    // 一键导入导出按钮栏
    auto *actionBar = new QFrame;
    actionBar->setObjectName("homeActionBar");
    actionBar->setFixedHeight(58);
    auto *csvAllLayout = new QHBoxLayout(actionBar);
    csvAllLayout->setContentsMargins(16, 8, 10, 8);
    csvAllLayout->setSpacing(10);

    auto *actionTextLayout = new QVBoxLayout;
    actionTextLayout->setContentsMargins(0, 0, 0, 0);
    actionTextLayout->setSpacing(0);
    auto *actionTitle = new QLabel("数据管理");
    actionTitle->setObjectName("homeActionTitle");
    auto *actionSub = new QLabel("快速备份或迁移全部档案");
    actionSub->setObjectName("homeActionSub");
    actionTextLayout->addWidget(actionTitle);
    actionTextLayout->addWidget(actionSub);

    homeImportAllBtn = new QPushButton("一键导入全部数据");
    homeImportAllBtn->setObjectName("homeImportAllBtn");
    homeImportAllBtn->setFixedHeight(40);
    homeImportAllBtn->setCursor(Qt::PointingHandCursor);

    homeExportAllBtn = new QPushButton("一键导出全部数据");
    homeExportAllBtn->setObjectName("homeExportAllBtn");
    homeExportAllBtn->setFixedHeight(40);
    homeExportAllBtn->setCursor(Qt::PointingHandCursor);

    homeCsvHelpBtn = new QPushButton("?");
    homeCsvHelpBtn->setObjectName("homeCsvHelpBtn");
    homeCsvHelpBtn->setFixedSize(40, 40);
    homeCsvHelpBtn->setCursor(Qt::PointingHandCursor);
    homeCsvHelpBtn->setToolTip("查看一键导入导出 CSV 格式说明");

    csvAllLayout->addLayout(actionTextLayout);
    csvAllLayout->addStretch(1);
    csvAllLayout->addWidget(homeImportAllBtn);
    csvAllLayout->addWidget(homeExportAllBtn);
    csvAllLayout->addWidget(homeCsvHelpBtn);

    mainLayout->addWidget(actionBar);
}

double MainWindow::scoreToGpa(double score) const {
    return DatabaseManager::scoreToGpa(score);
}

void MainWindow::updateHomePageStats() {
    if (homeChartLabel == nullptr)
        return;

    const int userId = User::getInstance().getId();
    const QStringList semesters = {"大一上", "大一下", "大二上", "大二下",
                                   "大三上", "大三下", "大四上", "大四下"};
    QVector<double> gpas(semesters.size(), -1.0);

    QVector<double> semesterGpaCreditSums(semesters.size(), 0.0);
    QVector<double> semesterCreditSums(semesters.size(), 0.0);

    QSqlQuery courseQuery;
    courseQuery.prepare(
        "SELECT semester, score, credit FROM courses WHERE user_id = :uid");
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
            if (credit <= 0.0)
                continue;

            totalGpaCreditSum += gpa * credit;
            totalCredits += credit;

            int index = semesters.indexOf(semester);
            if (index < 0) {
                for (int i = 0; i < semesters.size(); ++i) {
                    if (semester.contains(semesters[i])) {
                        index = i;
                        break;
                    }
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

    double avgGpa =
        totalCredits > 0.0 ? (totalGpaCreditSum / totalCredits) : 0.0;

    int internshipCount = 0;
    int competitionCount = 0;
    int projectCount = 0;
    QSqlQuery expQuery;
    expQuery.prepare("SELECT type, COUNT(*) FROM experiences WHERE user_id = "
                     ":uid GROUP BY type");
    expQuery.bindValue(":uid", userId);
    if (expQuery.exec()) {
        while (expQuery.next()) {
            QString type = expQuery.value(0).toString();
            int count = expQuery.value(1).toInt();
            if (type.contains("实习"))
                internshipCount += count;
            else if (type.contains("竞赛"))
                competitionCount += count;
            else if (type.contains("项目"))
                projectCount += count;
        }
    }

    int awardCount = 0;
    QSqlQuery awardQuery;
    awardQuery.prepare("SELECT COUNT(*) FROM awards WHERE user_id = :uid");
    awardQuery.bindValue(":uid", userId);
    if (awardQuery.exec() && awardQuery.next())
        awardCount = awardQuery.value(0).toInt();

    homeCourseCountLbl->setText(QString::number(totalCourses));
    homeGpaLbl->setText(QString::number(avgGpa, 'f', 2));
    homeCompetitionCountLbl->setText(QString::number(competitionCount));
    homeInternshipCountLbl->setText(QString::number(internshipCount));
    homeProjectCountLbl->setText(QString::number(projectCount));
    homeAwardCountLbl->setText(QString::number(awardCount));

    // 使用标签的真实尺寸绘图，避免大尺寸 pixmap 在较窄的 QLabel 中被裁切。
    const int width = qMax(360, homeChartLabel->width());
    const int height = qMax(220, homeChartLabel->height());
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRectF plot(48, 34, width - 62, height - 94);
    QPen gridPen(QColor(226, 232, 240));
    gridPen.setWidth(1);
    painter.setPen(gridPen);
    painter.setFont(QFont("PingFang SC", 9, QFont::DemiBold));
    painter.setBrush(Qt::NoBrush);

    for (int i = 0; i <= 4; ++i) {
        double y = plot.bottom() - plot.height() * i / 4.0;
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.setPen(QColor(100, 116, 139));
        painter.drawText(QRectF(10, y - 10, 40, 20),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(i, 'f', 1));
        painter.setPen(gridPen);
    }

    painter.setPen(QPen(QColor(203, 213, 225), 1));
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.drawLine(plot.bottomLeft(), plot.topLeft());

    QVector<QPointF> points;
    const double step = plot.width() / (semesters.size() - 1);
    painter.setPen(QColor(71, 85, 105));
    for (int i = 0; i < semesters.size(); ++i) {
        double x = plot.left() + i * step;
        painter.drawText(QRectF(x - 28, plot.bottom() + 10, 56, 28),
                         Qt::AlignCenter, semesters[i]);
        if (gpas[i] >= 0.0) {
            double y =
                plot.bottom() - plot.height() * qBound(0.0, gpas[i], 4.0) / 4.0;
            points.append(QPointF(x, y));
        }
    }

    if (points.size() >= 2) {
        QPainterPath path(points.first());
        for (int i = 1; i < points.size(); ++i)
            path.lineTo(points[i]);

        QPainterPath areaPath = path;
        areaPath.lineTo(points.last().x(), plot.bottom());
        areaPath.lineTo(points.first().x(), plot.bottom());
        areaPath.closeSubpath();
        QLinearGradient areaGradient(0, plot.top(), 0, plot.bottom());
        areaGradient.setColorAt(0, QColor(13, 148, 136, 72));
        areaGradient.setColorAt(1, QColor(13, 148, 136, 4));
        painter.setPen(Qt::NoPen);
        painter.setBrush(areaGradient);
        painter.drawPath(areaPath);

        QPen linePen(QColor(13, 148, 136), 4, Qt::SolidLine, Qt::RoundCap,
                     Qt::RoundJoin);
        painter.setPen(linePen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

    for (const QPointF &point : points) {
        painter.setPen(QPen(QColor(255, 255, 255), 4));
        painter.setBrush(QColor(13, 148, 136));
        painter.drawEllipse(point, 6, 6);
    }

    if (points.isEmpty()) {
        painter.setPen(QColor(148, 163, 184));
        painter.setFont(QFont("PingFang SC", 15, QFont::DemiBold));
        painter.drawText(plot, Qt::AlignCenter,
                         "暂无课程 GPA 数据\n录入课程后这里会自动生成折线图");
    }

    painter.setPen(QColor(100, 116, 139));
    painter.setFont(QFont("PingFang SC", 10, QFont::DemiBold));
    painter.drawText(QRectF(plot.left(), 4, plot.width(), 22),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     "横轴：学期    纵轴：平均 GPA");

    homeChartLabel->setPixmap(pixmap);
}

void MainWindow::buildExportPage() {
    if (ui->profilePage->layout() != nullptr)
        return;

    using namespace Theme;

    auto *mainLayout = new QVBoxLayout(ui->profilePage);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(Spacing::md);

    // ---- 辅助：创建带标题的 Card（风格参考 经历与荣誉 页面）----
    auto makeSectionCard = [](const QString &title) {
        auto *card = new QFrame;
        card->setFrameShape(QFrame::NoFrame);
        card->setStyleSheet(QStringLiteral(
            "QFrame { background: %1; border: 1px solid %2;"
            " border-radius: %3px; }"
        ).arg(Color::surface).arg(Color::outline).arg(Radius::sm));

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(Spacing::lg, Spacing::md, Spacing::lg, Spacing::md);
        layout->setSpacing(Spacing::sm);

        auto *titleLbl = new QLabel(title);
        titleLbl->setStyleSheet(QStringLiteral(
            "font-size: %1px; font-weight: %2; color: %3; background: transparent;"
        ).arg(TypeScale::h2).arg(FontWeight::bold).arg(Color::onSurface));
        layout->addWidget(titleLbl);
        return card;
    };

    // ===== 照片区域（标题栏内嵌导入按钮）=====
    QFrame *photoCard = new QFrame;
    photoCard->setFrameShape(QFrame::NoFrame);
    photoCard->setStyleSheet(QStringLiteral(
        "QFrame { background: %1; border: 1px solid %2;"
        " border-radius: %3px; }"
    ).arg(Color::surface).arg(Color::outline).arg(Radius::sm));
    auto *photoCardLayout = new QVBoxLayout(photoCard);
    photoCardLayout->setContentsMargins(Spacing::lg, Spacing::md, Spacing::lg, Spacing::md);
    photoCardLayout->setSpacing(Spacing::sm);

    // 标题行："个人照片" 标题 + 导入按钮
    auto *photoHeaderRow = new QHBoxLayout;
    photoHeaderRow->setSpacing(Spacing::sm);

    auto *photoTitleLbl = new QLabel("个人照片");
    photoTitleLbl->setStyleSheet(QStringLiteral(
        "font-size: %1px; font-weight: %2; color: %3; background: transparent;"
    ).arg(TypeScale::h2).arg(FontWeight::bold).arg(Color::onSurface));
    photoHeaderRow->addWidget(photoTitleLbl);
    photoHeaderRow->addStretch();

    selectPhotoBtn = new QPushButton("导入照片");
    selectPhotoBtn->setObjectName("selectPhotoBtn");
    selectPhotoBtn->setCursor(Qt::PointingHandCursor);
    selectPhotoBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: #FFFFFF; border: none;"
        " border-radius: %2px; min-height: 32px; font-size: %3px;"
        " font-weight: %4; padding: 0 16px; }"
        "QPushButton:hover { background: %5; }"
    ).arg(Color::primary).arg(Radius::sm)
     .arg(TypeScale::caption).arg(FontWeight::bold).arg(Color::accent));

    photoHeaderRow->addWidget(selectPhotoBtn);
    photoCardLayout->addLayout(photoHeaderRow);

    // 照片预览
    auto *photoRow = new QHBoxLayout;
    photoRow->setSpacing(Spacing::md);

    photoPreviewLbl = new QLabel;
    photoPreviewLbl->setObjectName("photoPreviewLbl");
    photoPreviewLbl->setFixedSize(96, 96);
    photoPreviewLbl->setAlignment(Qt::AlignCenter);
    photoPreviewLbl->setStyleSheet(QStringLiteral(
        "QLabel { background: %1; border: 1px dashed %2; border-radius: 48px;"
        " color: %3; font-size: %4px; }"
    ).arg(Color::background).arg(Color::outline)
     .arg(Color::onSurfaceMuted).arg(TypeScale::caption));
    photoPreviewLbl->setText("暂无照片");
    photoRow->addWidget(photoPreviewLbl);
    photoRow->addStretch();
    photoCardLayout->addLayout(photoRow);
    mainLayout->addWidget(photoCard);

    // ===== 技术能力区域 =====
    QFrame *skillsCard = makeSectionCard("技术能力");

    auto *skillsHeader = new QHBoxLayout;
    skillsHeader->setSpacing(Spacing::sm);

    skillsLbl = new QLabel;
    skillsLbl->setObjectName("skillsLbl");
    skillsLbl->setWordWrap(true);
    skillsLbl->setMinimumHeight(48);
    skillsLbl->setStyleSheet(QStringLiteral(
        "QLabel { background: %1; border: 1px solid %2; border-radius: %3px;"
        " color: %4; font-size: %5px; padding: 10px 14px; }"
    ).arg(Color::background).arg(Color::outline)
     .arg(Radius::sm).arg(Color::onSurfaceMuted).arg(TypeScale::body));
    skillsLbl->setText("暂无内容，点击编辑按钮添加");

    editSkillsBtn = new QPushButton("编辑");
    editSkillsBtn->setObjectName("editSkillsBtn");
    editSkillsBtn->setCursor(Qt::PointingHandCursor);
    editSkillsBtn->setFixedSize(64, 32);
    editSkillsBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: %1;"
        " border: 1px solid %1; border-radius: %2px;"
        " font-size: %3px; font-weight: %4; }"
        "QPushButton:hover { background: %1; color: #FFFFFF; }"
    ).arg(Color::primary).arg(Radius::sm)
     .arg(TypeScale::caption).arg(FontWeight::bold));

    skillsHeader->addStretch();
    skillsHeader->addWidget(editSkillsBtn);
    static_cast<QVBoxLayout*>(skillsCard->layout())->addWidget(skillsLbl);
    static_cast<QVBoxLayout*>(skillsCard->layout())->addLayout(skillsHeader);
    mainLayout->addWidget(skillsCard);

    // ===== 个人总结区域 =====
    QFrame *summaryCard = makeSectionCard("个人总结");

    auto *summaryHeader = new QHBoxLayout;
    summaryHeader->setSpacing(Spacing::sm);

    summaryLbl = new QLabel;
    summaryLbl->setObjectName("summaryLbl");
    summaryLbl->setWordWrap(true);
    summaryLbl->setMinimumHeight(48);
    summaryLbl->setStyleSheet(QStringLiteral(
        "QLabel { background: %1; border: 1px solid %2; border-radius: %3px;"
        " color: %4; font-size: %5px; padding: 10px 14px; }"
    ).arg(Color::background).arg(Color::outline)
     .arg(Radius::sm).arg(Color::onSurfaceMuted).arg(TypeScale::body));
    summaryLbl->setText("暂无内容，点击编辑按钮添加");

    editSummaryBtn = new QPushButton("编辑");
    editSummaryBtn->setObjectName("editSummaryBtn");
    editSummaryBtn->setCursor(Qt::PointingHandCursor);
    editSummaryBtn->setFixedSize(64, 32);
    editSummaryBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: %1;"
        " border: 1px solid %1; border-radius: %2px;"
        " font-size: %3px; font-weight: %4; }"
        "QPushButton:hover { background: %1; color: #FFFFFF; }"
    ).arg(Color::primary).arg(Radius::sm)
     .arg(TypeScale::caption).arg(FontWeight::bold));

    summaryHeader->addStretch();
    summaryHeader->addWidget(editSummaryBtn);
    static_cast<QVBoxLayout*>(summaryCard->layout())->addWidget(summaryLbl);
    static_cast<QVBoxLayout*>(summaryCard->layout())->addLayout(summaryHeader);
    mainLayout->addWidget(summaryCard);

    // ===== 保存按钮 =====
    saveResumeBtn = new QPushButton("保存到数据库");
    saveResumeBtn->setObjectName("saveResumeBtn");
    saveResumeBtn->setCursor(Qt::PointingHandCursor);
    saveResumeBtn->setFixedHeight(44);
    saveResumeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: #FFFFFF; border: none;"
        " border-radius: %2px; font-size: %3px; font-weight: %4; }"
        "QPushButton:hover { background: %5; }"
        "QPushButton:pressed { background: #0B5E57; }"
    ).arg(Color::primary).arg(Radius::sm)
     .arg(TypeScale::body).arg(FontWeight::bold).arg(Color::primaryHover));

    mainLayout->addWidget(saveResumeBtn);
    mainLayout->addStretch();

    // ---- 信号连接 ----
    connect(selectPhotoBtn, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(
            this, "导入个人照片", QString(),
            "JPEG 图片 (*.jpg *.jpeg);;所有文件 (*)");
        if (filePath.isEmpty())
            return;

        // 创建 photos 目录
        QString photosDir = QCoreApplication::applicationDirPath() + "/photos";
        QDir().mkpath(photosDir);

        // 复制照片到应用数据目录（如已存在则先删除，QFile::copy 不会自动覆盖）
        QFileInfo fi(filePath);
        int userId = User::getInstance().getId();
        QString newName = QString("user_%1_photo.%2")
                              .arg(userId)
                              .arg(fi.suffix().toLower());
        QString destPath = photosDir + "/" + newName;

        if (QFile::exists(destPath))
            QFile::remove(destPath);

        if (QFile::copy(filePath, destPath)) {
            m_photoPath = "photos/" + newName;
            QPixmap source(destPath);
            if (!source.isNull()) {
                // 裁切成圆形（KeepAspectRatio 确保图片完整可见）
                int size = photoPreviewLbl->width();
                QPixmap scaled = source.scaled(size, size,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
                // 居中放置
                int x = (size - scaled.width()) / 2;
                int y = (size - scaled.height()) / 2;

                QPixmap circular(size, size);
                circular.fill(Qt::transparent);
                QPainter painter(&circular);
                painter.setRenderHint(QPainter::Antialiasing, true);
                QPainterPath path;
                path.addEllipse(0, 0, size, size);
                painter.setClipPath(path);
                painter.drawPixmap(x, y, scaled);
                painter.end();

                photoPreviewLbl->setPixmap(circular);
                photoPreviewLbl->setText(QString());
            }
        } else {
            QMessageBox::warning(this, "提示", "照片复制失败，请重试");
        }
    });

    connect(editSkillsBtn, &QPushButton::clicked, this,
            &MainWindow::editSkills);
    connect(editSummaryBtn, &QPushButton::clicked, this,
            &MainWindow::editSummary);

    connect(saveResumeBtn, &QPushButton::clicked, this, [this]() {
        int userId = User::getInstance().getId();
        if (userId <= 0) {
            QMessageBox::warning(this, "提示", "请先登录");
            return;
        }

        QVariantMap profile;
        profile["skills"] = m_skillsText;
        profile["summary"] = m_summaryText;
        profile["photo_path"] = m_photoPath;

        if (DatabaseManager::getInstance().updateResumeProfile(userId, profile)) {
            QMessageBox::information(this, "保存成功", "简历资料已保存到数据库");
        } else {
            QMessageBox::critical(this, "保存失败", "数据库写入失败，请稍后再试");
        }
    });
}

void MainWindow::editSkills() {
    QDialog dialog(this);
    dialog.setWindowTitle("编辑技术能力");
    dialog.setMinimumWidth(480);
    dialog.setMinimumHeight(360);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 18);
    layout->setSpacing(14);

    auto *title = new QLabel("编辑技术能力");
    title->setStyleSheet("font-size:20px; font-weight:800; color:#0F172A;");
    layout->addWidget(title);

    auto *hint = new QLabel("每行填写一个技能，例如：C++ / Qt / SQLite / Git");
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#64748B; font-size:13px; font-weight:600;");
    layout->addWidget(hint);

    auto *edit = new QPlainTextEdit;
    edit->setPlaceholderText("每行一个技能...");
    edit->setPlainText(m_skillsText);
    edit->setStyleSheet(Theme::inputStyle());
    layout->addWidget(edit, 1);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText("保存");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        m_skillsText = edit->toPlainText().trimmed();
        if (skillsLbl) {
            if (m_skillsText.isEmpty())
                skillsLbl->setText("暂无内容，点击编辑按钮添加");
            else
                skillsLbl->setText(m_skillsText);
        }
    }
}

void MainWindow::editSummary() {
    QDialog dialog(this);
    dialog.setWindowTitle("编辑个人总结");
    dialog.setMinimumWidth(480);
    dialog.setMinimumHeight(420);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 18);
    layout->setSpacing(14);

    auto *title = new QLabel("编辑个人总结");
    title->setStyleSheet("font-size:20px; font-weight:800; color:#0F172A;");
    layout->addWidget(title);

    auto *hint = new QLabel("简要介绍自己的专业背景、核心优势和求职意向");
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#64748B; font-size:13px; font-weight:600;");
    layout->addWidget(hint);

    auto *edit = new QPlainTextEdit;
    edit->setPlaceholderText("请输入个人总结...");
    edit->setPlainText(m_summaryText);
    edit->setStyleSheet(Theme::inputStyle());
    layout->addWidget(edit, 1);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText("保存");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        m_summaryText = edit->toPlainText().trimmed();
        if (summaryLbl) {
            if (m_summaryText.isEmpty())
                summaryLbl->setText("暂无内容，点击编辑按钮添加");
            else
                summaryLbl->setText(m_summaryText);
        }
    }
}

void MainWindow::loadResumeProfile() {
    int userId = User::getInstance().getId();
    if (userId <= 0)
        return;

    QVariantMap profile =
        DatabaseManager::getInstance().getResumeProfile(userId);

    // 技术能力
    m_skillsText = profile.value("skills").toString();
    if (skillsLbl) {
        if (m_skillsText.isEmpty())
            skillsLbl->setText("暂无内容，点击编辑按钮添加");
        else
            skillsLbl->setText(m_skillsText);
    }

    // 个人总结
    m_summaryText = profile.value("summary").toString();
    if (summaryLbl) {
        if (m_summaryText.isEmpty())
            summaryLbl->setText("暂无内容，点击编辑按钮添加");
        else
            summaryLbl->setText(m_summaryText);
    }

    // 照片（圆形裁切）
    m_photoPath = profile.value("photo_path").toString();
    if (photoPreviewLbl) {
        if (!m_photoPath.isEmpty()) {
            QString fullPath =
                QCoreApplication::applicationDirPath() + "/" + m_photoPath;
            QPixmap source(fullPath);
            if (!source.isNull()) {
                int size = photoPreviewLbl->width();
                QPixmap scaled = source.scaled(size, size,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
                int x = (size - scaled.width()) / 2;
                int y = (size - scaled.height()) / 2;

                QPixmap circular(size, size);
                circular.fill(Qt::transparent);
                QPainter painter(&circular);
                painter.setRenderHint(QPainter::Antialiasing, true);
                QPainterPath path;
                path.addEllipse(0, 0, size, size);
                painter.setClipPath(path);
                painter.drawPixmap(x, y, scaled);
                painter.end();

                photoPreviewLbl->setPixmap(circular);
                photoPreviewLbl->setText(QString());
            } else {
                photoPreviewLbl->setText("照片文件丢失");
            }
        } else {
            photoPreviewLbl->setPixmap(QPixmap());
            photoPreviewLbl->setText("暂无照片");
        }
    }
}

void MainWindow::InitFrame() {
    ui->sidebarFrame->setFixedWidth(268);
    ui->userProfileWidget->setFixedHeight(204);
    ui->userNameLbl->setMinimumHeight(42);
    ui->detailLbl->setMinimumHeight(28);
    ui->majorLbl->setMinimumHeight(48);
    ui->userNameLbl->setWordWrap(false);
    ui->detailLbl->setWordWrap(false);
    ui->majorLbl->setWordWrap(true);

    if (editProfileBtn == nullptr) {
        editProfileBtn = new QPushButton("编辑资料", ui->userProfileWidget);
        editProfileBtn->setObjectName("editProfileBtn");
        editProfileBtn->setCursor(Qt::PointingHandCursor);
        editProfileBtn->setFlat(true);
        ui->verticalLayout->addWidget(editProfileBtn);
        connect(editProfileBtn, &QPushButton::clicked, this,
                &MainWindow::openEditProfileDialog);
    }

    ui->verticalLayout->setContentsMargins(20, 17, 20, 17);
    ui->verticalLayout->setSpacing(8);
    ui->verticalLayout_3->setContentsMargins(14, 18, 14, 18);
    ui->verticalLayout_3->setSpacing(18);
    ui->verticalLayout_2->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout_2->setSpacing(14);
    ui->verticalSpacer_2->changeSize(20, 0, QSizePolicy::Minimum,
                                     QSizePolicy::Fixed);

    const QList<QPushButton *> navButtonsForSize = {
        ui->navHomeBtn, ui->navCourseBtn, ui->navExpBtn, ui->navExportBtn};
    auto *profileShadow = new QGraphicsDropShadowEffect(ui->userProfileWidget);
    profileShadow->setBlurRadius(34);
    profileShadow->setOffset(0, 10);
    profileShadow->setColor(QColor(70, 95, 120, 18));
    ui->userProfileWidget->setGraphicsEffect(profileShadow);

    for (QPushButton *button : navButtonsForSize) {
        button->setFixedHeight(58);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setFlat(true);
        button->setAutoDefault(false);
        button->setDefault(false);
        button->setAttribute(Qt::WA_StyledBackground, true);

        auto *shadow = new QGraphicsDropShadowEffect(button);
        shadow->setBlurRadius(16);
        shadow->setOffset(0, 5);
        shadow->setColor(QColor(35, 50, 65, 14));
        button->setGraphicsEffect(shadow);
    }

    auto setActiveNav = [this](QPushButton *activeButton) {
        const QList<QPushButton *> navButtons = {
            ui->navHomeBtn, ui->navCourseBtn, ui->navExpBtn, ui->navExportBtn};
        for (QPushButton *button : navButtons) {
            button->setProperty("active", button == activeButton);
            button->style()->unpolish(button);
            button->style()->polish(button);
            button->update();
        }
    };

    ui->navHomeBtn->setText("01  首页总览");
    ui->navCourseBtn->setText("02  课程与成绩");
    ui->navExpBtn->setText("03  经历与荣誉");
    ui->navExportBtn->setText("04  简历导出");

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
        ui->currentPageLbl->setText("经历 & 荣誉");
        setActiveNav(ui->navExpBtn);
    });
    connect(ui->navExportBtn, &QPushButton::clicked, this, [=] {
        ui->stackedWidget->setCurrentIndex(3);
        ui->currentPageLbl->setText("简历导出");
        setActiveNav(ui->navExportBtn);
        loadResumeProfile();
    });

    // CSV 导入导出按钮信号
    connect(importCourseCsvBtn, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(
            this, "导入课程 CSV", QString(),
            "CSV 文件 (*.csv);;所有文件 (*)");
        if (!filePath.isEmpty())
            importCoursesFromCsv(filePath);
    });
    connect(exportCourseCsvBtn, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getSaveFileName(
            this, "导出课程 CSV",
            QString("courses_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd")),
            "CSV 文件 (*.csv);;所有文件 (*)");
        if (!filePath.isEmpty())
            exportCoursesToCsv(filePath);
    });
    connect(importExpCsvBtn, &QPushButton::clicked, this, [this]() {
        if (m_expTabActive) {
            QString filePath = QFileDialog::getOpenFileName(
                this, "导入经历 CSV", QString(),
                "CSV 文件 (*.csv);;所有文件 (*)");
            if (!filePath.isEmpty())
                importExperiencesFromCsv(filePath);
        } else {
            QString filePath = QFileDialog::getOpenFileName(
                this, "导入荣誉 CSV", QString(),
                "CSV 文件 (*.csv);;所有文件 (*)");
            if (!filePath.isEmpty())
                importAwardsFromCsv(filePath);
        }
    });
    connect(exportExpCsvBtn, &QPushButton::clicked, this, [this]() {
        if (m_expTabActive) {
            QString filePath = QFileDialog::getSaveFileName(
                this, "导出经历 CSV",
                QString("experiences_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd")),
                "CSV 文件 (*.csv);;所有文件 (*)");
            if (!filePath.isEmpty())
                exportExperiencesToCsv(filePath);
        } else {
            QString filePath = QFileDialog::getSaveFileName(
                this, "导出荣誉 CSV",
                QString("awards_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd")),
                "CSV 文件 (*.csv);;所有文件 (*)");
            if (!filePath.isEmpty())
                exportAwardsToCsv(filePath);
        }
    });

    // CSV 格式帮助按钮
    connect(csvHelpCourseBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, "CSV 格式说明 — 课程",
            QString("<h3>表头</h3>"
                    "<pre>课程名称,学分,成绩,学期</pre>"
                    "<h3>字段说明</h3>"
                    "<table>"
                    "<tr><td><b>课程名称</b></td><td>课程名字（必填）</td></tr>"
                    "<tr><td><b>学分</b></td><td>数值，必须 &gt; 0（必填）</td></tr>"
                    "<tr><td><b>成绩</b></td><td>数值，范围 0–100（必填）</td></tr>"
                    "<tr><td><b>学期</b></td><td>大一上 / 大一下 / 大二上 / 大二下<br>"
                    "大三上 / 大三下 / 大四上 / 大四下（必填）</td></tr>"
                    "</table>"
                    "<h3>示例</h3>"
                    "<pre>课程名称,学分,成绩,学期\n"
                    "高等数学,4,92,大一上\n"
                    "线性代数,3,85,大一下</pre>"
                    "<p style='color:#64748B'>导入时绩点会自动根据成绩换算，无需手动填写。</p>"));
    });

    connect(csvHelpExpBtn, &QPushButton::clicked, this, [this]() {
        if (m_expTabActive) {
            QMessageBox::information(this, "CSV 格式说明 — 经历",
                QString("<h3>表头</h3>"
                        "<pre>标题,类型,时间,描述</pre>"
                        "<h3>字段说明</h3>"
                        "<table>"
                        "<tr><td><b>标题</b></td><td>经历标题（必填）</td></tr>"
                        "<tr><td><b>类型</b></td><td>实习 / 竞赛 / 项目 / 其他（必填）</td></tr>"
                        "<tr><td><b>时间</b></td><td>日期，建议 yyyy-MM-dd</td></tr>"
                        "<tr><td><b>描述</b></td><td>详细描述内容</td></tr>"
                        "</table>"
                        "<h3>示例</h3>"
                        "<pre>标题,类型,时间,描述\n"
                        "数学建模竞赛,竞赛,2024-03-15,获得省级一等奖\n"
                        "暑期实习,实习,2024-07-01,在XX公司实习</pre>"));
        } else {
            QMessageBox::information(this, "CSV 格式说明 — 荣誉",
                QString("<h3>表头</h3>"
                        "<pre>奖项名称,荣誉级别,获奖时间,奖金金额</pre>"
                        "<h3>字段说明</h3>"
                        "<table>"
                        "<tr><td><b>奖项名称</b></td><td>荣誉名称（必填）</td></tr>"
                        "<tr><td><b>荣誉级别</b></td><td>国家级 / 省级 / 校级 / 院级（必填）</td></tr>"
                        "<tr><td><b>获奖时间</b></td><td>日期，建议 yyyy-MM-dd</td></tr>"
                        "<tr><td><b>奖金金额</b></td><td>整数数值，无则为 0</td></tr>"
                        "</table>"
                        "<h3>示例</h3>"
                        "<pre>奖项名称,荣誉级别,获奖时间,奖金金额\n"
                        "国家奖学金,国家级,2024-09-01,8000\n"
                        "优秀学生干部,校级,2024-06-15,500</pre>"));
        }
    });

    // 首页一键导入导出全部数据
    connect(homeImportAllBtn, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(
            this, "一键导入全部数据", QString(),
            "CSV 文件 (*.csv);;所有文件 (*)");
        if (!filePath.isEmpty())
            importAllFromCsv(filePath);
    });
    connect(homeExportAllBtn, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getSaveFileName(
            this, "一键导出全部数据",
            QString("college_data_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd")),
            "CSV 文件 (*.csv);;所有文件 (*)");
        if (!filePath.isEmpty())
            exportAllToCsv(filePath);
    });
    connect(homeCsvHelpBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, "CSV 格式说明 — 一键导入导出",
            QString("<h3>文件格式</h3>"
                    "<p>使用 <b>#SECTION: 名称</b> 标记不同数据区域，"
                    "每个区域包含自己的表头和数据行。</p>"
                    "<h3>完整示例</h3>"
                    "<pre style='line-height:1.6'>"
                    "#SECTION: 课程\n"
                    "课程名称,学分,成绩,学期\n"
                    "高等数学,4,92,大一上\n"
                    "线性代数,3,85,大一下\n"
                    "\n"
                    "#SECTION: 经历\n"
                    "标题,类型,时间,描述\n"
                    "数学建模竞赛,竞赛,2024-03-15,获得省级一等奖\n"
                    "暑期实习,实习,2024-07-01,在XX公司实习\n"
                    "\n"
                    "#SECTION: 荣誉\n"
                    "奖项名称,荣誉级别,获奖时间,奖金金额\n"
                    "国家奖学金,国家级,2024-09-01,8000\n"
                    "优秀学生干部,校级,2024-06-15,500\n"
                    "</pre>"
                    "<h3>字段说明</h3>"
                    "<table>"
                    "<tr><td colspan='2'><b>【课程】</b></td></tr>"
                    "<tr><td>课程名称</td><td>课程名字（必填）</td></tr>"
                    "<tr><td>学分</td><td>数值 &gt; 0（必填）</td></tr>"
                    "<tr><td>成绩</td><td>数值 0–100（必填）</td></tr>"
                    "<tr><td>学期</td><td>大一上/大一下/大二上/大二下/大三上/大三下/大四上/大四下（必填）</td></tr>"
                    "<tr><td colspan='2'><br><b>【经历】</b></td></tr>"
                    "<tr><td>标题</td><td>经历标题（必填）</td></tr>"
                    "<tr><td>类型</td><td>实习/竞赛/项目/其他（必填）</td></tr>"
                    "<tr><td>时间</td><td>日期，建议 yyyy-MM-dd</td></tr>"
                    "<tr><td>描述</td><td>详细描述内容</td></tr>"
                    "<tr><td colspan='2'><br><b>【荣誉】</b></td></tr>"
                    "<tr><td>奖项名称</td><td>荣誉名称（必填）</td></tr>"
                    "<tr><td>荣誉级别</td><td>国家级/省级/校级/院级（必填）</td></tr>"
                    "<tr><td>获奖时间</td><td>日期，建议 yyyy-MM-dd</td></tr>"
                    "<tr><td>奖金金额</td><td>整数数值，无则为 0</td></tr>"
                    "</table>"
                    "<p style='color:#64748B'>提示：空白行会被自动跳过，#SECTION: 不区分先后顺序。</p>"));
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
    if (!user.isLoggedIn())
        return;

    QDialog dialog(this);
    dialog.setWindowTitle("修改个人信息");
    dialog.setMinimumWidth(420);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 24, 26, 22);
    layout->setSpacing(16);

    auto *title = new QLabel("修改个人信息");
    title->setStyleSheet("font-size:22px; font-weight:900; color:#0F172A;");
    layout->addWidget(title);

    auto *hint = new QLabel("用户名暂不修改，学校、年级、性别和专业可以随时更新"
                            "。保存后左上角信息会立即刷新。");
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#64748B; font-size:13px; font-weight:700;");
    layout->addWidget(hint);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignTop);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(14);

    auto *gradeBox = new QComboBox(&dialog);
    gradeBox->addItems({"大一", "大二", "大三", "大四", "已毕业", "其他"});
    int gradeIndex = gradeBox->findText(user.getGrade());
    if (gradeIndex >= 0)
        gradeBox->setCurrentIndex(gradeIndex);

    auto *genderBox = new QComboBox(&dialog);
    genderBox->addItems({"男", "女", "其他", "不显示"});
    int genderIndex = genderBox->findText(user.getGender());
    if (genderIndex >= 0)
        genderBox->setCurrentIndex(genderIndex);

    auto *schoolEdit = new QLineEdit(user.getSchool(), &dialog);
    schoolEdit->setPlaceholderText("例如：对外经济贸易大学");

    auto *majorEdit = new QLineEdit(user.getMajor(), &dialog);
    majorEdit->setPlaceholderText("例如：数据科学与大数据技术");

    form->addRow("学校：", schoolEdit);
    form->addRow("年级：", gradeBox);
    form->addRow("性别：", genderBox);
    form->addRow("专业：", majorEdit);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Save)->setText("保存修改");
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    QString grade = gradeBox->currentText();
    QString gender = genderBox->currentText();
    if (gender == "不显示")
        gender.clear();
    QString major = majorEdit->text().trimmed();
    QString school = schoolEdit->text().trimmed();

    if (school.isEmpty() || major.isEmpty()) {
        QMessageBox::warning(this, "提示", "学校和专业不能为空");
        return;
    }

    if (DatabaseManager::getInstance().updateUserInfo(user.getId(), grade,
                                                      gender, major, school)) {
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
    courseModel->setEditStrategy(QSqlTableModel::OnRowChange);
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
    ui->courseTableView->setColumnHidden(7, true); // 隐藏 semester_order 排序列

    courseModel->select();

    // 双击编辑后自动重算绩点和学期排序值
    connect(courseModel, &QSqlTableModel::dataChanged, this,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
        const QStringList semesters = {"大一上","大一下","大二上","大二下",
                                        "大三上","大三下","大四上","大四下"};
        for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
            int col = topLeft.column();
            if (col <= 4 && 4 <= bottomRight.column()) {
                // 成绩列被修改，重算绩点
                double score = courseModel->data(
                    courseModel->index(row, 4)).toDouble();
                courseModel->setData(courseModel->index(row, 6),
                                     scoreToGpa(score));
            }
            if (col <= 5 && 5 <= bottomRight.column()) {
                // 学期列被修改，重算排序值
                QString sem = courseModel->data(
                    courseModel->index(row, 5)).toString();
                int order = semesters.indexOf(sem);
                if (order >= 0)
                    courseModel->setData(courseModel->index(row, 7), order);
            }
        }
        updateTotalStats();
        updateHomePageStats();
    });

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

        if (name.isEmpty())
            return;

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

    auto result =
        QMessageBox::question(this, "确认删除", "确定要删除该门课程吗？");
    if (result != QMessageBox::Yes)
        return;

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

    ui->labelStats->setText(
        QString("总课程: %1 | 算术平均: %2 | 加权 GPA: %3 | 总学分: %4")
            .arg(stats["count"].toInt())
            .arg(stats["avg"].toDouble(), 0, 'f', 2)
            .arg(stats["gpa"].toDouble(), 0, 'f', 2)
            .arg(stats["totalCredits"].toDouble(), 0, 'f', 1));
}

// ==================== 经历 & 荣誉页（合并） ====================

void MainWindow::InitExpPage() {
    int userId = User::getInstance().getId();

    // ---- 切换标签按钮 ----
    expTabBtn = new QPushButton("课外活动", ui->expPage);
    expTabBtn->setObjectName("expTabBtn");
    expTabBtn->setGeometry(28, 14, 130, 38);
    expTabBtn->setCursor(Qt::PointingHandCursor);

    awardTabBtn = new QPushButton("个人荣誉", ui->expPage);
    awardTabBtn->setObjectName("awardTabBtn");
    awardTabBtn->setGeometry(166, 14, 130, 38);
    awardTabBtn->setCursor(Qt::PointingHandCursor);

    // 标签切换样式
    const QString tabStyle = R"(
        QPushButton#expTabBtn, QPushButton#awardTabBtn {
            background: #FFFFFF;
            color: #64748B;
            border: 1px solid #D2D8E1;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 850;
            letter-spacing: 0.3px;
        }
        QPushButton#expTabBtn:hover, QPushButton#awardTabBtn:hover {
            border-color: #00B7C7;
            color: #111827;
        }
        QPushButton#expTabBtn[active="true"], QPushButton#awardTabBtn[active="true"] {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                        stop:0 #2563EB,
                        stop:0.56 #0EA5E9,
                        stop:1 #14B8A6);
            color: #FFFFFF;
            border: 1px solid rgba(255, 255, 255, 0.72);
            font-weight: 900;
        }
    )";
    expTabBtn->setStyleSheet(tabStyle);
    awardTabBtn->setStyleSheet(tabStyle);

    auto setActiveTab = [this](QPushButton *active) {
        for (QPushButton *btn : {expTabBtn, awardTabBtn}) {
            btn->setProperty("active", btn == active);
            btn->style()->unpolish(btn);
            btn->style()->polish(btn);
            btn->update();
        }
    };

    // ---- 课外活动模型与 UI ----
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
    expModel->setEditStrategy(QSqlTableModel::OnRowChange);
    expModel->setFilter(QString("user_id = %1").arg(userId));

    expModel->setHeaderData(2, Qt::Horizontal, "标题");
    expModel->setHeaderData(3, Qt::Horizontal, "类型");
    expModel->setHeaderData(4, Qt::Horizontal, "时间");
    expModel->setHeaderData(5, Qt::Horizontal, "描述");

    expModel->setSort(4, Qt::AscendingOrder);

    ui->expTableView->setModel(expModel);
    ui->expTableView->setColumnHidden(0, true);
    ui->expTableView->setColumnHidden(1, true);

    expModel->select();

    // ---- 个人荣誉模型与 UI ----
    ui->addAwardDateLine->setCalendarPopup(true);
    ui->addAwardDateLine->setDate(QDate::currentDate());
    ui->addAwardDateLine->setDisplayFormat("yyyy-MM-dd");

    ui->addAwardLine->setPlaceholderText("示例: 国家奖学金");

    // 奖金金额输入控件
    if (addAwardAmountLbl == nullptr) {
        addAwardAmountLbl = new QLabel("奖金金额", ui->addAwardFrame);
        addAwardAmountLbl->setObjectName("addAwardAmountLbl");
        addAwardAmountLbl->setGeometry(36, 158, 88, 36);
        addAwardAmountLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
    if (addAwardAmountSpin == nullptr) {
        addAwardAmountSpin = new QDoubleSpinBox(ui->addAwardFrame);
        addAwardAmountSpin->setObjectName("addAwardAmountSpin");
        addAwardAmountSpin->setGeometry(134, 158, 180, 36);
        addAwardAmountSpin->setPrefix("¥ ");
        addAwardAmountSpin->setRange(0.0, 999999.0);
        addAwardAmountSpin->setDecimals(0);
        addAwardAmountSpin->setSingleStep(500.0);
        addAwardAmountSpin->setValue(0.0);
        addAwardAmountSpin->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }

    ui->awardTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->awardTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    setupTableView(ui->awardTableView);

    awardModel = new QSqlTableModel(this);
    awardModel->setTable("awards");
    awardModel->setEditStrategy(QSqlTableModel::OnRowChange);
    awardModel->setFilter(QString("user_id = %1").arg(userId));

    awardModel->setHeaderData(2, Qt::Horizontal, "奖项名称");
    awardModel->setHeaderData(3, Qt::Horizontal, "荣誉级别");
    awardModel->setHeaderData(4, Qt::Horizontal, "获奖时间");
    awardModel->setHeaderData(5, Qt::Horizontal, "奖金金额");

    awardModel->setSort(4, Qt::AscendingOrder);

    ui->awardTableView->setModel(awardModel);
    ui->awardTableView->setColumnHidden(0, true);
    ui->awardTableView->setColumnHidden(1, true);

    awardModel->select();

    // ---- 切换逻辑：显示 / 隐藏对应控件 ----
    auto showExpTab = [this, setActiveTab]() {
        m_expTabActive = true;
        ui->expTableView->show();
        ui->addExpFrame->show();
        ui->addExpTitle->show();

        ui->awardTableView->hide();
        ui->addAwardFrame->hide();
        if (addAwardAmountLbl)
            addAwardAmountLbl->hide();
        if (addAwardAmountSpin)
            addAwardAmountSpin->hide();

        setActiveTab(expTabBtn);
    };

    auto showAwardTab = [this, setActiveTab]() {
        m_expTabActive = false;
        ui->expTableView->hide();
        ui->addExpFrame->hide();
        ui->addExpTitle->hide();

        ui->awardTableView->show();
        ui->addAwardFrame->show();
        if (addAwardAmountLbl)
            addAwardAmountLbl->show();
        if (addAwardAmountSpin)
            addAwardAmountSpin->show();

        setActiveTab(awardTabBtn);
    };

    connect(expTabBtn, &QPushButton::clicked, this, showExpTab);
    connect(awardTabBtn, &QPushButton::clicked, this, showAwardTab);

    // 默认显示课外活动标签
    showExpTab();
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
    auto result =
        QMessageBox::question(this, "确认删除", "确定要删除该经历吗?");
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

void MainWindow::on_addAwardBtn_clicked() {
    int userId = User::getInstance().getId();
    QString name = ui->addAwardLine->text();
    QString level = ui->addAwardLevelCBox->currentText();
    QDate temp_date = ui->addAwardDateLine->date();
    double amount = addAwardAmountSpin ? addAwardAmountSpin->value() : 0.0;

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
        awardModel->setData(awardModel->index(row, 5), amount);

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
    if (addAwardAmountSpin)
        addAwardAmountSpin->setValue(0.0);
}

void MainWindow::on_delAwardBtn_clicked() {
    QModelIndex currentIndex = ui->awardTableView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "提示", "请先在表格中点击选择一行内容");
        return;
    }
    auto result =
        QMessageBox::question(this, "确认删除", "确定要删除该荣誉吗?");
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

// ==================== CSV 导入导出辅助函数 ====================

static void writeCsvRow(QTextStream &stream, const QStringList &fields) {
    QStringList escaped;
    for (const QString &field : fields) {
        QString f = field;
        bool needsQuoting = f.contains(',') || f.contains('"') ||
                            f.contains('\n') || f.contains('\r');
        if (needsQuoting) {
            f.replace("\"", "\"\"");
            f = "\"" + f + "\"";
        }
        escaped.append(f);
    }
    stream << escaped.join(',') << "\n";
}

static QStringList parseCsvLine(const QString &line) {
    QStringList fields;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        QChar c = line.at(i);
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line.at(i + 1) == '"') {
                    field += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field += c;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.append(field.trimmed());
                field.clear();
            } else {
                field += c;
            }
        }
    }
    fields.append(field.trimmed());
    return fields;
}

// ==================== 课程 CSV 导入导出 ====================

void MainWindow::importCoursesFromCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导入失败",
                              "无法打开文件：" + file.errorString());
        return;
    }

    QTextStream stream(&file);
    // Qt6: QTextStream defaults to UTF-8, setCodec() removed

    if (stream.atEnd()) {
        QMessageBox::warning(this, "导入失败", "CSV 文件为空");
        return;
    }

    QString headerLine = stream.readLine();
    if (headerLine.startsWith(QChar(0xFEFF)))
        headerLine = headerLine.mid(1);

    QStringList expectedHeader = {"课程名称", "学分", "成绩", "学期"};
    QStringList header = parseCsvLine(headerLine);
    if (header.size() < 4 ||
        header[0] != expectedHeader[0] ||
        header[1] != expectedHeader[1] ||
        header[2] != expectedHeader[2] ||
        header[3] != expectedHeader[3]) {
        QMessageBox::warning(this, "格式错误",
            "CSV 表头不匹配。期望的列：\n课程名称,学分,成绩,学期\n\n"
            "请参考 CSV_FORMAT.md 了解正确格式。");
        return;
    }

    int userId = User::getInstance().getId();
    int successCount = 0;
    int failCount = 0;
    int lineNum = 1;
    const QStringList validSemesters = {"大一上","大一下","大二上","大二下",
                                        "大三上","大三下","大四上","大四下"};

    QSqlQuery query;
    query.prepare("INSERT INTO courses (user_id, name, credit, score, semester, gpa, semester_order) "
                  "VALUES (:uid, :name, :credit, :score, :semester, :gpa, :order)");

    while (!stream.atEnd()) {
        ++lineNum;
        QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;

        QStringList fields = parseCsvLine(line);
        if (fields.size() < 4) {
            ++failCount;
            continue;
        }

        QString name = fields[0];
        if (name.isEmpty()) { ++failCount; continue; }

        bool ok;
        double credit = fields[1].toDouble(&ok);
        if (!ok || credit <= 0) { ++failCount; continue; }

        double score = fields[2].toDouble(&ok);
        if (!ok || score < 0 || score > 100) { ++failCount; continue; }

        QString semester = fields[3];
        int semOrder = validSemesters.indexOf(semester);
        if (semOrder < 0) { ++failCount; continue; }

        query.bindValue(":uid", userId);
        query.bindValue(":name", name);
        query.bindValue(":credit", credit);
        query.bindValue(":score", score);
        query.bindValue(":semester", semester);
        query.bindValue(":gpa", scoreToGpa(score));
        query.bindValue(":order", semOrder);

        if (query.exec()) {
            ++successCount;
        } else {
            ++failCount;
        }
    }

    file.close();
    courseModel->select();
    updateTotalStats();
    updateHomePageStats();
    QMessageBox::information(this, "导入完成",
        QString("成功导入 %1 条，失败 %2 条").arg(successCount).arg(failCount));
}

void MainWindow::exportCoursesToCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败",
                              "无法写入文件：" + file.errorString());
        return;
    }

    QTextStream stream(&file);
    // Qt6: QTextStream defaults to UTF-8, setCodec() removed
    stream << QChar(0xFEFF);
    writeCsvRow(stream, {"课程名称", "学分", "成绩", "学期"});

    int rowCount = courseModel->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        QString name = courseModel->data(courseModel->index(row, 2)).toString();
        QString credit = courseModel->data(courseModel->index(row, 3)).toString();
        QString score = courseModel->data(courseModel->index(row, 4)).toString();
        QString semester = courseModel->data(courseModel->index(row, 5)).toString();
        writeCsvRow(stream, {name, credit, score, semester});
    }

    file.close();
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条课程数据").arg(rowCount));
}

// ==================== 经历 CSV 导入导出 ====================

void MainWindow::importExperiencesFromCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导入失败",
                              "无法打开文件：" + file.errorString());
        return;
    }

    QTextStream stream(&file);
    // Qt6: QTextStream defaults to UTF-8, setCodec() removed

    if (stream.atEnd()) {
        QMessageBox::warning(this, "导入失败", "CSV 文件为空");
        return;
    }

    QString headerLine = stream.readLine();
    if (headerLine.startsWith(QChar(0xFEFF)))
        headerLine = headerLine.mid(1);

    QStringList expectedHeader = {"标题", "类型", "时间", "描述"};
    QStringList header = parseCsvLine(headerLine);
    if (header.size() < 4 ||
        header[0] != expectedHeader[0] ||
        header[1] != expectedHeader[1] ||
        header[2] != expectedHeader[2] ||
        header[3] != expectedHeader[3]) {
        QMessageBox::warning(this, "格式错误",
            "CSV 表头不匹配。期望的列：\n标题,类型,时间,描述\n\n"
            "请参考 CSV_FORMAT.md 了解正确格式。");
        return;
    }

    int userId = User::getInstance().getId();
    const QStringList validTypes = {"实习", "竞赛", "项目", "其他"};
    int successCount = 0;
    int failCount = 0;
    int lineNum = 1;

    QSqlQuery query;
    query.prepare("INSERT INTO experiences (user_id, title, type, date, content) "
                  "VALUES (:uid, :title, :type, :date, :content)");

    while (!stream.atEnd()) {
        ++lineNum;
        QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;

        QStringList fields = parseCsvLine(line);
        if (fields.size() < 4) { ++failCount; continue; }

        QString title = fields[0];
        QString type = fields[1];
        QString date = fields[2];
        QString content = fields[3];

        if (title.isEmpty() || !validTypes.contains(type)) {
            ++failCount;
            continue;
        }

        query.bindValue(":uid", userId);
        query.bindValue(":title", title);
        query.bindValue(":type", type);
        query.bindValue(":date", date);
        query.bindValue(":content", content);

        if (query.exec()) {
            ++successCount;
        } else {
            ++failCount;
        }
    }

    file.close();
    expModel->select();
    updateHomePageStats();
    QMessageBox::information(this, "导入完成",
        QString("成功导入 %1 条，失败 %2 条").arg(successCount).arg(failCount));
}

void MainWindow::exportExperiencesToCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败",
                              "无法写入文件：" + file.errorString());
        return;
    }

    QTextStream stream(&file);
    // Qt6: QTextStream defaults to UTF-8, setCodec() removed
    stream << QChar(0xFEFF);
    writeCsvRow(stream, {"标题", "类型", "时间", "描述"});

    int rowCount = expModel->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        QString title   = expModel->data(expModel->index(row, 2)).toString();
        QString type    = expModel->data(expModel->index(row, 3)).toString();
        QString date    = expModel->data(expModel->index(row, 4)).toString();
        QString content = expModel->data(expModel->index(row, 5)).toString();
        writeCsvRow(stream, {title, type, date, content});
    }

    file.close();
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条经历数据").arg(rowCount));
}

// ==================== 荣誉 CSV 导入导出 ====================

void MainWindow::importAwardsFromCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导入失败",
                              "无法打开文件：" + file.errorString());
        return;
    }

    QTextStream stream(&file);
    // Qt6: QTextStream defaults to UTF-8, setCodec() removed

    if (stream.atEnd()) {
        QMessageBox::warning(this, "导入失败", "CSV 文件为空");
        return;
    }

    QString headerLine = stream.readLine();
    if (headerLine.startsWith(QChar(0xFEFF)))
        headerLine = headerLine.mid(1);

    QStringList expectedHeader = {"奖项名称", "荣誉级别", "获奖时间", "奖金金额"};
    QStringList header = parseCsvLine(headerLine);
    if (header.size() < 4 ||
        header[0] != expectedHeader[0] ||
        header[1] != expectedHeader[1] ||
        header[2] != expectedHeader[2] ||
        header[3] != expectedHeader[3]) {
        QMessageBox::warning(this, "格式错误",
            "CSV 表头不匹配。期望的列：\n奖项名称,荣誉级别,获奖时间,奖金金额\n\n"
            "请参考 CSV_FORMAT.md 了解正确格式。");
        return;
    }

    int userId = User::getInstance().getId();
    const QStringList validLevels = {"国家级", "省级", "校级", "院级"};
    int successCount = 0;
    int failCount = 0;
    int lineNum = 1;

    QSqlQuery query;
    query.prepare("INSERT INTO awards (user_id, name, level, date, amount) "
                  "VALUES (:uid, :name, :level, :date, :amount)");

    while (!stream.atEnd()) {
        ++lineNum;
        QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;

        QStringList fields = parseCsvLine(line);
        if (fields.size() < 4) { ++failCount; continue; }

        QString name = fields[0];
        QString level = fields[1];
        QString date = fields[2];
        bool ok;
        double amount = fields[3].toDouble(&ok);
        if (!ok) amount = 0.0;

        if (name.isEmpty() || !validLevels.contains(level)) {
            ++failCount;
            continue;
        }

        query.bindValue(":uid", userId);
        query.bindValue(":name", name);
        query.bindValue(":level", level);
        query.bindValue(":date", date);
        query.bindValue(":amount", amount);

        if (query.exec()) {
            ++successCount;
        } else {
            ++failCount;
        }
    }

    file.close();
    awardModel->select();
    updateHomePageStats();
    QMessageBox::information(this, "导入完成",
        QString("成功导入 %1 条，失败 %2 条").arg(successCount).arg(failCount));
}

void MainWindow::exportAwardsToCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败",
                              "无法写入文件：" + file.errorString());
        return;
    }

    QTextStream stream(&file);
    // Qt6: QTextStream defaults to UTF-8, setCodec() removed
    stream << QChar(0xFEFF);
    writeCsvRow(stream, {"奖项名称", "荣誉级别", "获奖时间", "奖金金额"});

    int rowCount = awardModel->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        QString name   = awardModel->data(awardModel->index(row, 2)).toString();
        QString level  = awardModel->data(awardModel->index(row, 3)).toString();
        QString date   = awardModel->data(awardModel->index(row, 4)).toString();
        QString amount = awardModel->data(awardModel->index(row, 5)).toString();
        writeCsvRow(stream, {name, level, date, amount});
    }

    file.close();
    QMessageBox::information(this, "导出成功",
        QString("已导出 %1 条荣誉数据").arg(rowCount));
}

// ==================== 一键导入导出全部数据 ====================

void MainWindow::exportAllToCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败",
                              "无法写入文件：" + file.errorString());
        return;
    }

    QTextStream stream(&file);
    // Qt6: QTextStream defaults to UTF-8, setCodec() removed
    stream << QChar(0xFEFF);

    int courseCount = courseModel->rowCount();
    int expCount = expModel->rowCount();
    int awardCount = awardModel->rowCount();

    // Section 1: 课程
    writeCsvRow(stream, {"#SECTION: 课程"});
    writeCsvRow(stream, {"课程名称", "学分", "成绩", "学期"});
    for (int row = 0; row < courseCount; ++row) {
        QString name     = courseModel->data(courseModel->index(row, 2)).toString();
        QString credit   = courseModel->data(courseModel->index(row, 3)).toString();
        QString score    = courseModel->data(courseModel->index(row, 4)).toString();
        QString semester = courseModel->data(courseModel->index(row, 5)).toString();
        writeCsvRow(stream, {name, credit, score, semester});
    }
    stream << "\n";

    // Section 2: 经历
    writeCsvRow(stream, {"#SECTION: 经历"});
    writeCsvRow(stream, {"标题", "类型", "时间", "描述"});
    for (int row = 0; row < expCount; ++row) {
        QString title   = expModel->data(expModel->index(row, 2)).toString();
        QString type    = expModel->data(expModel->index(row, 3)).toString();
        QString date    = expModel->data(expModel->index(row, 4)).toString();
        QString content = expModel->data(expModel->index(row, 5)).toString();
        writeCsvRow(stream, {title, type, date, content});
    }
    stream << "\n";

    // Section 3: 荣誉
    writeCsvRow(stream, {"#SECTION: 荣誉"});
    writeCsvRow(stream, {"奖项名称", "荣誉级别", "获奖时间", "奖金金额"});
    for (int row = 0; row < awardCount; ++row) {
        QString name   = awardModel->data(awardModel->index(row, 2)).toString();
        QString level  = awardModel->data(awardModel->index(row, 3)).toString();
        QString date   = awardModel->data(awardModel->index(row, 4)).toString();
        QString amount = awardModel->data(awardModel->index(row, 5)).toString();
        writeCsvRow(stream, {name, level, date, amount});
    }

    file.close();
    QMessageBox::information(this, "导出成功",
        QString("已导出全部数据：\n课程 %1 条 | 经历 %2 条 | 荣誉 %3 条")
            .arg(courseCount).arg(expCount).arg(awardCount));
}

void MainWindow::importAllFromCsv(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导入失败",
                              "无法打开文件：" + file.errorString());
        return;
    }

    QTextStream stream(&file);
    // Qt6: QTextStream defaults to UTF-8, setCodec() removed

    if (stream.atEnd()) {
        QMessageBox::warning(this, "导入失败", "CSV 文件为空");
        return;
    }

    enum Section { NONE, COURSES, EXPERIENCES, AWARDS };
    Section currentSection = NONE;
    bool headerValidated = false;
    bool headerBad = false;

    int courseOk = 0, courseFail = 0;
    int expOk = 0, expFail = 0;
    int awardOk = 0, awardFail = 0;

    int userId = User::getInstance().getId();

    const QStringList validSemesters = {"大一上","大一下","大二上","大二下",
                                        "大三上","大三下","大四上","大四下"};
    const QStringList validTypes = {"实习", "竞赛", "项目", "其他"};
    const QStringList validLevels = {"国家级", "省级", "校级", "院级"};

    const QStringList expectedCourseHdr = {"课程名称", "学分", "成绩", "学期"};
    const QStringList expectedExpHdr = {"标题", "类型", "时间", "描述"};
    const QStringList expectedAwardHdr = {"奖项名称", "荣誉级别", "获奖时间", "奖金金额"};

    // 预编译三条 INSERT 语句
    QSqlQuery courseQuery;
    courseQuery.prepare("INSERT INTO courses (user_id, name, credit, score, semester, gpa, semester_order) "
                        "VALUES (:uid, :name, :credit, :score, :semester, :gpa, :order)");

    QSqlQuery expQuery;
    expQuery.prepare("INSERT INTO experiences (user_id, title, type, date, content) "
                     "VALUES (:uid, :title, :type, :date, :content)");

    QSqlQuery awardQuery;
    awardQuery.prepare("INSERT INTO awards (user_id, name, level, date, amount) "
                       "VALUES (:uid, :name, :level, :date, :amount)");

    bool firstRead = true;
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (firstRead) {
            if (line.startsWith(QChar(0xFEFF)))
                line = line.mid(1);
            firstRead = false;
        }
        line = line.trimmed();
        if (line.isEmpty())
            continue;

        // 检测分段标记
        if (line.startsWith("#SECTION:")) {
            QString sec = line.mid(9).trimmed();
            if (sec == "课程")
                currentSection = COURSES;
            else if (sec == "经历")
                currentSection = EXPERIENCES;
            else if (sec == "荣誉")
                currentSection = AWARDS;
            else
                currentSection = NONE;
            headerValidated = false;
            headerBad = false;
            continue;
        }

        // 表头验证
        if (!headerValidated) {
            QStringList hdr = parseCsvLine(line);
            switch (currentSection) {
            case COURSES:
                headerBad = !(hdr.size() >= 4 &&
                              hdr[0] == expectedCourseHdr[0] &&
                              hdr[1] == expectedCourseHdr[1] &&
                              hdr[2] == expectedCourseHdr[2] &&
                              hdr[3] == expectedCourseHdr[3]);
                break;
            case EXPERIENCES:
                headerBad = !(hdr.size() >= 4 &&
                              hdr[0] == expectedExpHdr[0] &&
                              hdr[1] == expectedExpHdr[1] &&
                              hdr[2] == expectedExpHdr[2] &&
                              hdr[3] == expectedExpHdr[3]);
                break;
            case AWARDS:
                headerBad = !(hdr.size() >= 4 &&
                              hdr[0] == expectedAwardHdr[0] &&
                              hdr[1] == expectedAwardHdr[1] &&
                              hdr[2] == expectedAwardHdr[2] &&
                              hdr[3] == expectedAwardHdr[3]);
                break;
            default:
                headerBad = true;
                break;
            }
            headerValidated = true;
            continue;
        }

        // 表头不正确则跳过该段所有数据行
        if (headerBad)
            continue;

        // 数据行处理
        QStringList fields = parseCsvLine(line);

        switch (currentSection) {
        case COURSES: {
            if (fields.size() < 4) { ++courseFail; continue; }
            QString name = fields[0];
            if (name.isEmpty()) { ++courseFail; continue; }
            bool ok;
            double credit = fields[1].toDouble(&ok);
            if (!ok || credit <= 0) { ++courseFail; continue; }
            double score = fields[2].toDouble(&ok);
            if (!ok || score < 0 || score > 100) { ++courseFail; continue; }
            QString semester = fields[3];
            int semOrder = validSemesters.indexOf(semester);
            if (semOrder < 0) { ++courseFail; continue; }

            courseQuery.bindValue(":uid", userId);
            courseQuery.bindValue(":name", name);
            courseQuery.bindValue(":credit", credit);
            courseQuery.bindValue(":score", score);
            courseQuery.bindValue(":semester", semester);
            courseQuery.bindValue(":gpa", DatabaseManager::scoreToGpa(score));
            courseQuery.bindValue(":order", semOrder);

            if (courseQuery.exec()) ++courseOk; else ++courseFail;
            break;
        }
        case EXPERIENCES: {
            if (fields.size() < 4) { ++expFail; continue; }
            QString title = fields[0];
            QString type = fields[1];
            if (title.isEmpty() || !validTypes.contains(type)) {
                ++expFail; continue;
            }
            expQuery.bindValue(":uid", userId);
            expQuery.bindValue(":title", title);
            expQuery.bindValue(":type", type);
            expQuery.bindValue(":date", fields[2]);
            expQuery.bindValue(":content", fields[3]);

            if (expQuery.exec()) ++expOk; else ++expFail;
            break;
        }
        case AWARDS: {
            if (fields.size() < 4) { ++awardFail; continue; }
            QString name = fields[0];
            QString level = fields[1];
            if (name.isEmpty() || !validLevels.contains(level)) {
                ++awardFail; continue;
            }
            bool ok;
            double amount = fields[3].toDouble(&ok);
            if (!ok) amount = 0.0;

            awardQuery.bindValue(":uid", userId);
            awardQuery.bindValue(":name", name);
            awardQuery.bindValue(":level", level);
            awardQuery.bindValue(":date", fields[2]);
            awardQuery.bindValue(":amount", amount);

            if (awardQuery.exec()) ++awardOk; else ++awardFail;
            break;
        }
        default:
            break;
        }
    }

    file.close();

    // 刷新所有模型
    courseModel->select();
    expModel->select();
    awardModel->select();
    updateTotalStats();
    updateHomePageStats();

    QMessageBox::information(this, "导入完成",
        QString("导入结果：\n"
                "课程：成功 %1 条，失败 %2 条\n"
                "经历：成功 %3 条，失败 %4 条\n"
                "荣誉：成功 %5 条，失败 %6 条")
            .arg(courseOk).arg(courseFail)
            .arg(expOk).arg(expFail)
            .arg(awardOk).arg(awardFail));
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

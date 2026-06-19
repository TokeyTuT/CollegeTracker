#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "AddCourseDialog.h"
#include "DatabaseMannager.h"
#include "ResumeExporter.h"
#include "Theme.h"
#include "User.h"

#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QCursor>
#include <QDate>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
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
#include <QKeyEvent>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSlider>
#include <QSpacerItem>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVector>
#include <QtMath>

namespace {

constexpr qreal kHiResScale = 4.0;

QPixmap circularHiResPixmap(const QPixmap &source, int logicalSize) {
    if (source.isNull() || logicalSize <= 0)
        return {};

    const int pixelSize = qCeil(logicalSize * kHiResScale);
    const QPixmap scaled = source.scaled(
        pixelSize, pixelSize, Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation);
    const int x = (scaled.width() - pixelSize) / 2;
    const int y = (scaled.height() - pixelSize) / 2;

    QPixmap circular(pixelSize, pixelSize);
    circular.fill(Qt::transparent);
    QPainter painter(&circular);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    QPainterPath path;
    path.addEllipse(2, 2, pixelSize - 4, pixelSize - 4);
    painter.setClipPath(path);
    painter.drawPixmap(-x, -y, scaled);
    painter.end();
    circular.setDevicePixelRatio(kHiResScale);
    return circular;
}

class CoreCourseDelegate final : public QStyledItemDelegate {
public:
    explicit CoreCourseDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    QString displayText(const QVariant &value,
                        const QLocale &locale) const override {
        Q_UNUSED(locale);
        return value.toBool() ? QStringLiteral("是") : QStringLiteral("否");
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
                          const QModelIndex &) const override {
        auto *editor = new QComboBox(parent);
        editor->addItems({QStringLiteral("否"), QStringLiteral("是")});
        return editor;
    }

    void setEditorData(QWidget *editor,
                       const QModelIndex &index) const override {
        auto *combo = qobject_cast<QComboBox *>(editor);
        if (combo)
            combo->setCurrentIndex(index.data(Qt::EditRole).toBool() ? 1 : 0);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override {
        auto *combo = qobject_cast<QComboBox *>(editor);
        if (combo)
            model->setData(index, combo->currentIndex() == 1 ? 1 : 0,
                           Qt::EditRole);
    }
};

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    qApp->installEventFilter(this);
    resumeExporter = new ResumeExporter(this);
    applyModernStyle();
    buildHomePage();
    buildExportPage();
    loadResumeProfile();

    InitFrame();
    updateSidebarUserInfo();
    InitCoursePage();
    InitExpPage();
    updateHomePageStats();
}

MainWindow::~MainWindow() {
    qApp->removeEventFilter(this);
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

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (resumePreviewOverlay) {
        resumePreviewOverlay->setGeometry(ui->centralwidget->rect());
        if (resumePreviewOverlay->isVisible())
            updateResumeTemplateMagnifier();
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    Q_UNUSED(watched);

    const bool canPreview =
        ui && ui->stackedWidget->currentIndex() == 3 &&
        QApplication::activeModalWidget() == nullptr &&
        (QApplication::activeWindow() == this ||
         (resumePreviewOverlay && resumePreviewOverlay->isVisible()));

    if (canPreview && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Space) {
            if (!keyEvent->isAutoRepeat()) {
                if (resumePreviewOverlay &&
                    resumePreviewOverlay->isVisible()) {
                    hideResumeTemplateMagnifier();
                }
                else
                    showResumeTemplateMagnifier();
            }
            // 防止空格同时触发当前获得焦点的按钮。
            return true;
        }
    }

    if (canPreview && event->type() == QEvent::KeyRelease) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Space)
            return true;
    }

    if (event->type() == QEvent::ApplicationDeactivate ||
        event->type() == QEvent::WindowDeactivate) {
        hideResumeTemplateMagnifier();
    }

    return QMainWindow::eventFilter(watched, event);
}

#if 0
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

    resetCourseBtn = new QPushButton("初始化课程", ui->coursePage);
    resetCourseBtn->setObjectName("resetCourseBtn");
    resetCourseBtn->setGeometry(350, 38, 130, 40);
    resetCourseBtn->setCursor(Qt::PointingHandCursor);
    resetCourseBtn->setToolTip("清空当前用户的全部课程记录");

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
        QPushButton#logoutBtn {
            min-height: 40px; max-height: 42px;
            border-radius: %11px;
            border: 1px solid rgba(220,38,38,0.38);
            background: rgba(255,255,255,0.72);
            color: #B91C1C;
            font-size: %13px;
            font-weight: %9;
            padding: 0 16px;
        }
        QPushButton#logoutBtn:hover {
            background: #DC2626;
            border-color: #DC2626;
            color: #FFFFFF;
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
            font-size: 14px;
            font-weight: %9;
            border-radius: 10px;
        }
        QPushButton#homeImportAllBtn {
            background: transparent; color: %7; border: 2px solid %7;
        }
        QPushButton#homeImportAllBtn:hover {
            background: %7; color: #FFFFFF;
        }
        QPushButton#homeExportAllBtn {
            background: %7; color: #FFFFFF; border: none;
        }
        QPushButton#homeExportAllBtn:hover {
            background: #0B5E57;
        }

        /* 删除按钮 */
        QPushButton#deleteCourseBtn, QPushButton#resetCourseBtn,
        QPushButton#DelExpBtn, QPushButton#delAwardBtn {
            background: %20; color: %22; border: 1px solid %22;
        }
        QPushButton#deleteCourseBtn:hover, QPushButton#resetCourseBtn:hover,
        QPushButton#DelExpBtn:hover, QPushButton#delAwardBtn:hover {
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
        QFrame#homeChartCard {
            background: #FFFFFF;
            border: none;
            border-radius: 16px;
        }
        QFrame#homeStatCard {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #F0FDFA, stop:1 #ECFEFF);
            border: none;
            border-radius: 12px;
        }
        QFrame#homeChartCard { border-left: 5px solid %19; }
        QLabel#homeCardTitle {
            color: %2; font-size: %36px; font-weight: %6;
            background: transparent;
        }
        QLabel#homeChartLabel { background: transparent; border: none; }
        QLabel#homeStatTitle {
            color: #0F766E; font-size: 12px; font-weight: %14;
            background: transparent;
        }
        QLabel#homeStatValue {
            color: #0D9488; font-size: 26px; font-weight: %6;
            background: transparent;
        }
        QLabel#homeStatSub {
            color: #0F766E; font-size: 11px; font-weight: %14;
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

        /* ===== 首页底部操作栏卡片 ===== */
        QFrame#homeActionBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #F0FDFA, stop:1 #ECFEFF);
            border: none;
            border-radius: 12px;
        }
        QLabel#homeActionTitle {
            color: #0F766E; font-size: 14px; font-weight: 700;
            background: transparent;
        }
        QLabel#homeActionSub {
            color: #0F766E; font-size: 11px; font-weight: 400;
            background: transparent;
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

}
#endif

void MainWindow::applyModernStyle() {
    setWindowTitle("College Tracker");
    resize(1240, 780);
    setMinimumSize(1080, 690);

    ui->menubar->hide();
    ui->statusbar->hide();
    ui->mainContentFrame->setFrameShape(QFrame::NoFrame);
    ui->sidebarFrame->setFrameShape(QFrame::NoFrame);

    importCourseCsvBtn = new QPushButton("导入 CSV", ui->coursePage);
    importCourseCsvBtn->setObjectName("importCourseCsvBtn");
    exportCourseCsvBtn = new QPushButton("导出 CSV", ui->coursePage);
    exportCourseCsvBtn->setObjectName("exportCourseCsvBtn");
    csvHelpCourseBtn = new QPushButton("?", ui->coursePage);
    csvHelpCourseBtn->setObjectName("csvHelpCourseBtn");
    csvHelpCourseBtn->setToolTip("查看课程 CSV 格式说明");
    resetCourseBtn = new QPushButton("清空课程", ui->coursePage);
    resetCourseBtn->setObjectName("resetCourseBtn");
    resetCourseBtn->setToolTip("清空当前用户的全部课程记录");

    importExpCsvBtn = new QPushButton("导入 CSV", ui->expPage);
    importExpCsvBtn->setObjectName("importExpCsvBtn");
    exportExpCsvBtn = new QPushButton("导出 CSV", ui->expPage);
    exportExpCsvBtn->setObjectName("exportExpCsvBtn");
    csvHelpExpBtn = new QPushButton("?", ui->expPage);
    csvHelpExpBtn->setObjectName("csvHelpExpBtn");
    csvHelpExpBtn->setToolTip("查看 CSV 格式说明");

    for (QPushButton *button : {importCourseCsvBtn, exportCourseCsvBtn,
                                importExpCsvBtn, exportExpCsvBtn})
        button->setProperty("variant", "secondary");
    for (QPushButton *button : {csvHelpCourseBtn, csvHelpExpBtn}) {
        button->setProperty("variant", "tool");
        button->setFixedSize(40, 40);
    }

    resetCourseBtn->setProperty("variant", "quietDanger");
    ui->addCourseBtn->setProperty("variant", "primary");
    ui->addExpBtn->setProperty("variant", "primary");
    ui->addAwardBtn->setProperty("variant", "primary");
    ui->deleteCourseBtn->setProperty("variant", "danger");
    ui->DelExpBtn->setProperty("variant", "danger");
    ui->delAwardBtn->setProperty("variant", "danger");

    ui->courseToolbarLayout->insertWidget(0, importCourseCsvBtn);
    ui->courseToolbarLayout->insertWidget(1, exportCourseCsvBtn);
    ui->courseToolbarLayout->insertWidget(2, csvHelpCourseBtn);
    ui->courseToolbarLayout->insertWidget(3, resetCourseBtn);

    ui->expToolbarLayout->addStretch(1);
    ui->expToolbarLayout->addWidget(importExpCsvBtn);
    ui->expToolbarLayout->addWidget(exportExpCsvBtn);
    ui->expToolbarLayout->addWidget(csvHelpExpBtn);

    setStyleSheet(QStringLiteral(R"QSS(
        * {
            font-family: "Avenir Next", "PingFang SC";
            font-size: 14px;
            color: #17201D;
        }
        QMainWindow, QWidget#centralwidget { background: #F4F1EA; }
        QFrame#sidebarFrame { background: #315C53; border: none; }
        QFrame#mainContentFrame { background: #F4F1EA; border: none; }
        QStackedWidget, QWidget#homePage, QWidget#coursePage,
        QWidget#expPage, QWidget#profilePage { background: transparent; }

        QWidget#brandWidget { background: transparent; }
        QLabel#brandMarkLbl {
            background: #D97745; color: #FFF9F1; border-radius: 12px;
            font-size: 15px; font-weight: 900; letter-spacing: 1px;
        }
        QLabel#brandTitleLbl {
            color: #F8F2E8; font-size: 16px; font-weight: 900;
            letter-spacing: 2px;
        }
        QLabel#brandCaptionLbl {
            color: #9FB6AE; font-size: 10px; font-weight: 600;
            letter-spacing: 0.6px;
        }

        QWidget#userProfileWidget {
            background: #3B675E; border: 1px solid #537B72;
            border-radius: 14px;
        }
        QLabel#userNameLbl {
            color: #FFF9F1; font-size: 19px; font-weight: 800;
        }
        QLabel#detailLbl {
            color: #17352F; font-size: 11px; font-weight: 800;
            background: #BFD6CD; border-radius: 8px; padding: 4px 9px;
        }
        QLabel#majorLbl {
            color: #BFD0CA; font-size: 12px; font-weight: 500;
        }
        QLabel#sidebarAvatarLbl {
            background: #E5EEE9; color: #315C53;
            border: 2px solid #73968D; border-radius: 30px;
            font-size: 21px; font-weight: 850;
        }
        QPushButton#editProfileBtn {
            min-height: 30px; max-height: 30px;
            border: 1px solid #6C9087; border-radius: 8px;
            background: transparent; color: #E5EEE9;
            font-size: 12px; font-weight: 700; padding: 0 10px;
        }
        QPushButton#editProfileBtn:hover {
            background: #4A746B; border-color: #92ADA6;
        }
        QPushButton#logoutBtn {
            min-height: 40px; border: 1px solid #62877E;
            border-radius: 10px; background: transparent;
            color: #E1EBE7; font-size: 12px; font-weight: 700;
        }
        QPushButton#logoutBtn:hover {
            background: #B94B45; border-color: #B94B45; color: #FFF;
        }

        QLabel#currentPageLbl {
            color: #17201D; font-size: 27px; font-weight: 800;
            letter-spacing: -0.2px;
        }
        QLabel#headerSubtitleLbl {
            color: #707873; font-size: 12px; font-weight: 500;
        }

        #sidebarFrame QPushButton {
            min-height: 48px; max-height: 48px; border: none;
            border-radius: 10px; padding: 0 14px; text-align: left;
            color: #AFC4BC; background: transparent;
            font-size: 13px; font-weight: 650; letter-spacing: 0.4px;
        }
        #sidebarFrame QPushButton:hover {
            background: #416B62; color: #FFF9F1;
        }
        #sidebarFrame QPushButton[active="true"] {
            background: #F2EBDD; color: #17352F;
            border-left: 4px solid #D97745; padding-left: 12px;
            font-weight: 800;
        }

        QPushButton {
            min-height: 40px; border: 1px solid #1F6B5B;
            border-radius: 9px; padding: 0 17px; color: #FFF;
            background: #1F6B5B; font-weight: 700;
        }
        QPushButton:hover { background: #174F44; border-color: #174F44; }
        QPushButton:pressed { background: #113F36; }
        QPushButton:disabled {
            background: #D2CEC4; border-color: #D2CEC4; color: #8C938F;
        }
        QPushButton[variant="secondary"] {
            background: #FFFEFA; color: #38564F; border: 1px solid #D6D0C4;
        }
        QPushButton[variant="secondary"]:hover {
            background: #ECE8DF; border-color: #9DABA5;
        }
        QPushButton[variant="danger"] {
            background: #FFFEFA; color: #A8443F; border: 1px solid #E4C5BF;
        }
        QPushButton[variant="danger"]:hover {
            background: #B94B45; border-color: #B94B45; color: #FFF;
        }
        QPushButton[variant="quietDanger"] {
            background: transparent; color: #8E5B55; border: none;
            padding: 0 10px;
        }
        QPushButton[variant="quietDanger"]:hover {
            background: #F8E7E4; color: #A8443F;
        }
        QPushButton[variant="tool"], QPushButton#homeCsvHelpBtn {
            min-width: 40px; max-width: 40px; padding: 0;
            background: #FFFEFA; color: #59635F; border: 1px solid #D6D0C4;
            border-radius: 9px; font-size: 16px;
        }
        QPushButton[variant="tool"]:hover, QPushButton#homeCsvHelpBtn:hover {
            background: #F7E4D8; color: #A9502C; border-color: #E2A17E;
        }

        QLabel { color: #59635F; background: transparent; font-weight: 600; }
        QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox, QPlainTextEdit {
            min-height: 40px; border: 1px solid #D6D0C4;
            border-radius: 9px; padding: 0 12px; background: #FFFEFA;
            color: #17201D; selection-background-color: #BFD6CD;
            selection-color: #17201D;
        }
        QPlainTextEdit { padding: 10px 12px; }
        QLineEdit:hover, QComboBox:hover, QDateEdit:hover,
        QDoubleSpinBox:hover, QPlainTextEdit:hover { border-color: #9DABA5; }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus,
        QDoubleSpinBox:focus, QPlainTextEdit:focus {
            border: 2px solid #1F6B5B;
        }
        QComboBox::drop-down, QDateEdit::drop-down { border: none; width: 30px; }
        QComboBox QAbstractItemView {
            background: #FFFEFA; border: 1px solid #D6D0C4;
            selection-background-color: #DDEBE6;
            selection-color: #17201D; outline: none;
        }

        QFrame#addExpFrame, QFrame#addAwardFrame {
            background: #FFFEFA; border: 1px solid #DED8CC;
            border-radius: 13px;
        }
        QLabel#addExpTitle, QLabel#addAwardTitle {
            color: #25332F; font-size: 17px; font-weight: 800;
        }
        QTableView {
            background: #FFFEFA; alternate-background-color: #FAF8F3;
            border: 1px solid #DED8CC; border-radius: 12px;
            gridline-color: transparent; selection-background-color: #DDEBE6;
            selection-color: #17201D; outline: none;
        }
        QTableView::item {
            padding: 9px; border-bottom: 1px solid #EEEAE2;
        }
        QHeaderView::section {
            background: #E7E2D8; color: #46524E; border: none;
            border-right: 1px solid #D6D0C4; padding: 11px 9px;
            font-size: 12px; font-weight: 800;
        }
        QTableCornerButton::section { background: #E7E2D8; border: none; }
        QScrollBar:vertical {
            background: transparent; width: 10px; margin: 4px 2px;
        }
        QScrollBar::handle:vertical {
            background: #B7B1A6; border-radius: 4px; min-height: 28px;
        }
        QScrollBar::handle:vertical:hover { background: #7E8C86; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }

        QFrame#homeChartCard {
            background: #FFFEFA; border: 1px solid #DED8CC;
            border-radius: 14px;
        }
        QFrame#homeStatCard {
            background: #EBE7DE; border: none; border-radius: 11px;
        }
        QLabel#homeCardTitle {
            color: #25332F; font-size: 18px; font-weight: 800;
        }
        QLabel#homeChartLabel { background: transparent; border: none; }
        QLabel#homeStatTitle {
            color: #68716D; font-size: 11px; font-weight: 700;
        }
        QLabel#homeStatValue {
            color: #1F6B5B; font-size: 25px; font-weight: 800;
        }
        QLabel#homeStatSub {
            color: #8A918D; font-size: 10px; font-weight: 600;
        }
        QLabel#labelStats {
            background: #E9E4DA; border: none; border-radius: 11px;
            color: #38564F; font-size: 13px; font-weight: 750;
        }
        QFrame#homeActionBar {
            background: #203F38; border: none; border-radius: 12px;
        }
        QLabel#homeActionTitle {
            color: #FFF9F1; font-size: 13px; font-weight: 800;
        }
        QLabel#homeActionSub {
            color: #AEC2BA; font-size: 10px; font-weight: 500;
        }
        QPushButton#homeImportAllBtn {
            background: transparent; color: #E8F0EC; border: 1px solid #66837A;
        }
        QPushButton#homeImportAllBtn:hover { background: #31554C; }
        QPushButton#homeExportAllBtn {
            background: #D97745; color: #FFF; border-color: #D97745;
        }
        QPushButton#homeExportAllBtn:hover {
            background: #BC6033; border-color: #BC6033;
        }

        QPushButton#expTabBtn, QPushButton#awardTabBtn {
            min-height: 38px; max-height: 38px; background: transparent;
            color: #68716D; border: none; border-radius: 0; padding: 0 8px;
            font-size: 13px; font-weight: 700;
        }
        QPushButton#expTabBtn:hover, QPushButton#awardTabBtn:hover {
            color: #1F6B5B; background: transparent;
        }
        QPushButton#expTabBtn[active="true"],
        QPushButton#awardTabBtn[active="true"] {
            color: #1F6B5B; border-bottom: 3px solid #D97745;
            background: transparent; font-weight: 850;
        }

        QMessageBox, QDialog { background: #F4F1EA; }
        QToolTip {
            background: #203F38; color: #FFF9F1;
            border: 1px solid #31554C; padding: 6px 9px;
        }
    )QSS"));

    const QList<QPushButton *> buttons = findChildren<QPushButton *>();
    for (QPushButton *button : buttons)
        button->setCursor(Qt::PointingHandCursor);
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
        shadow->setBlurRadius(20);
        shadow->setOffset(0, 4);
        shadow->setColor(QColor(13, 148, 136, 14));
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
    metricsPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *metricsLayout = new QVBoxLayout(metricsPanel);
    metricsLayout->setContentsMargins(4, 6, 4, 0);
    metricsLayout->setSpacing(10);

    auto *metricsTitle = new QLabel("学习档案快照");
    metricsTitle->setObjectName("homeMetricsTitle");
    metricsTitle->setStyleSheet(QStringLiteral(
        "font-size: 18px; font-weight: 800; color: #25332F; background: transparent;"));
    auto *metricsSub = new QLabel("课程、实践与成果概览");
    metricsSub->setObjectName("homeMetricsSub");
    metricsSub->setStyleSheet(QStringLiteral(
        "font-size: 12px; font-weight: 600; color: #7A827E; background: transparent;"));
    auto *metricsHeader = new QHBoxLayout;
    metricsHeader->setContentsMargins(2, 0, 2, 0);
    metricsHeader->setSpacing(10);
    metricsHeader->addWidget(metricsTitle);
    metricsHeader->addWidget(metricsSub);
    metricsHeader->addStretch();
    metricsLayout->addLayout(metricsHeader);

    auto *statsLayout = new QGridLayout;
    statsLayout->setContentsMargins(0, 0, 0, 4);
    statsLayout->setHorizontalSpacing(10);
    statsLayout->setVerticalSpacing(0);

    auto makeStatCard = [](const QString &title, const QString &subTitle,
                           QLabel **valueLabel, const QString &tone) {
        auto *card = new QFrame;
        card->setObjectName("homeStatCard");
        card->setProperty("tone", tone);
        card->setFrameShape(QFrame::NoFrame);
        card->setFixedHeight(104);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto *shadow = new QGraphicsDropShadowEffect(card);
        shadow->setBlurRadius(16);
        shadow->setOffset(0, 4);
        shadow->setColor(QColor(13, 148, 136, 20));
        card->setGraphicsEffect(shadow);

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(16, 14, 16, 12);
        layout->setSpacing(3);

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
    actionBar->setFrameShape(QFrame::NoFrame);
    actionBar->setFixedHeight(58);
    {
        auto *actionShadow = new QGraphicsDropShadowEffect(actionBar);
        actionShadow->setBlurRadius(16);
        actionShadow->setOffset(0, 4);
        actionShadow->setColor(QColor(13, 148, 136, 20));
        actionBar->setGraphicsEffect(actionShadow);
    }
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

    // 以 4 倍分辨率绘制，再作为高 DPI pixmap 显示。
    // 这样文字、网格与折线在 Retina / 4K 屏幕上不会被系统二次放大。
    const int width = qMax(360, homeChartLabel->width());
    const int height = qMax(220, homeChartLabel->height());
    QPixmap pixmap(qCeil(width * kHiResScale),
                   qCeil(height * kHiResScale));
    pixmap.fill(Qt::transparent);
    pixmap.setDevicePixelRatio(kHiResScale);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 给纵轴刻度留出独立区域，避免折线端点压住 0.0–4.0 数值。
    constexpr qreal leftMargin = 68.0;
    constexpr qreal rightMargin = 30.0;
    constexpr qreal topMargin = 44.0;
    constexpr qreal bottomMargin = 62.0;
    QRectF plot(leftMargin, topMargin,
                width - leftMargin - rightMargin,
                height - topMargin - bottomMargin);
    QPen gridPen(QColor(226, 221, 211));
    gridPen.setWidth(1);
    painter.setPen(gridPen);
    painter.setFont(QFont("PingFang SC", 9, QFont::DemiBold));
    painter.setBrush(Qt::NoBrush);

    for (int i = 0; i <= 4; ++i) {
        double y = plot.bottom() - plot.height() * i / 4.0;
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        painter.setPen(QColor(101, 112, 107));
        painter.drawText(QRectF(8, y - 10, leftMargin - 22, 20),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(i, 'f', 1));
        painter.drawLine(QPointF(plot.left() - 5, y),
                         QPointF(plot.left(), y));
        painter.setPen(gridPen);
    }

    painter.setPen(QPen(QColor(207, 201, 190), 1));
    painter.drawLine(plot.bottomLeft(), plot.bottomRight());
    painter.drawLine(plot.bottomLeft(), plot.topLeft());

    QVector<QPointF> points;
    const double dataLeft = plot.left() + 10.0;
    const double dataRight = plot.right() - 10.0;
    const double step = (dataRight - dataLeft) / (semesters.size() - 1);
    painter.setPen(QColor(70, 82, 78));
    for (int i = 0; i < semesters.size(); ++i) {
        double x = dataLeft + i * step;
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
        areaGradient.setColorAt(0, QColor(31, 107, 91, 72));
        areaGradient.setColorAt(1, QColor(31, 107, 91, 4));
        painter.setPen(Qt::NoPen);
        painter.setBrush(areaGradient);
        painter.drawPath(areaPath);

        QPen linePen(QColor(31, 107, 91), 4, Qt::SolidLine, Qt::RoundCap,
                     Qt::RoundJoin);
        painter.setPen(linePen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

    for (const QPointF &point : points) {
        painter.setPen(QPen(QColor(255, 255, 255), 4));
        painter.setBrush(QColor(31, 107, 91));
        painter.drawEllipse(point, 6, 6);
    }

    if (points.isEmpty()) {
        painter.setPen(QColor(140, 147, 143));
        painter.setFont(QFont("PingFang SC", 15, QFont::DemiBold));
        painter.drawText(plot, Qt::AlignCenter,
                         "暂无课程 GPA 数据\n录入课程后这里会自动生成折线图");
    }

    painter.setPen(QColor(101, 112, 107));
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
    mainLayout->setContentsMargins(0, 6, 0, 0);
    mainLayout->setSpacing(12);

    // ---- 辅助：创建带标题的现代化卡片 ----
    auto makeSectionCard = [](const QString &title) {
        auto *card = new QFrame;
        card->setObjectName("resumeSectionCard");
        card->setFrameShape(QFrame::NoFrame);
        card->setStyleSheet(QStringLiteral(
            "QFrame#resumeSectionCard { background: #FFFEFA;"
            " border: 1px solid #DED8CC; border-radius: 12px; }"));

        auto *shadow = new QGraphicsDropShadowEffect(card);
        shadow->setBlurRadius(20);
        shadow->setOffset(0, 4);
        shadow->setColor(QColor(13, 148, 136, 14));
        card->setGraphicsEffect(shadow);

        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(20, 13, 20, 13);
        layout->setSpacing(6);

        auto *titleLbl = new QLabel(title);
        titleLbl->setObjectName("resumeSectionTitle");
        titleLbl->setStyleSheet(QStringLiteral(
            "font-size: %1px; font-weight: %2; color: %3; background: transparent;"
        ).arg(TypeScale::h2).arg(FontWeight::bold).arg(Color::onSurface));
        layout->addWidget(titleLbl);
        return card;
    };

    // ===== 照片区域（标题栏内嵌导入按钮）=====
    QFrame *photoCard = new QFrame(ui->profilePage);
    photoCard->setObjectName("resumePhotoCard");
    photoCard->setFrameShape(QFrame::NoFrame);
    photoCard->setStyleSheet(QStringLiteral(
        "QFrame#resumePhotoCard { background: #FFFEFA;"
        " border: 1px solid #DED8CC; border-radius: 12px; }"));
    {
        auto *shadow = new QGraphicsDropShadowEffect(photoCard);
        shadow->setBlurRadius(20);
        shadow->setOffset(0, 4);
        shadow->setColor(QColor(13, 148, 136, 14));
        photoCard->setGraphicsEffect(shadow);
    }
    auto *photoCardLayout = new QVBoxLayout(photoCard);
    photoCardLayout->setContentsMargins(24, 18, 24, 18);
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
        " border-radius: 8px; min-height: 32px; font-size: %2px;"
        " font-weight: %3; padding: 0 18px; }"
        "QPushButton:hover { background: %4; }"
    ).arg(Color::primary)
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
    // 头像属于个人资料：保留裁剪控件作为内部能力，不再显示在简历页。
    photoCard->hide();

    // ===== 简历模板预览画廊 =====
    auto *templateGallery = new QFrame;
    templateGallery->setObjectName("resumeTemplateGallery");
    templateGallery->setStyleSheet(
        "QFrame#resumeTemplateGallery { background:transparent; border:none; }");
    auto *galleryLayout = new QVBoxLayout(templateGallery);
    galleryLayout->setContentsMargins(0, 2, 0, 8);
    galleryLayout->setSpacing(14);

    auto *galleryHeader = new QHBoxLayout;
    galleryHeader->setSpacing(12);
    auto *galleryTitle = new QLabel("⌄  模板");
    galleryTitle->setStyleSheet(
        "color:#56615D; font-size:18px; font-weight:800;");
    galleryHeader->addWidget(galleryTitle);

    resumeTemplateDescriptionLbl = new QLabel(
        "传统学术排版，信息清晰，适合通用申请。");
    resumeTemplateDescriptionLbl->setStyleSheet(
        "color:#7A827E; font-size:12px; font-weight:600; padding-top:2px;");
    galleryHeader->addWidget(resumeTemplateDescriptionLbl, 1);
    auto *galleryHint = new QLabel("点击切换 · 按空格放大 / 返回");
    galleryHint->setStyleSheet(
        "color:#1F6B5B; font-size:11px; font-weight:700; padding-top:2px;");
    galleryHeader->addWidget(galleryHint);
    galleryLayout->addLayout(galleryHeader);

    resumeTemplateCombo = new QComboBox(ui->profilePage);
    resumeTemplateCombo->setObjectName("resumeTemplateCombo");
    resumeTemplateCombo->addItem("经典学术", "classic");
    resumeTemplateCombo->addItem("深海蓝双栏", "navy");
    resumeTemplateCombo->addItem("暖色编辑风", "editorial");
    resumeTemplateCombo->hide();

    struct TemplatePreview {
        QString name;
        QString resource;
    };
    const QList<TemplatePreview> previews = {
        {"经典学术", ":/previews/resume-classic.png"},
        {"深海蓝双栏", ":/previews/resume-navy.png"},
        {"暖色编辑风", ":/previews/resume-editorial.png"},
    };

    auto *templateGroup = new QButtonGroup(templateGallery);
    templateGroup->setExclusive(true);
    auto *previewRow = new QHBoxLayout;
    previewRow->setContentsMargins(6, 0, 6, 0);
    previewRow->setSpacing(28);

    auto makePaperIcon = [](const QString &resource) {
        constexpr int canvasWidth = 170;
        constexpr int canvasHeight = 226;
        constexpr int paperWidth = 148;
        constexpr int paperHeight = 210;
        const int canvasPixelWidth = qCeil(canvasWidth * kHiResScale);
        const int canvasPixelHeight = qCeil(canvasHeight * kHiResScale);
        const int paperPixelWidth = qCeil(paperWidth * kHiResScale);
        const int paperPixelHeight = qCeil(paperHeight * kHiResScale);

        QPixmap canvas(canvasPixelWidth, canvasPixelHeight);
        canvas.fill(Qt::transparent);

        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(35, 45, 42, 28));
        painter.drawRoundedRect(
            QRectF(13 * kHiResScale, 10 * kHiResScale,
                   paperPixelWidth, paperPixelHeight),
            6 * kHiResScale, 6 * kHiResScale);
        painter.setBrush(QColor(35, 45, 42, 13));
        painter.drawRoundedRect(
            QRectF(10 * kHiResScale, 7 * kHiResScale,
                   paperPixelWidth, paperPixelHeight),
            6 * kHiResScale, 6 * kHiResScale);

        const QPixmap source(resource);
        const QPixmap page = source.scaled(
            paperPixelWidth, paperPixelHeight, Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation);
        painter.drawPixmap(qCeil(7 * kHiResScale),
                           qCeil(4 * kHiResScale), page);
        painter.end();
        canvas.setDevicePixelRatio(kHiResScale);
        return QIcon(canvas);
    };

    for (int index = 0; index < previews.size(); ++index) {
        const TemplatePreview &preview = previews.at(index);
        auto *button = new QToolButton(templateGallery);
        button->setObjectName("resumeTemplatePreview");
        button->setCheckable(true);
        button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        button->setIcon(makePaperIcon(preview.resource));
        button->setIconSize(QSize(170, 226));
        button->setText(preview.name);
        button->setCursor(Qt::PointingHandCursor);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setFixedHeight(264);
        button->setStyleSheet(QStringLiteral(
            "QToolButton { background:transparent; color:#37443F;"
            " border:none; border-bottom:3px solid transparent;"
            " padding:2px 4px 7px; font-size:13px; font-weight:700; }"
            "QToolButton:hover { background:transparent; color:#1F6B5B; }"
            "QToolButton:checked { background:transparent; color:#174F44;"
            " border-bottom:3px solid #D97745; font-weight:850; }"));
        templateGroup->addButton(button, index);
        resumeTemplateCards.append(button);
        previewRow->addWidget(button, 1);

        connect(button, &QToolButton::clicked, this, [this, index]() {
            resumeTemplateCombo->setCurrentIndex(index);
        });
    }
    galleryLayout->addLayout(previewRow);
    mainLayout->addWidget(templateGallery);

    // ===== 技术能力区域 =====
    QFrame *skillsCard = makeSectionCard("技术能力");
    skillsCard->setFixedHeight(116);
    auto *skillsLayout = static_cast<QVBoxLayout *>(skillsCard->layout());
    auto *skillsTitle = skillsCard->findChild<QLabel *>("resumeSectionTitle");
    skillsLayout->removeWidget(skillsTitle);

    auto *skillsHeader = new QWidget;
    auto *skillsHeaderLayout = new QHBoxLayout(skillsHeader);
    skillsHeaderLayout->setContentsMargins(0, 0, 0, 0);
    skillsHeaderLayout->setSpacing(10);
    skillsHeaderLayout->addWidget(skillsTitle);
    skillsHeaderLayout->addStretch();
    editSkillsBtn = new QPushButton("编辑");
    editSkillsBtn->setObjectName("editSkillsBtn");
    editSkillsBtn->setCursor(Qt::PointingHandCursor);
    editSkillsBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:transparent; color:%1;"
        " border:1px solid %1; border-radius:7px;"
        " min-height:27px; max-height:27px;"
        " font-size:12px; font-weight:700; padding:0 16px; }"
        "QPushButton:hover { background:%1; color:#FFFFFF; }"
    ).arg(Color::primary));
    skillsHeaderLayout->addWidget(editSkillsBtn);
    skillsLayout->addWidget(skillsHeader);

    skillsLbl = new QLabel("尚未填写技能，点击编辑补充。", skillsCard);
    skillsLbl->setObjectName("resumeBodyText");
    skillsLbl->setWordWrap(true);
    skillsLbl->setMinimumHeight(30);
    skillsLbl->setStyleSheet(
        "color:#59635F; font-size:13px; font-weight:550;"
        "background:transparent; padding:2px 0;");
    skillsLayout->addWidget(skillsLbl);

    mainLayout->addWidget(skillsCard);

    // ===== 个人总结区域 =====
    QFrame *summaryCard = makeSectionCard("个人总结");
    summaryCard->setFixedHeight(124);
    auto *summaryLayout = static_cast<QVBoxLayout *>(summaryCard->layout());
    auto *summaryTitle = summaryCard->findChild<QLabel *>("resumeSectionTitle");
    summaryLayout->removeWidget(summaryTitle);

    auto *summaryHeader = new QWidget;
    auto *summaryHeaderLayout = new QHBoxLayout(summaryHeader);
    summaryHeaderLayout->setContentsMargins(0, 0, 0, 0);
    summaryHeaderLayout->setSpacing(10);
    summaryHeaderLayout->addWidget(summaryTitle);
    summaryHeaderLayout->addStretch();
    editSummaryBtn = new QPushButton("编辑");
    editSummaryBtn->setObjectName("editSummaryBtn");
    editSummaryBtn->setCursor(Qt::PointingHandCursor);
    editSummaryBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background:transparent; color:%1;"
        " border:1px solid %1; border-radius:7px;"
        " min-height:27px; max-height:27px;"
        " font-size:12px; font-weight:700; padding:0 16px; }"
        "QPushButton:hover { background:%1; color:#FFFFFF; }"
    ).arg(Color::primary));
    summaryHeaderLayout->addWidget(editSummaryBtn);
    summaryLayout->addWidget(summaryHeader);

    summaryLbl = new QLabel("尚未填写个人总结，点击编辑补充。", summaryCard);
    summaryLbl->setObjectName("resumeBodyText");
    summaryLbl->setWordWrap(true);
    summaryLbl->setMinimumHeight(34);
    summaryLbl->setStyleSheet(
        "color:#59635F; font-size:13px; font-weight:550;"
        "background:transparent; padding:2px 0;");
    summaryLayout->addWidget(summaryLbl);

    mainLayout->addWidget(summaryCard);

    // ===== 预览与导出操作区 =====
    auto *exportActions = new QHBoxLayout;
    exportActions->setSpacing(Spacing::sm);
    exportActions->addStretch();

    previewResumeBtn = new QPushButton("在浏览器中预览");
    previewResumeBtn->setObjectName("previewResumeBtn");
    previewResumeBtn->setCursor(Qt::PointingHandCursor);
    previewResumeBtn->setMinimumHeight(42);
    previewResumeBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: %1;"
        " border: 2px solid %1; border-radius: 9px;"
        " font-size: 14px; font-weight: 700; padding: 0 22px; }"
        "QPushButton:hover { background: %1; color: #FFFFFF; }"
    ).arg(Color::primary));
    exportActions->addWidget(previewResumeBtn);

    exportResumePdfBtn = new QPushButton("导出 PDF");
    exportResumePdfBtn->setObjectName("exportResumePdfBtn");
    exportResumePdfBtn->setCursor(Qt::PointingHandCursor);
    exportResumePdfBtn->setMinimumHeight(42);
    exportResumePdfBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: %1; color: #FFFFFF; border: none;"
        " border-radius: 9px; font-size: 14px; font-weight: 700;"
        " padding: 0 26px; }"
        "QPushButton:hover { background: %2; }"
        "QPushButton:disabled { background: #94A3B8; }"
    ).arg(Color::primary).arg(Color::accent));
    exportActions->addWidget(exportResumePdfBtn);

    mainLayout->addLayout(exportActions);
    mainLayout->addStretch();

    // 按空格开关的沉浸式模板预览层。
    resumePreviewOverlay = new QWidget(ui->centralwidget);
    resumePreviewOverlay->setObjectName("resumePreviewOverlay");
    resumePreviewOverlay->setAttribute(Qt::WA_StyledBackground, true);
    resumePreviewOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    resumePreviewOverlay->setStyleSheet(QStringLiteral(
        "QWidget#resumePreviewOverlay { background:rgba(18,28,25,232); }"
        "QLabel#resumePreviewOverlayTitle { color:#FFF9F1;"
        " font-size:20px; font-weight:850; background:transparent; }"
        "QLabel#resumePreviewOverlayHint { color:#BCCBC5;"
        " font-size:12px; font-weight:650; background:transparent; }"
        "QLabel#resumePreviewLargeLbl { background:#FFFFFF;"
        " border:1px solid rgba(255,255,255,0.32); }"));
    resumePreviewOverlay->setGeometry(ui->centralwidget->rect());

    auto *overlayLayout = new QVBoxLayout(resumePreviewOverlay);
    overlayLayout->setContentsMargins(34, 24, 34, 28);
    overlayLayout->setSpacing(12);

    auto *overlayHeader = new QHBoxLayout;
    resumePreviewTitleLbl = new QLabel("简历模板");
    resumePreviewTitleLbl->setObjectName("resumePreviewOverlayTitle");
    overlayHeader->addWidget(resumePreviewTitleLbl);
    overlayHeader->addStretch();
    auto *overlayHint = new QLabel("再按一次空格返回");
    overlayHint->setObjectName("resumePreviewOverlayHint");
    overlayHeader->addWidget(overlayHint);
    overlayLayout->addLayout(overlayHeader);

    resumePreviewLargeLbl = new QLabel;
    resumePreviewLargeLbl->setObjectName("resumePreviewLargeLbl");
    resumePreviewLargeLbl->setAlignment(Qt::AlignCenter);
    resumePreviewLargeLbl->setSizePolicy(
        QSizePolicy::Expanding, QSizePolicy::Expanding);
    overlayLayout->addWidget(resumePreviewLargeLbl, 1, Qt::AlignCenter);

    auto *previewShadow =
        new QGraphicsDropShadowEffect(resumePreviewLargeLbl);
    previewShadow->setBlurRadius(38);
    previewShadow->setOffset(0, 12);
    previewShadow->setColor(QColor(0, 0, 0, 100));
    resumePreviewLargeLbl->setGraphicsEffect(previewShadow);
    resumePreviewOverlay->hide();

    // ---- 圆形裁切辅助函数 ----
    auto makeCircularPixmap = [](const QPixmap &source, int size) {
        return circularHiResPixmap(source, size);
    };

    // ---- 信号连接 ----
    connect(selectPhotoBtn, &QPushButton::clicked, this, [this, makeCircularPixmap]() {
        QWidget *photoDialogParent = QApplication::activeModalWidget();
        if (!photoDialogParent)
            photoDialogParent = this;
        QString filePath = QFileDialog::getOpenFileName(
            photoDialogParent, "导入个人照片", QString(),
            "JPEG 图片 (*.jpg *.jpeg);;所有文件 (*)");
        if (filePath.isEmpty())
            return;

        QPixmap source(filePath);
        if (source.isNull()) {
            QMessageBox::warning(this, "提示", "无法读取所选图片");
            return;
        }

        // 创建 photos 目录
        QString photosDir = QCoreApplication::applicationDirPath() + "/photos";
        if (!QDir().mkpath(photosDir)) {
            QMessageBox::warning(this, "提示", "无法创建照片目录");
            return;
        }

        // 统一保存为无透明通道的 JPEG，避免透明圆图写入 JPEG 后产生黑角。
        int userId = User::getInstance().getId();
        QString newName = QString("user_%1_photo.jpg").arg(userId);
        QString destPath = photosDir + "/" + newName;

        // ===== 手动框选头像对话框 =====
        QDialog cropDialog(photoDialogParent);
        cropDialog.setWindowTitle("框选头像 — 拖动圆形选区定位");
        cropDialog.setMinimumSize(520, 650);

        auto *cdLayout = new QVBoxLayout(&cropDialog);
        cdLayout->setContentsMargins(24, 20, 24, 16);
        cdLayout->setSpacing(14);

        auto *cdTitle = new QLabel("拖动圆形选区，框选头像区域");
        cdTitle->setStyleSheet("font-size:18px; font-weight:800; color:#0F172A;");
        cdLayout->addWidget(cdTitle);

        // ---- 自定义绘制控件（支持旋转）----
        class CropCanvas : public QWidget {
        public:
            QPixmap sourcePixmap;       // 原始图片
            QPixmap displayPixmap;      // 旋转后的显示用图片
            QPointF circleCenter;
            int circleRadius = 90;
            int rotationAngle = 0;      // 当前旋转角度（度）

            CropCanvas(QWidget *parent) : QWidget(parent) {
                setMinimumSize(420, 420);
                setMouseTracking(true);
                setCursor(Qt::OpenHandCursor);
            }

            void resetCircle() {
                circleCenter = QPointF(width() / 2.0, height() / 2.0);
                update();
            }

            void setRotation(int degrees) {
                rotationAngle = degrees;
                rebuildDisplayPixmap();
                // 旋转后重新居中圆形选区
                circleCenter = QPointF(width() / 2.0, height() / 2.0);
                update();
            }

        private:
            void rebuildDisplayPixmap() {
                if (sourcePixmap.isNull()) return;
                if (rotationAngle == 0) {
                    displayPixmap = sourcePixmap;
                } else {
                    QTransform t;
                    t.rotate(rotationAngle);
                    displayPixmap = sourcePixmap.transformed(t, Qt::SmoothTransformation);
                }
            }

        protected:
            void paintEvent(QPaintEvent *) override {
                QPainter p(this);
                p.setRenderHint(QPainter::Antialiasing, true);
                p.setRenderHint(QPainter::SmoothPixmapTransform, true);

                if (displayPixmap.isNull() && !sourcePixmap.isNull())
                    const_cast<CropCanvas*>(this)->rebuildDisplayPixmap();

                // 绘制显示图片（缩放以适配控件，保持比例）
                if (!displayPixmap.isNull()) {
                    QSize scaled = displayPixmap.size();
                    scaled.scale(width(), height(), Qt::KeepAspectRatio);
                    int sx = (width() - scaled.width()) / 2;
                    int sy = (height() - scaled.height()) / 2;
                    p.drawPixmap(sx, sy, scaled.width(), scaled.height(), displayPixmap);
                }

                // 半透明遮罩
                QColor dimColor(0, 0, 0, 120);
                QPainterPath fullPath;
                fullPath.addRect(rect());
                QPainterPath circlePath;
                circlePath.addEllipse(circleCenter, circleRadius, circleRadius);
                QPainterPath outsideCircle = fullPath.subtracted(circlePath);
                p.fillPath(outsideCircle, dimColor);

                // 圆形边框（内外双线）
                p.setPen(QPen(QColor(255, 255, 255, 220), 3));
                p.setBrush(Qt::NoBrush);
                p.drawEllipse(circleCenter, circleRadius, circleRadius);
                p.setPen(QPen(QColor(13, 148, 136), 2));
                p.drawEllipse(circleCenter, circleRadius + 1, circleRadius + 1);
            }

            void mousePressEvent(QMouseEvent *e) override {
                if (e->button() == Qt::LeftButton) {
                    m_dragging = true;
                    m_dragOffset = circleCenter - e->pos();
                    setCursor(Qt::ClosedHandCursor);
                }
            }

            void mouseMoveEvent(QMouseEvent *e) override {
                if (m_dragging) {
                    QPointF newCenter = e->pos() + m_dragOffset;
                    int r = circleRadius;
                    newCenter.setX(qBound((double)r, newCenter.x(), (double)(width() - r)));
                    newCenter.setY(qBound((double)r, newCenter.y(), (double)(height() - r)));
                    circleCenter = newCenter;
                    update();
                }
            }

            void mouseReleaseEvent(QMouseEvent *) override {
                m_dragging = false;
                setCursor(Qt::OpenHandCursor);
            }

        private:
            bool m_dragging = false;
            QPointF m_dragOffset;
        };

        auto *canvas = new CropCanvas(&cropDialog);
        canvas->sourcePixmap = source;
        canvas->resetCircle();
        cdLayout->addWidget(canvas, 1);

        // 半径滑块
        auto *radiusRow = new QHBoxLayout;
        auto *radiusLbl = new QLabel("选区大小：");
        radiusLbl->setStyleSheet("font-size:13px; font-weight:600; color:#334155;");
        radiusRow->addWidget(radiusLbl);

        auto *radiusSlider = new QSlider(Qt::Horizontal, &cropDialog);
        radiusSlider->setRange(40, 160);
        radiusSlider->setValue(90);
        radiusSlider->setStyleSheet(
            "QSlider::groove:horizontal { height:6px; background:#E2E8F0; border-radius:3px; }"
            "QSlider::handle:horizontal { width:18px; height:18px; margin:-6px 0;"
            " background:#0D9488; border-radius:9px; }");
        radiusRow->addWidget(radiusSlider, 1);

        auto *radiusVal = new QLabel("90px");
        radiusVal->setStyleSheet("font-size:12px; font-weight:600; color:#64748B; min-width:36px;");
        radiusRow->addWidget(radiusVal);

        connect(radiusSlider, &QSlider::valueChanged, &cropDialog, [canvas, radiusVal](int v) {
            canvas->circleRadius = v;
            radiusVal->setText(QString("%1px").arg(v));
            // 重新约束圆心位置
            QPointF c = canvas->circleCenter;
            c.setX(qBound((double)v, c.x(), (double)(canvas->width() - v)));
            c.setY(qBound((double)v, c.y(), (double)(canvas->height() - v)));
            canvas->circleCenter = c;
            canvas->update();
        });
        cdLayout->addLayout(radiusRow);

        // 旋转控制行：快捷按钮 + 滑块
        auto *rotateRow = new QHBoxLayout;
        auto *rotateLbl = new QLabel("旋转角度：");
        rotateLbl->setStyleSheet("font-size:13px; font-weight:600; color:#334155;");
        rotateRow->addWidget(rotateLbl);

        // 左转 90°
        auto *rotLeftBtn = new QPushButton("↺ -90°");
        rotLeftBtn->setFixedWidth(72);
        rotLeftBtn->setCursor(Qt::PointingHandCursor);
        rotLeftBtn->setStyleSheet(
            "QPushButton { background: transparent; color: #0D9488;"
            " border: 1px solid #0D9488; border-radius: 6px;"
            " font-size: 11px; font-weight: 700; padding: 4px 0; }"
            "QPushButton:hover { background: #0D9488; color: #FFFFFF; }");
        rotateRow->addWidget(rotLeftBtn);

        // 旋转滑块
        auto *rotateSlider = new QSlider(Qt::Horizontal, &cropDialog);
        rotateSlider->setRange(-180, 180);
        rotateSlider->setValue(0);
        rotateSlider->setStyleSheet(
            "QSlider::groove:horizontal { height:6px; background:#E2E8F0; border-radius:3px; }"
            "QSlider::handle:horizontal { width:18px; height:18px; margin:-6px 0;"
            " background:#0D9488; border-radius:9px; }");
        rotateRow->addWidget(rotateSlider, 1);

        auto *rotateVal = new QLabel("0°");
        rotateVal->setStyleSheet("font-size:12px; font-weight:600; color:#64748B; min-width:36px;");
        rotateRow->addWidget(rotateVal);

        // 右转 90°
        auto *rotRightBtn = new QPushButton("↻ +90°");
        rotRightBtn->setFixedWidth(72);
        rotRightBtn->setCursor(Qt::PointingHandCursor);
        rotRightBtn->setStyleSheet(
            "QPushButton { background: transparent; color: #0D9488;"
            " border: 1px solid #0D9488; border-radius: 6px;"
            " font-size: 11px; font-weight: 700; padding: 4px 0; }"
            "QPushButton:hover { background: #0D9488; color: #FFFFFF; }");
        rotateRow->addWidget(rotRightBtn);

        // 重置按钮
        auto *rotResetBtn = new QPushButton("⟲ 0°");
        rotResetBtn->setFixedWidth(60);
        rotResetBtn->setCursor(Qt::PointingHandCursor);
        rotResetBtn->setStyleSheet(
            "QPushButton { background: transparent; color: #94A3B8;"
            " border: 1px solid #CBD5E1; border-radius: 6px;"
            " font-size: 11px; font-weight: 700; padding: 4px 0; }"
            "QPushButton:hover { background: #F1F5F9; color: #64748B; }");
        rotateRow->addWidget(rotResetBtn);
        cdLayout->addLayout(rotateRow);

        // 旋转信号连接
        connect(rotateSlider, &QSlider::valueChanged, &cropDialog, [canvas, rotateVal](int v) {
            canvas->setRotation(v);
            rotateVal->setText(QString("%1°").arg(v));
        });
        connect(rotLeftBtn, &QPushButton::clicked, &cropDialog, [rotateSlider, canvas]() {
            int newVal = canvas->rotationAngle - 90;
            if (newVal < -180) newVal += 360;
            rotateSlider->setValue(newVal);
        });
        connect(rotRightBtn, &QPushButton::clicked, &cropDialog, [rotateSlider, canvas]() {
            int newVal = canvas->rotationAngle + 90;
            if (newVal > 180) newVal -= 360;
            rotateSlider->setValue(newVal);
        });
        connect(rotResetBtn, &QPushButton::clicked, &cropDialog, [rotateSlider]() {
            rotateSlider->setValue(0);
        });

        auto *cdButtons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &cropDialog);
        cdButtons->button(QDialogButtonBox::Ok)->setText("确定");
        cdButtons->button(QDialogButtonBox::Cancel)->setText("取消");
        cdLayout->addWidget(cdButtons);

        connect(cdButtons, &QDialogButtonBox::accepted, &cropDialog, &QDialog::accept);
        connect(cdButtons, &QDialogButtonBox::rejected, &cropDialog, &QDialog::reject);

        if (cropDialog.exec() != QDialog::Accepted)
            return;

        // 根据用户在控件坐标系中选定的圆形，映射回旋转后图片坐标后裁切
        {
            // 计算旋转后的图片
            QPixmap rotatedSource = source;
            if (canvas->rotationAngle != 0) {
                QTransform t;
                t.rotate(canvas->rotationAngle);
                rotatedSource = source.transformed(t, Qt::SmoothTransformation);
            }

            // 计算旋转后图片在 canvas 中的缩放矩形
            QSize scaledSz = rotatedSource.size();
            scaledSz.scale(canvas->width(), canvas->height(), Qt::KeepAspectRatio);
            int imgX = (canvas->width() - scaledSz.width()) / 2;
            int imgY = (canvas->height() - scaledSz.height()) / 2;

            // 圆心在缩放图片中的比例
            double fx = (canvas->circleCenter.x() - imgX) / scaledSz.width();
            double fy = (canvas->circleCenter.y() - imgY) / scaledSz.height();
            double fr = canvas->circleRadius / (double)qMin(scaledSz.width(), scaledSz.height());

            // 映射到旋转后图片
            int srcCx = qBound(0, (int)(fx * rotatedSource.width()), rotatedSource.width());
            int srcCy = qBound(0, (int)(fy * rotatedSource.height()), rotatedSource.height());
            int srcR = qBound(10, (int)(fr * qMin(rotatedSource.width(), rotatedSource.height())),
                              qMin(rotatedSource.width(), rotatedSource.height()) / 2);

            // 从旋转后图片中裁切正方形区域
            int cropX = qMax(0, srcCx - srcR);
            int cropY = qMax(0, srcCy - srcR);
            int cropW = qMin(srcR * 2, rotatedSource.width() - cropX);
            int cropH = qMin(srcR * 2, rotatedSource.height() - cropY);
            int cropSide = qMin(cropW, cropH);

            QPixmap cropped = rotatedSource.copy(cropX, cropY, cropSide, cropSide);
            if (!cropped.isNull()) {
                // 持久化用户实际框选的正方形区域，不在保存阶段再次裁切。
                // 应用内圆形预览因此在重新进入页面后仍与导入时一致；
                // 简历的 3:4 证件照比例由 HTML 的 object-fit: cover 负责。
                QPixmap savedCrop = cropped.scaled(
                    800, 800, Qt::IgnoreAspectRatio,
                    Qt::SmoothTransformation);

                const QString tempPath = destPath + ".tmp";
                QFile::remove(tempPath);
                if (!savedCrop.save(tempPath, "JPEG", 92)) {
                    QMessageBox::warning(this, "提示",
                                         "裁剪后的照片保存失败，请重试");
                    return;
                }

                const QString previousPhotoPath = m_photoPath;
                if (QFile::exists(destPath) && !QFile::remove(destPath)) {
                    QFile::remove(tempPath);
                    QMessageBox::warning(this, "提示",
                                         "旧照片正在被占用，无法替换");
                    return;
                }
                if (!QFile::rename(tempPath, destPath)) {
                    QFile::remove(tempPath);
                    QMessageBox::warning(this, "提示",
                                         "照片替换失败，请重试");
                    return;
                }

                m_photoPath = "photos/" + newName;

                // 直接使用持久化后的同一图像生成预览，避免即时预览和重载预览不一致。
                QPixmap preview =
                    makeCircularPixmap(savedCrop, photoPreviewLbl->width());
                photoPreviewLbl->setPixmap(preview);
                photoPreviewLbl->setText(QString());

                // 新文件保存成功后，再清理旧扩展名的历史照片。
                if (!previousPhotoPath.isEmpty() &&
                    previousPhotoPath != m_photoPath) {
                    QFile::remove(
                        QDir(QCoreApplication::applicationDirPath())
                            .filePath(previousPhotoPath));
                }

                // 自动保存
                saveResumeToDb();
            }
        }
    });

    connect(editSkillsBtn, &QPushButton::clicked, this,
            &MainWindow::editSkills);
    connect(editSummaryBtn, &QPushButton::clicked, this,
            &MainWindow::editSummary);

    connect(
        resumeTemplateCombo,
        QOverload<int>::of(&QComboBox::currentIndexChanged), this,
        [this](int index) {
            const QString templateId =
                resumeTemplateCombo->itemData(index).toString();
            for (int cardIndex = 0;
                 cardIndex < resumeTemplateCards.size(); ++cardIndex) {
                resumeTemplateCards.at(cardIndex)
                    ->setChecked(cardIndex == index);
            }
            if (resumePreviewOverlay && resumePreviewOverlay->isVisible())
                updateResumeTemplateMagnifier();
            if (templateId == "navy") {
                resumeTemplateDescriptionLbl->setText(
                    "左侧信息轨道与右侧履历，适合技术岗和项目型简历。");
            } else if (templateId == "editorial") {
                resumeTemplateDescriptionLbl->setText(
                    "暖灰纸张与编辑式编号，适合商科、研究和综合岗位。");
            } else {
                resumeTemplateDescriptionLbl->setText(
                    "传统学术排版，信息清晰，适合通用申请。");
            }
            saveResumeToDb();
        });

    connect(previewResumeBtn, &QPushButton::clicked, this, [this]() {
        saveResumeToDb();
        QString errorMessage;
        if (!resumeExporter->openPreview(User::getInstance().getId(),
                                         &errorMessage)) {
            QMessageBox::warning(this, "预览失败", errorMessage);
        }
    });

    connect(exportResumePdfBtn, &QPushButton::clicked, this, [this]() {
        saveResumeToDb();

        QString username = User::getInstance().getUsername().trimmed();
        username.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
        if (username.isEmpty())
            username = "Resume";

        QString outputDirectory =
            QStandardPaths::writableLocation(
                QStandardPaths::DocumentsLocation);
        if (outputDirectory.isEmpty())
            outputDirectory = QDir::homePath();
        const QString defaultPath =
            QDir(outputDirectory)
                .filePath(username + QStringLiteral("的Resume.pdf"));

        QString filePath = QFileDialog::getSaveFileName(
            this, "导出简历 PDF", defaultPath, "PDF 文件 (*.pdf)");
        if (filePath.isEmpty())
            return;
        if (!filePath.endsWith(".pdf", Qt::CaseInsensitive))
            filePath += ".pdf";

        exportResumePdfBtn->setEnabled(false);
        exportResumePdfBtn->setText("正在导出...");
        resumeExporter->exportPdf(User::getInstance().getId(), filePath);
    });

    connect(resumeExporter, &ResumeExporter::pdfExportFinished, this,
            [this](bool success, const QString &filePath,
                   const QString &errorMessage) {
                exportResumePdfBtn->setEnabled(true);
                exportResumePdfBtn->setText("导出 PDF");
                if (success) {
                    QMessageBox::information(
                        this, "导出成功",
                        QString("简历已导出到：\n%1").arg(filePath));
                } else {
                    QMessageBox::critical(this, "导出失败", errorMessage);
                }
            });
}

void MainWindow::showResumeTemplateMagnifier() {
    if (!resumePreviewOverlay || !resumeTemplateCombo ||
        ui->stackedWidget->currentIndex() != 3) {
        return;
    }

    resumePreviewOverlay->setGeometry(ui->centralwidget->rect());
    updateResumeTemplateMagnifier();
    resumePreviewOverlay->show();
    resumePreviewOverlay->raise();
}

void MainWindow::hideResumeTemplateMagnifier() {
    if (resumePreviewOverlay)
        resumePreviewOverlay->hide();
}

void MainWindow::updateResumeTemplateMagnifier() {
    if (!resumePreviewOverlay || !resumePreviewLargeLbl ||
        !resumeTemplateCombo) {
        return;
    }

    const QStringList resources = {
        ":/previews/resume-classic.png",
        ":/previews/resume-navy.png",
        ":/previews/resume-editorial.png",
    };
    const int index = qBound(
        0, resumeTemplateCombo->currentIndex(), resources.size() - 1);
    const QPixmap source(resources.at(index));
    if (source.isNull())
        return;

    resumePreviewTitleLbl->setText(
        resumeTemplateCombo->itemText(index) + " · 模板预览");

    const int maxWidth = qMax(260, resumePreviewOverlay->width() - 180);
    const int maxHeight = qMax(360, resumePreviewOverlay->height() - 105);
    QSize logicalSize = source.size();
    logicalSize.scale(maxWidth, maxHeight, Qt::KeepAspectRatio);

    // 预览层同样按 4 倍像素密度合成，避免放大时出现锯齿。
    const QSize pixelSize(
        qCeil(logicalSize.width() * kHiResScale),
        qCeil(logicalSize.height() * kHiResScale));
    QPixmap preview = source.scaled(
        pixelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    preview.setDevicePixelRatio(kHiResScale);

    resumePreviewLargeLbl->setFixedSize(logicalSize);
    resumePreviewLargeLbl->setPixmap(preview);
}

void MainWindow::saveResumeToDb() {
    int userId = User::getInstance().getId();
    if (userId <= 0)
        return;
    QVariantMap profile =
        DatabaseManager::getInstance().getResumeProfile(userId);
    profile["skills"] = m_skillsText;
    profile["summary"] = m_summaryText;
    profile["photo_path"] = m_photoPath;
    if (resumeTemplateCombo) {
        profile["template_id"] =
            resumeTemplateCombo
                ->itemData(resumeTemplateCombo->currentIndex())
                .toString();
    }
    DatabaseManager::getInstance().updateResumeProfile(userId, profile);
    updateSidebarAvatar();
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
            skillsLbl->setText(
                m_skillsText.isEmpty()
                    ? "尚未填写技能，点击编辑补充。"
                    : m_skillsText.split('\n', Qt::SkipEmptyParts).join("  ·  "));
        }
        saveResumeToDb();
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
            summaryLbl->setText(
                m_summaryText.isEmpty()
                    ? "尚未填写个人总结，点击编辑补充。"
                    : m_summaryText);
        }
        saveResumeToDb();
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
        skillsLbl->setText(
            m_skillsText.isEmpty()
                ? "尚未填写技能，点击编辑补充。"
                : m_skillsText.split('\n', Qt::SkipEmptyParts).join("  ·  "));
    }

    // 个人总结
    m_summaryText = profile.value("summary").toString();
    if (summaryLbl) {
        summaryLbl->setText(
            m_summaryText.isEmpty()
                ? "尚未填写个人总结，点击编辑补充。"
                : m_summaryText);
    }

    // 简历模板
    if (resumeTemplateCombo) {
        QString templateId =
            profile.value("template_id", "classic").toString();
        int templateIndex = resumeTemplateCombo->findData(templateId);
        if (templateIndex < 0)
            templateIndex = resumeTemplateCombo->findData("classic");
        const QSignalBlocker blocker(resumeTemplateCombo);
        resumeTemplateCombo->setCurrentIndex(templateIndex);
        for (int cardIndex = 0;
             cardIndex < resumeTemplateCards.size(); ++cardIndex) {
            resumeTemplateCards.at(cardIndex)
                ->setChecked(cardIndex == templateIndex);
        }

        if (templateId == "navy") {
            resumeTemplateDescriptionLbl->setText(
                "左侧信息轨道与右侧履历，适合技术岗和项目型简历。");
        } else if (templateId == "editorial") {
            resumeTemplateDescriptionLbl->setText(
                "暖灰纸张与编辑式编号，适合商科、研究和综合岗位。");
        } else {
            resumeTemplateDescriptionLbl->setText(
                "传统学术排版，信息清晰，适合通用申请。");
        }
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
                photoPreviewLbl->setPixmap(
                    circularHiResPixmap(source, size));
                photoPreviewLbl->setText(QString());
            } else {
                photoPreviewLbl->setText("照片文件丢失");
            }
        } else {
            photoPreviewLbl->setPixmap(QPixmap());
            photoPreviewLbl->setText("暂无照片");
        }
    }
    updateSidebarAvatar();
}

void MainWindow::InitFrame() {
    ui->sidebarFrame->setFixedWidth(252);
    ui->userProfileWidget->setMinimumHeight(190);
    ui->userNameLbl->setMinimumHeight(30);
    ui->detailLbl->setMaximumWidth(112);
    ui->majorLbl->setMinimumHeight(42);
    ui->userNameLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->detailLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->majorLbl->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ui->labelStats->setAlignment(Qt::AlignCenter);
    ui->userNameLbl->setWordWrap(false);
    ui->detailLbl->setWordWrap(false);
    ui->majorLbl->setWordWrap(true);

    if (sidebarAvatarLbl == nullptr) {
        auto *identityRow = new QWidget(ui->userProfileWidget);
        identityRow->setObjectName("identityRow");
        auto *identityLayout = new QHBoxLayout(identityRow);
        identityLayout->setContentsMargins(0, 0, 0, 0);
        identityLayout->setSpacing(12);

        sidebarAvatarLbl = new QLabel(identityRow);
        sidebarAvatarLbl->setObjectName("sidebarAvatarLbl");
        sidebarAvatarLbl->setFixedSize(60, 60);
        sidebarAvatarLbl->setAlignment(Qt::AlignCenter);
        identityLayout->addWidget(sidebarAvatarLbl);

        auto *identityTextLayout = new QVBoxLayout;
        identityTextLayout->setContentsMargins(0, 2, 0, 2);
        identityTextLayout->setSpacing(5);
        identityTextLayout->addWidget(ui->userNameLbl);
        identityTextLayout->addWidget(ui->detailLbl, 0, Qt::AlignLeft);
        identityTextLayout->addStretch();
        identityLayout->addLayout(identityTextLayout, 1);
        ui->verticalLayout->insertWidget(0, identityRow);
    }

    if (editProfileBtn == nullptr) {
        editProfileBtn = new QPushButton("编辑资料", ui->userProfileWidget);
        editProfileBtn->setObjectName("editProfileBtn");
        editProfileBtn->setCursor(Qt::PointingHandCursor);
        editProfileBtn->setFlat(true);
        ui->verticalLayout->addWidget(editProfileBtn);
        connect(editProfileBtn, &QPushButton::clicked, this,
                &MainWindow::openEditProfileDialog);
    }

    if (logoutBtn == nullptr) {
        logoutBtn = new QPushButton("退出登录", ui->sidebarFrame);
        logoutBtn->setObjectName("logoutBtn");
        logoutBtn->setCursor(Qt::PointingHandCursor);
        logoutBtn->setFixedHeight(42);
        logoutBtn->setToolTip("退出当前账号并返回登录页");
        ui->verticalLayout_3->addWidget(logoutBtn);
        connect(logoutBtn, &QPushButton::clicked,
                this, &MainWindow::logout);
    }

    ui->verticalLayout->setContentsMargins(16, 15, 16, 15);
    ui->verticalLayout->setSpacing(7);
    ui->verticalLayout_3->setContentsMargins(20, 24, 20, 22);
    ui->verticalLayout_3->setSpacing(18);
    ui->verticalLayout_2->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout_2->setSpacing(8);
    ui->verticalSpacer_2->changeSize(20, 6, QSizePolicy::Minimum,
                                     QSizePolicy::Fixed);
    updateSidebarAvatar();

    const QList<QPushButton *> navButtonsForSize = {
        ui->navHomeBtn, ui->navCourseBtn, ui->navExpBtn, ui->navExportBtn};
    for (QPushButton *button : navButtonsForSize) {
        button->setFixedHeight(48);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setFlat(true);
        button->setAutoDefault(false);
        button->setDefault(false);
        button->setAttribute(Qt::WA_StyledBackground, true);
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
    ui->headerSubtitleLbl->setText("把每一段成长，整理成清晰的轨迹。");
    setActiveNav(ui->navHomeBtn);

    connect(ui->navHomeBtn, &QPushButton::clicked, this, [=] {
        hideResumeTemplateMagnifier();
        ui->stackedWidget->setCurrentIndex(0);
        ui->currentPageLbl->setText("首页总览");
        ui->headerSubtitleLbl->setText("把每一段成长，整理成清晰的轨迹。");
        setActiveNav(ui->navHomeBtn);
        updateHomePageStats();
    });
    connect(ui->navCourseBtn, &QPushButton::clicked, this, [=] {
        hideResumeTemplateMagnifier();
        ui->stackedWidget->setCurrentIndex(1);
        ui->currentPageLbl->setText("课程与成绩");
        ui->headerSubtitleLbl->setText("记录课程、学分与绩点，随时看见学习节奏。");
        setActiveNav(ui->navCourseBtn);
    });
    connect(ui->navExpBtn, &QPushButton::clicked, this, [=] {
        hideResumeTemplateMagnifier();
        ui->stackedWidget->setCurrentIndex(2);
        ui->currentPageLbl->setText("经历与荣誉");
        ui->headerSubtitleLbl->setText("把课堂之外的投入，沉淀成可复用的履历。");
        setActiveNav(ui->navExpBtn);
    });
    connect(ui->navExportBtn, &QPushButton::clicked, this, [=] {
        hideResumeTemplateMagnifier();
        ui->stackedWidget->setCurrentIndex(3);
        ui->currentPageLbl->setText("简历导出");
        ui->headerSubtitleLbl->setText("整理关键信息，生成一份真正属于你的简历。");
        setActiveNav(ui->navExportBtn);
        loadResumeProfile();
    });

    connect(resetCourseBtn, &QPushButton::clicked,
            this, &MainWindow::resetAllCourses);

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
                    "<pre>课程名称,学分,成绩,学期,核心课程</pre>"
                    "<h3>字段说明</h3>"
                    "<table>"
                    "<tr><td><b>课程名称</b></td><td>课程名字（必填）</td></tr>"
                    "<tr><td><b>学分</b></td><td>数值，必须 &gt; 0（必填）</td></tr>"
                    "<tr><td><b>成绩</b></td><td>数值，范围 0–100（必填）</td></tr>"
                    "<tr><td><b>学期</b></td><td>大一上 / 大一下 / 大二上 / 大二下<br>"
                    "大三上 / 大三下 / 大四上 / 大四下（必填）</td></tr>"
                    "<tr><td><b>核心课程</b></td><td>是 / 否；标记是否展示在简历中</td></tr>"
                    "</table>"
                    "<h3>示例</h3>"
                    "<pre>课程名称,学分,成绩,学期,核心课程\n"
                    "高等数学,4,92,大一上,是\n"
                    "线性代数,3,85,大一下,否</pre>"
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
                    "课程名称,学分,成绩,学期,核心课程\n"
                    "高等数学,4,92,大一上,是\n"
                    "线性代数,3,85,大一下,否\n"
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
                    "<tr><td>核心课程</td><td>是/否；标记是否展示在简历中</td></tr>"
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
        updateSidebarAvatar();
    }
}

void MainWindow::updateSidebarAvatar() {
    if (!sidebarAvatarLbl)
        return;

    if (!m_photoPath.isEmpty()) {
        const QString fullPath =
            QDir(QCoreApplication::applicationDirPath()).filePath(m_photoPath);
        const QPixmap source(fullPath);
        if (!source.isNull()) {
            constexpr int size = 60;
            sidebarAvatarLbl->setPixmap(
                circularHiResPixmap(source, size));
            sidebarAvatarLbl->setText(QString());
            sidebarAvatarLbl->setToolTip("个人头像");
            return;
        }
    }

    sidebarAvatarLbl->setPixmap(QPixmap());
    QString initial = User::getInstance().getUsername().trimmed().left(1);
    if (initial.isEmpty())
        initial = QStringLiteral("我");
    sidebarAvatarLbl->setText(initial.toUpper());
    sidebarAvatarLbl->setToolTip("尚未设置个人头像");
}

void MainWindow::openEditProfileDialog() {
    User &user = User::getInstance();
    if (!user.isLoggedIn())
        return;

    QVariantMap primaryEducation;
    const QVariantList educationRecords =
        DatabaseManager::getInstance().getEducationRecords(user.getId());
    for (const QVariant &value : educationRecords) {
        const QVariantMap education = value.toMap();
        if (education.value("school").toString() == user.getSchool() &&
            education.value("major").toString() == user.getMajor()) {
            primaryEducation = education;
            break;
        }
    }
    if (primaryEducation.isEmpty() && !educationRecords.isEmpty())
        primaryEducation = educationRecords.first().toMap();

    auto yearFromDate = [](const QString &date) {
        const QRegularExpressionMatch match =
            QRegularExpression("(?:19|20)\\d{2}").match(date);
        return match.hasMatch() ? match.captured(0) : QString();
    };

    QDialog dialog(this);
    dialog.setWindowTitle("修改个人信息");
    dialog.setMinimumWidth(480);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 24, 26, 22);
    layout->setSpacing(16);

    auto *title = new QLabel("修改个人信息");
    title->setStyleSheet("font-size:22px; font-weight:900; color:#0F172A;");
    layout->addWidget(title);

    auto *hint = new QLabel("用户名暂不修改。保存后左上角信息会立即刷新。");
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#64748B; font-size:13px; font-weight:700;");
    layout->addWidget(hint);

    auto *photoSection = new QFrame(&dialog);
    photoSection->setObjectName("profilePhotoSection");
    photoSection->setStyleSheet(
        "QFrame#profilePhotoSection { background:#FFFEFA;"
        "border:1px solid #DED8CC; border-radius:12px; }");
    auto *photoLayout = new QHBoxLayout(photoSection);
    photoLayout->setContentsMargins(16, 14, 16, 14);
    photoLayout->setSpacing(14);

    auto *avatarPreview = new QLabel(photoSection);
    avatarPreview->setFixedSize(72, 72);
    avatarPreview->setAlignment(Qt::AlignCenter);
    avatarPreview->setStyleSheet(
        "background:#E5EEE9; color:#315C53; border:2px solid #8AA89F;"
        "border-radius:36px; font-size:22px; font-weight:850;");
    photoLayout->addWidget(avatarPreview);

    auto *photoTextLayout = new QVBoxLayout;
    photoTextLayout->setSpacing(3);
    auto *photoTitle = new QLabel("个人照片", photoSection);
    photoTitle->setStyleSheet(
        "color:#25332F; font-size:15px; font-weight:800;");
    auto *photoHint = new QLabel(
        "这张照片会显示在左侧资料卡，并用于简历导出。", photoSection);
    photoHint->setWordWrap(true);
    photoHint->setStyleSheet(
        "color:#7A827E; font-size:12px; font-weight:550;");
    photoTextLayout->addWidget(photoTitle);
    photoTextLayout->addWidget(photoHint);
    photoTextLayout->addStretch();
    photoLayout->addLayout(photoTextLayout, 1);

    auto *photoButtons = new QVBoxLayout;
    photoButtons->setSpacing(7);
    auto *changePhotoBtn = new QPushButton("更换照片", photoSection);
    changePhotoBtn->setCursor(Qt::PointingHandCursor);
    changePhotoBtn->setStyleSheet(
        "QPushButton { min-height:32px; background:#1F6B5B; color:#FFF;"
        "border:1px solid #1F6B5B; border-radius:8px; padding:0 14px;"
        "font-size:12px; font-weight:750; }"
        "QPushButton:hover { background:#174F44; }");
    auto *removePhotoBtn = new QPushButton("移除照片", photoSection);
    removePhotoBtn->setCursor(Qt::PointingHandCursor);
    removePhotoBtn->setStyleSheet(
        "QPushButton { min-height:30px; background:transparent;"
        "color:#A8443F; border:none; padding:0 8px;"
        "font-size:12px; font-weight:700; }"
        "QPushButton:hover { background:#F8E7E4; border-radius:7px; }");
    photoButtons->addWidget(changePhotoBtn);
    photoButtons->addWidget(removePhotoBtn);
    photoButtons->addStretch();
    photoLayout->addLayout(photoButtons);
    layout->addWidget(photoSection);

    const QString profileInitial = user.getUsername().trimmed().left(1);
    auto refreshPhotoPreview = [this, avatarPreview, profileInitial]() {
        if (!m_photoPath.isEmpty()) {
            const QString fullPath =
                QDir(QCoreApplication::applicationDirPath()).filePath(m_photoPath);
            const QPixmap source(fullPath);
            if (!source.isNull()) {
                constexpr int size = 72;
                avatarPreview->setPixmap(
                    circularHiResPixmap(source, size));
                avatarPreview->setText(QString());
                return;
            }
        }
        avatarPreview->setPixmap(QPixmap());
        avatarPreview->setText(profileInitial.isEmpty()
                                   ? QStringLiteral("我")
                                   : profileInitial.toUpper());
    };
    refreshPhotoPreview();

    connect(changePhotoBtn, &QPushButton::clicked, &dialog,
            [this, refreshPhotoPreview]() {
                if (selectPhotoBtn)
                    selectPhotoBtn->click();
                refreshPhotoPreview();
            });
    connect(removePhotoBtn, &QPushButton::clicked, &dialog,
            [this, &dialog, refreshPhotoPreview]() {
                if (m_photoPath.isEmpty())
                    return;
                if (QMessageBox::question(
                        &dialog, "移除照片",
                        "确定移除当前个人照片吗？简历中的照片也会同步移除。")
                    != QMessageBox::Yes) {
                    return;
                }

                QFile::remove(
                    QDir(QCoreApplication::applicationDirPath())
                        .filePath(m_photoPath));
                m_photoPath.clear();
                if (photoPreviewLbl) {
                    photoPreviewLbl->setPixmap(QPixmap());
                    photoPreviewLbl->setText("暂无照片");
                }
                saveResumeToDb();
                refreshPhotoPreview();
            });

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

    auto *educationYearsWidget = new QWidget(&dialog);
    auto *educationYearsLayout = new QHBoxLayout(educationYearsWidget);
    educationYearsLayout->setContentsMargins(0, 0, 0, 0);
    educationYearsLayout->setSpacing(8);

    auto *startYearEdit = new QLineEdit(
        yearFromDate(primaryEducation.value("start_date").toString()),
        educationYearsWidget);
    auto *endYearEdit = new QLineEdit(
        yearFromDate(primaryEducation.value("end_date").toString()),
        educationYearsWidget);
    const QRegularExpression yearPattern("(?:19|20)\\d{2}");
    startYearEdit->setValidator(
        new QRegularExpressionValidator(yearPattern, startYearEdit));
    endYearEdit->setValidator(
        new QRegularExpressionValidator(yearPattern, endYearEdit));
    startYearEdit->setPlaceholderText("20xx");
    endYearEdit->setPlaceholderText("20xx");
    startYearEdit->setMaxLength(4);
    endYearEdit->setMaxLength(4);
    startYearEdit->setMaximumWidth(90);
    endYearEdit->setMaximumWidth(90);

    educationYearsLayout->addWidget(startYearEdit);
    educationYearsLayout->addWidget(new QLabel("年 ～", educationYearsWidget));
    educationYearsLayout->addWidget(endYearEdit);
    educationYearsLayout->addWidget(new QLabel("年", educationYearsWidget));
    educationYearsLayout->addStretch();

    form->addRow("学校：", schoolEdit);
    form->addRow("年级：", gradeBox);
    form->addRow("性别：", genderBox);
    form->addRow("专业：", majorEdit);
    form->addRow("就读时间：", educationYearsWidget);

    auto *phoneEdit = new QLineEdit(user.getPhone(), &dialog);
    phoneEdit->setPlaceholderText("例如：13800138000");

    auto *emailEdit = new QLineEdit(user.getEmail(), &dialog);
    emailEdit->setPlaceholderText("例如：zhangsan@example.com");

    auto *jobTargetEdit = new QLineEdit(user.getJobTarget(), &dialog);
    jobTargetEdit->setPlaceholderText("例如：后端开发工程师");

    auto *websiteEdit = new QLineEdit(user.getWebsite(), &dialog);
    websiteEdit->setPlaceholderText("例如：https://github.com/zhangsan");

    form->addRow("电话：", phoneEdit);
    form->addRow("邮箱：", emailEdit);
    form->addRow("求职方向：", jobTargetEdit);
    form->addRow("个人网站：", websiteEdit);
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
    QString startYear = startYearEdit->text().trimmed();
    QString endYear = endYearEdit->text().trimmed();
    QString phone = phoneEdit->text().trimmed();
    QString email = emailEdit->text().trimmed();
    QString jobTarget = jobTargetEdit->text().trimmed();
    QString website = websiteEdit->text().trimmed();

    if (school.isEmpty() || major.isEmpty()) {
        QMessageBox::warning(this, "提示", "学校和专业不能为空");
        return;
    }
    if (startYear.isEmpty() != endYear.isEmpty()) {
        QMessageBox::warning(this, "提示",
                             "请同时填写入学年份和毕业年份");
        return;
    }
    if (!startYear.isEmpty() && startYear.toInt() > endYear.toInt()) {
        QMessageBox::warning(this, "提示",
                             "入学年份不能晚于毕业年份");
        return;
    }

    if (DatabaseManager::getInstance().updateUserInfo(user.getId(), grade,
                                                      gender, major, school,
                                                      startYear, endYear,
                                                      phone, email,
                                                      jobTarget, website)) {
        user.refresh();
        updateSidebarUserInfo();
        QMessageBox::information(this, "修改成功", "个人信息已经更新");
    } else {
        QMessageBox::critical(this, "修改失败", "数据库写入失败，请稍后再试");
    }
}

void MainWindow::logout() {
    const auto result = QMessageBox::question(
        this, "退出登录", "确定要退出当前账号并返回登录页面吗？",
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (result != QMessageBox::Yes)
        return;

    User::getInstance().logout();
    emit logoutRequested();
}

// ==================== 课程页 ====================

void MainWindow::InitCoursePage() {
    int userId = User::getInstance().getId();

    setupTableView(ui->courseTableView);
    ui->courseTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->courseTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->courseTableView->setToolTip(
        "按住 Cmd（macOS）或 Ctrl（Windows/Linux）可选择多行课程");

    courseModel = new QSqlTableModel(this);
    courseModel->setTable("courses");
    courseModel->setEditStrategy(QSqlTableModel::OnRowChange);
    courseModel->setFilter(QString("user_id = %1").arg(userId));

    courseModel->setHeaderData(2, Qt::Horizontal, "课程名称");
    courseModel->setHeaderData(3, Qt::Horizontal, "学分");
    courseModel->setHeaderData(4, Qt::Horizontal, "成绩");
    courseModel->setHeaderData(5, Qt::Horizontal, "学期");
    courseModel->setHeaderData(6, Qt::Horizontal, "绩点");
    courseModel->setHeaderData(8, Qt::Horizontal, "核心课程");

    // 按学期时间排序（semester_order 列：大一上=0 ... 大四下=7）
    courseModel->setSort(7, Qt::AscendingOrder);

    ui->courseTableView->setModel(courseModel);
    ui->courseTableView->setColumnHidden(0, true);
    ui->courseTableView->setColumnHidden(1, true);
    ui->courseTableView->setColumnHidden(7, true); // 隐藏 semester_order 排序列
    ui->courseTableView->setItemDelegateForColumn(
        8, new CoreCourseDelegate(ui->courseTableView));

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
        bool isCoreCourse = dialog.isCoreCourse();

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
            courseModel->setData(courseModel->index(row, 8),
                                 isCoreCourse ? 1 : 0);

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
    const QModelIndexList selectedRows =
        ui->courseTableView->selectionModel()->selectedRows(0);
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(
            this, "提示",
            "请先选择课程；按住 Cmd（macOS）或 Ctrl（Windows/Linux）可多选");
        return;
    }

    const int selectedCount = selectedRows.size();
    auto result = QMessageBox::question(
        this, "确认删除",
        selectedCount == 1
            ? "确定要删除选中的课程吗？"
            : QString("确定要删除选中的 %1 门课程吗？").arg(selectedCount));
    if (result != QMessageBox::Yes)
        return;

    QList<int> courseIds;
    courseIds.reserve(selectedRows.size());
    for (const QModelIndex &index : selectedRows) {
        const int courseId =
            courseModel->data(courseModel->index(index.row(), 0)).toInt();
        if (courseId > 0)
            courseIds.append(courseId);
    }
    if (courseIds.isEmpty()) {
        QMessageBox::critical(this, "删除失败",
                              "无法读取选中课程的数据库编号。");
        return;
    }

    if (!courseModel->submitAll()) {
        QMessageBox::critical(this, "删除失败",
                              "保存当前课程修改失败，请稍后再试。");
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) {
        QMessageBox::critical(this, "删除失败",
                              "无法开始数据库事务，请稍后再试。");
        return;
    }

    QSqlQuery query(db);
    query.prepare(
        "DELETE FROM courses WHERE id = :id AND user_id = :user_id");
    const int userId = User::getInstance().getId();
    for (int courseId : courseIds) {
        query.bindValue(":id", courseId);
        query.bindValue(":user_id", userId);
        if (!query.exec()) {
            const QString error = query.lastError().text();
            db.rollback();
            courseModel->select();
            QMessageBox::critical(this, "删除失败",
                                  "数据库写入失败：" + error);
            return;
        }
    }

    if (!db.commit()) {
        const QString error = db.lastError().text();
        db.rollback();
        courseModel->select();
        QMessageBox::critical(this, "删除失败",
                              "提交删除失败：" + error);
        return;
    }

    // 重新查询会立即压紧行号，避免被删除的位置残留空白行。
    courseModel->select();
    ui->courseTableView->clearSelection();
    updateTotalStats();
    updateHomePageStats();
}

void MainWindow::resetAllCourses() {
    const int courseCount = courseModel->rowCount();
    if (courseCount == 0) {
        QMessageBox::information(this, "初始化课程", "当前没有课程数据。");
        return;
    }

    const auto result = QMessageBox::warning(
        this, "初始化全部课程",
        QString("此操作将清空当前用户的全部 %1 门课程，且无法撤销。\n\n"
                "确定要继续吗？")
            .arg(courseCount),
        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
    if (result != QMessageBox::Yes)
        return;

    QSqlQuery query;
    query.prepare("DELETE FROM courses WHERE user_id = :user_id");
    query.bindValue(":user_id", User::getInstance().getId());
    if (!query.exec()) {
        QMessageBox::critical(this, "初始化失败",
                              "清空课程失败：" + query.lastError().text());
        return;
    }

    courseModel->select();
    ui->courseTableView->clearSelection();
    updateTotalStats();
    updateHomePageStats();
    QMessageBox::information(this, "初始化完成",
                             "当前用户的课程数据已全部清空。");
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
    ui->addExpFrame->setFixedHeight(224);
    ui->addAwardFrame->setFixedHeight(224);
    ui->expTableView->setMinimumHeight(250);
    ui->awardTableView->setMinimumHeight(250);

    // ---- 切换标签按钮 ----
    expTabBtn = new QPushButton("课外活动", ui->expPage);
    expTabBtn->setObjectName("expTabBtn");
    expTabBtn->setCursor(Qt::PointingHandCursor);

    awardTabBtn = new QPushButton("个人荣誉", ui->expPage);
    awardTabBtn->setObjectName("awardTabBtn");
    awardTabBtn->setCursor(Qt::PointingHandCursor);
    ui->expToolbarLayout->insertWidget(0, expTabBtn);
    ui->expToolbarLayout->insertWidget(1, awardTabBtn);

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
    for (int column = 6; column < expModel->columnCount(); ++column)
        ui->expTableView->setColumnHidden(column, true);

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
        addAwardAmountLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }
    if (addAwardAmountSpin == nullptr) {
        addAwardAmountSpin = new QDoubleSpinBox(ui->addAwardFrame);
        addAwardAmountSpin->setObjectName("addAwardAmountSpin");
        addAwardAmountSpin->setPrefix("¥ ");
        addAwardAmountSpin->setRange(0.0, 999999.0);
        addAwardAmountSpin->setDecimals(0);
        addAwardAmountSpin->setSingleStep(500.0);
        addAwardAmountSpin->setValue(0.0);
        addAwardAmountSpin->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
    ui->awardFormGrid->addWidget(addAwardAmountLbl, 1, 2);
    ui->awardFormGrid->addWidget(addAwardAmountSpin, 1, 3);

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
    for (int column = 6; column < awardModel->columnCount(); ++column)
        ui->awardTableView->setColumnHidden(column, true);

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

static bool parseCoreCourseValue(const QString &text, bool &isCoreCourse) {
    const QString value = text.trimmed().toLower();
    if (value.isEmpty() || value == "否" || value == "0" ||
        value == "false" || value == "no" || value == "非核心") {
        isCoreCourse = false;
        return true;
    }
    if (value == "是" || value == "1" || value == "true" ||
        value == "yes" || value == "核心") {
        isCoreCourse = true;
        return true;
    }
    return false;
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
    const bool hasCoreCourseColumn =
        header.size() >= 5 && header[4] == "核心课程";
    if (header.size() < 4 ||
        header[0] != expectedHeader[0] ||
        header[1] != expectedHeader[1] ||
        header[2] != expectedHeader[2] ||
        header[3] != expectedHeader[3] ||
        (header.size() >= 5 && !hasCoreCourseColumn)) {
        QMessageBox::warning(this, "格式错误",
            "CSV 表头不匹配。期望的列：\n"
            "课程名称,学分,成绩,学期,核心课程\n\n"
            "旧版四列表头仍可导入，核心课程会默认为“否”。\n\n"
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
    query.prepare(
        "INSERT INTO courses (user_id, name, credit, score, semester, gpa, "
        "semester_order, is_core) "
        "VALUES (:uid, :name, :credit, :score, :semester, :gpa, :order, "
        ":is_core)");

    while (!stream.atEnd()) {
        ++lineNum;
        QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;

        QStringList fields = parseCsvLine(line);
        if (fields.size() < (hasCoreCourseColumn ? 5 : 4)) {
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

        bool isCoreCourse = false;
        if (hasCoreCourseColumn &&
            !parseCoreCourseValue(fields[4], isCoreCourse)) {
            ++failCount;
            continue;
        }

        query.bindValue(":uid", userId);
        query.bindValue(":name", name);
        query.bindValue(":credit", credit);
        query.bindValue(":score", score);
        query.bindValue(":semester", semester);
        query.bindValue(":gpa", scoreToGpa(score));
        query.bindValue(":order", semOrder);
        query.bindValue(":is_core", isCoreCourse ? 1 : 0);

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
    writeCsvRow(stream,
                {"课程名称", "学分", "成绩", "学期", "核心课程"});

    int rowCount = courseModel->rowCount();
    for (int row = 0; row < rowCount; ++row) {
        QString name = courseModel->data(courseModel->index(row, 2)).toString();
        QString credit = courseModel->data(courseModel->index(row, 3)).toString();
        QString score = courseModel->data(courseModel->index(row, 4)).toString();
        QString semester = courseModel->data(courseModel->index(row, 5)).toString();
        QString isCoreCourse =
            courseModel->data(courseModel->index(row, 8)).toBool()
                ? "是" : "否";
        writeCsvRow(stream,
                    {name, credit, score, semester, isCoreCourse});
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
    writeCsvRow(stream,
                {"课程名称", "学分", "成绩", "学期", "核心课程"});
    for (int row = 0; row < courseCount; ++row) {
        QString name     = courseModel->data(courseModel->index(row, 2)).toString();
        QString credit   = courseModel->data(courseModel->index(row, 3)).toString();
        QString score    = courseModel->data(courseModel->index(row, 4)).toString();
        QString semester = courseModel->data(courseModel->index(row, 5)).toString();
        QString isCoreCourse =
            courseModel->data(courseModel->index(row, 8)).toBool()
                ? "是" : "否";
        writeCsvRow(stream,
                    {name, credit, score, semester, isCoreCourse});
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
    bool courseHeaderHasCoreCourse = false;

    int courseOk = 0, courseFail = 0;
    int expOk = 0, expFail = 0;
    int awardOk = 0, awardFail = 0;

    int userId = User::getInstance().getId();

    const QStringList validSemesters = {"大一上","大一下","大二上","大二下",
                                        "大三上","大三下","大四上","大四下"};
    const QStringList validTypes = {"实习", "竞赛", "项目", "其他"};
    const QStringList validLevels = {"国家级", "省级", "校级", "院级"};

    const QStringList expectedCourseHdr =
        {"课程名称", "学分", "成绩", "学期"};
    const QStringList expectedExpHdr = {"标题", "类型", "时间", "描述"};
    const QStringList expectedAwardHdr = {"奖项名称", "荣誉级别", "获奖时间", "奖金金额"};

    // 预编译三条 INSERT 语句
    QSqlQuery courseQuery;
    courseQuery.prepare(
        "INSERT INTO courses (user_id, name, credit, score, semester, gpa, "
        "semester_order, is_core) "
        "VALUES (:uid, :name, :credit, :score, :semester, :gpa, :order, "
        ":is_core)");

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
            if (currentSection == COURSES)
                courseHeaderHasCoreCourse = false;
            continue;
        }

        // 表头验证
        if (!headerValidated) {
            QStringList hdr = parseCsvLine(line);
            switch (currentSection) {
            case COURSES:
                courseHeaderHasCoreCourse =
                    hdr.size() >= 5 && hdr[4] == "核心课程";
                headerBad = !(hdr.size() >= 4 &&
                              hdr[0] == expectedCourseHdr[0] &&
                              hdr[1] == expectedCourseHdr[1] &&
                              hdr[2] == expectedCourseHdr[2] &&
                              hdr[3] == expectedCourseHdr[3] &&
                              (hdr.size() < 5 ||
                               courseHeaderHasCoreCourse));
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
            if (fields.size() <
                (courseHeaderHasCoreCourse ? 5 : 4)) {
                ++courseFail;
                continue;
            }
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

            bool isCoreCourse = false;
            if (courseHeaderHasCoreCourse &&
                !parseCoreCourseValue(fields[4], isCoreCourse)) {
                ++courseFail;
                continue;
            }

            courseQuery.bindValue(":uid", userId);
            courseQuery.bindValue(":name", name);
            courseQuery.bindValue(":credit", credit);
            courseQuery.bindValue(":score", score);
            courseQuery.bindValue(":semester", semester);
            courseQuery.bindValue(":gpa", DatabaseManager::scoreToGpa(score));
            courseQuery.bindValue(":order", semOrder);
            courseQuery.bindValue(":is_core", isCoreCourse ? 1 : 0);

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

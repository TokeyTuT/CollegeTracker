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

    ui->expTableView->setGeometry(28, 18, 754, 230);
    ui->addExpFrame->setGeometry(28, 292, 754, 220);
    ui->addExpTitle->setGeometry(38, 258, 180, 28);

    ui->awardTableView->setGeometry(28, 18, 754, 230);
    ui->addAwardFrame->setGeometry(28, 292, 754, 205);

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
    ui->addAwardBtn->setGeometry(436, 146, 116, 38);
    ui->delAwardBtn->setGeometry(570, 146, 126, 38);

    // 只兼容浅色模式：不用系统深色调色板，所有控件颜色都明确写死。
    const QString qss = R"(
        * {
            font-family: "Microsoft YaHei", "PingFang SC", "Helvetica Neue", Arial;
            font-size: 14px;
            color: #171A1F;
        }
        QMainWindow, QWidget#centralwidget {
            background: #E7EBF0;
        }
        QFrame#sidebarFrame {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                        stop:0 #F6FAFD,
                        stop:0.48 #EEF5FA,
                        stop:1 #E8F0F7);
            border-right: 1px solid rgba(148, 163, 184, 0.24);
        }
        QWidget#userProfileWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                        stop:0 rgba(255,255,255,0.88),
                        stop:0.54 rgba(244,249,253,0.80),
                        stop:1 rgba(232,241,249,0.72));
            border: 1px solid rgba(190, 211, 228, 0.68);
            border-radius: 30px;
        }
        QLabel#userNameLbl {
            color: #172033;
            font-size: 23px;
            font-weight: 900;
            letter-spacing: 0.35px;
            qproperty-alignment: AlignCenter;
            background: transparent;
            border: none;
        }
        QLabel#detailLbl {
            color: #2563EB;
            font-size: 14px;
            font-weight: 850;
            qproperty-alignment: AlignCenter;
            background: rgba(219, 234, 254, 0.66);
            border: 1px solid rgba(147, 197, 253, 0.54);
            border-radius: 14px;
            padding: 3px 12px;
        }
        QLabel#majorLbl {
            color: #64748B;
            font-size: 13px;
            font-weight: 750;
            qproperty-alignment: AlignCenter;
            background: transparent;
            border: none;
            padding: 0px;
        }
        QFrame#mainContentFrame {
            background: #F8FAFC;
            border: none;
            border-left: 1px solid rgba(148, 163, 184, 0.28);
        }
        QLabel#currentPageLbl {
            color: #111827;
            font-size: 28px;
            font-weight: 900;
            letter-spacing: 0.4px;
            qproperty-alignment: AlignLeft | AlignVCenter;
            background: transparent;
            border-left: 6px solid #00B7C7;
            padding-left: 14px;
        }

        /* 左侧导航：胶囊按钮 + 细边框，整体和用户信息卡片自然衔接 */
        #sidebarFrame QPushButton {
            min-height: 58px;
            max-height: 58px;
            border: 1px solid rgba(171, 190, 207, 0.58);
            border-radius: 29px;
            padding-left: 24px;
            padding-right: 18px;
            text-align: left;
            color: #334155;
            background: rgba(255, 255, 255, 0.46);
            font-size: 15px;
            font-weight: 850;
            letter-spacing: 0.25px;
        }
        #sidebarFrame QPushButton:hover {
            background: rgba(255, 255, 255, 0.76);
            border: 1px solid rgba(56, 189, 248, 0.55);
            color: #0F172A;
        }
        #sidebarFrame QPushButton:pressed {
            background: rgba(226,232,240,0.78);
            border: 1px solid rgba(14, 165, 233, 0.46);
        }
        #sidebarFrame QPushButton[active="true"] {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                        stop:0 #2563EB,
                        stop:0.56 #0EA5E9,
                        stop:1 #14B8A6);
            color: #FFFFFF;
            border: 1px solid rgba(255, 255, 255, 0.72);
            border-radius: 29px;
        }

        QPushButton {
            min-height: 36px;
            border: 1px solid #111827;
            border-radius: 4px;
            padding: 0 20px;
            color: #FFFFFF;
            background: #171A1F;
            font-weight: 900;
            letter-spacing: 0.2px;
        }
        QPushButton:hover {
            background: #252B34;
            border-color: #00B7C7;
        }
        QPushButton:pressed { background: #05070A; }
        QPushButton:disabled { background: #CBD0D7; border-color: #CBD0D7; color: #FFFFFF; }
        QPushButton#deleteCourseBtn, QPushButton#DelExpBtn, QPushButton#delAwardBtn {
            background: #FFFFFF;
            color: #D62828;
            border: 1px solid #D62828;
        }
        QPushButton#deleteCourseBtn:hover, QPushButton#DelExpBtn:hover, QPushButton#delAwardBtn:hover {
            background: #D62828;
            color: #FFFFFF;
        }
        QStackedWidget, QWidget#homePage, QWidget#coursePage, QWidget#expPage, QWidget#awardPage, QWidget#profilePage {
            background: transparent;
        }
        QFrame#addExpFrame, QFrame#addAwardFrame {
            background: #FFFFFF;
            border: 1px solid #D2D8E1;
            border-top: 4px solid #171A1F;
            border-radius: 6px;
        }
        QLabel#addExpTitle {
            color: #111827;
            font-size: 20px;
            font-weight: 900;
            background: transparent;
            qproperty-alignment: AlignLeft | AlignVCenter;
        }
        QLabel {
            color: #303846;
            background: transparent;
            font-weight: 700;
        }
        QLineEdit, QComboBox, QDateEdit {
            min-height: 36px;
            border: 1px solid #B9C1CE;
            border-radius: 4px;
            padding: 0 12px;
            background: #FFFFFF;
            color: #111827;
            selection-background-color: #00B7C7;
            selection-color: #FFFFFF;
        }
        QLineEdit:hover, QComboBox:hover, QDateEdit:hover {
            border: 1px solid #00B7C7;
            background: #FBFCFD;
        }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus {
            border: 2px solid #00B7C7;
            background: #FFFFFF;
        }
        QComboBox::drop-down, QDateEdit::drop-down {
            border: none;
            width: 28px;
        }
        QComboBox QAbstractItemView {
            background: #FFFFFF;
            border: 1px solid #B9C1CE;
            selection-background-color: #171A1F;
            selection-color: #FFFFFF;
            outline: none;
        }
        QTableView {
            background: #FFFFFF;
            alternate-background-color: #F4F6F8;
            border: 1px solid #D2D8E1;
            border-top: 4px solid #171A1F;
            border-radius: 6px;
            gridline-color: transparent;
            selection-background-color: #DFFBFF;
            selection-color: #111827;
            padding: 6px;
            outline: none;
        }
        QTableView::item {
            min-height: 36px;
            padding: 8px;
            border-bottom: 1px solid #E8ECF1;
        }
        QHeaderView::section {
            background: #171A1F;
            color: #FFFFFF;
            border: none;
            border-right: 1px solid #343B46;
            padding: 12px 10px;
            font-size: 13px;
            font-weight: 900;
            letter-spacing: 0.3px;
        }
        QTableCornerButton::section {
            background: #171A1F;
            border: none;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 10px;
            margin: 6px 2px 6px 0px;
        }
        QScrollBar::handle:vertical {
            background: #9AA4B2;
            border-radius: 3px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover { background: #00B7C7; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QLabel#labelStats {
            background: #FFFFFF;
            border: 1px solid #D2D8E1;
            border-left: 5px solid #00B7C7;
            border-radius: 4px;
            color: #111827;
            font-size: 15px;
            font-weight: 900;
            qproperty-alignment: AlignCenter;
        }
        QMessageBox, QDialog {
            background: #FFFFFF;
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
    // 首页暂时保持空白，后续可按需求继续添加内容。
    if (ui->homePage->layout() != nullptr) return;
    auto *layout = new QVBoxLayout(ui->homePage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

void MainWindow::buildExportPage() {
    if (ui->profilePage->layout() != nullptr) return;

    // 简历导出页面正文先保持空白，后续按需求再添加导出内容。
    auto *layout = new QVBoxLayout(ui->profilePage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

void MainWindow::InitFrame() {
    ui->sidebarFrame->setFixedWidth(238);
    ui->userProfileWidget->setFixedHeight(158);
    ui->userNameLbl->setMinimumHeight(42);
    ui->detailLbl->setMinimumHeight(28);
    ui->majorLbl->setMinimumHeight(26);
    ui->userNameLbl->setWordWrap(false);
    ui->detailLbl->setWordWrap(false);
    ui->majorLbl->setWordWrap(false);
    ui->verticalLayout->setContentsMargins(22, 22, 22, 18);
    ui->verticalLayout->setSpacing(8);
    ui->verticalLayout_3->setContentsMargins(14, 18, 14, 18);
    ui->verticalLayout_3->setSpacing(18);
    ui->verticalLayout_2->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout_2->setSpacing(14);
    ui->verticalSpacer_2->changeSize(20, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);

    const QList<QPushButton*> navButtonsForSize = {
        ui->navHomeBtn, ui->navCourseBtn, ui->navExpBtn, ui->navAwardBtn, ui->navExportBtn
    };
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
        ui->majorLbl->setText(user.getMajor());
        QString detail = user.getGrade();
        if (!user.getGender().isEmpty()) {
            detail = user.getGrade() + " | " + user.getGender();
        }
        ui->detailLbl->setText(detail);
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

    ui->courseTableView->setModel(courseModel);
    ui->courseTableView->setColumnHidden(0, true);
    ui->courseTableView->setColumnHidden(1, true);

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

        if (name.isEmpty()) return;

        int row = courseModel->rowCount();
        if (courseModel->insertRow(row)) {
            courseModel->setData(courseModel->index(row, 1), userId);
            courseModel->setData(courseModel->index(row, 2), name);
            courseModel->setData(courseModel->index(row, 3), credit);
            courseModel->setData(courseModel->index(row, 4), score);
            courseModel->setData(courseModel->index(row, 5), semester);

            if (courseModel->submitAll()) {
                ui->courseTableView->selectRow(row);
                qDebug() << "成功录入：" << name << " | " << semester;
                updateTotalStats();
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

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "DatabaseMannager.h"
#include "User.h"
#include "AddCourseDialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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

void MainWindow::InitFrame() {
    QString defaultTitle = QString("首页总览");
    ui->stackedWidget->setCurrentIndex(0);
    ui->currentPageLbl->setText(defaultTitle);
    connect(ui->navHomeBtn, &QPushButton::clicked, this, [=] {
        ui->stackedWidget->setCurrentIndex(0);
    });
    connect(ui->navCourseBtn, &QPushButton::clicked, this, [=] {
        ui->stackedWidget->setCurrentIndex(1);
    });
    connect(ui->navExpBtn, &QPushButton::clicked, this, [=] {
        ui->stackedWidget->setCurrentIndex(2);
    });
    connect(ui->navAwardBtn, &QPushButton::clicked, this, [=] {
        ui->stackedWidget->setCurrentIndex(3);
    });
    connect(ui->navExportBtn, &QPushButton::clicked, this, [=] {
        ui->stackedWidget->setCurrentIndex(4);
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

    ui->addExpTitleLine->setPlaceholderText("示例:家里蹲算法工程师）");
    ui->addExpDescLine->setPlaceholderText("示例:天天蹲家里打代码😃");

    ui->expTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->expTableView->setSelectionMode(QAbstractItemView::SingleSelection);

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

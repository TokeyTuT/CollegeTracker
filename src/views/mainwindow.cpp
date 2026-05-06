#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include"AddCourseDialog.h"

#include "DatabaseMannager.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{


    ui->setupUi(this);

    //设表
    setupCourseModel();
    //设置初始时主页面的标题与页面
    QString defaultTitle = QString("首页总览");
    ui->stackedWidget->setCurrentIndex(0);
    //更新状态栏
    updateTotalStats();
    //将左侧导航栏的按钮与翻页关联
    ui->currentPageLbl->setText(defaultTitle);
    connect(ui->navHomeBtn,&QPushButton::clicked,this,[=]{
        ui->stackedWidget->setCurrentIndex(0);
    });
    connect(ui->navCourseBtn,&QPushButton::clicked,this,[=]{
        ui->stackedWidget->setCurrentIndex(1);
    });
    connect(ui->navExpBtn,&QPushButton::clicked,this,[=]{
        ui->stackedWidget->setCurrentIndex(2);
    });
    connect(ui->navAwardBtn,&QPushButton::clicked,this,[=]{
        ui->stackedWidget->setCurrentIndex(3);
    });
    connect(ui->navExportBtn,&QPushButton::clicked,this,[=]{
        ui->stackedWidget->setCurrentIndex(4);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_navHomeBtn_clicked()
{
    ui->currentPageLbl->setText(ui->navHomeBtn->text());
}


void MainWindow::on_navCourseBtn_clicked()
{
    ui->currentPageLbl->setText(ui->navCourseBtn->text());
}


void MainWindow::on_navExpBtn_clicked()
{
    ui->currentPageLbl->setText(ui->navExpBtn->text());
}


void MainWindow::on_navExportBtn_clicked()
{
    ui->currentPageLbl->setText(ui->navExportBtn->text());
}


void MainWindow::on_navAwardBtn_clicked()
{
    ui->currentPageLbl->setText(ui->navAwardBtn->text());
}


//更新结果
void MainWindow::updateTotalStats() {
    // 直接管数据库要结果
    QVariantMap stats = DatabaseManager::getInstance().getTotalStats();

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


void MainWindow::setupCourseModel()
{
    // 确保点击单元格时是选中整行，而不是只选中一个格

    ui->courseTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    // 限制用户只能选中一行，方便下面单行删除
    ui->courseTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    //初始化模型
    courseModel = new QSqlTableModel(this);
    courseModel->setTable("courses");

    //设置策略 —— 点击保存后才提交
    courseModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    //设置表头
    courseModel->setHeaderData(1, Qt::Horizontal, "课程名称");
    courseModel->setHeaderData(2, Qt::Horizontal, "学分");
    courseModel->setHeaderData(3, Qt::Horizontal, "成绩");
    courseModel->setHeaderData(4, Qt::Horizontal, "学期");

    //将模型添加到 Table View 中
    ui->courseTableView->setModel(courseModel);

    //隐藏主键
    ui->courseTableView->setColumnHidden(0,true);

    //数据查询
    courseModel->select();
}


//添加课程
void MainWindow::on_addCourseBtn_clicked() {
    AddCourseDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.getName();
        double credit = dialog.getCredit();
        double score = dialog.getScore();
        QString semester = dialog.getSemester();

        if (name.isEmpty()) return;

        int row = courseModel->rowCount();
        if (courseModel->insertRow(row)) {
            // 按照数据库字段顺序填充
            courseModel->setData(courseModel->index(row, 1), name);
            courseModel->setData(courseModel->index(row, 2), credit);
            courseModel->setData(courseModel->index(row, 3), score);
            courseModel->setData(courseModel->index(row, 4), semester);

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
//添加经历，指定三种经历 —— 实习，竞赛，项目
void MainWindow::on_addExpBtn_clicked()
{

}

// 删除选定的列(单行删除)
void MainWindow::on_deleteCourseBtn_clicked()
{
    QModelIndex currentIndex = ui->courseTableView->currentIndex();
    if(!currentIndex.isValid()){
        QMessageBox::warning(this, "提示", "请先在表格中点击选择一行内容");
        return;
    }

    auto result = QMessageBox::question(this, "确认删除", "确定要删除该门课程吗？");
    if (result != QMessageBox::Yes) return;

    // 执行删除
    int row = currentIndex.row();
    courseModel->removeRow(row);

    // 提交更改到数据库文件
    if (courseModel->submitAll()) {
        qDebug() << "行号" << row << "已成功从数据库抹除";
        updateTotalStats();
    } else {
        courseModel->revertAll(); // 提交失败则恢复界面状态
        qDebug() << "提交失败";
    }
}





#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "DatabaseMannager.h"
#include"addCourseDialog.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{


    ui->setupUi(this);

    //初始化 sideBar 与 mainFrame
    InitFrame();

    //初始化课程页
    InitCoursePage();

    //初始化课外经历页
    InitExpPage();

    //初始化个人荣誉页（待写）
    InitAwardPage();

}



MainWindow::~MainWindow()
{
    delete courseModel;
    delete ui;
}

//初始化总体 UI 框架
void MainWindow::InitFrame(){
    //设置初始时主页面的标题与页面
    QString defaultTitle = QString("首页总览");
    ui->stackedWidget->setCurrentIndex(0);
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

//初始化课程页
void MainWindow::InitCoursePage()
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
    //更新状态栏
    updateTotalStats();
}

//初始化个人经历页
void MainWindow::InitExpPage()
{
    //给添加addExpCBox设置下拉菜单
    ui->addExpTypeCBox->addItem("实习");
    ui->addExpTypeCBox->addItem("竞赛");
    ui->addExpTypeCBox->addItem("项目");
    ui->addExpTypeCBox->addItem("其他");
    ui->addExpTypeCBox->setCurrentIndex(0);//设置起始索引
    //添加课外生活时，给日期遍历组件初始化
    ui->addExpDateLine->setCalendarPopup(true); //弹出日期选择框
    ui->addExpDateLine->setDate(QDate::currentDate()); //默认显示今天
    ui->addExpDateLine->setDisplayFormat("yyyy-MM-dd");//显示的格式

    //给文本框添加默认文字
    ui->addExpTitleLine->setPlaceholderText("示例:家里蹲算法工程师）");
    ui->addExpDescLine->setPlaceholderText("示例:天天蹲家里打代码😃");


    //初始化 expTableView
    ui->expTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->expTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    // 初始化模型
    expModel = new QSqlTableModel(this);
    expModel->setTable("Experiences");

    // 设置车落，点击保存后才能提交
    expModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    //设置表头
    expModel->setHeaderData(1,Qt::Horizontal,"标题");
    expModel->setHeaderData(2,Qt::Horizontal,"类型");
    expModel->setHeaderData(3,Qt::Horizontal,"时间");
    expModel->setHeaderData(4,Qt::Horizontal,"描述");

    //将模型添加到 TableView 中
    ui->expTableView->setModel(expModel);
    ui->expTableView->setColumnHidden(0,true);

    //数据查询
    expModel->select();
}

//参考上面的 InitExpPage 和 InitCoursePage 初始化
void MainWindow::InitAwardPage()
{
    //给 addAwardCBox 设置下拉菜单


    //给日期遍历组件初始化


    //给文本框添加默认文字(可选，可写可不写)


    //初始化 awardTableView


    //初始化模型


    //设置表头(奖项等级、奖项标题、获奖时间)


    //数据查询

}




//将左侧 sidebar 栏导航按钮绑定到各自的 page 中
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



//以下为课程页的实现代码
//课程列状态栏更新结果（每次删除，添加更新成绩信息）
void MainWindow::updateTotalStats() {
    //从数据库中查找
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
        QMessageBox::critical(this, "错误", "数据库写入失败！");
    }
}




//以下为课外经历的代码
//添加经历，指定三种经历 —— 实习，竞赛，项目，其他
void MainWindow::on_addExpBtn_clicked()
{
    QString title = ui->addExpTitleLine->text();
    QString type = ui->addExpTypeCBox->currentText();
    QDate temp_date = ui->addExpDateLine->date();
    if(temp_date > QDate::currentDate()){ // 不能选比今天还大的日期
        QMessageBox::warning(this, "提示", "不能选择比今天还大的日期！");
    }
    else{
        QString desc = ui->addExpDescLine->text();


        //存储日期的时候要用字符串，所以还要格式化成字符串s
        QString date = temp_date.toString("yyyy-MM-dd");

        if(type.isEmpty() || title.isEmpty()){
            QMessageBox::warning(this, "提示", "标题不能为空！");
        }
        else{
            int row = expModel->rowCount();
            if(expModel->insertRow(row)){
                // 按照数据库填充
                expModel->setData(expModel->index(row,1),title);
                expModel->setData(expModel->index(row,2),type);
                expModel->setData(expModel->index(row,3),date);
                expModel->setData(expModel->index(row,4),desc);

                if(expModel->submitAll()){
                    ui->expTableView->selectRow(row);
                    qDebug()<<"写入成功";
                }else{
                    qDebug()<<"数据库写入失败";
                }
            }
        }
    }
    //清空输入框
    ui->addExpTitleLine->clear();
    ui->addExpDateLine->setDate(QDate::currentDate());
    ui->addExpTypeCBox->setCurrentIndex(0);
    ui->addExpDescLine->clear();
}

//删除选定的列 (单行删除)
void MainWindow::on_DelExpBtn_clicked()
{
    QModelIndex currentIndex = ui->expTableView->currentIndex();
    if(!currentIndex.isValid()){
        QMessageBox::warning(this, "提示", "请先在表格中点击选择一行内容");
        return;
    }
    auto result = QMessageBox::question(this,"确认删除","确定要删除该经历吗?");
    if(result!=QMessageBox::Yes){
        return;
    }

    //执行删除
    int row = currentIndex.row();
    expModel->removeRow(row);

    //提交
    if(expModel->submitAll()){
        qDebug()<<"行号"<<row<<"已从数据库中抹除";
    }else{
        //失败就回溯恢复界面
        expModel->revert();
        qDebug()<<"提交失败";
        QMessageBox::critical(this, "错误", "数据库写入失败！");
    }
}

//以下为个人荣誉的代码
//添加荣誉
void MainWindow::on_addAwardBtn_clicked()
{
    //参考上面添加经历的代码，基本上一样，只要改几个关键参数即可
}

// 删除选定行的列(单行删除)
void MainWindow::on_delAwardBtn_clicked()
{
    //参考上面删除经历的代码，基本上一样，只要改几个关键参数即可
}


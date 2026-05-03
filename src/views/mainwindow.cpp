#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{


    ui->setupUi(this);
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


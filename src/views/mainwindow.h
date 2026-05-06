#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QSqlTableModel>
#include<QMessageBox>
#include<QInputDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;



private slots:
    void on_navHomeBtn_clicked();

    void on_navCourseBtn_clicked();

    void on_navExpBtn_clicked();

    void on_navExportBtn_clicked();

    void on_navAwardBtn_clicked();


    void on_addCourseBtn_clicked();

    void on_deleteCourseBtn_clicked();

    void on_addExpBtn_clicked();

    void on_DelExpBtn_clicked();

private:
    Ui::MainWindow *ui;
    QSqlTableModel *courseModel; //从数据库中读取课程表，转为模型
    QSqlTableModel *expModel; // 从数据库中读取经历表，转为模型

    void InitFrame(); //初始化导航栏

    void InitCoursePage(); //初始化课程页

    void InitExpPage(); //初始化课外经历页


    void updateTotalStats(); //更新课程状态栏  —— (计算 GPA)
};
#endif // MAINWINDOW_H

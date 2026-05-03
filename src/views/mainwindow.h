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

private:
    Ui::MainWindow *ui;
    QSqlTableModel *courseModel; //从数据库中读取课程表，转为模型
    void setupCourseModel();
    void updateTotalStats();
};
#endif // MAINWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlTableModel>
#include <QMessageBox>
#include <QInputDialog>
#include <QGridLayout>
#include <QFrame>

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
    void on_navLogoutBtn_clicked();
    void on_addCourseBtn_clicked();
    void on_deleteCourseBtn_clicked();
    void on_addExpBtn_clicked();
    void on_DelExpBtn_clicked();
    void on_addAwardBtn_clicked();
    void on_delAwardBtn_clicked();

signals:
    void loggedOut();

private:
    Ui::MainWindow *ui;
    QSqlTableModel *courseModel;
    QSqlTableModel *expModel;
    QSqlTableModel *awardModel;

    // Dashboard 卡片数值标签
    QLabel *dashCourseCount;
    QLabel *dashAvgScore;
    QLabel *dashGPA;
    QLabel *dashTotalCredits;
    QLabel *dashExpCount;
    QLabel *dashAwardCount;

    void InitFrame();
    void InitHomePage();
    void InitCoursePage();
    void InitExpPage();
    void InitAwardPage();

    void updateTotalStats();
    void updateSidebarUserInfo();
    void setActiveNavButton(QPushButton *activeBtn);
};

#endif // MAINWINDOW_H

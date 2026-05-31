#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlTableModel>
#include <QMessageBox>
#include <QInputDialog>

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

    void InitFrame();
    void InitCoursePage();
    void InitExpPage();
    void InitAwardPage();

    void updateTotalStats();
    void updateSidebarUserInfo();
};

#endif // MAINWINDOW_H

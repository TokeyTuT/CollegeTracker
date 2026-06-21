#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class CoursePage;
class ExperiencePage;
class HomePage;
class QLabel;
class QPushButton;
class ResumePage;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

signals:
    void logoutRequested();

private:
    Ui::MainWindow *ui;
    HomePage *m_homePage = nullptr;
    CoursePage *m_coursePage = nullptr;
    ExperiencePage *m_experiencePage = nullptr;
    ResumePage *m_resumePage = nullptr;
    QLabel *m_sidebarAvatarLabel = nullptr;
    QPushButton *m_editProfileButton = nullptr;
    QPushButton *m_logoutButton = nullptr;

    void createPages();
    void initFrame();
    void applyModernStyle();
    void setCurrentPage(int index, const QString &title,
                        const QString &subtitle, QPushButton *activeButton);
    void updateSidebarUserInfo();
    void updateSidebarAvatar();
    void openEditProfileDialog();
    void logout();
};

#endif // MAINWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlTableModel>
#include <QMessageBox>
#include <QInputDialog>
#include <QTableView>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QDoubleSpinBox>

class QShowEvent;
class QResizeEvent;
class QEvent;
class QComboBox;
class QToolButton;
class ResumeExporter;

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

signals:
    void logoutRequested();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_navHomeBtn_clicked();
    void on_navCourseBtn_clicked();
    void on_navExpBtn_clicked();
    void on_navExportBtn_clicked();
    void on_addCourseBtn_clicked();
    void on_deleteCourseBtn_clicked();
    void on_addExpBtn_clicked();
    void on_DelExpBtn_clicked();
    void on_addAwardBtn_clicked();
    void on_delAwardBtn_clicked();

private:
    Ui::MainWindow *ui;
    QSqlTableModel *courseModel;
    QSqlTableModel *expModel;
    QSqlTableModel *awardModel;

    QLabel *homeChartLabel = nullptr;
    QLabel *homeCourseCountLbl = nullptr;
    QLabel *homeGpaLbl = nullptr;
    QLabel *homeCompetitionCountLbl = nullptr;
    QLabel *homeInternshipCountLbl = nullptr;
    QLabel *homeProjectCountLbl = nullptr;
    QLabel *homeAwardCountLbl = nullptr;
    QLabel *sidebarAvatarLbl = nullptr;
    QPushButton *editProfileBtn = nullptr;
    QPushButton *logoutBtn = nullptr;
    QPushButton *expTabBtn = nullptr;
    QPushButton *awardTabBtn = nullptr;
    QPushButton *importCourseCsvBtn = nullptr;
    QPushButton *exportCourseCsvBtn = nullptr;
    QPushButton *resetCourseBtn = nullptr;
    QPushButton *importExpCsvBtn = nullptr;
    QPushButton *exportExpCsvBtn = nullptr;
    QPushButton *csvHelpCourseBtn = nullptr;
    QPushButton *csvHelpExpBtn = nullptr;
    QPushButton *homeImportAllBtn = nullptr;
    QPushButton *homeExportAllBtn = nullptr;
    QPushButton *homeCsvHelpBtn = nullptr;
    QDoubleSpinBox *addAwardAmountSpin = nullptr;
    QLabel *addAwardAmountLbl = nullptr;

    // 简历导出页控件
    QLabel *photoPreviewLbl = nullptr;
    QPushButton *selectPhotoBtn = nullptr;
    QLabel *skillsLbl = nullptr;
    QPushButton *editSkillsBtn = nullptr;
    QLabel *summaryLbl = nullptr;
    QPushButton *editSummaryBtn = nullptr;
    QComboBox *resumeTemplateCombo = nullptr;
    QList<QToolButton *> resumeTemplateCards;
    QLabel *resumeTemplateDescriptionLbl = nullptr;
    QWidget *resumePreviewOverlay = nullptr;
    QLabel *resumePreviewLargeLbl = nullptr;
    QLabel *resumePreviewTitleLbl = nullptr;
    QPushButton *previewResumeBtn = nullptr;
    QPushButton *exportResumePdfBtn = nullptr;
    ResumeExporter *resumeExporter = nullptr;
    QString m_photoPath;
    QString m_avatarPath;
    QString m_skillsText;
    QString m_summaryText;

    bool m_expTabActive = true;  // true = 课外活动, false = 个人荣誉

    void InitFrame();
    void InitCoursePage();
    void InitExpPage();

    void updateTotalStats();
    void updateSidebarUserInfo();
    void updateSidebarAvatar();
    void openEditProfileDialog();
    void logout();
    void applyModernStyle();
    void setupTableView(QTableView *tableView);
    void buildHomePage();
    void buildExportPage();
    void loadResumeProfile();
    void editSkills();
    void editSummary();
    void saveResumeToDb();
    void showResumeTemplateMagnifier();
    void hideResumeTemplateMagnifier();
    void updateResumeTemplateMagnifier();
    void updateHomePageStats();
    void resetAllCourses();
    double scoreToGpa(double score) const;

    // CSV 导入导出
    void importCoursesFromCsv(const QString &filePath);
    void exportCoursesToCsv(const QString &filePath);
    void importExperiencesFromCsv(const QString &filePath);
    void exportExperiencesToCsv(const QString &filePath);
    void importAwardsFromCsv(const QString &filePath);
    void exportAwardsToCsv(const QString &filePath);
    void importAllFromCsv(const QString &filePath);
    void exportAllToCsv(const QString &filePath);
};

#endif // MAINWINDOW_H

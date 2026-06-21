#ifndef EXPERIENCEPAGE_H
#define EXPERIENCEPAGE_H

#include <QWidget>

class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QSqlTableModel;
class QTableView;

class ExperiencePage : public QWidget {
    Q_OBJECT

public:
    explicit ExperiencePage(QWidget *parent = nullptr);
    ~ExperiencePage() override = default;

public slots:
    void refresh();

signals:
    void dataChanged();

private:
    QSqlTableModel *m_experienceModel = nullptr;
    QSqlTableModel *m_awardModel = nullptr;
    bool m_experienceTabActive = true;

    QPushButton *m_experienceTabButton = nullptr;
    QPushButton *m_awardTabButton = nullptr;
    QPushButton *m_importButton = nullptr;
    QPushButton *m_exportButton = nullptr;
    QPushButton *m_helpButton = nullptr;
    QPushButton *m_resetButton = nullptr;
    QTableView *m_experienceTable = nullptr;
    QTableView *m_awardTable = nullptr;
    QFrame *m_experienceForm = nullptr;
    QFrame *m_awardForm = nullptr;
    QComboBox *m_experienceType = nullptr;
    QLineEdit *m_experienceTitle = nullptr;
    QDateEdit *m_experienceDate = nullptr;
    QLineEdit *m_experienceDescription = nullptr;
    QLineEdit *m_awardName = nullptr;
    QDateEdit *m_awardDate = nullptr;
    QComboBox *m_awardLevel = nullptr;
    QDoubleSpinBox *m_awardAmount = nullptr;

    void buildUi();
    void setupModels();
    void setExperienceTabActive(bool experienceActive);
    void addExperience();
    void deleteExperience();
    void addAward();
    void deleteAward();
    void resetAllExperiences();
    void resetAllAwards();
    void importExperiencesFromCsv(const QString &filePath);
    void exportExperiencesToCsv(const QString &filePath);
    void importAwardsFromCsv(const QString &filePath);
    void exportAwardsToCsv(const QString &filePath);
    void showCsvHelp();
};

#endif // EXPERIENCEPAGE_H

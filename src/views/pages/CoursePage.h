#ifndef COURSEPAGE_H
#define COURSEPAGE_H

#include <QWidget>

class QLabel;
class QPushButton;
class QSqlTableModel;
class QTableView;

class CoursePage : public QWidget {
    Q_OBJECT

public:
    explicit CoursePage(QWidget *parent = nullptr);
    ~CoursePage() override = default;

public slots:
    void refresh();

signals:
    void dataChanged();

private:
    QSqlTableModel *m_model = nullptr;
    QTableView *m_tableView = nullptr;
    QLabel *m_statsLabel = nullptr;
    QPushButton *m_importButton = nullptr;
    QPushButton *m_exportButton = nullptr;
    QPushButton *m_helpButton = nullptr;
    QPushButton *m_resetButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_addButton = nullptr;

    void buildUi();
    void setupModel();
    void updateStats();
    void addCourse();
    void deleteSelectedCourses();
    void resetAllCourses();
    void importFromCsv(const QString &filePath);
    void exportToCsv(const QString &filePath);
    void showCsvHelp();
};

#endif // COURSEPAGE_H

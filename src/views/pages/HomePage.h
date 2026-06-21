#ifndef HOMEPAGE_H
#define HOMEPAGE_H

#include <QWidget>

class QLabel;

class HomePage : public QWidget {
    Q_OBJECT

public:
    explicit HomePage(QWidget *parent = nullptr);
    ~HomePage() override = default;

public slots:
    void refresh();

signals:
    void allDataChanged();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *m_chartLabel = nullptr;
    QLabel *m_courseCountLabel = nullptr;
    QLabel *m_gpaLabel = nullptr;
    QLabel *m_competitionCountLabel = nullptr;
    QLabel *m_internshipCountLabel = nullptr;
    QLabel *m_projectCountLabel = nullptr;
    QLabel *m_awardCountLabel = nullptr;

    void buildUi();
    void importAllFromCsv(const QString &filePath);
    void exportAllToCsv(const QString &filePath);
    void showCsvHelp();
};

#endif // HOMEPAGE_H

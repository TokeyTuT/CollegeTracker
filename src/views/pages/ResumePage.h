#ifndef RESUMEPAGE_H
#define RESUMEPAGE_H

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QToolButton;
class ResumeExporter;

class ResumePage : public QWidget {
    Q_OBJECT

public:
    explicit ResumePage(QWidget *parent = nullptr);
    ~ResumePage() override;

    QString photoPath() const;
    void choosePhoto(QWidget *dialogParent = nullptr);
    void removePhoto();
    void hideTemplatePreview();

public slots:
    void refresh();

signals:
    void photoChanged(const QString &photoPath);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *m_photoPreview = nullptr;
    QLabel *m_skillsLabel = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QLabel *m_templateDescription = nullptr;
    QComboBox *m_templateCombo = nullptr;
    QList<QToolButton *> m_templateCards;
    QWidget *m_previewOverlay = nullptr;
    QLabel *m_previewLargeLabel = nullptr;
    QLabel *m_previewTitleLabel = nullptr;
    QPushButton *m_exportPdfButton = nullptr;
    ResumeExporter *m_exporter = nullptr;
    QString m_photoPath;
    QString m_skillsText;
    QString m_summaryText;

    void buildUi();
    void saveProfile();
    void editSkills();
    void editSummary();
    void showTemplatePreview();
    void updateTemplatePreview();
    void updateTemplateDescription();
};

#endif // RESUMEPAGE_H

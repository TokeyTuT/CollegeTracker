#ifndef RESUMEEXPORTER_H
#define RESUMEEXPORTER_H

#include <QObject>
#include <QString>

class ResumeExporter : public QObject
{
    Q_OBJECT

public:
    explicit ResumeExporter(QObject *parent = nullptr);

    QString generateHtml(int userId, QString *errorMessage = nullptr) const;
    QString generatePreviewFile(int userId,
                                QString *errorMessage = nullptr) const;
    bool openPreview(int userId, QString *errorMessage = nullptr) const;
    void exportPdf(int userId, const QString &filePath);

signals:
    void pdfExportFinished(bool success, const QString &filePath,
                           const QString &errorMessage);

private:
    static QString escapedMultiline(const QString &text);
    static QString textList(const QString &text);
    static QString imageDataUrl(const QString &photoPath);
    static QString safeFileComponent(const QString &text);
    static QString findChromiumBrowser();
};

#endif // RESUMEEXPORTER_H

#ifndef RESUMETEMPLATEREGISTRY_H
#define RESUMETEMPLATEREGISTRY_H

#include <QList>
#include <QString>

struct ResumeTemplateDefinition {
    QString id;
    QString displayName;
    QString description;
    QString htmlResource;
    QString previewResource;
};

class ResumeTemplateRegistry final
{
public:
    static const QList<ResumeTemplateDefinition> &templates();
    static const ResumeTemplateDefinition &defaultTemplate();
    static const ResumeTemplateDefinition &findById(const QString &id);
    static QString normalizedId(const QString &id);

private:
    ResumeTemplateRegistry() = delete;
};

#endif // RESUMETEMPLATEREGISTRY_H

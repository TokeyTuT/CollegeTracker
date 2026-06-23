#include "ResumeTemplateRegistry.h"

const QList<ResumeTemplateDefinition> &ResumeTemplateRegistry::templates()
{
    static const QList<ResumeTemplateDefinition> registeredTemplates = {
        {
            QStringLiteral("classic"),
            QStringLiteral("经典学术"),
            QStringLiteral("传统学术排版，信息清晰，适合通用申请。"),
            QStringLiteral(":/templates/resume_template.html"),
            QStringLiteral(":/previews/resume-classic.png"),
        },
        {
            QStringLiteral("navy"),
            QStringLiteral("深海蓝双栏"),
            QStringLiteral(
                "左侧信息轨道与右侧履历，适合技术岗和项目型简历。"),
            QStringLiteral(":/templates/resume_template_navy.html"),
            QStringLiteral(":/previews/resume-navy.png"),
        },
        {
            QStringLiteral("editorial"),
            QStringLiteral("暖色编辑风"),
            QStringLiteral(
                "暖灰纸张与编辑式编号，适合商科、研究和综合岗位。"),
            QStringLiteral(":/templates/resume_template_editorial.html"),
            QStringLiteral(":/previews/resume-editorial.png"),
        },
    };
    return registeredTemplates;
}

const ResumeTemplateDefinition &ResumeTemplateRegistry::defaultTemplate()
{
    return templates().first();
}

const ResumeTemplateDefinition &
ResumeTemplateRegistry::findById(const QString &id)
{
    const QString normalized = id.trimmed().toLower();
    for (const ResumeTemplateDefinition &resumeTemplate : templates()) {
        if (resumeTemplate.id == normalized)
            return resumeTemplate;
    }
    return defaultTemplate();
}

QString ResumeTemplateRegistry::normalizedId(const QString &id)
{
    return findById(id).id;
}

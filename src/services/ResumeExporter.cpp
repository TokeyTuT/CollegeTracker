#include "ResumeExporter.h"

#include "DatabaseMannager.h"

#include <QCoreApplication>
#include <QBuffer>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QUuid>
#include <QVariantList>
#include <QVariantMap>

namespace {

QString html(const QVariant &value) {
    return value.toString().toHtmlEscaped();
}

QString joinedWithSeparator(const QStringList &parts) {
    return parts.join("<span class=\"separator\">|</span>");
}

QString linkHtml(const QString &url, const QString &label = QString()) {
    const QString trimmed = url.trimmed();
    if (trimmed.isEmpty())
        return {};

    QString href = trimmed;
    if (!href.contains("://") && !href.startsWith("mailto:"))
        href.prepend("https://");
    const QString visible = label.isEmpty() ? trimmed : label;
    return QString("<a href=\"%1\">%2</a>")
        .arg(href.toHtmlEscaped(), visible.toHtmlEscaped());
}

QString sectionHtml(const QString &title, const QString &body) {
    if (body.trimmed().isEmpty())
        return {};
    return QString(
               "<section><div class=\"section-title\">%1</div>%2</section>")
        .arg(title.toHtmlEscaped(), body);
}

QString dateRange(const QVariantMap &row) {
    const QString start = row.value("start_date").toString().trimmed();
    const QString end = row.value("end_date").toString().trimmed();
    const QRegularExpression yearOnly("(?:19|20)\\d{2}");
    if (yearOnly.match(start).hasMatch() &&
        yearOnly.match(start).captured(0) == start &&
        yearOnly.match(end).hasMatch() &&
        yearOnly.match(end).captured(0) == end) {
        return QString("%1～%2").arg(start, end);
    }
    if (!start.isEmpty() && !end.isEmpty())
        return start + " - " + end;
    if (yearOnly.match(start).hasMatch() &&
        yearOnly.match(start).captured(0) == start)
        return start;
    if (yearOnly.match(end).hasMatch() &&
        yearOnly.match(end).captured(0) == end)
        return end;
    return start.isEmpty() ? end : start;
}

QString experienceTitle(const QVariantMap &row) {
    QStringList prominent;
    const QString organization =
        row.value("organization").toString().trimmed();
    const QString title = row.value("title").toString().trimmed();
    const QString role = row.value("role").toString().trimmed();

    if (!organization.isEmpty())
        prominent << organization.toHtmlEscaped();
    if (!title.isEmpty())
        prominent << title.toHtmlEscaped();

    QString result;
    if (!prominent.isEmpty())
        result = "<strong>" + prominent.join(" | ") + "</strong>";
    if (!role.isEmpty()) {
        if (!result.isEmpty())
            result += ", ";
        result += role.toHtmlEscaped();
    }
    return result;
}

QString descriptionList(const QString &content) {
    QStringList lines =
        content.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
    QString result;
    for (QString line : lines) {
        line = line.trimmed();
        line.remove(QRegularExpression("^[\\-•·]+\\s*"));
        if (!line.isEmpty())
            result += "<li>" + line.toHtmlEscaped() + "</li>";
    }
    return result.isEmpty() ? QString() : "<ul>" + result + "</ul>";
}

QString experienceItem(const QVariantMap &row) {
    const QString title = experienceTitle(row);
    const QString date = html(row.value("date"));
    const QString bullets = descriptionList(row.value("content").toString());
    if (title.isEmpty() && date.isEmpty() && bullets.isEmpty())
        return {};

    return QString(
               "<div class=\"item\">"
               "<div class=\"item-header\">"
               "<div class=\"item-title\">%1</div>"
               "<div class=\"item-date\">%2</div>"
               "</div>%3</div>")
        .arg(title, date, bullets);
}

} // namespace

ResumeExporter::ResumeExporter(QObject *parent)
    : QObject(parent) {}

QString ResumeExporter::escapedMultiline(const QString &text) {
    QString result = text.toHtmlEscaped();
    result.replace("\r\n", "\n");
    result.replace('\r', '\n');
    result.replace("\n", "<br>");
    return result;
}

QString ResumeExporter::textList(const QString &text) {
    const QStringList lines =
        text.split(QRegularExpression("[\\r\\n]+"), Qt::SkipEmptyParts);
    QString items;
    for (QString line : lines) {
        line = line.trimmed();
        line.remove(QRegularExpression("^[\\-•·]+\\s*"));
        if (line.isEmpty())
            continue;

        QString rendered = line.toHtmlEscaped();
        const int colon = line.indexOf(QRegularExpression("[:：]"));
        if (colon > 0) {
            const QString key = line.left(colon).trimmed().toHtmlEscaped();
            const QString value =
                line.mid(colon + 1).trimmed().toHtmlEscaped();
            rendered = QString("<strong>%1</strong>: %2").arg(key, value);
        }
        items += "<li>" + rendered + "</li>";
    }
    return items.isEmpty() ? QString() : "<ul>" + items + "</ul>";
}

QString ResumeExporter::imageDataUrl(const QString &photoPath) {
    if (photoPath.trimmed().isEmpty())
        return {};

    QFileInfo info(photoPath);
    QString fullPath = photoPath;
    if (info.isRelative())
        fullPath = QDir(QCoreApplication::applicationDirPath())
                       .filePath(photoPath);

    QImage image(fullPath);
    if (image.isNull())
        return {};

    // 兼容旧版保存的“圆形 JPEG”：透明区域被 JPEG 压成黑角。
    // 对这种正方形图片取圆内最大的 3:4 中央矩形，仅影响导出数据，
    // 不直接覆盖用户的原始文件。
    if (image.width() == image.height() && image.width() >= 100) {
        const auto isDarkOrTransparent = [&image](int x, int y) {
            const QColor color = image.pixelColor(x, y);
            return color.alpha() < 24 ||
                   (color.red() < 24 && color.green() < 24 &&
                    color.blue() < 24);
        };
        const int inset = qMax(1, image.width() / 40);
        const bool legacyCircular =
            isDarkOrTransparent(inset, inset) &&
            isDarkOrTransparent(image.width() - inset - 1, inset) &&
            isDarkOrTransparent(inset, image.height() - inset - 1) &&
            isDarkOrTransparent(image.width() - inset - 1,
                                image.height() - inset - 1);
        if (legacyCircular) {
            const int cropWidth = qRound(image.width() * 0.60);
            const int cropHeight = qRound(image.height() * 0.80);
            image = image.copy((image.width() - cropWidth) / 2,
                               (image.height() - cropHeight) / 2,
                               cropWidth, cropHeight);
        }
    }

    QByteArray imageBytes;
    QBuffer buffer(&imageBytes);
    if (!buffer.open(QIODevice::WriteOnly) ||
        !image.save(&buffer, "JPEG", 92))
        return {};

    return QString("data:image/jpeg;base64,%1")
        .arg(QString::fromLatin1(imageBytes.toBase64()));
}

QString ResumeExporter::safeFileComponent(const QString &text) {
    QString safe = text.trimmed();
    safe.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
    return safe.isEmpty() ? QStringLiteral("Resume") : safe;
}

QString ResumeExporter::findChromiumBrowser() {
#ifdef Q_OS_MACOS
    const QStringList macCandidates = {
        "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
        "/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge",
        "/Applications/Chromium.app/Contents/MacOS/Chromium"
    };
    for (const QString &candidate : macCandidates) {
        if (QFileInfo::exists(candidate))
            return candidate;
    }
#endif

    const QStringList executableNames = {
#ifdef Q_OS_WIN
        "chrome.exe", "msedge.exe", "chromium.exe"
#else
        "google-chrome", "google-chrome-stable", "microsoft-edge",
        "microsoft-edge-stable", "chromium", "chromium-browser"
#endif
    };
    for (const QString &name : executableNames) {
        const QString executable = QStandardPaths::findExecutable(name);
        if (!executable.isEmpty())
            return executable;
    }

#ifdef Q_OS_WIN
    const QStringList roots = {
        qEnvironmentVariable("PROGRAMFILES"),
        qEnvironmentVariable("PROGRAMFILES(X86)"),
        qEnvironmentVariable("LOCALAPPDATA")
    };
    const QStringList relativePaths = {
        "Google/Chrome/Application/chrome.exe",
        "Microsoft/Edge/Application/msedge.exe"
    };
    for (const QString &root : roots) {
        for (const QString &relativePath : relativePaths) {
            const QString candidate = QDir(root).filePath(relativePath);
            if (QFileInfo::exists(candidate))
                return candidate;
        }
    }
#endif
    return {};
}

QString ResumeExporter::generateHtml(int userId,
                                     QString *errorMessage) const {
    const QVariantMap data =
        DatabaseManager::getInstance().getResumeData(userId);
    const QVariantMap user = data.value("user").toMap();
    const QVariantMap profile = data.value("profile").toMap();
    const QVariantList education = data.value("education").toList();
    const QVariantList experiences = data.value("experiences").toList();
    const QVariantList awards = data.value("awards").toList();
    const QVariantList coreCourses = data.value("core_courses").toList();
    const QVariantMap stats = data.value("course_stats").toMap();

    if (user.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("当前用户信息不存在");
        return {};
    }

    QString templateId =
        profile.value("template_id").toString().trimmed().toLower();
    QString templateResource = ":/templates/resume_template.html";
    if (templateId == "navy")
        templateResource = ":/templates/resume_template_navy.html";
    else if (templateId == "editorial")
        templateResource = ":/templates/resume_template_editorial.html";
    else
        templateId = "classic";

    QFile templateFile(templateResource);
    if (!templateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法读取内置简历模板");
        return {};
    }

    QTextStream templateStream(&templateFile);
    templateStream.setCodec("UTF-8");
    QString document = templateStream.readAll();

    QString fullName = profile.value("full_name").toString().trimmed();
    if (fullName.isEmpty())
        fullName = user.value("username").toString().trimmed();

    QStringList contactLine1;
    const QString phone = profile.value("phone").toString().trimmed();
    const QString email = profile.value("email").toString().trimmed();
    if (!phone.isEmpty())
        contactLine1 << phone.toHtmlEscaped();
    if (!email.isEmpty())
        contactLine1 << QString("<a href=\"mailto:%1\">%2</a>")
                            .arg(email.toHtmlEscaped(),
                                 email.toHtmlEscaped());

    QStringList contactLine2;
    const QString target = profile.value("job_target").toString().trimmed();
    if (!target.isEmpty())
        contactLine2 << target.toHtmlEscaped();
    const QString github = profile.value("github_url").toString().trimmed();
    if (!github.isEmpty())
        contactLine2 << linkHtml(github, "GitHub");
    const QString website = profile.value("website_url").toString().trimmed();
    if (!website.isEmpty())
        contactLine2 << QString("个人网站：%1").arg(website.toHtmlEscaped());

    if (contactLine1.isEmpty() && contactLine2.isEmpty()) {
        const QString school = user.value("school").toString().trimmed();
        const QString major = user.value("major").toString().trimmed();
        if (!school.isEmpty())
            contactLine1 << school.toHtmlEscaped();
        if (!major.isEmpty())
            contactLine1 << major.toHtmlEscaped();
    }

    QString contactInfo;
    if (!contactLine1.isEmpty())
        contactInfo +=
            "<div class=\"contact-line\">" +
            joinedWithSeparator(contactLine1) + "</div>";
    if (!contactLine2.isEmpty())
        contactInfo +=
            "<div class=\"contact-line\">" +
            joinedWithSeparator(contactLine2) + "</div>";

    QString photoBlock;
    const QString photoUrl =
        imageDataUrl(profile.value("photo_path").toString());
    if (!photoUrl.isEmpty()) {
        photoBlock =
            QString("<div class=\"header-right\"><img src=\"%1\" "
                    "alt=\"个人照片\" class=\"profile-photo\"></div>")
                .arg(photoUrl);
    }

    const QString summary =
        profile.value("summary").toString().trimmed();
    const QString summarySection =
        summary.isEmpty()
            ? QString()
            : sectionHtml("个人总结",
                          "<div class=\"item-desc\">" +
                              escapedMultiline(summary) + "</div>");

    QString educationBody;
    for (const QVariant &value : education) {
        const QVariantMap row = value.toMap();
        QStringList details;
        const QString major = row.value("major").toString().trimmed();
        const QString degree = row.value("degree").toString().trimmed();
        if (!major.isEmpty())
            details << major.toHtmlEscaped();
        if (!degree.isEmpty())
            details << "<i>" + degree.toHtmlEscaped() + "</i>";

        QString itemTitle =
            "<strong>" + html(row.value("school")) + "</strong>";
        if (!details.isEmpty())
            itemTitle += ", " + details.join(", ");

        QString description =
            row.value("description").toString().trimmed();
        QString descriptionBlock;
        if (!description.isEmpty()) {
            descriptionBlock =
                "<div class=\"item-desc\">" +
                escapedMultiline(description) + "</div>";
        }

        educationBody +=
            QString("<div class=\"item\"><div class=\"item-header\">"
                    "<div class=\"item-title\">%1</div>"
                    "<div class=\"item-date\">%2</div>"
                    "</div>%3</div>")
                .arg(itemTitle, dateRange(row).toHtmlEscaped(),
                     descriptionBlock);
    }

    QStringList academicParts;
    if (stats.value("count").toInt() > 0) {
        academicParts
            << QString("GPA %1 / 4.0")
                   .arg(stats.value("gpa").toDouble(), 0, 'f', 2)
            << QString("平均成绩 %1")
                   .arg(stats.value("avg").toDouble(), 0, 'f', 1)
            << QString("总学分 %1")
                   .arg(stats.value("totalCredits").toDouble(), 0, 'f', 1);
    }
    QStringList courseNames;
    for (const QVariant &value : coreCourses)
        courseNames << value.toMap().value("name").toString().toHtmlEscaped();
    if (!courseNames.isEmpty())
        academicParts << "核心课程：" + courseNames.join("、");
    if (!academicParts.isEmpty()) {
        educationBody +=
            "<div class=\"item-desc academic-summary\">" +
            academicParts.join("<span class=\"separator\">|</span>") +
            "</div>";
    }

    const QString educationSection =
        sectionHtml("教育背景", educationBody);
    const QString skillsSection =
        sectionHtml("技术能力",
                    textList(profile.value("skills").toString()));

    QString internshipBody;
    QString projectAwardBody;
    QString otherBody;
    for (const QVariant &value : experiences) {
        const QVariantMap row = value.toMap();
        const QString type = row.value("type").toString().trimmed();
        const QString item = experienceItem(row);
        if (type == "实习")
            internshipBody += item;
        else if (type == "项目" || type == "竞赛")
            projectAwardBody += item;
        else
            otherBody += item;
    }

    QString awardItems;
    for (const QVariant &value : awards) {
        const QVariantMap row = value.toMap();
        QString item = "<strong>" + html(row.value("name")) + "</strong>";
        const QString level = row.value("level").toString().trimmed();
        const QString date = row.value("date").toString().trimmed();
        const QString description =
            row.value("description").toString().trimmed();
        if (!level.isEmpty())
            item += ", " + level.toHtmlEscaped();
        if (!date.isEmpty())
            item += ", " + date.toHtmlEscaped();
        if (!description.isEmpty())
            item += " — " + description.toHtmlEscaped();
        awardItems += "<li>" + item + "</li>";
    }
    if (!awardItems.isEmpty())
        projectAwardBody += "<ul>" + awardItems + "</ul>";

    const QString internshipSection =
        sectionHtml("实习经历", internshipBody);
    const QString projectAwardSection =
        sectionHtml("竞赛获奖/项目作品", projectAwardBody);
    const QString otherSection =
        sectionHtml("社区参与/实践其他", otherBody);

    const bool hasBody =
        !summarySection.isEmpty() || !educationSection.isEmpty() ||
        !skillsSection.isEmpty() || !internshipSection.isEmpty() ||
        !projectAwardSection.isEmpty() || !otherSection.isEmpty();
    const QString emptyMessage =
        hasBody ? QString()
                : QString("<div class=\"empty-resume\">"
                          "请先完善简历资料后再生成预览。</div>");

    document.replace("{{DOCUMENT_TITLE}}",
                     (fullName + "的简历").toHtmlEscaped());
    document.replace("{{FULL_NAME}}", fullName.toHtmlEscaped());
    document.replace("{{CONTACT_INFO}}", contactInfo);
    document.replace("{{PHOTO_BLOCK}}", photoBlock);
    document.replace("{{SUMMARY_SECTION}}", summarySection);
    document.replace("{{EDUCATION_SECTION}}", educationSection);
    document.replace("{{SKILLS_SECTION}}", skillsSection);
    document.replace("{{INTERNSHIP_SECTION}}", internshipSection);
    document.replace("{{PROJECT_AWARD_SECTION}}", projectAwardSection);
    document.replace("{{OTHER_SECTION}}", otherSection);
    document.replace("{{EMPTY_MESSAGE}}", emptyMessage);
    return document;
}

QString ResumeExporter::generatePreviewFile(
    int userId, QString *errorMessage) const {
    const QString document = generateHtml(userId, errorMessage);
    if (document.isEmpty())
        return {};

    const QString previewDir =
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
            .filePath("CollegeTrackerResumePreview");
    if (!QDir().mkpath(previewDir)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法创建简历预览目录");
        return {};
    }

    const QVariantMap user =
        DatabaseManager::getInstance().getUserInfo(userId);
    const QString username =
        safeFileComponent(user.value("username").toString());
    QString templateId =
        DatabaseManager::getInstance()
            .getResumeProfile(userId)
            .value("template_id")
            .toString()
            .trimmed()
            .toLower();
    if (templateId != "navy" && templateId != "editorial")
        templateId = "classic";
    const QString filePath =
        QDir(previewDir)
            .filePath(username + "_Resume_" + templateId + "_Preview.html");

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text |
                   QIODevice::Truncate)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法写入简历预览文件：") +
                            file.errorString();
        return {};
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << document;
    file.close();
    return filePath;
}

bool ResumeExporter::openPreview(int userId, QString *errorMessage) const {
    const QString filePath = generatePreviewFile(userId, errorMessage);
    if (filePath.isEmpty())
        return false;
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) {
        if (errorMessage)
            *errorMessage = QStringLiteral("无法调用系统浏览器打开简历预览");
        return false;
    }
    return true;
}

void ResumeExporter::exportPdf(int userId, const QString &filePath) {
    QString errorMessage;
    const QString previewPath =
        generatePreviewFile(userId, &errorMessage);
    if (previewPath.isEmpty()) {
        emit pdfExportFinished(false, filePath, errorMessage);
        return;
    }

    const QString browser = findChromiumBrowser();
    if (browser.isEmpty()) {
        emit pdfExportFinished(
            false, filePath,
            QStringLiteral(
                "未找到可用于导出 PDF 的 Chrome、Edge 或 Chromium 浏览器"));
        return;
    }

    if (QFileInfo::exists(filePath) && !QFile::remove(filePath)) {
        emit pdfExportFinished(
            false, filePath,
            QStringLiteral("无法覆盖已有 PDF 文件，请检查文件是否被占用"));
        return;
    }

    const QString profileDir =
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
            .filePath(
                QString("CollegeTrackerPdfProfile_%1_%2")
                    .arg(QCoreApplication::applicationPid())
                    .arg(QUuid::createUuid().toString(QUuid::Id128)));
    QDir().mkpath(profileDir);

    auto *process = new QProcess(this);
    process->setProperty("exportReported", false);
    process->setProgram(browser);
    process->setArguments({
        "--headless",
        "--disable-gpu",
        "--no-pdf-header-footer",
        "--allow-file-access-from-files",
        "--user-data-dir=" + profileDir,
        "--print-to-pdf=" + QDir::toNativeSeparators(filePath),
        QUrl::fromLocalFile(previewPath).toString(QUrl::FullyEncoded)
    });

    connect(process, &QProcess::errorOccurred, this,
            [this, process, filePath,
             profileDir](QProcess::ProcessError error) {
                if (error != QProcess::FailedToStart ||
                    process->property("exportReported").toBool())
                    return;
                process->setProperty("exportReported", true);
                QDir(profileDir).removeRecursively();
                emit pdfExportFinished(
                    false, filePath,
                    QStringLiteral("无法启动浏览器完成 PDF 导出：") +
                        process->errorString());
                process->deleteLater();
            });

    connect(
        process,
        QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        this,
        [this, process, filePath, profileDir](int exitCode,
                                             QProcess::ExitStatus exitStatus) {
            if (process->property("exportReported").toBool()) {
                QDir(profileDir).removeRecursively();
                process->deleteLater();
                return;
            }
            process->setProperty("exportReported", true);
            const QFileInfo output(filePath);
            const bool success =
                output.exists() && output.size() > 0 &&
                (process->property("pdfReady").toBool() ||
                 (exitStatus == QProcess::NormalExit && exitCode == 0));
            QString errorMessage;
            if (!success) {
                errorMessage =
                    QString::fromLocal8Bit(process->readAllStandardError())
                        .trimmed();
                if (errorMessage.isEmpty())
                    errorMessage = QStringLiteral("浏览器未能生成 PDF 文件");
            }

            QDir(profileDir).removeRecursively();
            emit pdfExportFinished(success, filePath, errorMessage);
            process->deleteLater();
        });

    process->start();

    // 某些 macOS Chrome 版本在 PDF 已落盘后仍会短暂保留后台进程。
    // 轮询文件能让 UI 及时恢复，同时仍保留进程退出时的错误处理。
    auto *outputWatcher = new QTimer(process);
    outputWatcher->setInterval(250);
    connect(outputWatcher, &QTimer::timeout, this,
            [this, process, outputWatcher, filePath]() {
                if (process->property("exportReported").toBool()) {
                    outputWatcher->stop();
                    return;
                }
                const QFileInfo output(filePath);
                if (!output.exists() || output.size() <= 0)
                    return;

                process->setProperty("pdfReady", true);
                outputWatcher->stop();
                process->terminate();
                QTimer::singleShot(1000, process, [process]() {
                    if (process->state() != QProcess::NotRunning)
                        process->kill();
                });
            });
    outputWatcher->start();

    QTimer::singleShot(30000, process,
                       [this, process, filePath, profileDir]() {
                           if (process->property("exportReported").toBool())
                               return;
                           process->setProperty("exportReported", true);
                           process->kill();
                           QFile::remove(filePath);
                           QDir(profileDir).removeRecursively();
                           emit pdfExportFinished(
                               false, filePath,
                               QStringLiteral("PDF 导出超时，请重试"));
                       });
}

#include "AppDataPaths.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace AppDataPaths {

namespace {

QString legacyDataDirectory() {
    return QCoreApplication::applicationDirPath();
}

bool copyDirectoryContents(const QString &sourcePath,
                           const QString &destinationPath) {
    const QDir source(sourcePath);
    if (!source.exists())
        return true;
    if (!QDir().mkpath(destinationPath))
        return false;

    QDirIterator iterator(
        sourcePath,
        QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
        QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString sourceItem = iterator.next();
        const QFileInfo sourceInfo(sourceItem);
        const QString relativePath = source.relativeFilePath(sourceItem);
        const QString destinationItem =
            QDir(destinationPath).filePath(relativePath);

        if (sourceInfo.isDir()) {
            if (!QDir().mkpath(destinationItem))
                return false;
            continue;
        }

        const QFileInfo destinationInfo(destinationItem);
        if (destinationInfo.exists())
            continue;
        if (!QDir().mkpath(destinationInfo.absolutePath()) ||
            !QFile::copy(sourceItem, destinationItem)) {
            return false;
        }
    }
    return true;
}

} // namespace

QString dataDirectory() {
    static const QString directory = [] {
        QString path =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (path.isEmpty())
            path = QDir::home().filePath(QStringLiteral(".CollegeTracker"));
        if (!QDir().mkpath(path))
            qWarning() << "无法创建应用数据目录：" << path;
        return QDir::cleanPath(path);
    }();
    return directory;
}

QString databaseFilePath() {
    return QDir(dataDirectory())
        .filePath(QStringLiteral("college_tracker.db"));
}

QString photosDirectory() {
    const QString path =
        QDir(dataDirectory()).filePath(QStringLiteral("photos"));
    if (!QDir().mkpath(path))
        qWarning() << "无法创建照片目录：" << path;
    return path;
}

QString storedPhotoPath(const QString &fileName) {
    return QDir(QStringLiteral("photos")).filePath(fileName);
}

QString resolveStoredPath(const QString &storedPath) {
    const QString trimmedPath = storedPath.trimmed();
    if (trimmedPath.isEmpty() ||
        trimmedPath.startsWith(QStringLiteral(":/"))) {
        return trimmedPath;
    }

    const QFileInfo pathInfo(trimmedPath);
    if (pathInfo.isAbsolute())
        return trimmedPath;

    const QString relativePath = QDir::cleanPath(trimmedPath);
    if (relativePath == QStringLiteral("..") ||
        relativePath.startsWith(QStringLiteral("../"))) {
        return {};
    }

    const QString currentPath =
        QDir(dataDirectory()).filePath(relativePath);
    if (QFileInfo::exists(currentPath))
        return currentPath;

    // 兼容旧版本：旧头像曾经保存在可执行文件旁边。
    const QString legacyPath =
        QDir(legacyDataDirectory()).filePath(relativePath);
    return QFileInfo::exists(legacyPath) ? legacyPath : currentPath;
}

void migrateLegacyData() {
    static bool migrationAttempted = false;
    if (migrationAttempted)
        return;
    migrationAttempted = true;

    const QString legacyDirectory =
        QDir::cleanPath(legacyDataDirectory());
    const QString currentDirectory =
        QDir::cleanPath(dataDirectory());
    if (legacyDirectory == currentDirectory)
        return;

    const QString legacyDatabase =
        QDir(legacyDirectory).filePath(QStringLiteral("college_tracker.db"));
    const QString currentDatabase = databaseFilePath();
    if (!QFileInfo::exists(currentDatabase) &&
        QFileInfo::exists(legacyDatabase) &&
        !QFile::copy(legacyDatabase, currentDatabase)) {
        qWarning() << "迁移旧数据库失败：" << legacyDatabase
                   << "->" << currentDatabase;
    }

    const QString legacyPhotos =
        QDir(legacyDirectory).filePath(QStringLiteral("photos"));
    if (!copyDirectoryContents(legacyPhotos, photosDirectory()))
        qWarning() << "迁移旧照片目录失败：" << legacyPhotos;
}

} // namespace AppDataPaths

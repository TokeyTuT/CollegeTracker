#include "AvatarUtils.h"
#include "AppDataPaths.h"

#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace AvatarUtils {

namespace {
constexpr qreal kHiResScale = 4.0;
}

QString defaultAvatarResource() {
    return QStringLiteral(":/avatars/default-goblin.png");
}

QString resolvedPhotoPath(const QString &photoPath) {
    if (photoPath.trimmed().isEmpty())
        return defaultAvatarResource();

    const QString fullPath = AppDataPaths::resolveStoredPath(photoPath);
    return QFileInfo::exists(fullPath) ? fullPath : defaultAvatarResource();
}

QPixmap circularPixmap(const QPixmap &source, int logicalSize) {
    if (source.isNull() || logicalSize <= 0)
        return {};

    const int pixelSize = qCeil(logicalSize * kHiResScale);
    const QPixmap scaled =
        source.scaled(pixelSize, pixelSize, Qt::KeepAspectRatioByExpanding,
                      Qt::SmoothTransformation);
    const int x = (scaled.width() - pixelSize) / 2;
    const int y = (scaled.height() - pixelSize) / 2;

    QPixmap result(pixelSize, pixelSize);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    QPainterPath path;
    path.addEllipse(2, 2, pixelSize - 4, pixelSize - 4);
    painter.setClipPath(path);
    painter.drawPixmap(-x, -y, scaled);
    result.setDevicePixelRatio(kHiResScale);
    return result;
}

QPixmap circularAvatar(const QString &photoPath, int logicalSize) {
    return circularPixmap(QPixmap(resolvedPhotoPath(photoPath)), logicalSize);
}

} // namespace AvatarUtils

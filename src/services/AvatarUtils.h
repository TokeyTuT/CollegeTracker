#ifndef AVATARUTILS_H
#define AVATARUTILS_H

#include <QPixmap>
#include <QString>

namespace AvatarUtils {

QString defaultAvatarResource();
QString resolvedPhotoPath(const QString &photoPath);
QPixmap circularPixmap(const QPixmap &source, int logicalSize);
QPixmap circularAvatar(const QString &photoPath, int logicalSize);

} // namespace AvatarUtils

#endif // AVATARUTILS_H

#ifndef APPDATAPATHS_H
#define APPDATAPATHS_H

#include <QString>

namespace AppDataPaths {

QString dataDirectory();
QString databaseFilePath();
QString photosDirectory();
QString storedPhotoPath(const QString &fileName);
QString resolveStoredPath(const QString &storedPath);
void migrateLegacyData();

} // namespace AppDataPaths

#endif // APPDATAPATHS_H

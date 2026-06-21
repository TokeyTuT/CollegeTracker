#ifndef CSVUTILS_H
#define CSVUTILS_H

#include <QStringList>

class QTextStream;

namespace CsvUtils {

void writeCsvRow(QTextStream &stream, const QStringList &fields);
QStringList parseCsvLine(const QString &line);
bool parseCoreCourseValue(const QString &text, bool &isCoreCourse);

} // namespace CsvUtils

#endif // CSVUTILS_H

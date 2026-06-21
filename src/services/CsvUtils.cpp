#include "CsvUtils.h"

#include <QTextStream>

namespace CsvUtils {

void writeCsvRow(QTextStream &stream, const QStringList &fields) {
    QStringList escaped;
    for (const QString &field : fields) {
        QString value = field;
        const bool needsQuoting =
            value.contains(',') || value.contains('"') || value.contains('\n') ||
            value.contains('\r');
        if (needsQuoting) {
            value.replace("\"", "\"\"");
            value = "\"" + value + "\"";
        }
        escaped.append(value);
    }
    stream << escaped.join(',') << "\n";
}

QStringList parseCsvLine(const QString &line) {
    QStringList fields;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar character = line.at(i);
        if (inQuotes) {
            if (character == '"') {
                if (i + 1 < line.size() && line.at(i + 1) == '"') {
                    field += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                field += character;
            }
        } else if (character == '"') {
            inQuotes = true;
        } else if (character == ',') {
            fields.append(field.trimmed());
            field.clear();
        } else {
            field += character;
        }
    }

    fields.append(field.trimmed());
    return fields;
}

bool parseCoreCourseValue(const QString &text, bool &isCoreCourse) {
    const QString value = text.trimmed().toLower();
    if (value.isEmpty() || value == "否" || value == "0" || value == "false" ||
        value == "no" || value == "非核心") {
        isCoreCourse = false;
        return true;
    }
    if (value == "是" || value == "1" || value == "true" || value == "yes" ||
        value == "核心") {
        isCoreCourse = true;
        return true;
    }
    return false;
}

} // namespace CsvUtils

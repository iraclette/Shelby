#include "Csv.h"

namespace csv {

QVector<QStringList> parse(const QByteArray &data) {
    QString text = QString::fromUtf8(data);
    if (text.startsWith(QChar(0xFEFF))) text.remove(0, 1);

    // The separator that appears most in the header line, outside quotes.
    QChar separator = ',';
    {
        int best = -1;
        const QString header = text.section('\n', 0, 0);
        for (const QChar candidate : {QChar(','), QChar(';'), QChar('\t')}) {
            const int count = header.count(candidate);
            if (count > best) {
                best = count;
                separator = candidate;
            }
        }
    }

    QVector<QStringList> rows;
    QStringList row;
    QString field;
    bool quoted = false;
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (quoted) {
            if (c == '"' && i + 1 < text.size() && text.at(i + 1) == '"') {
                field += '"';
                ++i;
            } else if (c == '"') {
                quoted = false;
            } else {
                field += c;
            }
        } else if (c == '"') {
            quoted = true;
        } else if (c == separator) {
            row.append(field);
            field.clear();
        } else if (c == '\n' || c == '\r') {
            if (c == '\r' && i + 1 < text.size() && text.at(i + 1) == '\n') ++i;
            row.append(field);
            field.clear();
            rows.append(row);
            row.clear();
        } else {
            field += c;
        }
    }
    if (!field.isEmpty() || !row.isEmpty()) {
        row.append(field);
        rows.append(row);
    }

    // Spreadsheets often leave fully empty lines at the end.
    while (!rows.isEmpty() && rows.last().join(QString()).trimmed().isEmpty()) rows.removeLast();
    return rows;
}

QByteArray write(const QVector<QStringList> &rows) {
    QString out = QChar(0xFEFF);
    for (const QStringList &row : rows) {
        QStringList fields;
        for (QString field : row) {
            if (field.contains(',') || field.contains('"') || field.contains('\n') || field.contains('\r') ||
                field.contains(';')) {
                field = '"' + field.replace('"', "\"\"") + '"';
            }
            fields.append(field);
        }
        out += fields.join(',') + "\r\n";
    }
    return out.toUtf8();
}

} // namespace csv

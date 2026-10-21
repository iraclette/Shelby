#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

// Spreadsheet files for the product import/export. CSV because every spreadsheet
// program opens and saves it; written as UTF-8 with a BOM so Excel shows Georgian
// text instead of mojibake.
namespace csv {

// Reads quoted fields, embedded line breaks and doubled quotes. The separator is
// whichever of , ; or tab the header line uses: Excel saves with ; in many locales.
QVector<QStringList> parse(const QByteArray &data);

QByteArray write(const QVector<QStringList> &rows);

} // namespace csv

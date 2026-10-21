#pragma once

#include <QDialog>
#include <QJsonArray>
#include <QStringList>
#include <QVector>

#include "SupabaseClient.h"

class QLabel;
class QPlainTextEdit;
class QPushButton;

// Inventory → Export… / Import…: the whole catalog as a spreadsheet (CSV) and back.
//
// One row per product, or one row per variant (with the product's columns
// repeated). Export and import use the same columns, so the way to change many
// products is: export, edit in Excel, import. Import matches products by barcode
// (or, for rows without one, by exact name) and variants by name; it never
// deletes anything, an empty cell leaves that field as it is, and stock is set to
// the number in the file. Running the same file twice is harmless.

// Asks where to save and writes every product there.
void exportProducts(SupabaseClient *client, QWidget *parent);

class ProductImportDialog : public QDialog {
    Q_OBJECT

public:
    // rows: the file's rows, header first (csv::parse).
    ProductImportDialog(SupabaseClient *client, QVector<QStringList> rows, QWidget *parent = nullptr);

    static QString columnName(int column); // in the console's language
    static QString stockColumnName(const QString &shopName);

signals:
    void imported(); // something was saved; the inventory should reload

private:
    void check(const QJsonArray &shops, const QJsonArray &categories, const QJsonArray &suppliers,
               const QJsonArray &products);
    void runBatch(int start);
    void translateNext();
    void finish(const QString &message);

    SupabaseClient *m_client;
    QVector<QStringList> m_rows;
    QJsonArray m_items; // what import_products receives, once checked
    QStringList m_toTranslate;
    int m_created = 0;
    int m_updated = 0;
    int m_variantsCreated = 0;
    int m_translated = 0;
    int m_translateFailed = 0;

    QLabel *m_summary;
    QPlainTextEdit *m_details;
    QPushButton *m_importButton;
    QPushButton *m_closeButton;
};

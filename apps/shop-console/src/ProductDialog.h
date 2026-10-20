#pragma once

#include <QDialog>
#include <QMap>
#include <QStringList>

#include "SupabaseClient.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QVBoxLayout;

// "INT-XXXXXX" code for products and variants without a barcode (same format as the website's).
// The prefix is the business's own (Settings → Checkout).
QString generateInternalSku();

// "New product" form. Image uploads finish in the background after it closes.
class ProductDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProductDialog(SupabaseClient *client, QWidget *parent = nullptr);

private:
    void save();
    void addPhotos();
    void promptForNewCategory();
    void promptForNewSupplier();
    void rebuildShopRows();
    void addVariantRow();
    void removeVariantRow(QWidget *rowWidget);
    void seedVariantStock(const QVector<ProductVariant> &createdVariants);

    SupabaseClient *m_client;

    QLineEdit *m_name;
    QPlainTextEdit *m_description;
    QComboBox *m_category;
    QComboBox *m_supplier;
    QDoubleSpinBox *m_costPrice;
    QDoubleSpinBox *m_sellPrice;
    QSpinBox *m_lowStockThreshold;
    QLineEdit *m_sku;
    QCheckBox *m_noBarcode;
    QListWidget *m_imageList;
    QPushButton *m_saveButton;
    QLabel *m_statusLabel;
    QFormLayout *m_shopStockLayout;
    QVBoxLayout *m_variantRowsLayout;

    QVector<Category> m_categories;
    QVector<Supplier> m_suppliers;
    QVector<Shop> m_shops;
    QMap<QString, QSpinBox *> m_shopSpinBoxes;
    QStringList m_pendingImagePaths;
    // Links productCreated -> variantsCreated for seedVariantStock.
    QString m_pendingProductId;

    // One entry per variant row currently in the form: the row widget (for
    // removal) plus its name/sku fields.
    struct VariantRow {
        QWidget *rowWidget;
        QLineEdit *name;
        QLineEdit *sku;
    };
    QVector<VariantRow> m_variantRows;
};

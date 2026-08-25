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

// "New product" form: name/category/supplier/prices, a low-stock alert
// threshold, an internal-code generator for items with no real barcode,
// per-shop stock quantities, optional color/size variants, and photo
// attachments. Saving creates the product row, upserts inventory_levels for
// every shop (or, if variants were added, creates the variants and seeds
// their own 0-stock rows instead), then kicks off image uploads (those
// continue in the background — the dialog doesn't block on them finishing).
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
    // Bridges productCreated -> variantsCreated (two separate async round
    // trips triggered from the same save()) so seedVariantStock knows which
    // product the just-created variants belong to.
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

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

// "New product" form: name/category/prices, an internal-code generator for
// items with no real barcode, per-shop stock quantities, and photo
// attachments. Saving creates the product row, upserts inventory_levels for
// every shop, then kicks off image uploads (those continue in the
// background — the dialog doesn't block on them finishing).
class ProductDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProductDialog(SupabaseClient *client, QWidget *parent = nullptr);

private:
    void save();
    void addPhotos();
    void promptForNewCategory();
    void rebuildShopRows();

    SupabaseClient *m_client;

    QLineEdit *m_name;
    QPlainTextEdit *m_description;
    QComboBox *m_category;
    QDoubleSpinBox *m_costPrice;
    QDoubleSpinBox *m_sellPrice;
    QLineEdit *m_sku;
    QCheckBox *m_noBarcode;
    QListWidget *m_imageList;
    QPushButton *m_saveButton;
    QLabel *m_statusLabel;
    QFormLayout *m_shopStockLayout;

    QVector<Category> m_categories;
    QVector<Shop> m_shops;
    QMap<QString, QSpinBox *> m_shopSpinBoxes;
    QStringList m_pendingImagePaths;
};

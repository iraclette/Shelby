#include "ProductDialog.h"

#include "Config.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
QString generateInternalSku() {
    const QString alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; // no O/0/I/1 to avoid label confusion
    QString code;
    for (int i = 0; i < 6; ++i) {
        code += alphabet.at(QRandomGenerator::global()->bounded(alphabet.length()));
    }
    return "INT-" + code;
}
} // namespace

ProductDialog::ProductDialog(SupabaseClient *client, QWidget *parent)
    : QDialog(parent), m_client(client) {
    setWindowTitle("New Product");
    resize(480, 720);

    m_name = new QLineEdit(this);
    m_description = new QPlainTextEdit(this);
    m_description->setFixedHeight(70);
    m_category = new QComboBox(this);
    m_supplier = new QComboBox(this);
    m_costPrice = new QDoubleSpinBox(this);
    m_costPrice->setRange(0, 999999);
    m_costPrice->setDecimals(2);
    m_costPrice->setPrefix("GEL ");
    m_sellPrice = new QDoubleSpinBox(this);
    m_sellPrice->setRange(0, 999999);
    m_sellPrice->setDecimals(2);
    m_sellPrice->setPrefix("GEL ");
    m_lowStockThreshold = new QSpinBox(this);
    m_lowStockThreshold->setRange(0, 999999);
    m_lowStockThreshold->setSpecialValueText("Disabled");

    m_sku = new QLineEdit(this);
    m_sku->setPlaceholderText("Scan or type the barcode");
    m_noBarcode = new QCheckBox("No barcode — generate an internal code", this);

    auto *addCategoryButton = new QPushButton("+ New", this);
    auto *categoryRow = new QHBoxLayout;
    categoryRow->addWidget(m_category, 1);
    categoryRow->addWidget(addCategoryButton);

    auto *addSupplierButton = new QPushButton("+ New", this);
    auto *supplierRow = new QHBoxLayout;
    supplierRow->addWidget(m_supplier, 1);
    supplierRow->addWidget(addSupplierButton);

    auto *form = new QFormLayout;
    form->addRow("Name", m_name);
    form->addRow("Description", m_description);
    form->addRow("Category", categoryRow);
    form->addRow("Supplier", supplierRow);
    form->addRow("Cost price", m_costPrice);
    form->addRow("Sell price", m_sellPrice);
    form->addRow("Low stock threshold", m_lowStockThreshold);
    form->addRow("Barcode / SKU", m_sku);
    form->addRow("", m_noBarcode);

    auto *photosGroup = new QGroupBox("Photos", this);
    m_imageList = new QListWidget(this);
    m_imageList->setIconSize(QSize(64, 64));
    m_imageList->setFixedHeight(120);
    auto *addPhotoButton = new QPushButton("Add photos…", this);
    auto *photosLayout = new QVBoxLayout(photosGroup);
    photosLayout->addWidget(m_imageList);
    photosLayout->addWidget(addPhotoButton);

    auto *stockGroup = new QGroupBox("Stock per shop", this);
    m_shopStockLayout = new QFormLayout(stockGroup);

    // Variants: name+sku row pairs, no per-shop stock here — initial stock
    // is 0 everywhere, filled in via the Inventory page afterward, exactly
    // like base-product stock already works. Leave empty for a product with
    // no variants (the common case — knives/leather goods).
    auto *variantsGroup = new QGroupBox("Variants (e.g. shades/sizes)", this);
    m_variantRowsLayout = new QVBoxLayout;
    auto *addVariantButton = new QPushButton("+ Add variant", this);
    auto *variantsLayout = new QVBoxLayout(variantsGroup);
    variantsLayout->addLayout(m_variantRowsLayout);
    variantsLayout->addWidget(addVariantButton, 0, Qt::AlignLeft);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #dc2626;");
    m_statusLabel->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_saveButton = buttons->addButton("Save", QDialogButtonBox::AcceptRole);
    m_saveButton->setObjectName("primaryButton");

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(photosGroup);
    layout->addWidget(stockGroup);
    layout->addWidget(variantsGroup);
    layout->addWidget(m_statusLabel);
    layout->addStretch();
    layout->addWidget(buttons);

    connect(addPhotoButton, &QPushButton::clicked, this, &ProductDialog::addPhotos);
    connect(addVariantButton, &QPushButton::clicked, this, &ProductDialog::addVariantRow);
    connect(m_saveButton, &QPushButton::clicked, this, &ProductDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(addCategoryButton, &QPushButton::clicked, this, &ProductDialog::promptForNewCategory);
    connect(addSupplierButton, &QPushButton::clicked, this, &ProductDialog::promptForNewSupplier);

    connect(m_noBarcode, &QCheckBox::toggled, this, [this](bool checked) {
        m_sku->setReadOnly(checked);
        if (checked) {
            m_sku->setText(generateInternalSku());
        } else {
            m_sku->clear();
        }
    });

    connect(m_client, &SupabaseClient::categoriesFetched, this, [this](const QVector<Category> &categories) {
        m_categories = categories;
        m_category->clear();
        for (const Category &category : m_categories) {
            m_category->addItem(category.name, category.id);
        }
    });
    m_client->fetchCategories();

    connect(m_client, &SupabaseClient::categoryCreated, this, [this](const Category &category) {
        m_categories.append(category);
        m_category->addItem(category.name, category.id);
        m_category->setCurrentIndex(m_category->count() - 1);
    });
    connect(m_client, &SupabaseClient::categoryCreateFailed, this, [this](const QString &message) {
        QMessageBox::warning(this, "Couldn't create category", message);
    });

    m_supplier->addItem("None", QString());
    connect(m_client, &SupabaseClient::suppliersFetched, this, [this](const QVector<Supplier> &suppliers) {
        m_suppliers = suppliers;
        m_supplier->clear();
        m_supplier->addItem("None", QString());
        for (const Supplier &supplier : m_suppliers) {
            m_supplier->addItem(supplier.name, supplier.id);
        }
    });
    m_client->fetchSuppliers();

    connect(m_client, &SupabaseClient::supplierCreated, this, [this](const Supplier &supplier) {
        m_suppliers.append(supplier);
        m_supplier->addItem(supplier.name, supplier.id);
        m_supplier->setCurrentIndex(m_supplier->count() - 1);
    });
    connect(m_client, &SupabaseClient::supplierCreateFailed, this, [this](const QString &message) {
        QMessageBox::warning(this, "Couldn't create supplier", message);
    });

    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        m_shops = shops;
        rebuildShopRows();
    });
    m_client->fetchShops();

    connect(m_client, &SupabaseClient::productCreateFailed, this, [this](const QString &message) {
        m_saveButton->setEnabled(true);
        m_statusLabel->setText(message);
    });

    // Bonus fix: previously this dialog never checked whether the
    // post-creation stock upsert actually succeeded, so a failure here
    // silently left every shop's stock at 0 with no error shown anywhere.
    connect(m_client, &SupabaseClient::inventoryLevelsUpsertFailed, this, [this](const QString &message) {
        m_statusLabel->setStyleSheet("color: #f59e0b;");
        m_statusLabel->setText("Product saved, but stock wasn't set: " + message);
    });

    connect(m_client, &SupabaseClient::variantsCreateFailed, this, [this](const QString &message) {
        m_statusLabel->setStyleSheet("color: #f59e0b;");
        m_statusLabel->setText("Product saved, but variants failed: " + message);
    });
    connect(m_client, &SupabaseClient::variantsCreated, this, &ProductDialog::seedVariantStock);

    connect(m_client, &SupabaseClient::productCreated, this, [this](const QString &productId) {
        m_pendingProductId = productId;

        QVector<ProductVariantInput> variantInputs;
        for (const VariantRow &row : m_variantRows) {
            const QString name = row.name->text().trimmed();
            if (name.isEmpty()) continue;
            variantInputs.append({name, row.sku->text().trimmed()});
        }

        if (variantInputs.isEmpty()) {
            QVector<InventoryLevelInput> levels;
            for (auto it = m_shopSpinBoxes.constBegin(); it != m_shopSpinBoxes.constEnd(); ++it) {
                levels.append({it.key(), it.value()->value()});
            }
            if (!levels.isEmpty()) {
                m_client->upsertInventoryLevels(productId, levels);
            }
        } else {
            // Deliberately no base-bucket row seeded for this product —
            // every sale for it now goes through a variant, so a "no
            // variant" row would sit at 0 forever. seedVariantStock (fired
            // on variantsCreated, once the created rows' ids come back)
            // seeds a 0-stock row per (variant, shop) instead.
            m_client->createVariants(productId, variantInputs);
        }

        for (int i = 0; i < m_pendingImagePaths.size(); ++i) {
            m_client->uploadProductImage(productId, m_pendingImagePaths.at(i), /*isPrimary=*/i == 0);
        }

        // Photos, stock levels, and variants finish saving in the
        // background; the product record itself already exists, so the
        // dialog can close now.
        accept();
    });
}

void ProductDialog::rebuildShopRows() {
    while (m_shopStockLayout->rowCount() > 0) {
        m_shopStockLayout->removeRow(0);
    }
    m_shopSpinBoxes.clear();

    for (const Shop &shop : m_shops) {
        auto *spin = new QSpinBox(this);
        spin->setRange(0, 999999);
        m_shopStockLayout->addRow(shop.name, spin);
        m_shopSpinBoxes.insert(shop.id, spin);
    }
}

void ProductDialog::addVariantRow() {
    auto *rowWidget = new QWidget(this);
    auto *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 0, 0, 0);

    auto *nameEdit = new QLineEdit(rowWidget);
    nameEdit->setPlaceholderText("e.g. Red");
    auto *skuEdit = new QLineEdit(rowWidget);
    skuEdit->setPlaceholderText("Optional barcode");
    auto *removeButton = new QPushButton("×", rowWidget);
    removeButton->setFixedWidth(28);

    rowLayout->addWidget(nameEdit, 1);
    rowLayout->addWidget(skuEdit, 1);
    rowLayout->addWidget(removeButton);

    m_variantRowsLayout->addWidget(rowWidget);
    m_variantRows.append({rowWidget, nameEdit, skuEdit});

    connect(removeButton, &QPushButton::clicked, this, [this, rowWidget]() { removeVariantRow(rowWidget); });
}

void ProductDialog::removeVariantRow(QWidget *rowWidget) {
    for (int i = 0; i < m_variantRows.size(); ++i) {
        if (m_variantRows[i].rowWidget == rowWidget) {
            m_variantRows.remove(i);
            break;
        }
    }
    m_variantRowsLayout->removeWidget(rowWidget);
    rowWidget->deleteLater();
}

void ProductDialog::seedVariantStock(const QVector<ProductVariant> &createdVariants) {
    if (m_pendingProductId.isEmpty() || m_shops.isEmpty()) return;

    QVector<InventoryLevelInput> zeroLevels;
    for (const Shop &shop : m_shops) zeroLevels.append({shop.id, 0});

    for (const ProductVariant &variant : createdVariants) {
        m_client->upsertInventoryLevels(m_pendingProductId, zeroLevels, variant.id);
    }
}

void ProductDialog::promptForNewCategory() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, "New category", "Category name:",
                                                 QLineEdit::Normal, {}, &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    m_client->createCategory(name.trimmed());
}

void ProductDialog::promptForNewSupplier() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, "New supplier", "Supplier name:",
                                                 QLineEdit::Normal, {}, &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    m_client->createSupplier(name.trimmed());
}

void ProductDialog::addPhotos() {
    const QStringList files = QFileDialog::getOpenFileNames(
        this, "Select photos", Config::load().photosSyncDir, "Images (*.png *.jpg *.jpeg *.webp)");
    for (const QString &path : files) {
        m_pendingImagePaths.append(path);
        auto *item = new QListWidgetItem(QIcon(QPixmap(path).scaled(
                                              64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation)),
                                          QFileInfo(path).fileName());
        m_imageList->addItem(item);
    }
}

void ProductDialog::save() {
    m_statusLabel->clear();

    if (m_name->text().trimmed().isEmpty()) {
        m_statusLabel->setText("Name is required.");
        return;
    }
    if (m_sku->text().trimmed().isEmpty()) {
        m_statusLabel->setText("A barcode/SKU is required — check \"No barcode\" to generate one.");
        return;
    }

    ProductInput input;
    input.name = m_name->text().trimmed();
    input.description = m_description->toPlainText().trimmed();
    input.categoryId = m_category->currentData().toString();
    input.supplierId = m_supplier->currentData().toString();
    input.costPrice = m_costPrice->value();
    input.sellPrice = m_sellPrice->value();
    input.lowStockThreshold = m_lowStockThreshold->value();
    input.sku = m_sku->text().trimmed();
    input.hasNativeBarcode = !m_noBarcode->isChecked();

    m_saveButton->setEnabled(false);
    m_statusLabel->setStyleSheet("color: #737373;");
    m_statusLabel->setText("Saving…");
    m_client->createProduct(input);
}

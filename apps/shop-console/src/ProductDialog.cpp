#include "ProductDialog.h"
#include "Theme.h"
#include "BusinessProfile.h"
#include "CategoriesPage.h"

#include "Config.h"
#include "Translations.h"

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

QString generateInternalSku() {
    const QString alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; // no O/0/I/1 to avoid label confusion
    QString code;
    for (int i = 0; i < 6; ++i) {
        code += alphabet.at(QRandomGenerator::global()->bounded(alphabet.length()));
    }
    return BusinessProfile::instance()->settings().skuPrefix + "-" + code;
}

ProductDialog::ProductDialog(SupabaseClient *client, QWidget *parent)
    : QDialog(parent), m_client(client) {
    setWindowTitle(tr("New Product"));
    resize(900, 520);

    m_name = new QLineEdit(this);
    m_description = new QPlainTextEdit(this);
    m_description->setFixedHeight(110);
    // What the description is written in; the other website languages are translated from it.
    m_descriptionLang = new QComboBox(this);
    for (const QString &code : translations::languageCodes())
        m_descriptionLang->addItem(translations::languageName(code), code);
    m_descriptionLang->setCurrentIndex(m_descriptionLang->findData(translations::startupLanguage()));
    m_category = new QComboBox(this);
    m_supplier = new QComboBox(this);
    m_costPrice = new QDoubleSpinBox(this);
    m_costPrice->setRange(0, 999999);
    m_costPrice->setDecimals(2);
    m_costPrice->setPrefix(moneyPrefix());
    m_costPrice->setSuffix(moneySuffix());
    m_sellPrice = new QDoubleSpinBox(this);
    m_sellPrice->setRange(0, 999999);
    m_sellPrice->setDecimals(2);
    m_sellPrice->setPrefix(moneyPrefix());
    m_sellPrice->setSuffix(moneySuffix());
    m_lowStockThreshold = new QSpinBox(this);
    m_lowStockThreshold->setRange(0, 999999);
    m_lowStockThreshold->setSpecialValueText(tr("Disabled"));
    m_lowStockThreshold->setValue(BusinessProfile::instance()->settings().defaultLowStockThreshold);

    m_sku = new QLineEdit(this);
    m_sku->setPlaceholderText(tr("Scan or type the barcode"));
    m_noBarcode = new QCheckBox(tr("No barcode — generate an internal code"), this);

    auto *addCategoryButton = new QPushButton(tr("+ New"), this);
    auto *categoryRow = new QHBoxLayout;
    categoryRow->addWidget(m_category, 1);
    categoryRow->addWidget(addCategoryButton);

    auto *addSupplierButton = new QPushButton(tr("+ New"), this);
    auto *supplierRow = new QHBoxLayout;
    supplierRow->addWidget(m_supplier, 1);
    supplierRow->addWidget(addSupplierButton);

    auto *form = new QFormLayout;
    form->addRow(tr("Name"), m_name);
    auto *descriptionColumn = new QVBoxLayout;
    descriptionColumn->addWidget(m_description);
    auto *descriptionLangRow = new QHBoxLayout;
    descriptionLangRow->addWidget(new QLabel(tr("Written in"), this));
    descriptionLangRow->addWidget(m_descriptionLang);
    descriptionLangRow->addStretch();
    descriptionColumn->addLayout(descriptionLangRow);
    form->addRow(tr("Description"), descriptionColumn);
    form->addRow(tr("Category"), categoryRow);
    form->addRow(tr("Supplier"), supplierRow);
    form->addRow(tr("Cost price"), m_costPrice);
    form->addRow(tr("Sell price"), m_sellPrice);
    form->addRow(tr("Low stock threshold"), m_lowStockThreshold);
    form->addRow(tr("Barcode / SKU"), m_sku);
    form->addRow("", m_noBarcode);

    auto *photosGroup = new QGroupBox(tr("Photos"), this);
    m_imageList = new QListWidget(this);
    m_imageList->setIconSize(QSize(64, 64));
    m_imageList->setFixedHeight(120);
    auto *addPhotoButton = new QPushButton(tr("Add photos…"), this);
    auto *photosLayout = new QVBoxLayout(photosGroup);
    photosLayout->addWidget(m_imageList);
    photosLayout->addWidget(addPhotoButton);

    auto *stockGroup = new QGroupBox(tr("Stock per shop"), this);
    m_shopStockLayout = new QFormLayout(stockGroup);

    // Variants start at 0 stock everywhere; set it from the Inventory page.
    auto *variantsGroup = new QGroupBox(tr("Variants (e.g. shades/sizes)"), this);
    m_variantRowsLayout = new QVBoxLayout;
    auto *addVariantButton = new QPushButton(tr("+ Add variant"), this);
    auto *variantsLayout = new QVBoxLayout(variantsGroup);
    variantsLayout->addLayout(m_variantRowsLayout);
    variantsLayout->addWidget(addVariantButton, 0, Qt::AlignLeft);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet(theme::css(theme::palette().danger));
    m_statusLabel->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_saveButton = buttons->addButton(tr("Save"), QDialogButtonBox::AcceptRole);
    m_saveButton->setObjectName("primaryButton");

    // Two columns so the dialog fits on screen: product details on the left,
    // photos/stock/variants on the right.
    auto *leftColumn = new QVBoxLayout;
    leftColumn->addLayout(form);
    leftColumn->addStretch();

    auto *rightColumn = new QVBoxLayout;
    rightColumn->addWidget(photosGroup);
    rightColumn->addWidget(stockGroup);
    rightColumn->addWidget(variantsGroup);
    rightColumn->addStretch();

    auto *columns = new QHBoxLayout;
    columns->setSpacing(16);
    columns->addLayout(leftColumn, 1);
    columns->addLayout(rightColumn, 1);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(columns, 1);
    layout->addWidget(m_statusLabel);
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
        m_categories = categoriesInTreeOrder(categories);
        m_category->clear();
        for (const Category &category : m_categories) {
            m_category->addItem(categoryLabel(category, m_categories), category.id);
        }
    });
    m_client->fetchCategories();

    connect(m_client, &SupabaseClient::categoryCreated, this, [this](const Category &category) {
        m_categories.append(category);
        m_category->addItem(categoryLabel(category, m_categories), category.id);
        m_category->setCurrentIndex(m_category->count() - 1);
    });
    connect(m_client, &SupabaseClient::categoryCreateFailed, this, [this](const QString &message) {
        QMessageBox::warning(this, tr("Couldn't create category"), message);
    });

    m_supplier->addItem(tr("None"), QString());
    connect(m_client, &SupabaseClient::suppliersFetched, this, [this](const QVector<Supplier> &suppliers) {
        m_suppliers = suppliers;
        m_supplier->clear();
        m_supplier->addItem(tr("None"), QString());
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
        QMessageBox::warning(this, tr("Couldn't create supplier"), message);
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

    connect(m_client, &SupabaseClient::inventoryLevelsUpsertFailed, this, [this](const QString &message) {
        m_statusLabel->setStyleSheet(theme::css(theme::palette().warning));
        m_statusLabel->setText(tr("Product saved, but stock wasn't set: ") + message);
    });

    connect(m_client, &SupabaseClient::variantsCreateFailed, this, [this](const QString &message) {
        m_statusLabel->setStyleSheet(theme::css(theme::palette().warning));
        m_statusLabel->setText(tr("Product saved, but variants failed: ") + message);
    });
    connect(m_client, &SupabaseClient::variantsCreated, this, &ProductDialog::seedVariantStock);

    connect(m_client, &SupabaseClient::productCreated, this, [this](const QString &productId) {
        m_pendingProductId = productId;
        // Fills in the other website languages on the server; nothing here waits for it.
        if (!m_description->toPlainText().trimmed().isEmpty()) m_client->translateDescription(productId);

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
            // No base stock row for variant products; seedVariantStock adds one per variant instead.
            m_client->createVariants(productId, variantInputs);
        }

        for (int i = 0; i < m_pendingImagePaths.size(); ++i) {
            m_client->uploadProductImage(productId, m_pendingImagePaths.at(i), /*isPrimary=*/i == 0);
        }

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
    nameEdit->setPlaceholderText(tr("e.g. Red"));
    auto *skuEdit = new QLineEdit(rowWidget);
    skuEdit->setPlaceholderText(tr("Optional barcode"));
    auto *removeButton = new QPushButton("×", rowWidget);
    removeButton->setObjectName("compactButton");
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
    CategoriesPage::promptForNewCategory(this, m_client, m_categories);
}

void ProductDialog::promptForNewSupplier() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("New supplier"), tr("Supplier name:"),
                                                 QLineEdit::Normal, {}, &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    m_client->createSupplier(name.trimmed());
}

void ProductDialog::addPhotos() {
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Select photos"), Config::load().photosSyncDir, tr("Images (*.png *.jpg *.jpeg *.webp)"));
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
        m_statusLabel->setText(tr("Name is required."));
        return;
    }
    if (m_sku->text().trimmed().isEmpty()) {
        m_statusLabel->setText(tr("A barcode/SKU is required — check \"No barcode\" to generate one."));
        return;
    }

    ProductInput input;
    input.name = m_name->text().trimmed();
    input.description = m_description->toPlainText().trimmed();
    input.descriptionLang = m_descriptionLang->currentData().toString();
    input.categoryId = m_category->currentData().toString();
    input.supplierId = m_supplier->currentData().toString();
    input.costPrice = m_costPrice->value();
    input.sellPrice = m_sellPrice->value();
    input.lowStockThreshold = m_lowStockThreshold->value();
    input.sku = m_sku->text().trimmed();
    input.hasNativeBarcode = !m_noBarcode->isChecked();

    m_saveButton->setEnabled(false);
    m_statusLabel->setStyleSheet(theme::css(theme::palette().textMuted));
    m_statusLabel->setText(tr("Saving…"));
    m_client->createProduct(input);
}

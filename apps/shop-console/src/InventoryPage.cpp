#include "InventoryPage.h"
#include "ProductDialog.h"
#include "ProductPhotosDialog.h"
#include "SupabaseClient.h"
#include "TransferStockDialog.h"

#include <QColor>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonValue>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

InventoryPage::InventoryPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton("Refresh", this);
    toolbar->addWidget(refreshButton);
    auto *newProductButton = new QPushButton("New product", this);
    toolbar->addWidget(newProductButton);
    auto *photosButton = new QPushButton("Photos…", this);
    toolbar->addWidget(photosButton);
    auto *transferButton = new QPushButton("Transfer Stock", this);
    toolbar->addWidget(transferButton);
    toolbar->addStretch();

    connect(refreshButton, &QPushButton::clicked, this, [this]() { m_client->fetchProducts(); });
    connect(newProductButton, &QPushButton::clicked, this, [this]() {
        ProductDialog dialog(m_client, this);
        if (dialog.exec() == QDialog::Accepted) {
            m_client->fetchProducts();
        }
    });
    connect(photosButton, &QPushButton::clicked, this, [this]() {
        const int row = m_table->currentRow();
        if (row < 0 || row >= m_rows.size()) {
            emit statusMessage("Select a product row first.");
            return;
        }
        const Product &product = m_products[m_rows[row].productIndex];
        ProductPhotosDialog dialog(m_client, product.id, product.name, this);
        dialog.exec();
    });
    connect(transferButton, &QPushButton::clicked, this, [this]() {
        if (m_shops.size() < 2) {
            emit statusMessage("Need at least two shops to transfer stock between.");
            return;
        }
        const int row = m_table->currentRow();
        QString preselectedProductId;
        QString preselectedVariantId;
        if (row >= 0 && row < m_rows.size()) {
            const InventoryRow &r = m_rows[row];
            preselectedProductId = m_products[r.productIndex].id;
            if (r.variantIndex >= 0) preselectedVariantId = m_products[r.productIndex].variants[r.variantIndex].id;
        }
        TransferStockDialog dialog(m_client, m_products, m_shops, preselectedProductId, preselectedVariantId, this);
        if (dialog.exec() == QDialog::Accepted) {
            emit statusMessage("Stock transferred.");
            m_client->fetchProducts();
        }
    });

    m_table = new QTableWidget(this);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_table->setAlternatingRowColors(true);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::itemChanged, this, &InventoryPage::handleItemChanged);

    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        m_shops = shops;
        rebuildTable();
    });
    m_client->fetchShops();

    connect(m_client, &SupabaseClient::suppliersFetched, this, [this](const QVector<Supplier> &suppliers) {
        m_suppliers = suppliers;
    });
    m_client->fetchSuppliers();

    connect(m_client, &SupabaseClient::supplierCreated, this, [this](const Supplier &supplier) {
        m_suppliers.append(supplier);
        if (m_pendingSupplierRow >= 0 && m_pendingSupplierRow < m_rows.size()) {
            const Product &product = m_products[m_rows[m_pendingSupplierRow].productIndex];
            m_client->updateProductField(product.id, "supplier_id", supplier.id);
        }
        m_pendingSupplierRow = -1;
    });
    connect(m_client, &SupabaseClient::supplierCreateFailed, this, [this](const QString &message) {
        emit statusMessage("Couldn't create supplier: " + message);
        m_pendingSupplierRow = -1;
        m_client->fetchProducts();
    });

    connect(m_client, &SupabaseClient::productsFetched, this, [this](const QVector<Product> &products) {
        m_products = products;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::productsFetchFailed, this, [this](const QString &message) {
        emit statusMessage("Failed to load products: " + message);
    });
    m_client->fetchProducts();

    connect(m_client, &SupabaseClient::productUpdateFailed, this, [this](const QString &message) {
        emit statusMessage("Update failed: " + message);
        m_client->fetchProducts();
    });
    connect(m_client, &SupabaseClient::inventoryLevelsUpsertFailed, this, [this](const QString &message) {
        emit statusMessage("Stock update failed: " + message);
        m_client->fetchProducts();
    });
    connect(m_client, &SupabaseClient::variantUpdateFailed, this, [this](const QString &message) {
        emit statusMessage("Variant update failed: " + message);
        m_client->fetchProducts();
    });
}

void InventoryPage::rebuildTable() {
    m_populating = true;

    m_rows.clear();
    for (int p = 0; p < m_products.size(); ++p) {
        const Product &product = m_products[p];
        if (product.variants.isEmpty()) {
            m_rows.append({p, -1});
        } else {
            for (int v = 0; v < product.variants.size(); ++v) m_rows.append({p, v});
        }
    }

    QStringList headers{"Name", "Variant", "Supplier", "SKU", "Sell price", "Cost price"};
    for (const Shop &shop : m_shops) headers << shop.name;
    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);

    m_table->setRowCount(m_rows.size());
    for (int row = 0; row < m_rows.size(); ++row) {
        const InventoryRow &r = m_rows[row];
        const Product &product = m_products[r.productIndex];
        const bool isVariant = r.variantIndex >= 0;
        const ProductVariant *variant = isVariant ? &product.variants[r.variantIndex] : nullptr;

        auto *nameItem = new QTableWidgetItem(product.name);
        nameItem->setData(Qt::UserRole, product.id);
        m_table->setItem(row, kColName, nameItem);

        auto *variantItem = new QTableWidgetItem(variant ? variant->name : "—");
        if (!variant) variantItem->setFlags(variantItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColVariant, variantItem);

        QString supplierName;
        for (const Supplier &supplier : m_suppliers) {
            if (supplier.id == product.supplierId) {
                supplierName = supplier.name;
                break;
            }
        }
        m_table->setItem(row, kColSupplier, new QTableWidgetItem(supplierName));

        m_table->setItem(row, kColSku, new QTableWidgetItem(variant ? variant->sku : product.sku));
        m_table->setItem(row, kColSellPrice, new QTableWidgetItem(QString::number(product.sellPrice, 'f', 2)));
        m_table->setItem(row, kColCostPrice, new QTableWidgetItem(QString::number(product.costPrice, 'f', 2)));

        for (int s = 0; s < m_shops.size(); ++s) {
            const int quantity = variant ? variant->stockByShop.value(m_shops[s].id, 0)
                                          : product.stockByShop.value(m_shops[s].id, 0);
            auto *quantityItem = new QTableWidgetItem(QString::number(quantity));
            if (product.lowStockThreshold > 0 && quantity <= product.lowStockThreshold) {
                quantityItem->setForeground(QColor("#f59e0b"));
            }
            m_table->setItem(row, kFixedColumnCount + s, quantityItem);
        }
    }

    m_populating = false;
    emit statusMessage(QString("Connected to Supabase. %1 product(s) found.").arg(m_products.size()));
}

void InventoryPage::applySupplierEdit(int row, const QString &typedName) {
    const Product &product = m_products[m_rows[row].productIndex];

    if (typedName.isEmpty()) {
        m_client->updateProductField(product.id, "supplier_id", QJsonValue(QJsonValue::Null));
        return;
    }

    for (const Supplier &supplier : m_suppliers) {
        if (supplier.name.compare(typedName, Qt::CaseInsensitive) == 0) {
            m_client->updateProductField(product.id, "supplier_id", supplier.id);
            return;
        }
    }

    m_pendingSupplierRow = row;
    m_client->createSupplier(typedName);
}

void InventoryPage::handleItemChanged(QTableWidgetItem *item) {
    if (m_populating) return;

    const int row = item->row();
    const int col = item->column();
    if (row < 0 || row >= m_rows.size()) return;

    const InventoryRow &r = m_rows[row];
    const Product &product = m_products[r.productIndex];
    const bool isVariant = r.variantIndex >= 0;
    const QString variantId = isVariant ? product.variants[r.variantIndex].id : QString();
    const QString text = item->text().trimmed();

    if (col == kColName) {
        const QString oldName = product.name;
        m_client->updateProductField(product.id, "name", text);
        m_client->logAuditEvent("product_updated", "product", product.id,
                                 QString("name: %1 -> %2").arg(oldName, text));
    } else if (col == kColVariant) {
        if (!isVariant) {
            m_client->fetchProducts(); // "—" isn't meant to be editable
            return;
        }
        const QString oldName = product.variants[r.variantIndex].name;
        m_client->updateVariantField(variantId, "name", text);
        m_client->logAuditEvent("product_updated", "product", product.id,
                                 QString("variant name: %1 -> %2").arg(oldName, text));
    } else if (col == kColSupplier) {
        applySupplierEdit(row, text);
    } else if (col == kColSku) {
        if (isVariant) {
            const QString oldSku = product.variants[r.variantIndex].sku;
            m_client->updateVariantField(variantId, "sku", text);
            m_client->logAuditEvent("product_updated", "product", product.id,
                                     QString("variant sku: %1 -> %2").arg(oldSku, text));
        } else {
            const QString oldSku = product.sku;
            m_client->updateProductField(product.id, "sku", text);
            m_client->logAuditEvent("product_updated", "product", product.id,
                                     QString("sku: %1 -> %2").arg(oldSku, text));
        }
    } else if (col == kColSellPrice || col == kColCostPrice) {
        bool ok = false;
        const double value = text.toDouble(&ok);
        if (!ok || value < 0) {
            emit statusMessage("Invalid price — must be a non-negative number.");
            m_client->fetchProducts();
            return;
        }
        const bool isSell = col == kColSellPrice;
        const double oldValue = isSell ? product.sellPrice : product.costPrice;
        m_client->updateProductField(product.id, isSell ? "sell_price" : "cost_price", value);
        m_client->logAuditEvent(
            "product_updated", "product", product.id,
            QString("%1: %2 -> %3")
                .arg(QString(isSell ? "sell_price" : "cost_price"))
                .arg(oldValue, 0, 'f', 2)
                .arg(value, 0, 'f', 2));
    } else {
        const int shopIndex = col - kFixedColumnCount;
        if (shopIndex < 0 || shopIndex >= m_shops.size()) return;

        bool ok = false;
        const int quantity = text.toInt(&ok);
        if (!ok || quantity < 0) {
            emit statusMessage("Invalid quantity — must be a non-negative whole number.");
            m_client->fetchProducts();
            return;
        }
        const QString shopId = m_shops[shopIndex].id;
        const int oldQuantity = isVariant ? product.variants[r.variantIndex].stockByShop.value(shopId, 0)
                                           : product.stockByShop.value(shopId, 0);
        m_client->upsertInventoryLevels(product.id, {{shopId, quantity}}, variantId);
        m_client->logAuditEvent("inventory_adjusted", "inventory_levels", product.id,
                                 QString("%1 at %2: %3 -> %4")
                                     .arg(product.name, m_shops[shopIndex].name)
                                     .arg(oldQuantity)
                                     .arg(quantity));
    }
}

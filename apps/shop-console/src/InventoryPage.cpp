#include "InventoryPage.h"
#include "ProductDialog.h"
#include "ProductPhotosDialog.h"
#include "SupabaseClient.h"
#include "TransferStockDialog.h"

#include <QHBoxLayout>
#include <QHeaderView>
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
        if (row < 0 || row >= m_products.size()) {
            emit statusMessage("Select a product row first.");
            return;
        }
        const Product &product = m_products[row];
        ProductPhotosDialog dialog(m_client, product.id, product.name, this);
        dialog.exec();
    });
    connect(transferButton, &QPushButton::clicked, this, [this]() {
        if (m_shops.size() < 2) {
            emit statusMessage("Need at least two shops to transfer stock between.");
            return;
        }
        const int row = m_table->currentRow();
        const QString preselectedId = (row >= 0 && row < m_products.size()) ? m_products[row].id : QString();
        TransferStockDialog dialog(m_client, m_products, m_shops, preselectedId, this);
        if (dialog.exec() == QDialog::Accepted) {
            emit statusMessage("Stock transferred.");
            m_client->fetchProducts();
        }
    });

    m_table = new QTableWidget(this);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::itemChanged, this, &InventoryPage::handleItemChanged);

    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        m_shops = shops;
        rebuildTable();
    });
    m_client->fetchShops();

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
}

void InventoryPage::rebuildTable() {
    m_populating = true;

    QStringList headers{"Name", "SKU", "Sell price", "Cost price"};
    for (const Shop &shop : m_shops) headers << shop.name;
    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);

    m_table->setRowCount(m_products.size());
    for (int row = 0; row < m_products.size(); ++row) {
        const Product &product = m_products[row];

        auto *nameItem = new QTableWidgetItem(product.name);
        nameItem->setData(Qt::UserRole, product.id);
        m_table->setItem(row, 0, nameItem);
        m_table->setItem(row, 1, new QTableWidgetItem(product.sku));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::number(product.sellPrice, 'f', 2)));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(product.costPrice, 'f', 2)));

        for (int s = 0; s < m_shops.size(); ++s) {
            const int quantity = product.stockByShop.value(m_shops[s].id, 0);
            m_table->setItem(row, kFixedColumnCount + s, new QTableWidgetItem(QString::number(quantity)));
        }
    }

    m_populating = false;
    emit statusMessage(QString("Connected to Supabase. %1 product(s) found.").arg(m_products.size()));
}

void InventoryPage::handleItemChanged(QTableWidgetItem *item) {
    if (m_populating) return;

    const int row = item->row();
    const int col = item->column();
    const QTableWidgetItem *nameItem = m_table->item(row, 0);
    const QString productId = nameItem ? nameItem->data(Qt::UserRole).toString() : QString();
    if (productId.isEmpty()) return;

    const QString text = item->text().trimmed();

    if (col == 0) {
        m_client->updateProductField(productId, "name", text);
    } else if (col == 1) {
        m_client->updateProductField(productId, "sku", text);
    } else if (col == 2 || col == 3) {
        bool ok = false;
        const double value = text.toDouble(&ok);
        if (!ok || value < 0) {
            emit statusMessage("Invalid price — must be a non-negative number.");
            m_client->fetchProducts();
            return;
        }
        m_client->updateProductField(productId, col == 2 ? "sell_price" : "cost_price", value);
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
        m_client->upsertInventoryLevels(productId, {{m_shops[shopIndex].id, quantity}});
    }
}

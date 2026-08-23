#include "AdminWindow.h"
#include "ProductDialog.h"
#include "SupabaseClient.h"

#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

AdminWindow::AdminWindow(SupabaseClient *client, QWidget *parent)
    : QMainWindow(parent)
    , m_client(client) {
    setWindowTitle("Shop Console — Inventory Admin");
    this -> showMaximized();

    auto *toolbar = addToolBar("main");
    toolbar->setMovable(false);
    auto *title = new QLabel("Inventory Admin", this);
    title->setStyleSheet("font-weight: 600; padding: 0 8px;");
    toolbar->addWidget(title);
    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
    auto *refreshButton = new QPushButton("Refresh", this);
    toolbar->addWidget(refreshButton);
    auto *newProductButton = new QPushButton("New product", this);
    toolbar->addWidget(newProductButton);
    auto *signOutButton = new QPushButton("Sign out", this);
    toolbar->addWidget(signOutButton);
    connect(signOutButton, &QPushButton::clicked, this, &AdminWindow::signedOut);
    connect(refreshButton, &QPushButton::clicked, this, [this]() { m_client->fetchProducts(); });
    connect(newProductButton, &QPushButton::clicked, this, [this]() {
        ProductDialog dialog(m_client, this);
        if (dialog.exec() == QDialog::Accepted) {
            m_client->fetchProducts();
        }
    });

    m_table = new QTableWidget(this);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    setCentralWidget(m_table);

    connect(m_table, &QTableWidget::itemChanged, this, &AdminWindow::handleItemChanged);

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
        statusBar()->showMessage("Failed to load products: " + message);
    });
    m_client->fetchProducts();

    connect(m_client, &SupabaseClient::productUpdateFailed, this, [this](const QString &message) {
        statusBar()->showMessage("Update failed: " + message);
        m_client->fetchProducts();
    });
    connect(m_client, &SupabaseClient::inventoryLevelsUpsertFailed, this, [this](const QString &message) {
        statusBar()->showMessage("Stock update failed: " + message);
        m_client->fetchProducts();
    });
}

void AdminWindow::rebuildTable() {
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
    statusBar()->showMessage(QString("Connected to Supabase. %1 product(s) found.").arg(m_products.size()));
}

void AdminWindow::handleItemChanged(QTableWidgetItem *item) {
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
            statusBar()->showMessage("Invalid price — must be a non-negative number.");
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
            statusBar()->showMessage("Invalid quantity — must be a non-negative whole number.");
            m_client->fetchProducts();
            return;
        }
        m_client->upsertInventoryLevels(productId, {{m_shops[shopIndex].id, quantity}});
    }
}

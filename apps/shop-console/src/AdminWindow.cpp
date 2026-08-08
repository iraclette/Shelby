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
    resize(800, 600);

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
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"Name", "SKU", "Sell price", "Cost price", "Stock total", "Mostly in"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    setCentralWidget(m_table);

    connect(m_client, &SupabaseClient::productsFetched, this, [this](const QVector<Product> &products) {
        m_table->setRowCount(products.size());
        for (int row = 0; row < products.size(); ++row) {
            const Product &product = products[row];
            m_table->setItem(row, 0, new QTableWidgetItem(product.name));
            m_table->setItem(row, 1, new QTableWidgetItem(product.sku));
            m_table->setItem(row, 2, new QTableWidgetItem(QString::number(product.sellPrice, 'f', 2)));
            m_table->setItem(row, 3, new QTableWidgetItem(QString::number(product.costPrice, 'f', 2)));
            m_table->setItem(row, 4, new QTableWidgetItem(QString::number(product.stockTotal)));
            m_table->setItem(row, 5, new QTableWidgetItem(product.mostlyInShop));
        }
        statusBar()->showMessage(QString("Connected to Supabase. %1 product(s) found.").arg(products.size()));
    });

    connect(m_client, &SupabaseClient::productsFetchFailed, this, [this](const QString &message) {
        statusBar()->showMessage("Failed to load products: " + message);
    });

    m_client->fetchProducts();
}

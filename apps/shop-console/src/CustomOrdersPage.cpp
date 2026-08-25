#include "CustomOrdersPage.h"
#include "SupabaseClient.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
const QStringList kValidStatuses{"new", "reviewed", "quoted", "in_progress", "completed", "cancelled"};
}

CustomOrdersPage::CustomOrdersPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton("Refresh", this);
    toolbar->addWidget(refreshButton);
    toolbar->addStretch();

    connect(refreshButton, &QPushButton::clicked, this, [this]() { m_client->fetchCustomLeatherOrders(); });

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"Received", "Customer", "Contact", "Preferred shop", "Description", "Status"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(kColCreated, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColCustomer, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColShop, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColStatus, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::itemChanged, this, &CustomOrdersPage::handleItemChanged);

    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        m_shops = shops;
        rebuildTable();
    });
    m_client->fetchShops();

    connect(m_client, &SupabaseClient::customLeatherOrdersFetched, this,
            [this](const QVector<CustomLeatherOrder> &orders) {
        m_orders = orders;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::customLeatherOrdersFetchFailed, this, [this](const QString &message) {
        emit statusMessage("Failed to load custom orders: " + message);
    });
    m_client->fetchCustomLeatherOrders();

    connect(m_client, &SupabaseClient::customLeatherOrderUpdateFailed, this, [this](const QString &message) {
        emit statusMessage("Update failed: " + message);
        m_client->fetchCustomLeatherOrders();
    });
    connect(m_client, &SupabaseClient::customLeatherOrderUpdated, this,
            [this]() { m_client->fetchCustomLeatherOrders(); });
}

void CustomOrdersPage::rebuildTable() {
    m_populating = true;

    m_table->setRowCount(m_orders.size());
    for (int row = 0; row < m_orders.size(); ++row) {
        const CustomLeatherOrder &order = m_orders[row];

        auto *createdItem = new QTableWidgetItem(QString(order.createdAt).replace('T', ' ').left(19));
        createdItem->setData(Qt::UserRole, order.id);
        createdItem->setFlags(createdItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColCreated, createdItem);

        auto *customerItem = new QTableWidgetItem(order.customerName);
        customerItem->setFlags(customerItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColCustomer, customerItem);

        QString contact = order.customerEmail;
        if (!order.customerPhone.isEmpty()) contact += " / " + order.customerPhone;
        auto *contactItem = new QTableWidgetItem(contact);
        contactItem->setFlags(contactItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColContact, contactItem);

        QString shopName;
        for (const Shop &shop : m_shops) {
            if (shop.id == order.preferredShopId) {
                shopName = shop.name;
                break;
            }
        }
        auto *shopItem = new QTableWidgetItem(shopName);
        shopItem->setFlags(shopItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColShop, shopItem);

        auto *descriptionItem = new QTableWidgetItem(order.itemDescription);
        descriptionItem->setFlags(descriptionItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColDescription, descriptionItem);

        m_table->setItem(row, kColStatus, new QTableWidgetItem(order.status));
    }

    m_populating = false;
    emit statusMessage(QString("%1 custom order request(s).").arg(m_orders.size()));
}

void CustomOrdersPage::handleItemChanged(QTableWidgetItem *item) {
    if (m_populating || item->column() != kColStatus) return;

    const int row = item->row();
    const QTableWidgetItem *idItem = m_table->item(row, kColCreated);
    const QString orderId = idItem ? idItem->data(Qt::UserRole).toString() : QString();
    if (orderId.isEmpty()) return;

    const QString status = item->text().trimmed().toLower();
    if (!kValidStatuses.contains(status)) {
        emit statusMessage("Invalid status — must be one of: " + kValidStatuses.join(", "));
        m_client->fetchCustomLeatherOrders();
        return;
    }

    m_client->updateCustomLeatherOrderField(orderId, "status", status);
}

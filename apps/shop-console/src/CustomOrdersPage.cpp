#include "CustomOrdersPage.h"
#include "ShopTime.h"
#include "SupabaseClient.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
// Stored value and how it reads, in the order a request moves through them.
QList<QPair<QString, QString>> statuses() {
    return {{"new", QObject::tr("New")},           {"reviewed", QObject::tr("Reviewed")},
            {"quoted", QObject::tr("Quoted")},     {"in_progress", QObject::tr("In progress")},
            {"completed", QObject::tr("Completed")}, {"cancelled", QObject::tr("Cancelled")}};
}
} // namespace

CustomOrdersPage::CustomOrdersPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton(tr("Refresh"), this);
    toolbar->addWidget(refreshButton);
    toolbar->addStretch();

    connect(refreshButton, &QPushButton::clicked, this, [this]() { m_client->fetchCustomLeatherOrders(); });

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({tr("Received"), tr("Customer"), tr("Contact"), tr("Preferred shop"), tr("Description"), tr("Status")});
    // Contact and description share whatever room is left; the rest fit their contents.
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColContact, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(kColDescription, QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

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
        emit statusMessage(tr("Failed to load custom orders: ") + message);
    });
    m_client->fetchCustomLeatherOrders();

    connect(m_client, &SupabaseClient::customLeatherOrderUpdateFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Update failed: ") + message);
        m_client->fetchCustomLeatherOrders();
    });
    connect(m_client, &SupabaseClient::customLeatherOrderUpdated, this,
            [this]() { m_client->fetchCustomLeatherOrders(); });
}

void CustomOrdersPage::rebuildTable() {

    m_table->setRowCount(m_orders.size());
    for (int row = 0; row < m_orders.size(); ++row) {
        const CustomLeatherOrder &order = m_orders[row];

        auto *createdItem = new QTableWidgetItem(shoptime::format(order.createdAt, "yyyy-MM-dd HH:mm:ss"));
        createdItem->setData(Qt::UserRole, order.id);
        createdItem->setFlags(createdItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColCreated, createdItem);

        auto *customerItem = new QTableWidgetItem(order.customerName);
        customerItem->setFlags(customerItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColCustomer, customerItem);

        // Newer requests have only one of the two (the website asks for "phone or email").
        QStringList contactParts;
        if (!order.customerEmail.isEmpty()) contactParts << order.customerEmail;
        if (!order.customerPhone.isEmpty()) contactParts << order.customerPhone;
        const QString contact = contactParts.join(" / ");
        auto *contactItem = new QTableWidgetItem(contact);
        contactItem->setFlags(contactItem->flags() & ~Qt::ItemIsEditable);
        contactItem->setToolTip(contact);
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
        descriptionItem->setToolTip(order.itemDescription);
        m_table->setItem(row, kColDescription, descriptionItem);

        auto *statusCombo = new QComboBox(m_table);
        for (const auto &[key, label] : statuses()) statusCombo->addItem(label, key);
        if (statusCombo->findData(order.status) < 0) statusCombo->addItem(order.status, order.status);
        statusCombo->setCurrentIndex(statusCombo->findData(order.status));
        connect(statusCombo, &QComboBox::activated, this, [this, id = order.id, current = order.status, statusCombo]() {
            const QString status = statusCombo->currentData().toString();
            if (status != current) m_client->updateCustomLeatherOrderField(id, "status", status);
        });
        m_table->setCellWidget(row, kColStatus, statusCombo);
        // The column fits the longest status, not just the one showing (cell widgets aren't measured).
        m_table->horizontalHeader()->setSectionResizeMode(kColStatus, QHeaderView::Fixed);
        m_table->setColumnWidth(kColStatus, statusCombo->sizeHint().width() + 20);
    }

    emit statusMessage(tr("%n custom order requests.", nullptr, int(m_orders.size())));
}

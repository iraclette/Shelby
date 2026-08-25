#include "AuditLogPage.h"
#include "SupabaseClient.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <algorithm>

namespace {
// entity_type -> filter category. Kept separate from the raw action string
// so "product_updated" and any future product-flavored action both land
// under the same "Product" filter without needing to enumerate actions.
QString categoryForEntityType(const QString &entityType) {
    if (entityType == "product") return "Product";
    if (entityType == "profile") return "Staff";
    if (entityType == "inventory_levels") return "Inventory";
    return "Other";
}

QString prettyAction(const QString &action) {
    if (action == "product_updated") return "Product edited";
    if (action == "inventory_adjusted") return "Stock adjusted";
    if (action == "staff_updated") return "Staff updated";
    if (action == "staff_created") return "Staff created";
    if (action == "staff_deleted") return "Staff removed";
    return action;
}
} // namespace

AuditLogPage::AuditLogPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton("Refresh", this);
    toolbar->addWidget(refreshButton);
    toolbar->addSpacing(16);
    toolbar->addWidget(new QLabel("Show:", this));
    m_categoryFilter = new QComboBox(this);
    m_categoryFilter->addItems({"All", "Product", "Staff", "Inventory", "Stock transfer"});
    toolbar->addWidget(m_categoryFilter);
    toolbar->addStretch();

    connect(refreshButton, &QPushButton::clicked, this, [this]() {
        m_client->fetchAuditLog();
        m_client->fetchStockTransfers();
    });
    connect(m_categoryFilter, &QComboBox::currentIndexChanged, this, &AuditLogPage::rebuildTable);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"Time", "Actor", "Action", "Detail"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

    connect(m_client, &SupabaseClient::profilesFetched, this, [this](const QVector<Profile> &profiles) {
        m_profiles = profiles;
        rebuildTable();
    });
    m_client->fetchProfiles();

    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        m_shops = shops;
        rebuildTable();
    });
    m_client->fetchShops();

    connect(m_client, &SupabaseClient::auditLogFetched, this, [this](const QVector<AuditLogEntry> &entries) {
        m_entries = entries;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::auditLogFetchFailed, this, [this](const QString &message) {
        emit statusMessage("Failed to load activity log: " + message);
    });
    m_client->fetchAuditLog();

    connect(m_client, &SupabaseClient::stockTransfersFetched, this,
            [this](const QVector<StockTransferSummary> &transfers) {
        m_transfers = transfers;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::stockTransfersFetchFailed, this, [this](const QString &message) {
        emit statusMessage("Failed to load stock transfers: " + message);
    });
    m_client->fetchStockTransfers();
}

void AuditLogPage::rebuildTable() {
    struct Row {
        QString timestamp;
        QString actorName;
        QString action;
        QString detail;
        QString category;
    };
    QVector<Row> rows;

    for (const AuditLogEntry &entry : m_entries) {
        QString actorName = entry.actorId;
        for (const Profile &profile : m_profiles) {
            if (profile.id == entry.actorId) {
                actorName = profile.fullName;
                break;
            }
        }
        rows.append({entry.createdAt, actorName, prettyAction(entry.action), entry.detail,
                      categoryForEntityType(entry.entityType)});
    }

    for (const StockTransferSummary &transfer : m_transfers) {
        QString actorName = transfer.staffId;
        for (const Profile &profile : m_profiles) {
            if (profile.id == transfer.staffId) {
                actorName = profile.fullName;
                break;
            }
        }
        QString fromShop = transfer.fromShopId, toShop = transfer.toShopId;
        for (const Shop &shop : m_shops) {
            if (shop.id == transfer.fromShopId) fromShop = shop.name;
            if (shop.id == transfer.toShopId) toShop = shop.name;
        }
        QString productLabel = transfer.variantName.isEmpty()
            ? transfer.productName
            : QString("%1 (%2)").arg(transfer.productName, transfer.variantName);
        rows.append({transfer.createdAt, actorName, "Stock transferred",
                      QString("%1x %2: %3 → %4").arg(transfer.quantity).arg(productLabel, fromShop, toShop),
                      "Stock transfer"});
    }

    std::sort(rows.begin(), rows.end(),
              [](const Row &a, const Row &b) { return a.timestamp > b.timestamp; });

    const QString filter = m_categoryFilter->currentText();
    QVector<Row> filtered;
    for (const Row &row : rows) {
        if (filter == "All" || row.category == filter) filtered.append(row);
    }

    m_table->setRowCount(filtered.size());
    for (int i = 0; i < filtered.size(); ++i) {
        const Row &row = filtered[i];
        m_table->setItem(i, 0, new QTableWidgetItem(QString(row.timestamp).replace('T', ' ').left(19)));
        m_table->setItem(i, 1, new QTableWidgetItem(row.actorName));
        m_table->setItem(i, 2, new QTableWidgetItem(row.action));
        m_table->setItem(i, 3, new QTableWidgetItem(row.detail));
    }

    emit statusMessage(QString("%1 activity entr%2.").arg(filtered.size()).arg(filtered.size() == 1 ? "y" : "ies"));
}

#include "EmployeesPage.h"
#include "EmployeeDialog.h"
#include "SupabaseClient.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {
const QStringList kValidRoles{"owner", "admin", "staff"};
}

EmployeesPage::EmployeesPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton("Refresh", this);
    toolbar->addWidget(refreshButton);
    auto *newEmployeeButton = new QPushButton("New employee", this);
    toolbar->addWidget(newEmployeeButton);
    auto *removeButton = new QPushButton("Remove", this);
    toolbar->addWidget(removeButton);
    toolbar->addStretch();

    connect(refreshButton, &QPushButton::clicked, this, [this]() { m_client->fetchProfiles(); });
    connect(newEmployeeButton, &QPushButton::clicked, this, [this]() {
        EmployeeDialog dialog(m_client, m_shops, this);
        if (dialog.exec() == QDialog::Accepted) {
            m_client->fetchProfiles();
        }
    });
    connect(removeButton, &QPushButton::clicked, this, &EmployeesPage::removeSelected);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"Name", "Email", "Role", "Shop"});
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::itemChanged, this, &EmployeesPage::handleItemChanged);

    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        m_shops = shops;
    });
    m_client->fetchShops();

    connect(m_client, &SupabaseClient::profilesFetched, this, [this](const QVector<Profile> &profiles) {
        m_profiles = profiles;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::profilesFetchFailed, this, [this](const QString &message) {
        emit statusMessage("Failed to load employees: " + message);
    });
    m_client->fetchProfiles();

    connect(m_client, &SupabaseClient::profileUpdateFailed, this, [this](const QString &message) {
        emit statusMessage("Update failed: " + message);
        m_client->fetchProfiles();
    });
    connect(m_client, &SupabaseClient::profileUpdated, this, [this]() { m_client->fetchProfiles(); });

    connect(m_client, &SupabaseClient::staffAccountCreateFailed, this, [this](const QString &message) {
        QMessageBox::warning(this, "Couldn't create employee", message);
    });
    connect(m_client, &SupabaseClient::staffAccountDeleted, this, [this]() {
        emit statusMessage("Employee removed.");
        m_client->fetchProfiles();
    });
    connect(m_client, &SupabaseClient::staffAccountDeleteFailed, this, [this](const QString &message) {
        emit statusMessage("Couldn't remove employee: " + message);
    });
}

void EmployeesPage::rebuildTable() {
    m_populating = true;

    m_table->setRowCount(m_profiles.size());
    for (int row = 0; row < m_profiles.size(); ++row) {
        const Profile &profile = m_profiles[row];

        auto *nameItem = new QTableWidgetItem(profile.fullName);
        nameItem->setData(Qt::UserRole, profile.id);
        m_table->setItem(row, kColName, nameItem);

        auto *emailItem = new QTableWidgetItem(profile.email);
        emailItem->setFlags(emailItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColEmail, emailItem);

        m_table->setItem(row, kColRole, new QTableWidgetItem(profile.role));

        QString shopName;
        for (const Shop &shop : m_shops) {
            if (shop.id == profile.shopId) {
                shopName = shop.name;
                break;
            }
        }
        m_table->setItem(row, kColShop, new QTableWidgetItem(shopName));
    }

    m_populating = false;
    emit statusMessage(QString("Connected to Supabase. %1 employee(s) found.").arg(m_profiles.size()));
}

void EmployeesPage::handleItemChanged(QTableWidgetItem *item) {
    if (m_populating) return;

    const int row = item->row();
    const int col = item->column();
    const QTableWidgetItem *nameItem = m_table->item(row, kColName);
    const QString profileId = nameItem ? nameItem->data(Qt::UserRole).toString() : QString();
    if (profileId.isEmpty()) return;

    const QString text = item->text().trimmed();
    const Profile &profile = m_profiles[row];

    if (col == kColName) {
        m_client->updateProfileField(profileId, "full_name", text);
    } else if (col == kColRole) {
        const QString role = text.toLower();
        if (!kValidRoles.contains(role)) {
            emit statusMessage("Invalid role — must be owner, admin, or staff.");
            m_client->fetchProfiles();
            return;
        }
        m_client->updateProfileField(profileId, "role", role);
        m_client->logAuditEvent("staff_updated", "profile", profileId,
                                 QString("%1 role: %2 -> %3").arg(profile.fullName, profile.role, role));
    } else if (col == kColShop) {
        if (text.isEmpty()) {
            m_client->updateProfileField(profileId, "shop_id", QJsonValue(QJsonValue::Null));
            m_client->logAuditEvent("staff_updated", "profile", profileId,
                                     QString("%1 shop: unassigned").arg(profile.fullName));
            return;
        }
        QString shopId;
        for (const Shop &shop : m_shops) {
            if (shop.name.compare(text, Qt::CaseInsensitive) == 0) {
                shopId = shop.id;
                break;
            }
        }
        if (shopId.isEmpty()) {
            emit statusMessage("Unknown shop — leave blank for none, or type an existing shop's name.");
            m_client->fetchProfiles();
            return;
        }
        m_client->updateProfileField(profileId, "shop_id", shopId);
        m_client->logAuditEvent("staff_updated", "profile", profileId,
                                 QString("%1 shop: %2").arg(profile.fullName, text));
    }
}

void EmployeesPage::removeSelected() {
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_profiles.size()) {
        emit statusMessage("Select an employee row first.");
        return;
    }
    const Profile &profile = m_profiles[row];
    const auto reply = QMessageBox::question(this, "Remove employee",
        QString("Remove %1's login? This can't be undone.").arg(profile.fullName));
    if (reply != QMessageBox::Yes) return;

    m_client->deleteStaffAccount(profile.id);
}

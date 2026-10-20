#include "EmployeesPage.h"
#include "EmployeeDialog.h"
#include "SupabaseClient.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

EmployeesPage::EmployeesPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton(tr("Refresh"), this);
    toolbar->addWidget(refreshButton);
    auto *newEmployeeButton = new QPushButton(tr("New employee"), this);
    toolbar->addWidget(newEmployeeButton);
    auto *removeButton = new QPushButton(tr("Remove"), this);
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
    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Email"), tr("Role"), tr("Shop")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::itemChanged, this, &EmployeesPage::handleItemChanged);

    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        m_shops = shops;
        rebuildTable();
    });
    m_client->fetchShops();

    connect(m_client, &SupabaseClient::profilesFetched, this, [this](const QVector<Profile> &profiles) {
        m_profiles = profiles;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::profilesFetchFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Failed to load employees: ") + message);
    });
    m_client->fetchProfiles();

    connect(m_client, &SupabaseClient::profileUpdateFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Update failed: ") + message);
        m_client->fetchProfiles();
    });
    connect(m_client, &SupabaseClient::profileUpdated, this, [this]() { m_client->fetchProfiles(); });

    connect(m_client, &SupabaseClient::staffAccountCreateFailed, this, [this](const QString &message) {
        QMessageBox::warning(this, tr("Couldn't create employee"), message);
    });
    connect(m_client, &SupabaseClient::staffAccountDeleted, this, [this]() {
        emit statusMessage(tr("Employee removed."));
        m_client->fetchProfiles();
    });
    connect(m_client, &SupabaseClient::staffAccountDeleteFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Couldn't remove employee: ") + message);
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

        // Role and shop are picked from a list, in the console's language, not typed in.
        auto *roleCombo = new QComboBox(m_table);
        for (const QString &role : staffRoles()) roleCombo->addItem(roleLabel(role), role);
        if (roleCombo->findData(profile.role) < 0) roleCombo->addItem(profile.role, profile.role);
        roleCombo->setCurrentIndex(roleCombo->findData(profile.role));
        connect(roleCombo, &QComboBox::activated, this, [this, profile, roleCombo]() {
            const QString role = roleCombo->currentData().toString();
            if (role == profile.role) return;
            m_client->updateProfileField(profile.id, "role", role);
            m_client->logAuditEvent("staff_updated", "profile", profile.id,
                                    QString("%1 role: %2 -> %3").arg(profile.fullName, profile.role, role));
        });
        m_table->setCellWidget(row, kColRole, roleCombo);
        // The column fits the longest role, not just the one showing (cell widgets aren't measured).
        m_table->horizontalHeader()->setSectionResizeMode(kColRole, QHeaderView::Fixed);
        m_table->setColumnWidth(kColRole, roleCombo->sizeHint().width() + 20);

        auto *shopCombo = new QComboBox(m_table);
        shopCombo->addItem(tr("No shop"), QString());
        for (const Shop &shop : m_shops) shopCombo->addItem(shop.name, shop.id);
        shopCombo->setCurrentIndex(qMax(0, shopCombo->findData(profile.shopId)));
        connect(shopCombo, &QComboBox::activated, this, [this, profile, shopCombo]() {
            const QString shopId = shopCombo->currentData().toString();
            if (shopId == profile.shopId) return;
            m_client->updateProfileField(profile.id, "shop_id",
                                         shopId.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(shopId));
            m_client->logAuditEvent("staff_updated", "profile", profile.id,
                                    shopId.isEmpty() ? QString("%1 shop: unassigned").arg(profile.fullName)
                                                     : QString("%1 shop: %2").arg(profile.fullName, shopCombo->currentText()));
        });
        m_table->setCellWidget(row, kColShop, shopCombo);
    }

    m_populating = false;
    emit statusMessage(tr("%n employees.", nullptr, int(m_profiles.size())));
}

void EmployeesPage::handleItemChanged(QTableWidgetItem *item) {
    if (m_populating) return;

    const int row = item->row();
    const int col = item->column();
    const QTableWidgetItem *nameItem = m_table->item(row, kColName);
    const QString profileId = nameItem ? nameItem->data(Qt::UserRole).toString() : QString();
    if (profileId.isEmpty()) return;

    const QString text = item->text().trimmed();
    if (col == kColName) m_client->updateProfileField(profileId, "full_name", text);
}

void EmployeesPage::removeSelected() {
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_profiles.size()) {
        emit statusMessage(tr("Select an employee row first."));
        return;
    }
    const Profile &profile = m_profiles[row];
    const auto reply = QMessageBox::question(this, tr("Remove employee"),
        tr("Remove %1's login? This can't be undone.").arg(profile.fullName));
    if (reply != QMessageBox::Yes) return;

    m_client->deleteStaffAccount(profile.id);
}

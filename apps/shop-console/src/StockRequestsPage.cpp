#include "StockRequestsPage.h"
#include "Theme.h"
#include "ShopTime.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

namespace {
QString statusLabel(const QString &status) {
    if (status == "new") return "New";
    if (status == "contacted") return "Contacted";
    if (status == "dismissed") return "Dismissed";
    return status;
}

QColor statusColor(const QString &status) {
    if (status == "new") return theme::palette().accent;
    if (status == "contacted") return theme::palette().success;
    return theme::palette().textDisabled;
}
} // namespace

StockRequestsPage::StockRequestsPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton(tr("Refresh"), this);
    toolbar->addWidget(refreshButton);
    m_filter = new QComboBox(this);
    m_filter->addItem(tr("New"), "new");
    m_filter->addItem(tr("All requests"), "");
    m_filter->addItem(tr("Contacted"), "contacted");
    m_filter->addItem(tr("Dismissed"), "dismissed");
    toolbar->addWidget(m_filter);
    toolbar->addStretch();
    m_dismissButton = new QPushButton(tr("Dismiss"), this);
    m_contactedButton = new QPushButton(tr("Mark contacted"), this);
    m_contactedButton->setObjectName("primaryButton");
    for (QPushButton *button : {m_dismissButton, m_contactedButton}) {
        button->setEnabled(false);
        toolbar->addWidget(button);
    }

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("Received"), tr("Product"), tr("Phone or email"), tr("Status")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table, 1);

    connect(refreshButton, &QPushButton::clicked, this, [this]() { m_client->fetchStockRequests(); });
    connect(m_filter, &QComboBox::currentIndexChanged, this, &StockRequestsPage::rebuildTable);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &StockRequestsPage::updateButtons);
    connect(m_contactedButton, &QPushButton::clicked, this, [this]() { setSelectedStatus("contacted"); });
    connect(m_dismissButton, &QPushButton::clicked, this, [this]() { setSelectedStatus("dismissed"); });

    connect(m_client, &SupabaseClient::stockRequestsFetched, this, [this](const QVector<StockRequest> &requests) {
        m_requests = requests;
        rebuildTable();
        emit newCountChanged(std::count_if(requests.begin(), requests.end(),
                                           [](const StockRequest &r) { return r.status == "new"; }));
    });
    connect(m_client, &SupabaseClient::stockRequestsFetchFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Failed to load stock requests: ") + message);
    });
    connect(m_client, &SupabaseClient::stockRequestUpdated, this, [this]() {
        emit statusMessage(tr("Request updated."));
        m_client->fetchStockRequests();
    });
    connect(m_client, &SupabaseClient::stockRequestUpdateFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Update failed: ") + message);
        m_client->fetchStockRequests();
    });
    m_client->fetchStockRequests();
}

const StockRequest *StockRequestsPage::selectedRequest() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return nullptr;
    const int row = rows.first().row();
    return row < m_visible.size() ? &m_requests[m_visible[row]] : nullptr;
}

void StockRequestsPage::setSelectedStatus(const QString &status) {
    const StockRequest *request = selectedRequest();
    if (!request || request->status == status) return;
    for (QPushButton *button : {m_dismissButton, m_contactedButton}) button->setEnabled(false);
    m_client->updateStockRequestStatus(request->id, status);
}

void StockRequestsPage::rebuildTable() {
    const QString filter = m_filter->currentData().toString();
    m_visible.clear();
    for (int i = 0; i < m_requests.size(); ++i) {
        if (filter.isEmpty() || m_requests[i].status == filter) m_visible.append(i);
    }

    m_table->setRowCount(m_visible.size());
    for (int row = 0; row < m_visible.size(); ++row) {
        const StockRequest &request = m_requests[m_visible[row]];
        m_table->setItem(row, 0, new QTableWidgetItem(shoptime::format(request.createdAt, "yyyy-MM-dd HH:mm")));
        m_table->setItem(row, 1, new QTableWidgetItem(request.productName));
        m_table->setItem(row, 2, new QTableWidgetItem(request.contact));
        auto *statusItem = new QTableWidgetItem(statusLabel(request.status));
        statusItem->setForeground(statusColor(request.status));
        m_table->setItem(row, 3, statusItem);
    }
    updateButtons();
}

void StockRequestsPage::updateButtons() {
    const StockRequest *request = selectedRequest();
    m_contactedButton->setEnabled(request && request->status != "contacted");
    m_dismissButton->setEnabled(request && request->status != "dismissed");
}

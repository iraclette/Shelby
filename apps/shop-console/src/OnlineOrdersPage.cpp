#include "OnlineOrdersPage.h"
#include "Theme.h"
#include "BusinessProfile.h"
#include "ShopTime.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

namespace {
bool isDelivery(const OnlineOrder &order) {
    return order.deliveryMethod == "inex_branch" || order.deliveryMethod == "inex_home";
}

// Database status -> what the shop calls it.
QString statusLabel(const OnlineOrder &order) {
    const QString &status = order.status;
    if (status == "paid") return isDelivery(order) ? QObject::tr("To ship") : QObject::tr("To prepare");
    if (status == "fulfilled") return isDelivery(order) ? QObject::tr("Shipped") : QObject::tr("Collected");
    if (status == "pending") return QObject::tr("Awaiting payment");
    if (status == "failed") return QObject::tr("Payment failed");
    if (status == "cancelled") return QObject::tr("Cancelled");
    return status;
}

QString methodLabel(const OnlineOrder &order) {
    if (order.deliveryMethod == "inex_branch") return QObject::tr("Inex branch · %1").arg(order.deliveryCity);
    if (order.deliveryMethod == "inex_home") return QObject::tr("Inex home · %1").arg(order.deliveryCity);
    return {};
}

QColor statusColor(const QString &status) {
    if (status == "paid") return theme::palette().accent;
    if (status == "fulfilled") return theme::palette().success;
    if (status == "failed" || status == "cancelled") return theme::palette().textDisabled;
    return theme::palette().warning;
}


// First 8 characters of the UUID: what the customer sees as "Order #…" on the website.
QString shortId(const QString &id) {
    return id.left(8).toUpper();
}
} // namespace

OnlineOrdersPage::OnlineOrdersPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton(tr("Refresh"), this);
    toolbar->addWidget(refreshButton);
    m_filter = new QComboBox(this);
    m_filter->addItem(tr("To prepare / ship"), "paid");
    m_filter->addItem(tr("All orders"), "");
    m_filter->addItem(tr("Collected / shipped"), "fulfilled");
    m_filter->addItem(tr("Unpaid / failed"), "unpaid");
    toolbar->addWidget(m_filter);
    toolbar->addStretch();
    m_collectedButton = new QPushButton(tr("Mark collected"), this);
    m_collectedButton->setObjectName("primaryButton");
    m_collectedButton->setEnabled(false);
    toolbar->addWidget(m_collectedButton);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({tr("Date"), tr("Order"), tr("Customer"), tr("Pickup / delivery"), tr("Total"), tr("Status")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    auto *detailsPanel = new QFrame(this);
    detailsPanel->setObjectName("card");
    auto *detailsLayout = new QVBoxLayout(detailsPanel);
    detailsLayout->setContentsMargins(16, 14, 16, 14);
    m_details = new QLabel(tr("Select an order to see what to prepare."), detailsPanel);
    m_details->setObjectName("totalsKey");
    m_details->setWordWrap(true);
    m_details->setTextInteractionFlags(Qt::TextSelectableByMouse); // copy a phone number or email
    m_details->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    detailsLayout->addWidget(m_details, 1);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(m_table);
    splitter->addWidget(detailsPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(splitter, 1);

    connect(refreshButton, &QPushButton::clicked, this, [this]() { m_client->fetchOnlineOrders(); });
    connect(m_filter, &QComboBox::currentIndexChanged, this, &OnlineOrdersPage::rebuildTable);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &OnlineOrdersPage::showDetails);
    connect(m_collectedButton, &QPushButton::clicked, this, [this]() {
        const OnlineOrder *order = selectedOrder();
        if (!order || order->status != "paid") return;
        const bool delivery = isDelivery(*order);
        const auto answer = QMessageBox::question(
            this, delivery ? tr("Mark shipped") : tr("Mark collected"),
            delivery ? tr("Has order #%1 been handed to Inex?").arg(shortId(order->id))
                     : tr("Has %1 picked up order #%2?").arg(order->customerName, shortId(order->id)));
        if (answer != QMessageBox::Yes) return;
        m_collectedButton->setEnabled(false);
        m_client->updateOnlineOrderStatus(order->id, "fulfilled");
    });

    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        m_shops = shops;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::onlineOrdersFetched, this, [this](const QVector<OnlineOrder> &orders) {
        m_orders = orders;
        rebuildTable();
        emit toPrepareChanged(std::count_if(orders.begin(), orders.end(),
                                            [](const OnlineOrder &order) { return order.status == "paid"; }));
    });
    connect(m_client, &SupabaseClient::onlineOrdersFetchFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Failed to load online orders: ") + message);
    });
    connect(m_client, &SupabaseClient::onlineOrderUpdated, this, [this]() {
        emit statusMessage(tr("Order updated."));
        m_client->fetchOnlineOrders();
    });
    connect(m_client, &SupabaseClient::onlineOrderUpdateFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Update failed: ") + message);
        m_client->fetchOnlineOrders();
    });
    m_client->fetchOnlineOrders();
}

QString OnlineOrdersPage::shopName(const QString &shopId) const {
    for (const Shop &shop : m_shops) {
        if (shop.id == shopId) return shop.name;
    }
    return {};
}

const OnlineOrder *OnlineOrdersPage::selectedOrder() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return nullptr;
    const int row = rows.first().row();
    return row < m_visible.size() ? &m_orders[m_visible[row]] : nullptr;
}

void OnlineOrdersPage::rebuildTable() {
    const QString filter = m_filter->currentData().toString();
    m_visible.clear();
    for (int i = 0; i < m_orders.size(); ++i) {
        const QString &status = m_orders[i].status;
        const bool show = filter.isEmpty() || status == filter
                          || (filter == "unpaid" && (status == "pending" || status == "failed" || status == "cancelled"));
        if (show) m_visible.append(i);
    }

    m_table->setRowCount(m_visible.size());
    for (int row = 0; row < m_visible.size(); ++row) {
        const OnlineOrder &order = m_orders[m_visible[row]];
        m_table->setItem(row, 0, new QTableWidgetItem(shoptime::format(order.createdAt, "yyyy-MM-dd HH:mm")));
        m_table->setItem(row, 1, new QTableWidgetItem("#" + shortId(order.id)));
        m_table->setItem(row, 2, new QTableWidgetItem(order.customerName));
        m_table->setItem(row, 3, new QTableWidgetItem(isDelivery(order) ? methodLabel(order) : shopName(order.pickupShopId)));
        auto *totalItem = new QTableWidgetItem(money(order.total));
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 4, totalItem);
        auto *statusItem = new QTableWidgetItem(statusLabel(order));
        statusItem->setForeground(statusColor(order.status));
        m_table->setItem(row, 5, statusItem);
    }
    showDetails();
}

void OnlineOrdersPage::showDetails() {
    const OnlineOrder *order = selectedOrder();
    m_collectedButton->setEnabled(order && order->status == "paid");
    m_collectedButton->setText(order && isDelivery(*order) ? tr("Mark shipped") : tr("Mark collected"));
    if (!order) {
        m_details->setText(m_visible.isEmpty() ? tr("No orders here.") : tr("Select an order to see what to prepare."));
        return;
    }

    const QString strong = theme::hex(theme::palette().text);
    QString html = QString("<p style='font-size:16px; font-weight:700; color:%1;'>%2</p>"
                           "<p style='color:%3; font-weight:600;'>%4</p>")
                       .arg(theme::hex(theme::palette().textStrong), tr("Order #%1").arg(shortId(order->id)),
                            statusColor(order->status).name(), statusLabel(*order));
    html += QString("<p><b style='color:%1;'>%2</b><br>%3%4</p>")
                .arg(strong, order->customerName.toHtmlEscaped(), order->customerEmail.toHtmlEscaped(),
                     order->customerPhone.isEmpty() ? QString() : "<br>" + order->customerPhone.toHtmlEscaped());
    if (isDelivery(*order)) {
        // Everything needed to fill in the Inex parcel form.
        const QString shipTo = order->deliveryMethod == "inex_home" ? tr("Ship with Inex to the door:")
                                                                    : tr("Ship with Inex to a branch:");
        html += QString("<p>%1<br><b style='color:%2;'>%3</b>%4<br>%5</p>")
                    .arg(shipTo, strong, order->deliveryCity.toHtmlEscaped(),
                         order->deliveryAddress.isEmpty()
                             ? QString()
                             : QString("<br><b style='color:%1;'>%2</b>").arg(strong, order->deliveryAddress.toHtmlEscaped()),
                         tr("Ship from: %1").arg(shopName(order->pickupShopId)));
    } else {
        html += QString("<p>%1 <b style='color:%2;'>%3</b></p>").arg(tr("Pick up at:"), strong, shopName(order->pickupShopId));
    }
    html += "<table width='100%' cellspacing='0' cellpadding='4'>";
    for (const OnlineOrderItem &item : order->items) {
        html += QString("<tr><td style='color:%1;'>%2 × %3</td><td align='right'>%4</td></tr>")
                    .arg(strong, QString::number(item.quantity), item.productName.toHtmlEscaped(),
                         money(item.quantity * item.unitPrice));
    }
    if (order->deliveryFee > 0) {
        html += QString("<tr><td>%1</td><td align='right'>%2</td></tr>").arg(tr("Delivery"), money(order->deliveryFee));
    }
    html += QString("<tr><td style='color:%1; font-weight:700; padding-top:8px;'>%2</td>"
                    "<td align='right' style='color:%1; font-weight:700; padding-top:8px;'>%3</td></tr></table>")
                .arg(strong, tr("Total"), money(order->total));
    m_details->setText(html);
}

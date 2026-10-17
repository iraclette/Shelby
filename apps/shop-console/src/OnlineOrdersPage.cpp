#include "OnlineOrdersPage.h"
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
    if (status == "paid") return isDelivery(order) ? "To ship" : "To prepare";
    if (status == "fulfilled") return isDelivery(order) ? "Shipped" : "Collected";
    if (status == "pending") return "Awaiting payment";
    if (status == "failed") return "Payment failed";
    if (status == "cancelled") return "Cancelled";
    return status;
}

QString methodLabel(const OnlineOrder &order) {
    if (order.deliveryMethod == "inex_branch") return "Inex branch · " + order.deliveryCity;
    if (order.deliveryMethod == "inex_home") return "Inex home · " + order.deliveryCity;
    return {};
}

QColor statusColor(const QString &status) {
    if (status == "paid") return QColor("#4f8cff");
    if (status == "fulfilled") return QColor("#22c55e");
    if (status == "failed" || status == "cancelled") return QColor("#6b7280");
    return QColor("#f59e0b");
}

QString money(double amount) {
    return QString("GEL %1").arg(amount, 0, 'f', 2);
}

// First 8 characters of the UUID: what the customer sees as "Order #…" on the website.
QString shortId(const QString &id) {
    return id.left(8).toUpper();
}
} // namespace

OnlineOrdersPage::OnlineOrdersPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton("Refresh", this);
    toolbar->addWidget(refreshButton);
    m_filter = new QComboBox(this);
    m_filter->addItem("To prepare / ship", "paid");
    m_filter->addItem("All orders", "");
    m_filter->addItem("Collected / shipped", "fulfilled");
    m_filter->addItem("Unpaid / failed", "unpaid");
    toolbar->addWidget(m_filter);
    toolbar->addStretch();
    m_collectedButton = new QPushButton("Mark collected", this);
    m_collectedButton->setObjectName("primaryButton");
    m_collectedButton->setEnabled(false);
    toolbar->addWidget(m_collectedButton);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({"Date", "Order", "Customer", "Pickup / delivery", "Total", "Status"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    auto *detailsPanel = new QFrame(this);
    detailsPanel->setObjectName("card");
    auto *detailsLayout = new QVBoxLayout(detailsPanel);
    detailsLayout->setContentsMargins(16, 14, 16, 14);
    m_details = new QLabel("Select an order to see what to prepare.", detailsPanel);
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
            this, delivery ? "Mark shipped" : "Mark collected",
            delivery ? QString("Has order #%1 been handed to Inex?").arg(shortId(order->id))
                     : QString("Has %1 picked up order #%2?").arg(order->customerName, shortId(order->id)));
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
        emit statusMessage("Failed to load online orders: " + message);
    });
    connect(m_client, &SupabaseClient::onlineOrderUpdated, this, [this]() {
        emit statusMessage("Order updated.");
        m_client->fetchOnlineOrders();
    });
    connect(m_client, &SupabaseClient::onlineOrderUpdateFailed, this, [this](const QString &message) {
        emit statusMessage("Update failed: " + message);
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
    m_collectedButton->setText(order && isDelivery(*order) ? "Mark shipped" : "Mark collected");
    if (!order) {
        m_details->setText(m_visible.isEmpty() ? "No orders here." : "Select an order to see what to prepare.");
        return;
    }

    QString html = QString("<p style='font-size:16px; font-weight:700; color:#f2f3f5;'>Order #%1</p>"
                           "<p style='color:%2; font-weight:600;'>%3</p>")
                       .arg(shortId(order->id), statusColor(order->status).name(), statusLabel(*order));
    html += QString("<p><b style='color:#e8e9ec;'>%1</b><br>%2%3</p>")
                .arg(order->customerName.toHtmlEscaped(), order->customerEmail.toHtmlEscaped(),
                     order->customerPhone.isEmpty() ? QString() : "<br>" + order->customerPhone.toHtmlEscaped());
    if (isDelivery(*order)) {
        // Everything needed to fill in the Inex parcel form.
        html += QString("<p>Ship with Inex to %1:<br><b style='color:#e8e9ec;'>%2</b>%3<br>Ship from: %4</p>")
                    .arg(order->deliveryMethod == "inex_home" ? "the door" : "a branch",
                         order->deliveryCity.toHtmlEscaped(),
                         order->deliveryAddress.isEmpty()
                             ? QString()
                             : "<br><b style='color:#e8e9ec;'>" + order->deliveryAddress.toHtmlEscaped() + "</b>",
                         shopName(order->pickupShopId));
    } else {
        html += QString("<p>Pick up at: <b style='color:#e8e9ec;'>%1</b></p>").arg(shopName(order->pickupShopId));
    }
    html += "<table width='100%' cellspacing='0' cellpadding='4'>";
    for (const OnlineOrderItem &item : order->items) {
        html += QString("<tr><td style='color:#e8e9ec;'>%1 × %2</td><td align='right'>%3</td></tr>")
                    .arg(item.quantity)
                    .arg(item.productName.toHtmlEscaped(), money(item.quantity * item.unitPrice));
    }
    if (order->deliveryFee > 0) {
        html += QString("<tr><td>Delivery</td><td align='right'>%1</td></tr>").arg(money(order->deliveryFee));
    }
    html += QString("<tr><td style='color:#e8e9ec; font-weight:700; padding-top:8px;'>Total</td>"
                    "<td align='right' style='color:#e8e9ec; font-weight:700; padding-top:8px;'>%1</td></tr></table>")
                .arg(money(order->total));
    m_details->setText(html);
}

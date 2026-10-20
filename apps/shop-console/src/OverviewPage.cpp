#include "OverviewPage.h"
#include "BusinessProfile.h"
#include "ShopTime.h"

#include <QDate>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>
#include <QTableWidget>
#include <QTime>
#include <QVBoxLayout>

#include <algorithm>

namespace {

// A rounded panel with a muted caption on top; content goes in the returned layout.
QVBoxLayout *addCard(QWidget *parent, QGridLayout *grid, int row, int column, const QString &caption,
                     int rowSpan = 1, int columnSpan = 1) {
    auto *card = new QFrame(parent);
    card->setObjectName("card");
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(6);
    auto *captionLabel = new QLabel(caption, card);
    captionLabel->setObjectName("cardCaption");
    layout->addWidget(captionLabel);
    grid->addWidget(card, row, column, rowSpan, columnSpan);
    return layout;
}

QTableWidget *makeTable(const QStringList &headers, QWidget *parent) {
    auto *table = new QTableWidget(parent);
    table->setObjectName("cardTable");
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setMinimumHeight(220);
    return table;
}

QTableWidgetItem *numberItem(const QString &text) {
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}
} // namespace

// Seven bars, one per day, today highlighted. Painted by hand — Qt Charts
// would be a whole extra module for one small chart.
class SalesChart : public QWidget {
public:
    explicit SalesChart(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(190);
        setStyleSheet("background: transparent;"); // sit on the card, not the app-wide QWidget background
    }

    void setDays(const QVector<QPair<QDate, double>> &days) {
        m_days = days;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRectF area = rect().adjusted(4, 22, -4, -24); // room for values above, day names below
        if (m_days.isEmpty()) return;

        double peak = 0;
        for (const auto &day : m_days) peak = qMax(peak, day.second);
        if (peak <= 0) {
            painter.setPen(QColor("#6b7280"));
            painter.drawText(rect(), Qt::AlignCenter, "No sales in the last 7 days yet.");
            return;
        }
        const double slot = area.width() / m_days.size();
        const double barWidth = qMin(44.0, slot * 0.56);

        QFont small = font();
        small.setPixelSize(11);
        painter.setFont(small);
        for (int i = 0; i < m_days.size(); ++i) {
            const bool today = i == m_days.size() - 1;
            const double value = m_days[i].second;
            const double height = peak > 0 ? (value / peak) * area.height() : 0;
            const double x = area.left() + slot * i + (slot - barWidth) / 2;
            const QRectF bar(x, area.bottom() - qMax(height, 2.0), barWidth, qMax(height, 2.0));

            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(today ? "#4f8cff" : "#2b3a5c"));
            painter.drawRoundedRect(bar, 4, 4);

            painter.setPen(QColor(today ? "#e8e9ec" : "#9aa0ac"));
            const QRectF valueRect(area.left() + slot * i, bar.top() - 18, slot, 16);
            if (value > 0) painter.drawText(valueRect, Qt::AlignCenter, QString::number(value, 'f', 0));
            const QRectF labelRect(area.left() + slot * i, area.bottom() + 6, slot, 16);
            painter.drawText(labelRect, Qt::AlignCenter, today ? "Today" : m_days[i].first.toString("ddd"));
        }
    }

private:
    QVector<QPair<QDate, double>> m_days;
};

OverviewPage::OverviewPage(SupabaseClient *client, const QString &userName, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *content = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(20, 18, 20, 20);
    contentLayout->setSpacing(14);

    auto *headerRow = new QHBoxLayout;
    auto *headerText = new QVBoxLayout;
    headerText->setSpacing(2);
    const int hour = QTime::currentTime().hour();
    const QString partOfDay = hour < 12 ? "Good morning" : hour < 18 ? "Good afternoon" : "Good evening";
    m_greeting = new QLabel(userName.isEmpty() ? partOfDay : QString("%1, %2").arg(partOfDay, userName), content);
    m_greeting->setObjectName("greeting");
    auto *dateLabel = new QLabel(shoptime::today().toString("dddd, d MMMM yyyy"), content);
    dateLabel->setObjectName("totalsKey");
    headerText->addWidget(m_greeting);
    headerText->addWidget(dateLabel);
    headerRow->addLayout(headerText);
    headerRow->addStretch();
    auto *refreshButton = new QPushButton("Refresh", content);
    headerRow->addWidget(refreshButton, 0, Qt::AlignTop);
    contentLayout->addLayout(headerRow);

    auto *grid = new QGridLayout;
    grid->setSpacing(12);
    for (int column = 0; column < 4; ++column) grid->setColumnStretch(column, 1);

    auto addKpi = [&](int column, const QString &caption, QLabel *&value, QLabel *&detail) {
        QVBoxLayout *card = addCard(content, grid, 0, column, caption);
        value = new QLabel("—", content);
        value->setObjectName("kpiValue");
        detail = new QLabel(content);
        detail->setObjectName("totalsKey");
        card->addWidget(value);
        card->addWidget(detail);
    };
    QLabel *itemsDetail = nullptr;
    QLabel *averageDetail = nullptr;
    addKpi(0, "Revenue today", m_todayRevenue, m_todaySalesCount);
    addKpi(1, "Items sold today", m_itemsSold, itemsDetail);
    addKpi(2, "Average sale", m_averageSale, averageDetail);
    addKpi(3, "Last 7 days", m_weekRevenue, m_weekSalesCount);
    itemsDetail->setText("across all shops");
    averageDetail->setText("today");

    QVBoxLayout *chartCard = addCard(content, grid, 1, 0, "Sales — last 7 days (" + currencyLabel() + ")", 1, 3);
    m_chart = new SalesChart(content);
    chartCard->addWidget(m_chart, 1);

    QVBoxLayout *attentionCard = addCard(content, grid, 1, 3, "Needs attention");
    auto addAttention = [&](const QString &label, const QString &section, QLabel *&count) {
        auto *row = new QPushButton(content);
        row->setObjectName("attentionRow");
        row->setCursor(Qt::PointingHandCursor);
        row->setMinimumHeight(40); // a button doesn't size itself from a child layout
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(10, 8, 10, 8);
        auto *text = new QLabel(label, row);
        text->setAttribute(Qt::WA_TransparentForMouseEvents);
        count = new QLabel("0", row);
        count->setObjectName("countPill");
        count->setAttribute(Qt::WA_TransparentForMouseEvents);
        rowLayout->addWidget(text, 1);
        rowLayout->addWidget(count);
        attentionCard->addWidget(row);
        connect(row, &QPushButton::clicked, this, [this, section]() { emit openSection(section); });
    };
    addAttention("Online orders", "Online Orders", m_toPrepareCount);
    addAttention("Messages", "Messages", m_unreadCount);
    addAttention("Custom orders", "Custom Orders", m_newOrdersCount);
    addAttention("Low stock", "Inventory", m_lowStockCount);
    attentionCard->addStretch();

    QVBoxLayout *lowStockCard = addCard(content, grid, 2, 0, "Low stock", 1, 2);
    m_lowStockTable = makeTable({"Product", "Shop", "In stock", "Alert at"}, content);
    lowStockCard->addWidget(m_lowStockTable);

    QVBoxLayout *topCard = addCard(content, grid, 2, 2, "Best sellers — last 7 days", 1, 2);
    m_topProductsTable = makeTable({"Product", "Sold", "Revenue"}, content);
    topCard->addWidget(m_topProductsTable);

    contentLayout->addLayout(grid);
    contentLayout->addStretch();

    auto *scroll = new QScrollArea(this);
    scroll->setWidget(content);
    scroll->setWidgetResizable(true);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scroll);

    connect(refreshButton, &QPushButton::clicked, this, [this]() {
        reload();
        m_client->fetchProducts();
        m_client->fetchContactMessages();
        m_client->fetchCustomLeatherOrders();
        m_client->fetchOnlineOrders();
    });

    // These lists are shared with their own pages, so whichever page loads
    // them, the overview stays current too.
    connect(m_client, &SupabaseClient::productsFetched, this, [this](const QVector<Product> &products) {
        m_products = products;
        rebuildStock();
    });
    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        m_shops = shops;
        rebuildStock();
    });
    connect(m_client, &SupabaseClient::onlineOrdersFetched, this, [this](const QVector<OnlineOrder> &orders) {
        setAttentionCount(m_toPrepareCount,
                          std::count_if(orders.begin(), orders.end(),
                                        [](const OnlineOrder &order) { return order.status == "paid"; }));
    });
    connect(m_client, &SupabaseClient::contactMessagesFetched, this, [this](const QVector<ContactMessage> &messages) {
        const int unread = std::count_if(messages.begin(), messages.end(),
                                         [](const ContactMessage &message) { return message.status == "unread"; });
        setAttentionCount(m_unreadCount, unread);
        emit unreadMessagesChanged(unread);
    });
    connect(m_client, &SupabaseClient::customLeatherOrdersFetched, this,
            [this](const QVector<CustomLeatherOrder> &orders) {
        const int fresh = std::count_if(orders.begin(), orders.end(),
                                        [](const CustomLeatherOrder &order) { return order.status == "new"; });
        setAttentionCount(m_newOrdersCount, fresh);
        emit newOrdersChanged(fresh);
    });
    m_client->fetchContactMessages();
    m_client->fetchCustomLeatherOrders();

    reload();
}

void OverviewPage::reload() {
    // Same day boundaries as the Daily Summary page, so the numbers agree.
    const QDate today = shoptime::today();
    m_client->fetchSalesBetween(shoptime::dayStartUtc(today.addDays(-6)), shoptime::dayStartUtc(today.addDays(1)),
                                [self = QPointer<OverviewPage>(this)](const QVector<SaleSummary> &sales,
                                                                     const QString &error) {
        if (!self) return; // signed out while the request was in flight
        if (!error.isEmpty()) {
            emit self->statusMessage("Couldn't load sales for the overview: " + error);
            return;
        }
        self->applySales(sales);
    });
}

void OverviewPage::applySales(const QVector<SaleSummary> &sales) {
    const QDate today = shoptime::today();
    QMap<QDate, double> revenueByDay;
    double todayRevenue = 0;
    int todaySales = 0;
    int todayItems = 0;
    double weekRevenue = 0;
    struct ProductTotals {
        QString name;
        int quantity = 0;
        double revenue = 0;
    };
    QMap<QString, ProductTotals> byProduct;

    for (const SaleSummary &sale : sales) {
        const QDate day = shoptime::dayOf(sale.soldAt);
        revenueByDay[day] += sale.total;
        weekRevenue += sale.total;
        const bool isToday = day == today;
        if (isToday) {
            todayRevenue += sale.total;
            ++todaySales;
        }
        for (const SaleItemSummary &item : sale.items) {
            if (isToday) todayItems += item.quantity;
            ProductTotals &totals = byProduct[item.productId];
            totals.name = item.productName;
            totals.quantity += item.quantity;
            totals.revenue += item.quantity * item.unitPrice;
        }
    }

    m_todayRevenue->setText(money(todayRevenue));
    m_todaySalesCount->setText(todaySales == 1 ? "1 sale" : QString("%1 sales").arg(todaySales));
    m_itemsSold->setText(QString::number(todayItems));
    m_averageSale->setText(todaySales > 0 ? money(todayRevenue / todaySales) : "—");
    m_weekRevenue->setText(money(weekRevenue));
    m_weekSalesCount->setText(QString("%1 sales").arg(sales.size()));

    QVector<QPair<QDate, double>> days;
    for (int offset = 6; offset >= 0; --offset) {
        const QDate day = today.addDays(-offset);
        days.append({day, revenueByDay.value(day, 0)});
    }
    m_chart->setDays(days);

    QVector<ProductTotals> top = byProduct.values();
    std::sort(top.begin(), top.end(), [](const ProductTotals &a, const ProductTotals &b) {
        return a.quantity != b.quantity ? a.quantity > b.quantity : a.revenue > b.revenue;
    });
    top.resize(qMin<qsizetype>(top.size(), 8));
    m_topProductsTable->setRowCount(top.size());
    for (int row = 0; row < top.size(); ++row) {
        m_topProductsTable->setItem(row, 0, new QTableWidgetItem(top[row].name));
        m_topProductsTable->setItem(row, 1, numberItem(QString::number(top[row].quantity)));
        m_topProductsTable->setItem(row, 2, numberItem(money(top[row].revenue)));
    }
}

void OverviewPage::rebuildStock() {
    struct LowRow {
        QString name;
        QString shop;
        int quantity;
        int threshold;
    };
    QVector<LowRow> rows;
    for (const Product &product : m_products) {
        if (product.lowStockThreshold <= 0) continue; // alert disabled
        for (const Shop &shop : m_shops) {
            if (product.variants.isEmpty()) {
                const int quantity = product.stockByShop.value(shop.id, 0);
                if (quantity <= product.lowStockThreshold) {
                    rows.append({product.name, shop.name, quantity, product.lowStockThreshold});
                }
                continue;
            }
            for (const ProductVariant &variant : product.variants) {
                const int quantity = variant.stockByShop.value(shop.id, 0);
                if (quantity <= product.lowStockThreshold) {
                    rows.append({QString("%1 (%2)").arg(product.name, variant.name), shop.name, quantity,
                                 product.lowStockThreshold});
                }
            }
        }
    }
    std::sort(rows.begin(), rows.end(), [](const LowRow &a, const LowRow &b) { return a.quantity < b.quantity; });

    m_lowStockTable->setRowCount(rows.size());
    for (int row = 0; row < rows.size(); ++row) {
        m_lowStockTable->setItem(row, 0, new QTableWidgetItem(rows[row].name));
        m_lowStockTable->setItem(row, 1, new QTableWidgetItem(rows[row].shop));
        auto *quantityItem = numberItem(QString::number(rows[row].quantity));
        quantityItem->setForeground(QColor(rows[row].quantity == 0 ? "#f87171" : "#f59e0b"));
        m_lowStockTable->setItem(row, 2, quantityItem);
        m_lowStockTable->setItem(row, 3, numberItem(QString::number(rows[row].threshold)));
    }
    setAttentionCount(m_lowStockCount, rows.size());
}

void OverviewPage::setAttentionCount(QLabel *label, int count) {
    label->setText(QString::number(count));
    // Grey when there's nothing to do, accent when there is.
    label->setProperty("active", count > 0);
    label->style()->unpolish(label);
    label->style()->polish(label);
}

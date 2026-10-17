#pragma once

#include <QWidget>

#include "SupabaseClient.h"

class QGridLayout;
class QLabel;
class QTableWidget;
class SalesChart;

// Landing page: today's numbers, the last 7 days, and what needs a look
// (unread messages, new custom orders, low stock).
class OverviewPage : public QWidget {
    Q_OBJECT

public:
    OverviewPage(SupabaseClient *client, const QString &userName, QWidget *parent = nullptr);

    void reload();

signals:
    void statusMessage(const QString &message);
    // A "needs attention" row was clicked; section is the sidebar label to open.
    void openSection(const QString &section);
    // Drives the sidebar badges.
    void unreadMessagesChanged(int count);
    void newOrdersChanged(int count);

private:
    void rebuildStock();
    void applySales(const QVector<SaleSummary> &sales);
    void setAttentionCount(QLabel *label, int count);

    SupabaseClient *m_client;

    QLabel *m_greeting;
    QLabel *m_todayRevenue;
    QLabel *m_todaySalesCount;
    QLabel *m_itemsSold;
    QLabel *m_averageSale;
    QLabel *m_weekRevenue;
    QLabel *m_weekSalesCount;
    QLabel *m_toPrepareCount;
    QLabel *m_unreadCount;
    QLabel *m_newOrdersCount;
    QLabel *m_lowStockCount;
    SalesChart *m_chart;
    QTableWidget *m_lowStockTable;
    QTableWidget *m_topProductsTable;

    QVector<Product> m_products;
    QVector<Shop> m_shops;
};

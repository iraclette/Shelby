#pragma once

#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QTableWidget;
class QTableWidgetItem;

// Custom leather order requests from the storefront's /handmade-leather form.
class CustomOrdersPage : public QWidget {
    Q_OBJECT

public:
    explicit CustomOrdersPage(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);

private:
    void rebuildTable();
    void handleItemChanged(QTableWidgetItem *item);

    SupabaseClient *m_client;
    QTableWidget *m_table;

    QVector<CustomLeatherOrder> m_orders;
    QVector<Shop> m_shops;
    bool m_populating = false;

    static constexpr int kColCreated = 0;
    static constexpr int kColCustomer = 1;
    static constexpr int kColContact = 2;
    static constexpr int kColShop = 3;
    static constexpr int kColDescription = 4;
    static constexpr int kColStatus = 5;
};

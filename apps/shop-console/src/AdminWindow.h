#pragma once

#include <QMainWindow>
#include <QVector>

#include "SupabaseClient.h"

class QTableWidget;

// Product list with inline editing: double-click a cell to change name,
// SKU, sell price, cost price, or a shop's stock quantity directly.
class AdminWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AdminWindow(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void signedOut();

private:
    void rebuildTable();
    void handleItemChanged(class QTableWidgetItem *item);

    SupabaseClient *m_client;
    QTableWidget *m_table;

    QVector<Shop> m_shops;
    QVector<Product> m_products;
    bool m_populating = false; // guards against itemChanged firing during rebuildTable()

    static constexpr int kFixedColumnCount = 4; // Name, SKU, Sell price, Cost price
};

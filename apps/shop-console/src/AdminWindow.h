#pragma once

#include <QMainWindow>
#include <QVector>

#include "SupabaseClient.h"

class QLabel;
class QTableWidget;

// Product list with inline editing: double-click a cell to change name,
// SKU, sell price, cost price, or a shop's stock quantity directly.
class AdminWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AdminWindow(SupabaseClient *client, QWidget *parent = nullptr);

    // Shows a clickable toolbar notice; clicking opens releaseUrl (the
    // release's GitHub page) in the default browser. See UpdateChecker.
    void showUpdateBanner(const QString &version, const QString &releaseUrl);

signals:
    void signedOut();

private:
    void rebuildTable();
    void handleItemChanged(class QTableWidgetItem *item);

    SupabaseClient *m_client;
    QTableWidget *m_table;
    QLabel *m_updateBanner;

    QVector<Shop> m_shops;
    QVector<Product> m_products;
    bool m_populating = false; // guards against itemChanged firing during rebuildTable()

    static constexpr int kFixedColumnCount = 4; // Name, SKU, Sell price, Cost price
};

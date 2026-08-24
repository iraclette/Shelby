#pragma once

#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QTableWidget;

// The product/stock table — this is everything AdminWindow used to be
// before it became a multi-page shell (see EmployeesPage, SoftwarePage).
// Moved near-verbatim; behavior is unchanged from before the split.
class InventoryPage : public QWidget {
    Q_OBJECT

public:
    explicit InventoryPage(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    // AdminWindow owns the shared status bar (a QMainWindow-only widget);
    // pages report through this instead of reaching for one directly.
    void statusMessage(const QString &message);

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

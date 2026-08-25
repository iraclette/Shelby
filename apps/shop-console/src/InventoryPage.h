#pragma once

#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QTableWidget;

// The product/stock table — this is everything AdminWindow used to be
// before it became a multi-page shell (see EmployeesPage, SoftwarePage).
// One row per variant for a product that has variants (Variant column shows
// "—" and stays non-editable for a plain product's own row); non-variant
// products keep exactly one row each, same as always.
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
    void applySupplierEdit(int row, const QString &typedName);

    SupabaseClient *m_client;
    QTableWidget *m_table;

    QVector<Shop> m_shops;
    QVector<Product> m_products;
    QVector<Supplier> m_suppliers;
    bool m_populating = false; // guards against itemChanged firing during rebuildTable()

    // One entry per table row: which product, and which of its variants
    // (-1 = the product's own row, no variant).
    struct InventoryRow {
        int productIndex;
        int variantIndex;
    };
    QVector<InventoryRow> m_rows;

    // Row awaiting a just-created supplier's id (see applySupplierEdit) —
    // only one such edit is ever in flight at a time.
    int m_pendingSupplierRow = -1;

    static constexpr int kColName = 0;
    static constexpr int kColVariant = 1;
    static constexpr int kColSupplier = 2;
    static constexpr int kColSku = 3;
    static constexpr int kColSellPrice = 4;
    static constexpr int kColCostPrice = 5;
    static constexpr int kFixedColumnCount = 6;
};

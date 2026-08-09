#pragma once

#include <QMainWindow>
#include <QString>
#include <QVector>

#include "SupabaseClient.h"

class QButtonGroup;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

struct CartLine {
    QString productId;
    QString name;
    int quantity = 0;
    double unitPrice = 0;
    double unitCost = 0;
};

// Checkout view: a product tile grid (grouped by category, like the
// Aronium-style POS the shops already use) for click-to-add, plus a
// barcode field for scanning — either adds a line to the cart. "Complete
// sale" records the sale + line items and decrements stock for this shop.
// Online-only for now — see the Shop Console plan for the offline-queue
// follow-up.
class PosWindow : public QMainWindow {
    Q_OBJECT

public:
    PosWindow(SupabaseClient *client, QString shopId, QWidget *parent = nullptr);

signals:
    void signedOut();

private:
    void handleBarcodeEntered();
    void addToCart(const Product &product);
    void removeSelectedLine();
    void completeSale();
    void refreshCartTable();
    void rebuildCategoryChips();
    void rebuildProductGrid();

    SupabaseClient *m_client;
    QString m_shopId;

    QLineEdit *m_barcodeInput;
    QTableWidget *m_cartTable;
    QLabel *m_totalLabel;
    QLabel *m_statusLabel;
    QPushButton *m_completeSaleButton;

    QHBoxLayout *m_categoryChipsLayout;
    QButtonGroup *m_categoryButtonGroup;
    QGridLayout *m_productGridLayout;
    QString m_selectedCategoryId; // empty = "All"

    QVector<Category> m_categories;
    QVector<Product> m_allProducts;
    QVector<CartLine> m_cart;
};

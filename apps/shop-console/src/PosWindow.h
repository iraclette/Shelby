#pragma once

#include <QDateTime>
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
    QString variantId; // empty = no variant
    QString name;       // "Product (Variant)" once a variant is picked
    int quantity = 0;
    double unitPrice = 0; // price actually charged, may be discounted below listPrice
    double listPrice = 0; // product's sell_price at the moment it was added to the cart
    double unitCost = 0;
};

// A cart paused mid-sale (see PosWindow::holdCart) so a cashier can serve
// someone else and come back to it. In-memory only — lost on app close or
// crash, same as a till's "suspend" button; not persisted to Supabase.
struct HeldSale {
    QDateTime heldAt;
    QVector<CartLine> cart;
};

// Checkout view: a product tile grid (grouped by category, like the
// Aronium-style POS the shops already use) for click-to-add, plus a
// barcode field for scanning — either adds a line to the cart. A product
// with variants (see 0015_product_variants.sql) shows a small picker
// before adding; scanning a variant's own barcode adds it directly. A cart
// can be held (paused) to serve another customer and resumed later.
// "Complete sale" records the sale + line items and decrements stock for
// this shop. If the server can't be reached, the sale is queued locally
// instead of failing, and a timer periodically retries syncing the queue.
class PosWindow : public QMainWindow {
    Q_OBJECT

public:
    PosWindow(SupabaseClient *client, QString shopId, QWidget *parent = nullptr);

    // Shows a clickable toolbar notice; clicking opens releaseUrl (the
    // release's GitHub page) in the default browser. See UpdateChecker.
    void showUpdateBanner(const QString &version, const QString &releaseUrl);

signals:
    void signedOut();

private:
    void handleBarcodeEntered();
    void addToCart(const Product &product);
    void promptForVariant(const Product &product, QWidget *anchor);
    void removeSelectedLine();
    void openDiscountDialog(int row);
    void completeSale();
    void refreshCartTable();
    void rebuildCategoryChips();
    void rebuildProductGrid();
    void updatePendingBadge(int count);
    void holdCart();
    void updateHeldButton();
    void openHeldSalesDialog();

    SupabaseClient *m_client;
    QString m_shopId;

    QLineEdit *m_barcodeInput;
    QTableWidget *m_cartTable;
    QLabel *m_totalLabel;
    QLabel *m_statusLabel;
    QLabel *m_pendingBadge;
    QLabel *m_updateBanner;
    QPushButton *m_completeSaleButton;
    QPushButton *m_holdButton;
    QPushButton *m_heldButton;

    QHBoxLayout *m_categoryChipsLayout;
    QButtonGroup *m_categoryButtonGroup;
    QGridLayout *m_productGridLayout;
    QString m_selectedCategoryId; // empty = "All"

    QVector<Category> m_categories;
    QVector<Product> m_allProducts;
    QVector<CartLine> m_cart;
    QVector<HeldSale> m_heldSales;
};

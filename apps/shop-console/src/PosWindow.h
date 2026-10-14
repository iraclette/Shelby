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

// In-memory only, lost on app close.
struct HeldSale {
    QDateTime heldAt;
    QVector<CartLine> cart;
};

// Checkout: product grid + barcode scan. Sales made offline are queued and synced later.
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
    // For variant products: true if any variant has stock.
    bool productHasStockAtShop(const Product &product) const;
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

#pragma once

#include <QDateTime>
#include <QMainWindow>
#include <QString>
#include <QVector>

#include "SupabaseClient.h"

class FlowLayout;
class QButtonGroup;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
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

// Checkout, laid out like Aronium: receipt on the left, product tiles on the
// right, function keys for everything. Sales made offline are queued and synced later.
class PosWindow : public QMainWindow {
    Q_OBJECT

public:
    // embedded: shown as a page inside AdminWindow (dev only), so no toolbar of
    // its own — the admin window and CheckoutPage's bar cover it (F7 still opens Returns).
    PosWindow(SupabaseClient *client, QString shopId, QWidget *parent = nullptr, bool embedded = false);

    // Practice: Pay runs the whole flow but records nothing and leaves stock alone.
    void setPracticeMode(bool practice) { m_practiceMode = practice; }

    // Shows a clickable toolbar notice; clicking opens releaseUrl (the
    // release's GitHub page) in the default browser. See UpdateChecker.
    void showUpdateBanner(const QString &version, const QString &releaseUrl);

signals:
    void signedOut();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void handleBarcodeEntered();
    void addToCart(const Product &product);
    void promptForVariant(const Product &product, QWidget *anchor);
    // For variant products: true if any variant has stock.
    bool productHasStockAtShop(const Product &product) const;
    int stockAtShop(const Product &product) const; // summed over variants for variant products
    int availableStock(const QString &productId, const QString &variantId) const;
    int selectedRow() const; // -1 when nothing is selected
    void removeSelectedLine();
    void changeSelectedQuantity(int delta);
    void promptForQuantity();
    void openDiscountDialog(int row);
    void voidCart();
    void completeSale();
    // selectRow is reselected after the rebuild; -1 keeps the current row if it still exists.
    void refreshCartTable(int selectRow = -1);
    void updateLineButtons();
    void rebuildCategoryChips();
    void rebuildProductGrid();
    QString categoryColor(const QString &categoryId) const;
    QString topLevelCategoryId(const QString &categoryId) const;
    bool inSelectedCategory(const Product &product) const;
    void showStatus(const QString &message, const QString &color = "#ef4444");
    void updatePendingBadge(int count);
    void holdCart();
    void updateHeldButton();
    void openHeldSalesDialog();

    SupabaseClient *m_client;
    QString m_shopId;

    QLineEdit *m_barcodeInput;
    QTableWidget *m_cartTable;
    QLabel *m_itemsCountLabel;
    QLabel *m_subtotalLabel;
    QLabel *m_savingsLabel;
    QWidget *m_savingsRow;
    QLabel *m_totalLabel;
    QLabel *m_statusLabel;
    QLabel *m_pendingBadge;
    QLabel *m_updateBanner;
    QPushButton *m_minusButton;
    QPushButton *m_plusButton;
    QPushButton *m_quantityButton;
    QPushButton *m_discountButton;
    QPushButton *m_removeButton;
    QPushButton *m_completeSaleButton;
    QPushButton *m_holdButton;
    QPushButton *m_heldButton;
    QPushButton *m_voidButton;

    FlowLayout *m_categoryChipsLayout;
    QWidget *m_subcategoryChips; // second row, shown while a category with subcategories is picked
    FlowLayout *m_subcategoryChipsLayout;
    QButtonGroup *m_categoryButtonGroup;
    QGridLayout *m_productGridLayout;
    QScrollArea *m_gridScroll;
    int m_tileColumns = 4; // follows the grid's width, see eventFilter
    QString m_selectedCategoryId; // empty = "All"
    QVector<Product> m_visibleProducts; // what the grid currently shows, for Enter-to-add

    QVector<Category> m_categories;
    QVector<Product> m_allProducts;
    QVector<CartLine> m_cart;
    QVector<HeldSale> m_heldSales;
    bool m_practiceMode = false;
    QString m_lastPaymentNote; // " Change: GEL 4.50" (in the business's currency), appended to the sale-recorded message
};

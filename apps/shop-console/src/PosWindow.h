#pragma once

#include <QMainWindow>
#include <QString>
#include <QVector>

#include "SupabaseClient.h"

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

// Checkout view: scan/type a barcode, it looks the product up and adds a
// line to the cart; "Complete sale" records the sale + line items and
// decrements stock for this shop. Online-only for now — see the Shop
// Console plan for the offline-queue follow-up.
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

    SupabaseClient *m_client;
    QString m_shopId;

    QLineEdit *m_barcodeInput;
    QTableWidget *m_cartTable;
    QLabel *m_totalLabel;
    QLabel *m_statusLabel;
    QPushButton *m_completeSaleButton;

    QVector<CartLine> m_cart;
};

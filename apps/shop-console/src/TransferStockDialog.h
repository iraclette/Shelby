#pragma once

#include <QDialog>
#include <QVector>

#include "SupabaseClient.h"

class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;

// Admin-only stock move between shops, optionally for one variant.
class TransferStockDialog : public QDialog {
    Q_OBJECT

public:
    TransferStockDialog(SupabaseClient *client, QVector<Product> products, QVector<Shop> shops,
                         const QString &preselectedProductId, const QString &preselectedVariantId,
                         QWidget *parent = nullptr);

private:
    void rebuildVariantCombo();
    void updateAvailable();
    void submit();

    SupabaseClient *m_client;
    QVector<Product> m_products;
    QVector<Shop> m_shops;

    QComboBox *m_productCombo;
    QComboBox *m_variantCombo; // hidden unless the selected product has variants
    QLabel *m_variantLabel;
    QComboBox *m_fromShopCombo;
    QComboBox *m_toShopCombo;
    QSpinBox *m_quantitySpin;
    QLabel *m_availableLabel;
    QLabel *m_statusLabel;
    QPushButton *m_submitButton;

    // Consumed (and cleared) the first time rebuildVariantCombo() runs, so
    // later product changes don't keep reapplying the initial preselection.
    QString m_preselectedVariantId;
};

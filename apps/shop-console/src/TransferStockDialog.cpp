#include "TransferStockDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

TransferStockDialog::TransferStockDialog(SupabaseClient *client, QVector<Product> products, QVector<Shop> shops,
                                          const QString &preselectedProductId, QWidget *parent)
    : QDialog(parent), m_client(client), m_products(std::move(products)), m_shops(std::move(shops)) {
    setWindowTitle("Transfer Stock");
    resize(360, 280);

    m_productCombo = new QComboBox(this);
    for (const Product &product : m_products) {
        m_productCombo->addItem(product.name, product.id);
    }
    if (!preselectedProductId.isEmpty()) {
        const int index = m_productCombo->findData(preselectedProductId);
        if (index >= 0) m_productCombo->setCurrentIndex(index);
    }

    m_fromShopCombo = new QComboBox(this);
    m_toShopCombo = new QComboBox(this);
    for (const Shop &shop : m_shops) {
        m_fromShopCombo->addItem(shop.name, shop.id);
        m_toShopCombo->addItem(shop.name, shop.id);
    }
    if (m_shops.size() > 1) m_toShopCombo->setCurrentIndex(1);

    m_quantitySpin = new QSpinBox(this);
    m_quantitySpin->setRange(1, 1);

    m_availableLabel = new QLabel(this);
    m_availableLabel->setStyleSheet("color: #9aa0ac;");

    auto *form = new QFormLayout;
    form->addRow("Product", m_productCombo);
    form->addRow("From shop", m_fromShopCombo);
    form->addRow("To shop", m_toShopCombo);
    form->addRow("Quantity", m_quantitySpin);
    form->addRow("", m_availableLabel);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #dc2626;");
    m_statusLabel->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_submitButton = buttons->addButton("Transfer", QDialogButtonBox::AcceptRole);
    m_submitButton->setObjectName("primaryButton");

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_statusLabel);
    layout->addStretch();
    layout->addWidget(buttons);

    connect(m_productCombo, &QComboBox::currentIndexChanged, this, &TransferStockDialog::updateAvailable);
    connect(m_fromShopCombo, &QComboBox::currentIndexChanged, this, &TransferStockDialog::updateAvailable);
    connect(m_submitButton, &QPushButton::clicked, this, &TransferStockDialog::submit);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_client, &SupabaseClient::stockTransferred, this, [this]() { accept(); });
    connect(m_client, &SupabaseClient::stockTransferFailed, this, [this](const QString &message) {
        m_submitButton->setEnabled(true);
        m_statusLabel->setText(message);
    });

    updateAvailable();
}

void TransferStockDialog::updateAvailable() {
    const QString productId = m_productCombo->currentData().toString();
    const QString fromShopId = m_fromShopCombo->currentData().toString();

    int available = 0;
    for (const Product &product : m_products) {
        if (product.id == productId) {
            available = product.stockByShop.value(fromShopId, 0);
            break;
        }
    }

    m_availableLabel->setText(QString("%1 available at this shop").arg(available));
    m_quantitySpin->setRange(1, qMax(available, 1));
    if (available == 0) m_quantitySpin->setValue(1);
}

void TransferStockDialog::submit() {
    m_statusLabel->clear();

    const QString fromShopId = m_fromShopCombo->currentData().toString();
    const QString toShopId = m_toShopCombo->currentData().toString();
    if (fromShopId == toShopId) {
        m_statusLabel->setText("Pick two different shops.");
        return;
    }

    m_submitButton->setEnabled(false);
    m_statusLabel->setStyleSheet("color: #737373;");
    m_statusLabel->setText("Transferring…");

    m_client->transferStock(m_productCombo->currentData().toString(), fromShopId, toShopId,
                             m_quantitySpin->value());
}

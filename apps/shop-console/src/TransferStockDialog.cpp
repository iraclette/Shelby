#include "TransferStockDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

TransferStockDialog::TransferStockDialog(SupabaseClient *client, QVector<Product> products, QVector<Shop> shops,
                                          const QString &preselectedProductId, const QString &preselectedVariantId,
                                          QWidget *parent)
    : QDialog(parent)
    , m_client(client)
    , m_products(std::move(products))
    , m_shops(std::move(shops))
    , m_preselectedVariantId(preselectedVariantId) {
    setWindowTitle("Transfer Stock");
    resize(360, 320);

    m_productCombo = new QComboBox(this);
    for (const Product &product : m_products) {
        m_productCombo->addItem(product.name, product.id);
    }
    if (!preselectedProductId.isEmpty()) {
        const int index = m_productCombo->findData(preselectedProductId);
        if (index >= 0) m_productCombo->setCurrentIndex(index);
    }

    m_variantCombo = new QComboBox(this);

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
    m_variantLabel = new QLabel("Variant", this);
    form->addRow(m_variantLabel, m_variantCombo);
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

    connect(m_productCombo, &QComboBox::currentIndexChanged, this, &TransferStockDialog::rebuildVariantCombo);
    connect(m_variantCombo, &QComboBox::currentIndexChanged, this, &TransferStockDialog::updateAvailable);
    connect(m_fromShopCombo, &QComboBox::currentIndexChanged, this, &TransferStockDialog::updateAvailable);
    connect(m_submitButton, &QPushButton::clicked, this, &TransferStockDialog::submit);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_client, &SupabaseClient::stockTransferred, this, [this]() { accept(); });
    connect(m_client, &SupabaseClient::stockTransferFailed, this, [this](const QString &message) {
        m_submitButton->setEnabled(true);
        m_statusLabel->setText(message);
    });

    rebuildVariantCombo();
}

void TransferStockDialog::rebuildVariantCombo() {
    m_variantCombo->blockSignals(true);
    m_variantCombo->clear();

    const QString productId = m_productCombo->currentData().toString();
    const Product *selectedProduct = nullptr;
    for (const Product &product : m_products) {
        if (product.id == productId) {
            selectedProduct = &product;
            break;
        }
    }

    const bool hasVariants = selectedProduct && !selectedProduct->variants.isEmpty();
    m_variantCombo->setVisible(hasVariants);
    m_variantLabel->setVisible(hasVariants);

    if (hasVariants) {
        for (const ProductVariant &variant : selectedProduct->variants) {
            m_variantCombo->addItem(variant.name, variant.id);
        }
        if (!m_preselectedVariantId.isEmpty()) {
            const int index = m_variantCombo->findData(m_preselectedVariantId);
            if (index >= 0) m_variantCombo->setCurrentIndex(index);
        }
    }
    m_preselectedVariantId.clear();

    m_variantCombo->blockSignals(false);
    updateAvailable();
}

void TransferStockDialog::updateAvailable() {
    const QString productId = m_productCombo->currentData().toString();
    const QString fromShopId = m_fromShopCombo->currentData().toString();
    const QString variantId = m_variantCombo->isVisible() ? m_variantCombo->currentData().toString() : QString();

    int available = 0;
    for (const Product &product : m_products) {
        if (product.id != productId) continue;
        if (variantId.isEmpty()) {
            available = product.stockByShop.value(fromShopId, 0);
        } else {
            for (const ProductVariant &variant : product.variants) {
                if (variant.id == variantId) {
                    available = variant.stockByShop.value(fromShopId, 0);
                    break;
                }
            }
        }
        break;
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

    const QString variantId = m_variantCombo->isVisible() ? m_variantCombo->currentData().toString() : QString();
    m_client->transferStock(m_productCombo->currentData().toString(), fromShopId, toShopId, m_quantitySpin->value(),
                             variantId);
}

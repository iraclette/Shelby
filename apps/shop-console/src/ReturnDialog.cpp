#include "ReturnDialog.h"
#include "Theme.h"
#include "BusinessProfile.h"
#include "ShopTime.h"
#include "SupabaseClient.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

ReturnDialog::ReturnDialog(SupabaseClient *client, QString shopId, QWidget *parent)
    : QDialog(parent), m_client(client), m_shopId(std::move(shopId)) {
    setWindowTitle(tr("Process Return"));
    resize(420, 480);

    m_stack = new QStackedWidget(this);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_stack);

    // --- Page 0: pick a sale ---
    auto *salePage = new QWidget(this);
    auto *salePageLayout = new QVBoxLayout(salePage);
    salePageLayout->addWidget(new QLabel(tr("<b>Select a sale to return from</b>"), salePage));
    m_saleList = new QListWidget(salePage);
    salePageLayout->addWidget(m_saleList);
    m_stack->addWidget(salePage);

    connect(m_saleList, &QListWidget::itemDoubleClicked, this, [this]() { selectSale(m_saleList->currentRow()); });

    // --- Page 1: pick items + reason (contents rebuilt per selected sale) ---
    auto *itemsPage = new QWidget(this);
    m_itemsLayout = new QVBoxLayout(itemsPage);
    m_stack->addWidget(itemsPage);

    connect(m_client, &SupabaseClient::recentSalesFetched, this, [this](const QVector<SaleSummary> &sales) {
        m_sales = sales;
        m_saleList->clear();
        for (const SaleSummary &sale : m_sales) {
            m_saleList->addItem(tr("%1 — %2 (%n items)", nullptr, int(sale.items.size()))
                                     .arg(shoptime::format(sale.soldAt, "yyyy-MM-dd HH:mm"))
                                     .arg(money(sale.total)));
        }
    });
    connect(m_client, &SupabaseClient::recentSalesFetchFailed, this, [this](const QString &message) {
        QMessageBox::warning(this, tr("Couldn't load sales"), message);
    });
    m_client->fetchRecentSales(m_shopId);

    connect(m_client, &SupabaseClient::returnRecorded, this, [this]() { accept(); });
    connect(m_client, &SupabaseClient::returnRecordFailed, this, [this](const QString &message) {
        m_submitButton->setEnabled(true);
        m_statusLabel->setText(message);
    });
}

void ReturnDialog::selectSale(int row) {
    if (row < 0 || row >= m_sales.size()) return;
    m_selectedSaleIndex = row;
    showItemPicker();
}

void ReturnDialog::showItemPicker() {
    m_stack->setCurrentIndex(1);

    QLayoutItem *child;
    while ((child = m_itemsLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    m_itemSpinBoxes.clear();

    const SaleSummary &sale = m_sales[m_selectedSaleIndex];
    m_itemsLayout->addWidget(new QLabel(tr("<b>How many of each item are being returned?</b>"), nullptr));

    for (const SaleItemSummary &item : sale.items) {
        auto *row = new QHBoxLayout;
        const QString itemLabel = item.productName.isEmpty() ? item.productId : item.productName;
        const QString displayName =
            item.variantName.isEmpty() ? itemLabel : QString("%1 (%2)").arg(itemLabel, item.variantName);
        auto *label = new QLabel(tr("%1 (bought %2, %3 each)")
                                      .arg(displayName)
                                      .arg(item.quantity)
                                      .arg(money(item.unitPrice)));
        auto *spin = new QSpinBox;
        spin->setRange(0, item.quantity);
        row->addWidget(label, 1);
        row->addWidget(spin);
        m_itemsLayout->addLayout(row);
        m_itemSpinBoxes.append(spin);
    }

    m_reasonInput = new QLineEdit;
    m_reasonInput->setPlaceholderText(tr("Reason (optional)"));
    m_itemsLayout->addWidget(m_reasonInput);

    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet(theme::css(theme::palette().danger));
    m_statusLabel->setWordWrap(true);
    m_itemsLayout->addWidget(m_statusLabel);

    m_itemsLayout->addStretch();

    auto *buttons = new QDialogButtonBox;
    auto *backButton = buttons->addButton(tr("Back"), QDialogButtonBox::ResetRole);
    buttons->addButton(QDialogButtonBox::Cancel);
    m_submitButton = buttons->addButton(tr("Process Return"), QDialogButtonBox::AcceptRole);
    m_submitButton->setObjectName("primaryButton");
    m_itemsLayout->addWidget(buttons);

    connect(backButton, &QPushButton::clicked, this, [this]() { m_stack->setCurrentIndex(0); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_submitButton, &QPushButton::clicked, this, &ReturnDialog::submit);
}

void ReturnDialog::submit() {
    m_statusLabel->clear();

    const SaleSummary &sale = m_sales[m_selectedSaleIndex];
    QVector<ReturnItemInput> items;
    for (int i = 0; i < m_itemSpinBoxes.size(); ++i) {
        const int quantity = m_itemSpinBoxes[i]->value();
        if (quantity > 0) {
            items.append({sale.items[i].saleItemId, quantity});
        }
    }

    if (items.isEmpty()) {
        m_statusLabel->setText(tr("Select at least one item to return."));
        return;
    }

    m_submitButton->setEnabled(false);
    m_statusLabel->setStyleSheet(theme::css(theme::palette().textMuted));
    m_statusLabel->setText(tr("Processing…"));
    m_client->submitReturn(m_shopId, sale.id, m_reasonInput->text().trimmed(), items);
}

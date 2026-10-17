#include "CheckoutPage.h"
#include "PosWindow.h"
#include "ReturnDialog.h"
#include "SupabaseClient.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

CheckoutPage::CheckoutPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *bar = new QWidget(this);
    bar->setObjectName("pageBar");
    auto *barLayout = new QHBoxLayout(bar);
    barLayout->setContentsMargins(12, 8, 12, 8);
    barLayout->addWidget(new QLabel("Shop:", this));
    m_shopCombo = new QComboBox(this);
    m_shopCombo->setMinimumWidth(180);
    barLayout->addWidget(m_shopCombo);
    barLayout->addSpacing(16);
    m_recordCheck = new QCheckBox("Record real sales", this);
    barLayout->addWidget(m_recordCheck);
    m_modeLabel = new QLabel(this);
    barLayout->addWidget(m_modeLabel);
    barLayout->addStretch();
    auto *returnsButton = new QPushButton("Returns  F7", this);
    barLayout->addWidget(returnsButton);
    layout->addWidget(bar);

    m_registerLayout = new QVBoxLayout;
    m_registerLayout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(m_registerLayout, 1);

    auto updateModeLabel = [this]() {
        const bool practice = !m_recordCheck->isChecked();
        m_modeLabel->setText(practice ? "Practice mode — sales aren't saved and stock isn't touched."
                                      : "Live — sales are recorded and stock goes down.");
        m_modeLabel->setStyleSheet(practice ? "color: #a855f7;" : "color: #f59e0b; font-weight: 600;");
        if (m_register) m_register->setPracticeMode(practice);
    };
    updateModeLabel();

    connect(m_recordCheck, &QCheckBox::toggled, this, [this, updateModeLabel](bool checked) {
        if (checked) {
            const auto answer = QMessageBox::question(
                this, "Record real sales",
                "Sales made here will be recorded for the selected shop and its stock will go down. Continue?");
            if (answer != QMessageBox::Yes) {
                const QSignalBlocker blocker(m_recordCheck);
                m_recordCheck->setChecked(false);
            }
        }
        updateModeLabel();
    });

    connect(returnsButton, &QPushButton::clicked, this, [this]() {
        const QString shopId = m_shopCombo->currentData().toString();
        if (shopId.isEmpty()) return;
        // Returns always go to the server — practice mode doesn't cover them.
        ReturnDialog dialog(m_client, shopId, this);
        dialog.exec();
    });

    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        const QString current = m_shopCombo->currentData().toString();
        const QSignalBlocker blocker(m_shopCombo);
        m_shopCombo->clear();
        for (const Shop &shop : shops) m_shopCombo->addItem(shop.name, shop.id);
        const int index = qMax(0, m_shopCombo->findData(current));
        m_shopCombo->setCurrentIndex(index);
        if (m_shopCombo->count() > 0 && m_shopCombo->itemData(index).toString() != current) {
            openRegister(m_shopCombo->itemData(index).toString());
        }
    });
    connect(m_shopCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0) openRegister(m_shopCombo->itemData(index).toString());
    });
    m_client->fetchShops();
}

void CheckoutPage::openRegister(const QString &shopId) {
    // A register is tied to one shop (stock, sales), so switching shops means a fresh one.
    if (m_register) {
        m_registerLayout->removeWidget(m_register);
        m_register->deleteLater();
    }
    m_register = new PosWindow(m_client, shopId, this, /*embedded=*/true);
    m_register->setWindowFlags(Qt::Widget);
    m_register->setPracticeMode(!m_recordCheck->isChecked());
    m_registerLayout->addWidget(m_register);
    m_register->show();
}

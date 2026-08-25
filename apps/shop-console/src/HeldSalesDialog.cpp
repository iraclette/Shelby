#include "HeldSalesDialog.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

HeldSalesDialog::HeldSalesDialog(QVector<HeldSale> &heldSales, bool cartIsEmpty, QWidget *parent)
    : QDialog(parent), m_heldSales(heldSales), m_cartIsEmpty(cartIsEmpty) {
    setWindowTitle("Held Sales");
    resize(480, 320);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"Held at", "Items", "Total", ""});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    if (!cartIsEmpty) {
        auto *hint = new QLabel("Complete or hold the current cart before resuming a held sale.", this);
        hint->setStyleSheet("color: #9aa0ac;");
        hint->setWordWrap(true);
        layout->addWidget(hint);
    }
    layout->addWidget(m_table, 1);
    layout->addWidget(buttons);

    rebuildTable();
}

void HeldSalesDialog::rebuildTable() {
    m_table->setRowCount(m_heldSales.size());

    for (int row = 0; row < m_heldSales.size(); ++row) {
        const HeldSale &held = m_heldSales[row];

        int itemCount = 0;
        double total = 0;
        for (const CartLine &line : held.cart) {
            itemCount += line.quantity;
            total += line.quantity * line.unitPrice;
        }

        m_table->setItem(row, 0, new QTableWidgetItem(held.heldAt.toString("hh:mm:ss")));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(itemCount)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString("GEL %1").arg(total, 0, 'f', 2)));

        auto *actionsWidget = new QWidget(this);
        auto *actionsLayout = new QHBoxLayout(actionsWidget);
        actionsLayout->setContentsMargins(0, 0, 0, 0);

        auto *resumeButton = new QPushButton("Resume", this);
        resumeButton->setEnabled(m_cartIsEmpty);
        actionsLayout->addWidget(resumeButton);
        connect(resumeButton, &QPushButton::clicked, this, [this, row]() {
            m_didResume = true;
            m_resumedCart = m_heldSales[row].cart;
            m_heldSales.remove(row);
            accept();
        });

        auto *discardButton = new QPushButton("Discard", this);
        actionsLayout->addWidget(discardButton);
        connect(discardButton, &QPushButton::clicked, this, [this, row]() {
            if (QMessageBox::question(this, "Discard held sale", "Discard this held sale? This can't be undone.") !=
                QMessageBox::Yes) {
                return;
            }
            m_heldSales.remove(row);
            rebuildTable();
        });

        m_table->setCellWidget(row, 3, actionsWidget);
    }
}

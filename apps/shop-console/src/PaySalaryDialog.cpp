#include "PaySalaryDialog.h"
#include "SupabaseClient.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

PaySalaryDialog::PaySalaryDialog(SupabaseClient *client, QString staffId, QString shopId,
                                  const QString &employeeName, QString periodStartIso, QString periodEndIso,
                                  double suggestedAmount, QWidget *parent)
    : QDialog(parent), m_client(client), m_staffId(std::move(staffId)), m_shopId(std::move(shopId)),
      m_periodStartIso(std::move(periodStartIso)), m_periodEndIso(std::move(periodEndIso)) {
    setWindowTitle("Record Salary Payment");
    resize(360, 220);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(
        QString("<b>%1</b><br>Period: %2 – %3").arg(employeeName, m_periodStartIso, m_periodEndIso), this));

    m_amountSpin = new QDoubleSpinBox(this);
    m_amountSpin->setRange(0, 1000000);
    m_amountSpin->setDecimals(2);
    m_amountSpin->setSuffix(" GEL");
    m_amountSpin->setValue(qMax(0.0, suggestedAmount));

    m_noteInput = new QLineEdit(this);
    m_noteInput->setPlaceholderText("Note (optional)");

    auto *form = new QFormLayout;
    form->addRow("Amount", m_amountSpin);
    form->addRow("Note", m_noteInput);
    layout->addLayout(form);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #dc2626;");
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);
    layout->addStretch();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_submitButton = buttons->addButton("Record Payment", QDialogButtonBox::AcceptRole);
    m_submitButton->setObjectName("primaryButton");
    layout->addWidget(buttons);

    connect(m_submitButton, &QPushButton::clicked, this, &PaySalaryDialog::submit);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_client, &SupabaseClient::salaryPaymentRecorded, this, [this]() { accept(); });
    connect(m_client, &SupabaseClient::salaryPaymentRecordFailed, this, [this](const QString &message) {
        m_submitButton->setEnabled(true);
        m_statusLabel->setText(message);
    });
}

void PaySalaryDialog::submit() {
    m_statusLabel->clear();

    if (m_amountSpin->value() <= 0) {
        m_statusLabel->setText("Enter an amount greater than zero.");
        return;
    }

    m_submitButton->setEnabled(false);
    m_client->recordSalaryPayment(m_staffId, m_shopId, m_periodStartIso, m_periodEndIso, m_amountSpin->value(),
                                   m_noteInput->text().trimmed());
}

#include "PaymentDialog.h"
#include "Theme.h"
#include "BusinessProfile.h"

#include <QButtonGroup>
#include <QDoubleValidator>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>

#include <cmath>

namespace {
// "50" for whole amounts, "2.50" otherwise.
QString amountText(double amount) {
    return std::fmod(amount, 1.0) == 0 ? QString::number(amount, 'f', 0) : QString::number(amount, 'f', 2);
}
} // namespace

PaymentDialog::PaymentDialog(double total, QWidget *parent)
    : QDialog(parent), m_total(total) {
    setWindowTitle(tr("Payment"));
    setMinimumWidth(460);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    auto *dueFrame = new QFrame(this);
    dueFrame->setObjectName("totalsPanel");
    auto *dueLayout = new QHBoxLayout(dueFrame);
    dueLayout->setContentsMargins(14, 10, 14, 10);
    auto *dueKey = new QLabel(tr("TOTAL DUE"), this);
    dueKey->setObjectName("totalKey");
    auto *dueValue = new QLabel(money(total), this);
    dueValue->setObjectName("totalValue");
    dueValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    dueLayout->addWidget(dueKey, 0, Qt::AlignBottom);
    dueLayout->addWidget(dueValue, 1);
    layout->addWidget(dueFrame);

    // Same chip look as the category filter: one filled = selected.
    auto *methodRow = new QHBoxLayout;
    m_cashButton = new QPushButton(tr("Cash"), this);
    m_cardButton = new QPushButton(tr("Card"), this);
    auto *methodGroup = new QButtonGroup(this);
    for (QPushButton *button : {m_cashButton, m_cardButton}) {
        button->setObjectName("categoryChip");
        button->setCheckable(true);
        button->setMinimumHeight(40);
        methodGroup->addButton(button);
        methodRow->addWidget(button, 1);
    }
    layout->addLayout(methodRow);
    // Settings → Checkout: only the methods the business takes; with just one there's nothing to pick.
    const BusinessSettings &business = BusinessProfile::instance()->settings();
    const bool takesCash = business.paymentMethods.contains("cash");
    const bool takesCard = business.paymentMethods.contains("card");
    m_cashButton->setVisible(takesCash && takesCard);
    m_cardButton->setVisible(takesCash && takesCard);
    const bool startWithCard = !takesCash || (takesCard && business.defaultPaymentMethod == "card");

    m_cashPanel = new QWidget(this);
    auto *cashLayout = new QVBoxLayout(m_cashPanel);
    cashLayout->setContentsMargins(0, 0, 0, 0);
    cashLayout->setSpacing(8);

    auto *receivedLabel = new QLabel(tr("Cash received"), this);
    receivedLabel->setObjectName("totalsKey");
    cashLayout->addWidget(receivedLabel);
    // A plain line edit, not a spin box: no arrows to fumble, and typing
    // "50" over the selected amount just works.
    m_receivedEdit = new QLineEdit(this);
    m_receivedEdit->setObjectName("cashReceived");
    auto *validator = new QDoubleValidator(0, 1000000, 2, m_receivedEdit);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::c());
    m_receivedEdit->setValidator(validator);
    setReceived(total);
    cashLayout->addWidget(m_receivedEdit);

    auto *quickGrid = new QGridLayout;
    quickGrid->setSpacing(6);
    auto *exactButton = new QPushButton(tr("Exact"), this);
    quickGrid->addWidget(exactButton, 0, 0);
    connect(exactButton, &QPushButton::clicked, this, [this]() { setReceived(m_total); });
    int index = 1;
    // The quick-cash buttons, like Aronium's (Settings → Checkout).
    for (double note : BusinessProfile::instance()->settings().quickCashAmounts) {
        auto *noteButton = new QPushButton(amountText(note), this);
        // A note smaller than the bill can't be "the" payment on its own.
        noteButton->setEnabled(note >= total);
        connect(noteButton, &QPushButton::clicked, this, [this, note]() { setReceived(note); });
        quickGrid->addWidget(noteButton, index / 4, index % 4);
        ++index;
    }
    cashLayout->addLayout(quickGrid);
    layout->addWidget(m_cashPanel);

    auto *changeFrame = new QFrame(this);
    changeFrame->setObjectName("totalsPanel");
    auto *changeLayout = new QHBoxLayout(changeFrame);
    changeLayout->setContentsMargins(14, 10, 14, 10);
    m_changeKey = new QLabel(this);
    m_changeKey->setObjectName("totalKey");
    m_changeLabel = new QLabel(this);
    m_changeLabel->setObjectName("totalValue");
    m_changeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    changeLayout->addWidget(m_changeKey, 0, Qt::AlignBottom);
    changeLayout->addWidget(m_changeLabel, 1);
    layout->addWidget(changeFrame);

    auto *buttonsRow = new QHBoxLayout;
    auto *cancelButton = new QPushButton(tr("Cancel  Esc"), this);
    cancelButton->setAutoDefault(false);
    m_confirmButton = new QPushButton(tr("Complete sale  Enter"), this);
    m_confirmButton->setObjectName("payButton");
    m_confirmButton->setMinimumHeight(52);
    m_confirmButton->setDefault(true);
    buttonsRow->addWidget(cancelButton, 1);
    buttonsRow->addWidget(m_confirmButton, 2);
    layout->addLayout(buttonsRow);

    connect(m_cashButton, &QPushButton::clicked, this, [this]() { setCard(false); });
    connect(m_cardButton, &QPushButton::clicked, this, [this]() { setCard(true); });
    connect(m_receivedEdit, &QLineEdit::textChanged, this, &PaymentDialog::updateChange);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_confirmButton, &QPushButton::clicked, this, &QDialog::accept);
    // F12 again finishes the sale, so Pay → Pay works without the mouse.
    connect(new QShortcut(Qt::Key_F12, this), &QShortcut::activated, m_confirmButton, &QPushButton::click);

    (startWithCard ? m_cardButton : m_cashButton)->setChecked(true);
    setCard(startWithCard);
}

double PaymentDialog::change() const {
    if (m_card) return 0;
    return qMax(0.0, received() - m_total);
}

double PaymentDialog::received() const {
    return QLocale::c().toDouble(m_receivedEdit->text().trimmed());
}

void PaymentDialog::setReceived(double amount) {
    m_receivedEdit->setText(QString::number(amount, 'f', 2));
    m_receivedEdit->selectAll();
}

void PaymentDialog::setCard(bool card) {
    m_card = card;
    m_cashPanel->setVisible(!card);
    updateChange();
    adjustSize();
    if (!card) {
        m_receivedEdit->setFocus();
        m_receivedEdit->selectAll();
    }
}

void PaymentDialog::updateChange() {
    if (m_card) {
        m_changeKey->setText(tr("CARD"));
        m_changeLabel->setText(tr("No change"));
        m_changeLabel->setStyleSheet(theme::css(theme::palette().textMuted));
        m_confirmButton->setEnabled(true);
        return;
    }

    // Compared in tetri so 10.1 - 10.1 never reads as a hair short.
    const qint64 receivedTetri = std::llround(received() * 100);
    const qint64 totalTetri = std::llround(m_total * 100);
    const bool enough = receivedTetri >= totalTetri;
    m_changeKey->setText(enough ? tr("CHANGE") : tr("STILL DUE"));
    m_changeLabel->setText(money((enough ? receivedTetri - totalTetri : totalTetri - receivedTetri) / 100.0));
    m_changeLabel->setStyleSheet(theme::css(enough ? theme::palette().success : theme::palette().danger));
    m_confirmButton->setEnabled(enough);
}

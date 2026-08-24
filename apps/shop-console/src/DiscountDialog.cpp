#include "DiscountDialog.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

DiscountDialog::DiscountDialog(const QString &productName, double listPrice, double currentPrice, QWidget *parent)
    : QDialog(parent), m_listPrice(listPrice), m_resultPrice(currentPrice) {
    setWindowTitle("Apply Discount");
    resize(320, 200);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(
        QString("<b>%1</b><br>List price: GEL %2").arg(productName).arg(listPrice, 0, 'f', 2), this));

    auto *modeRow = new QHBoxLayout;
    m_percentRadio = new QRadioButton("% off", this);
    m_amountRadio = new QRadioButton("GEL off", this);
    auto *modeGroup = new QButtonGroup(this);
    modeGroup->addButton(m_percentRadio);
    modeGroup->addButton(m_amountRadio);
    modeRow->addWidget(m_percentRadio);
    modeRow->addWidget(m_amountRadio);
    layout->addLayout(modeRow);

    m_valueSpin = new QDoubleSpinBox(this);
    m_valueSpin->setDecimals(2);
    layout->addWidget(m_valueSpin);

    m_previewLabel = new QLabel(this);
    m_previewLabel->setStyleSheet("font-weight: 600;");
    layout->addWidget(m_previewLabel);
    layout->addStretch();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);

    connect(m_percentRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (!checked) return;
        m_valueSpin->setSuffix(" %");
        m_valueSpin->setRange(0, 100);
        updatePreview();
    });
    connect(m_amountRadio, &QRadioButton::toggled, this, [this](bool checked) {
        if (!checked) return;
        m_valueSpin->setSuffix(" GEL");
        m_valueSpin->setRange(0, m_listPrice);
        updatePreview();
    });
    connect(m_valueSpin, &QDoubleSpinBox::valueChanged, this, &DiscountDialog::updatePreview);
    connect(buttons, &QDialogButtonBox::accepted, this, &DiscountDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Preselect whatever discount is already on this line (in % form) so
    // reopening the dialog to tweak it doesn't silently reset to zero.
    if (listPrice > 0 && currentPrice < listPrice) {
        m_percentRadio->setChecked(true);
        m_valueSpin->setValue((1.0 - currentPrice / listPrice) * 100.0);
    } else {
        m_percentRadio->setChecked(true);
    }
    updatePreview();
}

double DiscountDialog::computePrice() const {
    double price;
    if (m_percentRadio->isChecked()) {
        price = m_listPrice * (1.0 - m_valueSpin->value() / 100.0);
    } else {
        price = m_listPrice - m_valueSpin->value();
    }
    return qBound(0.0, price, m_listPrice);
}

void DiscountDialog::updatePreview() {
    m_previewLabel->setText(QString("New price: GEL %1").arg(computePrice(), 0, 'f', 2));
}

void DiscountDialog::accept() {
    m_resultPrice = computePrice();
    QDialog::accept();
}

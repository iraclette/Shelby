#pragma once

#include <QDialog>
#include <QString>

class QDoubleSpinBox;
class QLabel;
class QRadioButton;

// Local only: PosWindow reads resultUnitPrice() after exec().
class DiscountDialog : public QDialog {
    Q_OBJECT

public:
    // currentPrice lets reopening the dialog on an already-discounted line
    // preselect the discount that's already applied, instead of resetting it.
    DiscountDialog(const QString &productName, double listPrice, double currentPrice, QWidget *parent = nullptr);

    double resultUnitPrice() const { return m_resultPrice; }

private:
    double computePrice() const;
    void updatePreview();
    void accept() override;

    double m_listPrice;
    double m_resultPrice;

    QRadioButton *m_percentRadio;
    QRadioButton *m_amountRadio;
    QDoubleSpinBox *m_valueSpin;
    QLabel *m_previewLabel;
};

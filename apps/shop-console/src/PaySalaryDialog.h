#pragma once

#include <QDialog>
#include <QString>

class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class SupabaseClient;

// Records one salary payout; amount is prefilled with the outstanding balance.
class PaySalaryDialog : public QDialog {
    Q_OBJECT

public:
    PaySalaryDialog(SupabaseClient *client, QString staffId, QString shopId, const QString &employeeName,
                     QString periodStartIso, QString periodEndIso, double suggestedAmount,
                     QWidget *parent = nullptr);

private:
    void submit();

    SupabaseClient *m_client;
    QString m_staffId;
    QString m_shopId;
    QString m_periodStartIso;
    QString m_periodEndIso;

    QDoubleSpinBox *m_amountSpin;
    QLineEdit *m_noteInput;
    QLabel *m_statusLabel;
    QPushButton *m_submitButton;
};

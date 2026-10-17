#pragma once

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;

// Local only: PosWindow records the sale after exec() and reads change() for
// the status line. The method isn't sent to the server.
class PaymentDialog : public QDialog {
    Q_OBJECT

public:
    PaymentDialog(double total, QWidget *parent = nullptr);

    bool paidByCard() const { return m_card; }
    double change() const; // 0 for card

private:
    void setCard(bool card);
    void updateChange();
    double received() const; // what's typed in the cash field; 0 if blank
    void setReceived(double amount);

    double m_total;
    bool m_card = false;

    QPushButton *m_cashButton;
    QPushButton *m_cardButton;
    QWidget *m_cashPanel;
    QLineEdit *m_receivedEdit;
    QLabel *m_changeKey;
    QLabel *m_changeLabel;
    QPushButton *m_confirmButton;
};

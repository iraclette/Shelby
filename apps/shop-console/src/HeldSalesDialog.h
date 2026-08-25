#pragma once

#include <QDialog>
#include <QVector>

#include "PosWindow.h"

class QTableWidget;

// Lists carts paused via PosWindow::holdCart. Operates on heldSales by
// reference — both Resume and Discard mutate the caller's list directly,
// so PosWindow just needs to check didResume()/resumedCart() afterward and
// refresh its "Held (N)" button.
class HeldSalesDialog : public QDialog {
    Q_OBJECT

public:
    // cartIsEmpty gates Resume — swapping out an in-progress cart silently
    // would be a real "where did my items go" surprise for a cashier.
    HeldSalesDialog(QVector<HeldSale> &heldSales, bool cartIsEmpty, QWidget *parent = nullptr);

    bool didResume() const { return m_didResume; }
    QVector<CartLine> resumedCart() const { return m_resumedCart; }

private:
    void rebuildTable();

    QVector<HeldSale> &m_heldSales;
    bool m_cartIsEmpty;
    QTableWidget *m_table;

    bool m_didResume = false;
    QVector<CartLine> m_resumedCart;
};

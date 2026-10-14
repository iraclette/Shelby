#pragma once

#include <QDialog>
#include <QVector>

#include "SupabaseClient.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QVBoxLayout;

// Pick a recent sale, then how many of each item to return.
class ReturnDialog : public QDialog {
    Q_OBJECT

public:
    ReturnDialog(SupabaseClient *client, QString shopId, QWidget *parent = nullptr);

private:
    void showSalePicker();
    void selectSale(int row);
    void showItemPicker();
    void submit();

    SupabaseClient *m_client;
    QString m_shopId;

    QVector<SaleSummary> m_sales;
    int m_selectedSaleIndex = -1;

    QStackedWidget *m_stack;

    QListWidget *m_saleList;

    QVBoxLayout *m_itemsLayout;
    QVector<QSpinBox *> m_itemSpinBoxes; // parallel to m_sales[m_selectedSaleIndex].items
    QLineEdit *m_reasonInput;
    QLabel *m_statusLabel;
    QPushButton *m_submitButton;
};

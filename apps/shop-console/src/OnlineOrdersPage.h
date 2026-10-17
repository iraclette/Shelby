#pragma once

#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;

// Orders paid on the website, waiting to be picked up. The shop prepares
// them and marks them collected once the customer has them.
class OnlineOrdersPage : public QWidget {
    Q_OBJECT

public:
    explicit OnlineOrdersPage(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);
    void toPrepareChanged(int count); // paid, not yet collected: drives the sidebar badge

private:
    void rebuildTable();
    void showDetails();
    const OnlineOrder *selectedOrder() const;
    QString shopName(const QString &shopId) const;

    SupabaseClient *m_client;
    QComboBox *m_filter;
    QTableWidget *m_table;
    QLabel *m_details;
    QPushButton *m_collectedButton;

    QVector<OnlineOrder> m_orders;
    QVector<int> m_visible; // table row -> index into m_orders
    QVector<Shop> m_shops;
};

#pragma once

#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QComboBox;
class QPushButton;
class QTableWidget;

// "Let me know when it's back" requests left on out-of-stock products on the
// storefront. The shop calls or emails once it's restocked, then marks them contacted.
class StockRequestsPage : public QWidget {
    Q_OBJECT

public:
    explicit StockRequestsPage(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);
    void newCountChanged(int count); // not yet contacted: drives the sidebar badge

private:
    void rebuildTable();
    void updateButtons();
    void setSelectedStatus(const QString &status);
    const StockRequest *selectedRequest() const;

    SupabaseClient *m_client;
    QComboBox *m_filter;
    QTableWidget *m_table;
    QPushButton *m_contactedButton;
    QPushButton *m_dismissButton;

    QVector<StockRequest> m_requests;
    QVector<int> m_visible; // table row -> index into m_requests
};

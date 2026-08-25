#pragma once

#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QComboBox;
class QTableWidget;

// Activity trail: product/inventory/staff edits from audit_log (see
// 0018_audit_log.sql — actions that don't already have their own ledger),
// merged with stock_transfers (which already has its own table) into one
// "what happened" view, newest first.
class AuditLogPage : public QWidget {
    Q_OBJECT

public:
    explicit AuditLogPage(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);

private:
    void rebuildTable();

    SupabaseClient *m_client;

    QComboBox *m_categoryFilter;
    QTableWidget *m_table;

    QVector<Profile> m_profiles;
    QVector<Shop> m_shops;
    QVector<AuditLogEntry> m_entries;
    QVector<StockTransferSummary> m_transfers;
};

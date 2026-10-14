#pragma once

#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QTableWidget;

// Create/remove go through the staff-admin Edge Function (needs the service-role key).
class EmployeesPage : public QWidget {
    Q_OBJECT

public:
    explicit EmployeesPage(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);

private:
    void rebuildTable();
    void handleItemChanged(class QTableWidgetItem *item);
    void removeSelected();

    SupabaseClient *m_client;
    QTableWidget *m_table;

    QVector<Shop> m_shops;
    QVector<Profile> m_profiles;
    bool m_populating = false;

    static constexpr int kColName = 0;
    static constexpr int kColEmail = 1;
    static constexpr int kColRole = 2;
    static constexpr int kColShop = 3;
};

#pragma once

#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QTableWidget;
class QTableWidgetItem;

// Contact Us submissions from the storefront's /contact form — the admin's
// "Help" inbox. Best-effort emailed too (see contact_messages.email_sent),
// but this page is the reliable copy.
class MessagesPage : public QWidget {
    Q_OBJECT

public:
    explicit MessagesPage(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);

private:
    void rebuildTable();
    void handleItemChanged(QTableWidgetItem *item);

    SupabaseClient *m_client;
    QTableWidget *m_table;

    QVector<ContactMessage> m_messages;
    bool m_populating = false;

    static constexpr int kColCreated = 0;
    static constexpr int kColName = 1;
    static constexpr int kColEmail = 2;
    static constexpr int kColMessage = 3;
    static constexpr int kColStatus = 4;
};

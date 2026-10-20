#pragma once

#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QTableWidget;

// Contact Us submissions from the storefront's /contact form.
class MessagesPage : public QWidget {
    Q_OBJECT

public:
    explicit MessagesPage(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);

private:
    void rebuildTable();
    void openMessage(int row);

    SupabaseClient *m_client;
    QTableWidget *m_table;

    QVector<ContactMessage> m_messages;

    static constexpr int kColCreated = 0;
    static constexpr int kColName = 1;
    static constexpr int kColEmail = 2;
    static constexpr int kColMessage = 3;
    static constexpr int kColStatus = 4;
};

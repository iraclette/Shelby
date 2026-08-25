#include "MessagesPage.h"
#include "SupabaseClient.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <algorithm>

namespace {
const QStringList kValidStatuses{"unread", "read"};
}

MessagesPage::MessagesPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton("Refresh", this);
    toolbar->addWidget(refreshButton);
    toolbar->addStretch();

    connect(refreshButton, &QPushButton::clicked, this, [this]() { m_client->fetchContactMessages(); });

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"Received", "Name", "Email", "Message", "Status"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(kColCreated, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColName, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColStatus, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::itemChanged, this, &MessagesPage::handleItemChanged);
    connect(m_table, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem *item) {
        const int row = item->row();
        if (row < 0 || row >= m_messages.size() || m_messages[row].status != "unread") return;
        m_client->updateContactMessageField(m_messages[row].id, "status", "read");
    });

    connect(m_client, &SupabaseClient::contactMessagesFetched, this, [this](const QVector<ContactMessage> &messages) {
        m_messages = messages;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::contactMessagesFetchFailed, this, [this](const QString &message) {
        emit statusMessage("Failed to load messages: " + message);
    });
    m_client->fetchContactMessages();

    connect(m_client, &SupabaseClient::contactMessageUpdateFailed, this, [this](const QString &message) {
        emit statusMessage("Update failed: " + message);
        m_client->fetchContactMessages();
    });
    connect(m_client, &SupabaseClient::contactMessageUpdated, this, [this]() { m_client->fetchContactMessages(); });
}

void MessagesPage::rebuildTable() {
    m_populating = true;

    m_table->setRowCount(m_messages.size());
    for (int row = 0; row < m_messages.size(); ++row) {
        const ContactMessage &message = m_messages[row];

        auto *createdItem = new QTableWidgetItem(QString(message.createdAt).replace('T', ' ').left(19));
        createdItem->setFlags(createdItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColCreated, createdItem);

        auto *nameItem = new QTableWidgetItem(message.name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColName, nameItem);

        auto *emailItem = new QTableWidgetItem(message.email);
        emailItem->setFlags(emailItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColEmail, emailItem);

        auto *messageItem = new QTableWidgetItem(message.message);
        messageItem->setFlags(messageItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColMessage, messageItem);

        m_table->setItem(row, kColStatus, new QTableWidgetItem(message.status));
    }

    m_populating = false;
    const int unread = std::count_if(m_messages.begin(), m_messages.end(),
                                      [](const ContactMessage &m) { return m.status == "unread"; });
    emit statusMessage(QString("%1 message(s), %2 unread. Double-click a row to mark it read.")
                            .arg(m_messages.size())
                            .arg(unread));
}

void MessagesPage::handleItemChanged(QTableWidgetItem *item) {
    if (m_populating || item->column() != kColStatus) return;

    const int row = item->row();
    if (row < 0 || row >= m_messages.size()) return;

    const QString status = item->text().trimmed().toLower();
    if (!kValidStatuses.contains(status)) {
        emit statusMessage("Invalid status — must be 'unread' or 'read'.");
        m_client->fetchContactMessages();
        return;
    }

    m_client->updateContactMessageField(m_messages[row].id, "status", status);
}

#include "MessagesPage.h"
#include "ShopTime.h"
#include "SupabaseClient.h"

#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <algorithm>

MessagesPage::MessagesPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton(tr("Refresh"), this);
    toolbar->addWidget(refreshButton);
    toolbar->addStretch();

    connect(refreshButton, &QPushButton::clicked, this, [this]() { m_client->fetchContactMessages(); });

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({tr("Received"), tr("Name"), tr("Phone / email"), tr("Message"), tr("Status")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(kColCreated, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColName, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kColStatus, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::cellClicked, this, [this](int row) { openMessage(row); });

    connect(m_client, &SupabaseClient::contactMessagesFetched, this, [this](const QVector<ContactMessage> &messages) {
        m_messages = messages;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::contactMessagesFetchFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Failed to load messages: ") + message);
    });
    m_client->fetchContactMessages();

    connect(m_client, &SupabaseClient::contactMessageUpdateFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Update failed: ") + message);
        m_client->fetchContactMessages();
    });
    connect(m_client, &SupabaseClient::contactMessageUpdated, this, [this]() { m_client->fetchContactMessages(); });
}

void MessagesPage::rebuildTable() {
    m_table->setRowCount(m_messages.size());
    for (int row = 0; row < m_messages.size(); ++row) {
        const ContactMessage &message = m_messages[row];

        auto *createdItem = new QTableWidgetItem(shoptime::format(message.createdAt, "yyyy-MM-dd HH:mm:ss"));
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

    const int unread = std::count_if(m_messages.begin(), m_messages.end(),
                                      [](const ContactMessage &m) { return m.status == "unread"; });
    emit statusMessage(tr("%n messages, %1 unread. Click a message to open it.", nullptr, int(m_messages.size()))
                            .arg(unread));
}

// The whole message in its own window, big enough to read. Opening it marks it read.
void MessagesPage::openMessage(int row) {
    if (row < 0 || row >= m_messages.size()) return;
    const ContactMessage message = m_messages[row];
    if (message.status == "unread") m_client->updateContactMessageField(message.id, "status", "read");

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Message from %1").arg(message.name));
    dialog.resize(560, 440);

    auto *name = new QLabel(message.name, &dialog);
    name->setStyleSheet("font-size: 20px; font-weight: 600;");
    name->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *contact = new QLabel(message.email, &dialog);
    contact->setStyleSheet("font-size: 14px;");
    contact->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto *copyButton = new QPushButton(tr("Copy"), &dialog);
    connect(copyButton, &QPushButton::clicked, &dialog, [message, copyButton]() {
        QApplication::clipboard()->setText(message.email);
        copyButton->setText(tr("Copied"));
    });
    auto *contactRow = new QHBoxLayout;
    contactRow->addWidget(contact);
    contactRow->addWidget(copyButton);
    contactRow->addStretch();

    auto *received = new QLabel(tr("Received %1").arg(shoptime::format(message.createdAt, "d MMMM yyyy, HH:mm")), &dialog);
    received->setObjectName("brandSub");

    auto *body = new QPlainTextEdit(message.message, &dialog);
    body->setReadOnly(true);
    body->setStyleSheet("font-size: 15px;");

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    auto *unreadButton = buttons->addButton(tr("Mark as unread"), QDialogButtonBox::ActionRole);
    connect(unreadButton, &QPushButton::clicked, &dialog, [this, &dialog, message]() {
        m_client->updateContactMessageField(message.id, "status", "unread");
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(name);
    layout->addLayout(contactRow);
    layout->addWidget(received);
    layout->addSpacing(8);
    layout->addWidget(body, 1);
    layout->addWidget(buttons);
    dialog.exec();
}

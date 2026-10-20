#include "EmployeeDialog.h"
#include "Theme.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

EmployeeDialog::EmployeeDialog(SupabaseClient *client, QVector<Shop> shops, QWidget *parent)
    : QDialog(parent), m_client(client), m_shops(std::move(shops)) {
    setWindowTitle(tr("New Employee"));
    resize(360, 320);

    m_username = new QLineEdit(this);
    m_username->setPlaceholderText(tr("Short username, e.g. \"shelby\""));
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);
    m_fullName = new QLineEdit(this);

    m_role = new QComboBox(this);
    m_role->addItems({"staff", "admin", "owner"});

    m_shop = new QComboBox(this);
    m_shop->addItem("(none)", QString());
    for (const Shop &shop : m_shops) {
        m_shop->addItem(shop.name, shop.id);
    }

    auto *form = new QFormLayout;
    form->addRow(tr("Username"), m_username);
    form->addRow(tr("Password"), m_password);
    form->addRow(tr("Full name"), m_fullName);
    form->addRow(tr("Role"), m_role);
    form->addRow(tr("Shop"), m_shop);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet(theme::css(theme::palette().danger));
    m_statusLabel->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_saveButton = buttons->addButton(tr("Save"), QDialogButtonBox::AcceptRole);
    m_saveButton->setObjectName("primaryButton");

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_statusLabel);
    layout->addStretch();
    layout->addWidget(buttons);

    connect(m_saveButton, &QPushButton::clicked, this, &EmployeeDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_client, &SupabaseClient::staffAccountCreated, this, [this]() { accept(); });
    connect(m_client, &SupabaseClient::staffAccountCreateFailed, this, [this](const QString &message) {
        m_saveButton->setEnabled(true);
        m_statusLabel->setText(message);
    });
}

void EmployeeDialog::save() {
    m_statusLabel->clear();

    const QString username = m_username->text().trimmed();
    const QString password = m_password->text();
    if (username.isEmpty()) {
        m_statusLabel->setText(tr("Username is required."));
        return;
    }
    if (password.size() < 6) {
        m_statusLabel->setText(tr("Password must be at least 6 characters."));
        return;
    }

    m_saveButton->setEnabled(false);
    m_statusLabel->setStyleSheet(theme::css(theme::palette().textMuted));
    m_statusLabel->setText(tr("Creating…"));

    m_client->createStaffAccount(username, password, m_fullName->text().trimmed(), m_role->currentText(),
                                  m_shop->currentData().toString());
}

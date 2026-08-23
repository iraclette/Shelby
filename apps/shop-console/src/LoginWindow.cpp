#include "LoginWindow.h"
#include "SupabaseClient.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginWindow::LoginWindow(SupabaseClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client) {
    setWindowTitle("Shop Console");
    resize(760, 460);
    setMinimumSize(560, 380);

    // Left: brand panel. A warm gradient standing in for photography (no
    // product photos exist yet — see the storefront's .hero-atmosphere for
    // the same idea in CSS) so the login screen isn't just a bare form.
    auto *brandPanel = new QWidget(this);
    brandPanel->setObjectName("loginBrandPanel");
    auto *brandLayout = new QVBoxLayout(brandPanel);
    brandLayout->setContentsMargins(48, 0, 48, 0);
    brandLayout->addStretch();
    auto *brandTitle = new QLabel("Shelby", brandPanel);
    brandTitle->setStyleSheet("font-size: 32px; font-weight: 600; color: #f6f3ee; background: transparent;");
    auto *brandSubtitle = new QLabel("Black Eye Beauty  ·  Shelby  ·  End", brandPanel);
    brandSubtitle->setStyleSheet(
        "font-size: 11px; letter-spacing: 2px; color: #c8963e; background: transparent; "
        "text-transform: uppercase; margin-top: 6px;");
    brandLayout->addWidget(brandTitle);
    brandLayout->addWidget(brandSubtitle);
    brandLayout->addStretch();

    auto *formPanel = new QWidget(this);
    m_email = new QLineEdit(formPanel);
    m_email->setPlaceholderText("Username");

    m_password = new QLineEdit(formPanel);
    m_password->setPlaceholderText("Password");
    m_password->setEchoMode(QLineEdit::Password);

    m_signInButton = new QPushButton("Sign in", formPanel);
    m_signInButton->setObjectName("primaryButton");
    m_errorLabel = new QLabel(formPanel);
    m_errorLabel->setStyleSheet("color: #dc2626;");
    m_errorLabel->setWordWrap(true);

    auto *formLayout = new QVBoxLayout(formPanel);
    formLayout->setContentsMargins(48, 0, 48, 0);
    formLayout->addStretch();
    formLayout->addWidget(new QLabel("<b>Sign in</b>", formPanel));
    formLayout->addWidget(m_errorLabel);
    formLayout->addWidget(m_email);
    formLayout->addWidget(m_password);
    formLayout->addWidget(m_signInButton);
    formLayout->addStretch();

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(brandPanel, 2);
    layout->addWidget(formPanel, 3);

    connect(m_signInButton, &QPushButton::clicked, this, &LoginWindow::handleSignIn);
    connect(m_password, &QLineEdit::returnPressed, this, &LoginWindow::handleSignIn);

    connect(m_client, &SupabaseClient::signInFailed, this, [this](const QString &message) {
        m_signInButton->setEnabled(true);
        m_errorLabel->setText(message);
    });

    connect(m_client, &SupabaseClient::signInSucceeded, this, [this](const QString &userId) {
        m_client->fetchProfile(userId);
    });

    connect(m_client, &SupabaseClient::profileFetchFailed, this, [this](const QString &message) {
        m_signInButton->setEnabled(true);
        m_errorLabel->setText(message);
    });

    connect(m_client, &SupabaseClient::profileFetched, this, [this](const Profile &profile) {
        emit authenticated(profile.role, profile.shopId);
    });
}

void LoginWindow::handleSignIn() {
    m_errorLabel->clear();
    m_signInButton->setEnabled(false);

    // Staff log in with a short username (e.g. "shelby") rather than a real
    // email address — Supabase Auth still needs an email-shaped identity
    // under the hood, so anything without an "@" gets a fixed fake domain
    // appended. Accounts must be created in the dashboard using that same
    // form, e.g. "shelby@shop.local".
    QString identifier = m_email->text().trimmed();
    if (!identifier.contains('@')) {
        identifier += "@shop.local";
    }

    m_client->signIn(identifier, m_password->text());
}

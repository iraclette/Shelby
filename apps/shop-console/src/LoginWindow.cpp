#include "LoginWindow.h"
#include "SupabaseClient.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginWindow::LoginWindow(SupabaseClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client) {
    setWindowTitle("Shop Console");
    resize(360, 260);

    m_email = new QLineEdit(this);
    m_email->setPlaceholderText("Email");

    m_password = new QLineEdit(this);
    m_password->setPlaceholderText("Password");
    m_password->setEchoMode(QLineEdit::Password);

    m_signInButton = new QPushButton("Sign in", this);
    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color: #dc2626;");
    m_errorLabel->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(new QLabel("<b>Shop Console</b>", this));
    layout->addWidget(m_errorLabel);
    layout->addWidget(m_email);
    layout->addWidget(m_password);
    layout->addWidget(m_signInButton);
    layout->addStretch();

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
    m_client->signIn(m_email->text(), m_password->text());
}

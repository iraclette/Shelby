#include "LoginWindow.h"
#include "Theme.h"
#include "BusinessProfile.h"
#include "SessionStore.h"
#include "SupabaseClient.h"

#include <QAction>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

LoginWindow::LoginWindow(SupabaseClient *client, QWidget *parent)
    : QWidget(parent)
    , m_client(client) {
    setWindowTitle(tr("Shop Console"));
    resize(760, 460);
    setMinimumSize(560, 380);

    auto *brandPanel = new QWidget(this);
    brandPanel->setObjectName("loginBrandPanel");
    auto *brandLayout = new QVBoxLayout(brandPanel);
    brandLayout->setContentsMargins(48, 0, 48, 0);
    brandLayout->addStretch();
    // The business's logo and name (Settings → Business), cached from the last time this
    // computer was online, so they show before anyone signs in.
    auto *brandLogo = new QLabel(brandPanel);
    brandLogo->setFixedSize(96, 96);
    brandLogo->setStyleSheet("background: transparent;");
    auto *brandTitle = new QLabel(brandPanel);
    brandTitle->setObjectName("loginTitle");
    auto *brandSubtitle = new QLabel(tr("SHOP CONSOLE"), brandPanel);
    brandSubtitle->setObjectName("loginSubtitle");
    brandLayout->addWidget(brandLogo);
    brandLayout->addSpacing(14);
    brandLayout->addWidget(brandTitle);
    brandLayout->addWidget(brandSubtitle);
    brandLayout->addStretch();

    auto refreshBrand = [brandLogo, brandTitle]() {
        const BusinessProfile *business = BusinessProfile::instance();
        brandTitle->setText(business->settings().name);
        const QPixmap logo = business->roundLogo(96, brandLogo->devicePixelRatioF());
        brandLogo->setPixmap(logo);
        brandLogo->setVisible(!logo.isNull());
    };
    refreshBrand();
    connect(BusinessProfile::instance(), &BusinessProfile::changed, this, refreshBrand);

    auto *formPanel = new QWidget(this);
    m_email = new QLineEdit(formPanel);
    m_email->setPlaceholderText(tr("Username"));
    m_email->setText(sessionstore::lastUsername());

    m_password = new QLineEdit(formPanel);
    m_password->setPlaceholderText(tr("Password"));
    m_password->setEchoMode(QLineEdit::Password);
    auto *showPassword = m_password->addAction(QIcon(":/icons/eye.svg"), QLineEdit::TrailingPosition);
    showPassword->setToolTip(tr("Show password"));
    connect(showPassword, &QAction::triggered, this, [this, showPassword]() {
        const bool hidden = m_password->echoMode() == QLineEdit::Password;
        m_password->setEchoMode(hidden ? QLineEdit::Normal : QLineEdit::Password);
        showPassword->setIcon(QIcon(hidden ? ":/icons/eye-off.svg" : ":/icons/eye.svg"));
        showPassword->setToolTip(hidden ? tr("Hide password") : tr("Show password"));
    });

    // Off by default: on a shared till, the next person would otherwise ring up sales as you.
    m_staySignedIn = new QCheckBox(tr("Stay signed in on this computer"), formPanel);
    m_staySignedIn->setChecked(sessionstore::staySignedIn());

    m_signInButton = new QPushButton(tr("Sign in"), formPanel);
    m_signInButton->setObjectName("primaryButton");
    m_errorLabel = new QLabel(formPanel);
    m_errorLabel->setStyleSheet(theme::css(theme::palette().danger));
    m_errorLabel->setWordWrap(true);

    auto *formLayout = new QVBoxLayout(formPanel);
    formLayout->setContentsMargins(48, 0, 48, 0);
    formLayout->addStretch();
    formLayout->addWidget(new QLabel(tr("<b>Sign in</b>"), formPanel));
    formLayout->addWidget(m_errorLabel);
    formLayout->addWidget(m_email);
    formLayout->addWidget(m_password);
    formLayout->addWidget(m_staySignedIn);
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
        if (!isVisible()) return; // a background refresh, not this form
        setBusy(false);
        if (m_resuming) {
            m_resuming = false;
            sessionstore::clearSession();
            m_errorLabel->setText(tr("Please sign in again."));
            return;
        }
        m_errorLabel->setText(message);
    });

    connect(m_client, &SupabaseClient::signInSucceeded, this, [this](const QString &userId) {
        m_client->fetchProfile(userId);
    });

    connect(m_client, &SupabaseClient::profileFetchFailed, this, [this](const QString &message) {
        setBusy(false);
        m_resuming = false;
        m_errorLabel->setText(message);
    });

    connect(m_client, &SupabaseClient::profileFetched, this, [this](const Profile &profile) {
        m_resuming = false;
        setBusy(false);
        m_password->clear();
        emit authenticated(profile);
    });

    // Username remembered: start on the password.
    if (!m_email->text().isEmpty()) m_password->setFocus();
}

void LoginWindow::resumeSession(const QString &refreshToken) {
    m_resuming = true;
    setBusy(true);
    m_errorLabel->setText(tr("Signing you back in…"));
    m_client->restoreSession(refreshToken);
}

void LoginWindow::reset(const QString &message) {
    m_resuming = false;
    setBusy(false);
    m_password->clear();
    m_errorLabel->setText(message);
    m_password->setFocus();
}

void LoginWindow::setBusy(bool busy) {
    m_signInButton->setEnabled(!busy);
    m_email->setEnabled(!busy);
    m_password->setEnabled(!busy);
    m_staySignedIn->setEnabled(!busy);
}

void LoginWindow::handleSignIn() {
    if (!m_signInButton->isEnabled()) return; // already signing in
    m_errorLabel->clear();
    setBusy(true);

    sessionstore::setLastUsername(m_email->text().trimmed());
    sessionstore::setStaySignedIn(m_staySignedIn->isChecked());
    if (!m_staySignedIn->isChecked()) sessionstore::clearSession();

    // Usernames without an "@" get a fixed fake domain, e.g. shelby@shop.local.
    QString identifier = m_email->text().trimmed();
    if (!identifier.contains('@')) {
        identifier += "@shop.local";
    }

    m_client->signIn(identifier, m_password->text());
}

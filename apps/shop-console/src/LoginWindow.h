#pragma once

#include <QWidget>

#include "SupabaseClient.h"

class QLabel;
class QLineEdit;
class QPushButton;

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    // Emitted once sign-in succeeds and the caller's role has been resolved.
    void authenticated(const Profile &profile);

private:
    void handleSignIn();

    SupabaseClient *m_client;
    QLineEdit *m_email;
    QLineEdit *m_password;
    QPushButton *m_signInButton;
    QLabel *m_errorLabel;
};

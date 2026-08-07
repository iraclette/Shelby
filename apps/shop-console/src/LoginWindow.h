#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class SupabaseClient;

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    // Emitted once sign-in succeeds and the caller's role has been resolved.
    void authenticated(const QString &role, const QString &shopId);

private:
    void handleSignIn();

    SupabaseClient *m_client;
    QLineEdit *m_email;
    QLineEdit *m_password;
    QPushButton *m_signInButton;
    QLabel *m_errorLabel;
};

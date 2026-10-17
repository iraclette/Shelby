#pragma once

#include <QWidget>

#include "SupabaseClient.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(SupabaseClient *client, QWidget *parent = nullptr);

    // Signs in with a saved session instead of the form; falls back to the form if it's no longer valid.
    void resumeSession(const QString &refreshToken);
    // Back to an empty password field after signing out (message: why, e.g. the session expired).
    void reset(const QString &message = {});

signals:
    // Emitted once sign-in succeeds and the caller's role has been resolved.
    void authenticated(const Profile &profile);

private:
    void handleSignIn();
    void setBusy(bool busy);

    SupabaseClient *m_client;
    QLineEdit *m_email;
    QLineEdit *m_password;
    QPushButton *m_signInButton;
    QCheckBox *m_staySignedIn;
    QLabel *m_errorLabel;
    bool m_resuming = false;
};

#pragma once

#include <QMainWindow>

#include "SupabaseClient.h"

class QPushButton;
class QStackedWidget;
class UpdateChecker;

// Sidebar + one page per section; owns the toolbar and shared status bar.
class AdminWindow : public QMainWindow {
    Q_OBJECT

public:
    // isDev adds the Checkout page (see CheckoutPage) — the owner and admins don't need it.
    AdminWindow(SupabaseClient *client, UpdateChecker *updateChecker, bool isDev, QWidget *parent = nullptr);

    void reportUpdateCheckResult(bool ok, bool isNewer, const QString &version, const QString &releaseUrl,
                                  const QString &assetUrl, const QString &errorMessage);

signals:
    void signedOut();

private:
    UpdateChecker *m_updateChecker;
    QPushButton *m_updateButton;
    QString m_pendingAssetUrl;
    class SoftwarePage *m_softwarePage;
};

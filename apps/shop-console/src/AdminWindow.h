#pragma once

#include <QMainWindow>

#include "SupabaseClient.h"

class QPushButton;
class QStackedWidget;
class UpdateChecker;

// Multi-page admin shell: a left sidebar (Inventory / Employees / Software)
// next to a QStackedWidget holding one page per section. The window itself
// only owns what's global across every page — the top toolbar (title,
// update banner, Sign out) and the shared status bar each page reports
// through (see e.g. InventoryPage::statusMessage).
class AdminWindow : public QMainWindow {
    Q_OBJECT

public:
    AdminWindow(SupabaseClient *client, UpdateChecker *updateChecker, QWidget *parent = nullptr);

    // Shows an "Update now" toolbar button (only when isNewer); clicking
    // triggers UpdateChecker::downloadAndInstall(). Always forwarded to the
    // Software page too, success or failure — see UpdateChecker.
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

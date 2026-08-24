#pragma once

#include <QMainWindow>

#include "SupabaseClient.h"

class QLabel;
class QStackedWidget;

// Multi-page admin shell: a left sidebar (Inventory / Employees / Software)
// next to a QStackedWidget holding one page per section. The window itself
// only owns what's global across every page — the top toolbar (title,
// update banner, Sign out) and the shared status bar each page reports
// through (see e.g. InventoryPage::statusMessage).
class AdminWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AdminWindow(SupabaseClient *client, QWidget *parent = nullptr);

    // Shows a clickable toolbar notice (only when isNewer); clicking opens
    // releaseUrl in the default browser. Always forwarded to the Software
    // page too, success or failure — see UpdateChecker.
    void reportUpdateCheckResult(bool ok, bool isNewer, const QString &version, const QString &releaseUrl,
                                  const QString &errorMessage);

signals:
    void signedOut();

private:
    QLabel *m_updateBanner;
    class SoftwarePage *m_softwarePage;
};

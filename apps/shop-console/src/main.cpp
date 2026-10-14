#include "AdminWindow.h"
#include "Config.h"
#include "LoginWindow.h"
#include "PosWindow.h"
#include "SupabaseClient.h"
#include "Theme.h"
#include "UpdateChecker.h"

#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyleSheet(kAppStyleSheet);

    // Started early so it overlaps with typing the login.
    auto *updateChecker = new UpdateChecker(&app);
    updateChecker->checkForUpdate();

    const Config config = Config::load();
    if (!config.isValid()) {
        QMessageBox::critical(
            nullptr, "Shop Console",
            "Missing Supabase configuration.\n\n"
            "Create a .env file next to the executable (see .env.example) "
            "with SUPABASE_URL and SUPABASE_ANON_KEY.");
        return 1;
    }

    auto *client = new SupabaseClient(config.supabaseUrl, config.supabaseAnonKey, &app);

    auto *login = new LoginWindow(client);
    QMainWindow *activeWindow = nullptr;

    // The check usually finishes before any window exists, so buffer the result.
    bool haveUpdateResult = false;
    bool pendingOk = false;
    bool pendingIsNewer = false;
    QString pendingVersion;
    QString pendingReleaseUrl;
    QString pendingAssetUrl;
    QString pendingErrorMessage;

    auto applyUpdateBanner = [&]() {
        if (!haveUpdateResult) return;
        if (auto *admin = qobject_cast<AdminWindow *>(activeWindow)) {
            admin->reportUpdateCheckResult(pendingOk, pendingIsNewer, pendingVersion, pendingReleaseUrl,
                                            pendingAssetUrl, pendingErrorMessage);
        } else if (auto *pos = qobject_cast<PosWindow *>(activeWindow)) {
            if (pendingOk && pendingIsNewer) pos->showUpdateBanner(pendingVersion, pendingReleaseUrl);
        }
    };

    QObject::connect(updateChecker, &UpdateChecker::checkFinished, &app,
                      [&](bool ok, bool isNewer, const QString &version, const QString &releaseUrl,
                          const QString &assetUrl, const QString &errorMessage) {
        haveUpdateResult = true;
        pendingOk = ok;
        pendingIsNewer = isNewer;
        pendingVersion = version;
        pendingReleaseUrl = releaseUrl;
        pendingAssetUrl = assetUrl;
        pendingErrorMessage = errorMessage;
        applyUpdateBanner();
    });

    QObject::connect(updateChecker, &UpdateChecker::installStarting, &app, [&]() { app.quit(); });
    QObject::connect(updateChecker, &UpdateChecker::installFailed, &app, [&](const QString &message) {
        QMessageBox::warning(activeWindow, "Update failed", message);
    });

    QObject::connect(login, &LoginWindow::authenticated, &app,
                      [&](const QString &role, const QString &shopId) {
        login->hide();

        const bool isAdmin = role == "owner" || role == "admin";
        if (isAdmin) {
            activeWindow = new AdminWindow(client, updateChecker);
            QObject::connect(static_cast<AdminWindow *>(activeWindow), &AdminWindow::signedOut,
                              &app, [&]() {
                activeWindow->close();
                login->show();
            });
        } else {
            activeWindow = new PosWindow(client, shopId);
            QObject::connect(static_cast<PosWindow *>(activeWindow), &PosWindow::signedOut,
                              &app, [&]() {
                activeWindow->close();
                login->show();
            });
        }
        activeWindow->show();
        applyUpdateBanner();
    });

    login->show();
    return app.exec();
}

#include "AdminWindow.h"
#include "BusinessProfile.h"
#include "Config.h"
#include "LoginWindow.h"
#include "PosWindow.h"
#include "SessionStore.h"
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

    // The business's name, logo, currency and time zone: cached from last time, refreshed now
    // (no sign-in needed) and whenever someone saves them in Settings.
    QObject::connect(client, &SupabaseClient::businessSettingsFetched, BusinessProfile::instance(),
                     &BusinessProfile::apply);
    QObject::connect(client, &SupabaseClient::businessSettingsSaved, BusinessProfile::instance(),
                     &BusinessProfile::apply);
    client->fetchBusinessSettings();

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

    QObject::connect(updateChecker, &UpdateChecker::installStarting, &app, [&]() {
        // The installer relaunches the app; hand it the session so nobody re-types the password.
        if (!client->refreshToken().isEmpty()) {
            sessionstore::setRefreshToken(client->refreshToken());
            sessionstore::setResumeAfterUpdate(true);
        }
        app.quit();
    });

    // Refresh tokens are single-use, so keep the saved one current.
    QObject::connect(client, &SupabaseClient::sessionChanged, &app, [](const QString &refreshToken) {
        if (sessionstore::staySignedIn()) sessionstore::setRefreshToken(refreshToken);
    });

    auto backToLogin = [&](const QString &message) {
        client->signOut();
        sessionstore::clearSession();
        if (activeWindow) {
            activeWindow->close();
            activeWindow->deleteLater();
            activeWindow = nullptr;
        }
        login->reset(message);
        login->show();
    };
    QObject::connect(client, &SupabaseClient::sessionExpired, &app, [&]() {
        if (activeWindow) backToLogin("Your session ended. Please sign in again.");
    });
    QObject::connect(updateChecker, &UpdateChecker::installFailed, &app, [&](const QString &message) {
        QMessageBox::warning(activeWindow, "Update failed", message);
    });

    QObject::connect(login, &LoginWindow::authenticated, &app,
                      [&](const Profile &profile) {
        const bool isDev = profile.isDev;
        login->hide();
        // A session resumed after an update is only kept if "stay signed in" is on.
        if (!sessionstore::staySignedIn()) sessionstore::clearSession();

        // The launch check ran before anyone signed in, so it was stable-only.
        if (updateChecker->includesPrereleases() != isDev) {
            updateChecker->setIncludePrereleases(isDev);
            haveUpdateResult = false;
            updateChecker->checkForUpdate();
        }

        const bool isAdmin = profile.role == "owner" || profile.role == "admin";
        if (isAdmin) {
            activeWindow = new AdminWindow(client, updateChecker, profile);
            QObject::connect(static_cast<AdminWindow *>(activeWindow), &AdminWindow::signedOut,
                              &app, [&]() { backToLogin({}); });
        } else {
            activeWindow = new PosWindow(client, profile.shopId);
            QObject::connect(static_cast<PosWindow *>(activeWindow), &PosWindow::signedOut,
                              &app, [&]() { backToLogin({}); });
        }
        activeWindow->show();
        applyUpdateBanner();
    });

    login->show();
    const QString savedSession = sessionstore::refreshToken();
    if (!savedSession.isEmpty() && (sessionstore::staySignedIn() || sessionstore::resumeAfterUpdate())) {
        sessionstore::setResumeAfterUpdate(false); // one go only, even if it fails
        login->resumeSession(savedSession);
    } else {
        sessionstore::clearSession();
    }
    return app.exec();
}

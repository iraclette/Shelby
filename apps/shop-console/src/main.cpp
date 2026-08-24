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

    // Kicked off now so the network round-trip overlaps with someone typing
    // their login credentials — by the time a window exists to show the
    // notice on, the answer is usually already back.
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

    QObject::connect(updateChecker, &UpdateChecker::updateAvailable, &app,
                      [&](const QString &version, const QString &releaseUrl) {
        // If this fires before anyone's logged in, there's no window to put
        // the notice on yet — skipped for this launch rather than buffered;
        // it'll show up again next time regardless.
        if (auto *admin = qobject_cast<AdminWindow *>(activeWindow)) {
            admin->showUpdateBanner(version, releaseUrl);
        } else if (auto *pos = qobject_cast<PosWindow *>(activeWindow)) {
            pos->showUpdateBanner(version, releaseUrl);
        }
    });

    QObject::connect(login, &LoginWindow::authenticated, &app,
                      [&](const QString &role, const QString &shopId) {
        login->hide();

        const bool isAdmin = role == "owner" || role == "admin";
        if (isAdmin) {
            activeWindow = new AdminWindow(client);
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
    });

    login->show();
    return app.exec();
}

#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;

// Checks GitHub Releases on launch. Installs only when someone clicks "Update now":
// the release's ShopConsole-Setup.exe runs silently, swaps the files after the
// app quits, then relaunches it.
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    // No-op while an update is downloading, so a re-check can't reset the "Updating…" button.
    void checkForUpdate();

    // Dev accounts (profiles.is_dev) also get offered pre-releases (vX.Y.Z-rcN tags).
    void setIncludePrereleases(bool include) { m_includePrereleases = include; }
    bool includesPrereleases() const { return m_includePrereleases; }
    void downloadAndInstall(const QString &assetUrl);

signals:
    // On failure ok is false and the rest are empty.
    void checkFinished(bool ok, bool isNewer, const QString &version, const QString &releaseUrl,
                        const QString &assetUrl, const QString &errorMessage);

    void downloadProgress(int percent);
    // Caller should quit right away so the installer can replace the files.
    void installStarting();
    void installFailed(const QString &message);

private:
    void checkLatestStable();
    void checkLatestIncludingPrereleases();
    // tags sorted newest first: offers the newest one whose installer is
    // already downloadable (a tag shows up in the feed before its build finishes).
    void reportFirstPublished(const QStringList &tags, int generation);
    // Shared tail of both checks: tag "v1.2.3" (or "v1.2.3-rc1") -> checkFinished.
    void reportTag(const QString &tag);

    QNetworkAccessManager *m_network;
    bool m_includePrereleases = false;
    bool m_installing = false;
    // Bumped per check, so a slower earlier reply can't overwrite a newer one's result.
    int m_checkGeneration = 0;
};

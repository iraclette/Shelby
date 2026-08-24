#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// Checks GitHub Releases once per launch for a newer build than this one,
// and — only on explicit request (downloadAndInstall(), triggered by an
// "Update now" button someone actually clicks; never automatic) — fetches
// the new build and relaunches into it.
//
// checkFinished always fires exactly once with the full result (including
// failures, e.g. rate-limited), specifically so the Software page has
// something real to show instead of this failing invisibly (which is what
// happened before this existed — a rate-limit 403 with zero visible
// symptoms took a manual curl to find).
//
// Windows locks a running .exe, so downloadAndInstall() hands off to a
// small PowerShell script (update.ps1, shipped alongside the exe) that
// waits for this process to actually exit, then extracts the new build
// over the install directory and relaunches it. Never runs unattended —
// this app runs at POS terminals during business hours, and a surprise
// quit-and-relaunch mid-sale would be worse than the update itself.
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    void checkForUpdate();
    void downloadAndInstall(const QString &assetUrl);

signals:
    // releaseUrl is the release's GitHub page (has notes); assetUrl is the
    // actual downloadable zip, empty if the release had no zip asset — the
    // "Update now" action needs the latter, only ever going through
    // downloadAndInstall() when a person actually clicks it.
    // On failure, ok is false and the rest are empty; errorMessage is
    // always empty on success.
    void checkFinished(bool ok, bool isNewer, const QString &version, const QString &releaseUrl,
                        const QString &assetUrl, const QString &errorMessage);

    void downloadProgress(int percent);
    // Emitted once the new build is downloaded and the relaunch script has
    // been handed off — the caller should quit right away so the script's
    // "wait for the old process to exit" step doesn't just sit there.
    void installStarting();
    void installFailed(const QString &message);

private:
    QNetworkAccessManager *m_network;
};

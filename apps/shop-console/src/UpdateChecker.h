#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;

// Checks GitHub Releases once per launch for a newer build than this one.
// Never blocks, never pops a dialog — checkFinished always fires exactly
// once with the full result (including failures, e.g. rate-limited),
// specifically so the Software page has something real to show instead of
// this failing invisibly (which is what happened before this existed — a
// rate-limit 403 with zero visible symptoms took a manual curl to find).
//
// This only *notifies*; it never downloads or replaces the running .exe
// (Windows locks it while running, and silently self-updating is a lot of
// failure surface for an app a handful of people use — see the release
// process this pairs with).
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    void checkForUpdate();

signals:
    // releaseUrl is the release's GitHub page (has notes + the download),
    // not a raw asset link. On failure, ok is false and version/releaseUrl
    // are empty; errorMessage is always empty on success.
    void checkFinished(bool ok, bool isNewer, const QString &version, const QString &releaseUrl,
                        const QString &errorMessage);

private:
    QNetworkAccessManager *m_network;
};

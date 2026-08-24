#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;

// Checks GitHub Releases once per launch for a newer build than this one.
// Never blocks, never prompts on failure — offline, GitHub being down, or a
// malformed response all just mean no signal fires. This only *notifies*;
// it never downloads or replaces the running .exe (Windows locks it while
// running, and silently self-updating is a lot of failure surface for an
// app a handful of people use — see the release process this pairs with).
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    void checkForUpdate();

signals:
    // releaseUrl is the release's GitHub page (has notes + the download),
    // not a raw asset link.
    void updateAvailable(const QString &version, const QString &releaseUrl);

private:
    QNetworkAccessManager *m_network;
};

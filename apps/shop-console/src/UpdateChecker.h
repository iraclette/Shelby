#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// Checks GitHub Releases on launch. Installs only when someone clicks "Update now":
// update.ps1 swaps the files after the app quits, then relaunches it.
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    void checkForUpdate();
    void downloadAndInstall(const QString &assetUrl);

signals:
    // On failure ok is false and the rest are empty.
    void checkFinished(bool ok, bool isNewer, const QString &version, const QString &releaseUrl,
                        const QString &assetUrl, const QString &errorMessage);

    void downloadProgress(int percent);
    // Caller should quit right away so update.ps1 can proceed.
    void installStarting();
    void installFailed(const QString &message);

private:
    QNetworkAccessManager *m_network;
};

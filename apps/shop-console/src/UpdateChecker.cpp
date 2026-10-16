#include "UpdateChecker.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QVector>

namespace {

// "1.2.3" or "v1.2.3" -> {1, 2, 3}. False for anything else.
bool parseVersion(QString text, QVector<int> &parts) {
    if (text.startsWith('v') || text.startsWith('V')) text.remove(0, 1);
    if (text.isEmpty()) return false;

    QVector<int> result;
    for (const QString &segment : text.split('.')) {
        bool ok = false;
        const int value = segment.toInt(&ok);
        if (!ok || value < 0) return false;
        result.append(value);
    }
    while (result.size() < 3) result.append(0);
    parts = result;
    return true;
}

bool isNewer(const QVector<int> &candidate, const QVector<int> &current) {
    for (int i = 0; i < qMax(candidate.size(), current.size()); ++i) {
        const int c = i < candidate.size() ? candidate[i] : 0;
        const int cur = i < current.size() ? current[i] : 0;
        if (c != cur) return c > cur;
    }
    return false;
}

} // namespace

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this)) {
}

void UpdateChecker::checkForUpdate() {
    // Not the REST API: unauthenticated calls there are capped at 60/hour per IP,
    // which a shop's registers sharing one connection burn through. The plain
    // releases/latest page isn't limited and redirects to .../releases/tag/vX.Y.Z
    // (skipping pre-releases), so the tag can be read straight off the redirect.
    QNetworkRequest request(QUrl("https://github.com/iraclette/Shelby/releases/latest"));
    request.setRawHeader("User-Agent", "ShopConsole-UpdateChecker");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);

    QNetworkReply *reply = m_network->head(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        const QUrl target = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!target.isValid()) {
            const QString message = reply->error() != QNetworkReply::NoError
                                        ? reply->errorString()
                                        : QString("GitHub didn't point to a latest release.");
            emit checkFinished(false, false, {}, {}, {}, message);
            return;
        }

        const QUrl releaseUrl = reply->url().resolved(target);
        const QString path = releaseUrl.path();
        const QString tagMarker = "/releases/tag/";
        const int markerIndex = path.indexOf(tagMarker);
        if (markerIndex < 0) {
            emit checkFinished(false, false, {}, {}, {}, "GitHub's response didn't include a release tag.");
            return;
        }
        const QString tag = path.mid(markerIndex + tagMarker.size());

        // Every release attaches the installer under this fixed name (release.yml).
        const QString assetUrl =
            QString("https://github.com/iraclette/Shelby/releases/download/%1/ShopConsole-Setup.exe").arg(tag);

        const QString displayVersion = tag.startsWith('v') ? tag.mid(1) : tag;

        QVector<int> latest;
        QVector<int> current;
        if (!parseVersion(tag, latest) || !parseVersion(QStringLiteral(APP_VERSION), current)) {
            emit checkFinished(true, false, displayVersion, releaseUrl.toString(), assetUrl, {});
            return;
        }

        emit checkFinished(true, isNewer(latest, current), displayVersion, releaseUrl.toString(), assetUrl, {});
    });
}

void UpdateChecker::downloadAndInstall(const QString &assetUrl) {
    if (assetUrl.isEmpty()) {
        emit installFailed("This release has no downloadable build attached.");
        return;
    }

    QNetworkRequest request((QUrl(assetUrl)));
    request.setRawHeader("User-Agent", "ShopConsole-UpdateChecker");
    // GitHub asset links redirect to a signed S3/object-storage URL; Qt
    // doesn't follow redirects unless explicitly told to.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) emit downloadProgress(static_cast<int>(received * 100 / total));
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit installFailed("Download failed: " + reply->errorString());
            return;
        }

        const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        const QString setupPath = tempDir + "/ShopConsole-Setup.exe";
        QFile setupFile(setupPath);
        if (!setupFile.open(QIODevice::WriteOnly) || setupFile.write(reply->readAll()) < 0) {
            emit installFailed("Couldn't save the downloaded update to disk.");
            return;
        }
        setupFile.close();

        // The installer waits for/closes this app, swaps the files and relaunches
        // it (see the silent-only [Run] entry in ShopConsole.iss). Its log is the
        // place to look if an update ever doesn't stick.
        const QString installDir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
        const bool started = QProcess::startDetached(
            QDir::toNativeSeparators(setupPath),
            {"/VERYSILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/CLOSEAPPLICATIONS", "/DIR=" + installDir,
             "/LOG=" + QDir::toNativeSeparators(tempDir + "/ShopConsole-update.log")});

        if (!started) {
            emit installFailed("Couldn't start the installer.");
            return;
        }
        emit installStarting();
    });
}

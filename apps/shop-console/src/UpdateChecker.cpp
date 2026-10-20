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
#include <algorithm>
#include <QXmlStreamReader>

namespace {

// A tag split into its numbers and optional pre-release label:
// "v1.2.3" -> {1, 2, 3}, "";  "v1.2.3-rc1" -> {1, 2, 3}, "rc1".
struct Version {
    QVector<int> parts;
    QString prerelease; // empty = a stable release
};

bool parseVersion(QString text, Version &version) {
    if (text.startsWith('v') || text.startsWith('V')) text.remove(0, 1);
    const int dash = text.indexOf('-');
    const QString prerelease = dash < 0 ? QString() : text.mid(dash + 1);
    if (dash >= 0) text.truncate(dash);
    if (text.isEmpty()) return false;

    QVector<int> result;
    for (const QString &segment : text.split('.')) {
        bool ok = false;
        const int value = segment.toInt(&ok);
        if (!ok || value < 0) return false;
        result.append(value);
    }
    while (result.size() < 3) result.append(0);
    version = {result, prerelease};
    return true;
}

// "rc2" vs "rc10": letters compared as text, then the trailing number as a number.
int comparePrerelease(const QString &a, const QString &b) {
    auto split = [](const QString &label, QString &prefix, int &number) {
        int i = label.size();
        while (i > 0 && label[i - 1].isDigit()) --i;
        prefix = label.left(i);
        number = i < label.size() ? label.mid(i).toInt() : 0;
    };
    QString prefixA, prefixB;
    int numberA = 0, numberB = 0;
    split(a, prefixA, numberA);
    split(b, prefixB, numberB);
    if (const int byPrefix = QString::compare(prefixA, prefixB, Qt::CaseInsensitive)) return byPrefix;
    return numberA - numberB;
}

bool isNewer(const Version &candidate, const Version &current) {
    for (int i = 0; i < qMax(candidate.parts.size(), current.parts.size()); ++i) {
        const int c = i < candidate.parts.size() ? candidate.parts[i] : 0;
        const int cur = i < current.parts.size() ? current.parts[i] : 0;
        if (c != cur) return c > cur;
    }
    // Same numbers: 1.6.0 is newer than 1.6.0-rc2, which is newer than 1.6.0-rc1.
    if (candidate.prerelease.isEmpty() || current.prerelease.isEmpty()) {
        return candidate.prerelease.isEmpty() && !current.prerelease.isEmpty();
    }
    return comparePrerelease(candidate.prerelease, current.prerelease) > 0;
}

const QString kTagMarker = "/releases/tag/";

} // namespace

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this)) {
}

void UpdateChecker::checkForUpdate() {
    if (m_installing) return;
    ++m_checkGeneration;
    if (m_includePrereleases) {
        checkLatestIncludingPrereleases();
    } else {
        checkLatestStable();
    }
}

void UpdateChecker::checkLatestStable() {
    // Not the REST API: unauthenticated calls there are capped at 60/hour per IP,
    // which a shop's registers sharing one connection burn through. The plain
    // releases/latest page isn't limited and redirects to .../releases/tag/vX.Y.Z
    // (skipping pre-releases), so the tag can be read straight off the redirect.
    QNetworkRequest request(QUrl("https://github.com/iraclette/Shelby/releases/latest"));
    request.setRawHeader("User-Agent", "ShopConsole-UpdateChecker");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);

    QNetworkReply *reply = m_network->head(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, generation = m_checkGeneration]() {
        reply->deleteLater();
        if (generation != m_checkGeneration) return;

        const QUrl target = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!target.isValid()) {
            const QString message = reply->error() != QNetworkReply::NoError
                                        ? reply->errorString()
                                        : tr("GitHub didn't point to a latest release.");
            emit checkFinished(false, false, {}, {}, {}, message);
            return;
        }

        const QString path = reply->url().resolved(target).path();
        const int markerIndex = path.indexOf(kTagMarker);
        if (markerIndex < 0) {
            emit checkFinished(false, false, {}, {}, {}, tr("GitHub's response didn't include a release tag."));
            return;
        }
        reportTag(path.mid(markerIndex + kTagMarker.size()));
    });
}

void UpdateChecker::checkLatestIncludingPrereleases() {
    // releases/latest skips pre-releases, so read the releases Atom feed instead:
    // newest ~10 releases of every kind, and no API rate limit either.
    QNetworkRequest request(QUrl("https://github.com/iraclette/Shelby/releases.atom"));
    request.setRawHeader("User-Agent", "ShopConsole-UpdateChecker");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, generation = m_checkGeneration]() {
        reply->deleteLater();
        if (generation != m_checkGeneration) return;
        if (reply->error() != QNetworkReply::NoError) {
            emit checkFinished(false, false, {}, {}, {}, reply->errorString());
            return;
        }

        // Each <entry> has <link href=".../releases/tag/vX.Y.Z[-rcN]"/>.
        QVector<QPair<Version, QString>> tags;
        QXmlStreamReader xml(reply->readAll());
        while (!xml.atEnd()) {
            if (xml.readNext() == QXmlStreamReader::StartElement && xml.name() == QLatin1String("link")) {
                const QString href = xml.attributes().value("href").toString();
                const int markerIndex = href.indexOf(kTagMarker);
                Version version;
                const QString tag = markerIndex < 0 ? QString() : href.mid(markerIndex + kTagMarker.size());
                if (!tag.isEmpty() && parseVersion(tag, version)) tags.append({version, tag});
            }
        }

        if (tags.isEmpty()) {
            emit checkFinished(false, false, {}, {}, {}, tr("GitHub's releases feed didn't list any releases."));
            return;
        }
        std::sort(tags.begin(), tags.end(), [](const auto &a, const auto &b) { return isNewer(a.first, b.first); });
        QStringList newestFirst;
        for (const auto &entry : tags) newestFirst.append(entry.second);
        reportFirstPublished(newestFirst, generation);
    });
}

void UpdateChecker::reportFirstPublished(const QStringList &tags, int generation) {
    const QString tag = tags.first();
    Version candidate;
    Version current;
    // Nothing newer than what's running: no need to probe, it's "up to date".
    if (tags.size() == 1 || !parseVersion(tag, candidate) || !parseVersion(QStringLiteral(APP_VERSION), current)
        || !isNewer(candidate, current)) {
        reportTag(tag);
        return;
    }

    // The download link redirects to storage once the build has attached the
    // installer, and is a 404 until then.
    QNetworkRequest request(
        QUrl(QString("https://github.com/iraclette/Shelby/releases/download/%1/ShopConsole-Setup.exe").arg(tag)));
    request.setRawHeader("User-Agent", "ShopConsole-UpdateChecker");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    QNetworkReply *reply = m_network->head(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, tags, generation]() {
        reply->deleteLater();
        if (generation != m_checkGeneration) return;
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 404) {
            reportFirstPublished(tags.mid(1), generation); // still building — try the next one down
        } else {
            reportTag(tags.first()); // published, or the probe itself failed: let the download say why
        }
    });
}

void UpdateChecker::reportTag(const QString &tag) {
    const QString releaseUrl = "https://github.com/iraclette/Shelby" + kTagMarker + tag;
    // Every release attaches the installer under this fixed name (release.yml).
    const QString assetUrl =
        QString("https://github.com/iraclette/Shelby/releases/download/%1/ShopConsole-Setup.exe").arg(tag);
    const QString displayVersion = tag.startsWith('v') ? tag.mid(1) : tag;

    Version latest;
    Version current;
    if (!parseVersion(tag, latest) || !parseVersion(QStringLiteral(APP_VERSION), current)) {
        emit checkFinished(true, false, displayVersion, releaseUrl, assetUrl, {});
        return;
    }
    emit checkFinished(true, isNewer(latest, current), displayVersion, releaseUrl, assetUrl, {});
}

void UpdateChecker::downloadAndInstall(const QString &assetUrl) {
    m_installing = true;
    if (assetUrl.isEmpty()) {
        m_installing = false;
        emit installFailed(tr("This release has no downloadable build attached."));
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
            // Over HTTP/2 GitHub sends no reason phrase, which leaves Qt's
            // message as a bare "server replied:" — so name the status ourselves.
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString message = reply->errorString();
            if (status == 404) {
                message = tr("the installer isn't on GitHub yet (HTTP 404) — the release may still be building, "
                             "try again in a few minutes");
            } else if (status > 0) {
                message = tr("GitHub returned HTTP %1").arg(status);
            }
            m_installing = false;
            emit installFailed(tr("Download failed: %1").arg(message));
            return;
        }

        const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        const QString setupPath = tempDir + "/ShopConsole-Setup.exe";
        QFile setupFile(setupPath);
        if (!setupFile.open(QIODevice::WriteOnly) || setupFile.write(reply->readAll()) < 0) {
            m_installing = false;
            emit installFailed(tr("Couldn't save the downloaded update to disk."));
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
            m_installing = false;
            emit installFailed(tr("Couldn't start the installer."));
            return;
        }
        emit installStarting();
    });
}

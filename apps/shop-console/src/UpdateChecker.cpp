#include "UpdateChecker.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVector>

namespace {

// Parses "1.2.3" (or "v1.2.3") into {1, 2, 3}, padding missing trailing
// components with 0. Returns false, leaving parts untouched, if the string
// doesn't look like a dotted version number — callers treat that as "no
// update" rather than guessing at a malformed tag.
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

// GitHub's own error responses (rate limits, auth failures, ...) are
// {"message": "..."}. Prefer that verbatim over Qt's generic transport
// error string when it's present, since it's the actually-useful part —
// e.g. "API rate limit exceeded for 1.2.3.4." beats "Error transferring
// https://... - server replied: Forbidden".
QString describeError(const QByteArray &body, const QString &fallback) {
    const QString message = QJsonDocument::fromJson(body).object().value("message").toString();
    return message.isEmpty() ? fallback : message;
}

} // namespace

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this)) {
}

void UpdateChecker::checkForUpdate() {
    QNetworkRequest request(QUrl("https://api.github.com/repos/iraclette/Shelby/releases/latest"));
    // The GitHub API rejects requests with no User-Agent outright.
    request.setRawHeader("User-Agent", "ShopConsole-UpdateChecker");
    request.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray data = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            emit checkFinished(false, false, {}, {}, describeError(data, reply->errorString()));
            return;
        }

        const QJsonObject obj = QJsonDocument::fromJson(data).object();
        const QString tag = obj.value("tag_name").toString();
        const QString releaseUrl = obj.value("html_url").toString();
        if (tag.isEmpty() || releaseUrl.isEmpty()) {
            emit checkFinished(false, false, {}, {}, "GitHub's response didn't include a release tag.");
            return;
        }

        const QString displayVersion = tag.startsWith('v') ? tag.mid(1) : tag;

        QVector<int> latest;
        QVector<int> current;
        if (!parseVersion(tag, latest) || !parseVersion(QStringLiteral(APP_VERSION), current)) {
            emit checkFinished(true, false, displayVersion, releaseUrl, {});
            return;
        }

        emit checkFinished(true, isNewer(latest, current), displayVersion, releaseUrl, {});
    });
}

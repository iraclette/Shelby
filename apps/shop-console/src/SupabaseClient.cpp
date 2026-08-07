#include "SupabaseClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

SupabaseClient::SupabaseClient(QString baseUrl, QString anonKey, QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_baseUrl(std::move(baseUrl))
    , m_anonKey(std::move(anonKey)) {
    if (m_baseUrl.endsWith('/')) m_baseUrl.chop(1);
}

static QString extractErrorMessage(const QByteArray &body, const QString &fallback) {
    const QJsonObject obj = QJsonDocument::fromJson(body).object();
    for (const char *key : {"msg", "message", "error_description", "error"}) {
        const QString value = obj.value(key).toString();
        if (!value.isEmpty()) return value;
    }
    return fallback;
}

void SupabaseClient::signIn(const QString &email, const QString &password) {
    QUrl url(m_baseUrl + "/auth/v1/token");
    QUrlQuery query;
    query.addQueryItem("grant_type", "password");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("apikey", m_anonKey.toUtf8());

    const QJsonObject body{{"email", email}, {"password", password}};
    QNetworkReply *reply = m_network->post(request, QJsonDocument(body).toJson());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray data = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            emit signInFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        const QJsonObject obj = QJsonDocument::fromJson(data).object();
        m_accessToken = obj.value("access_token").toString();
        m_userId = obj.value("user").toObject().value("id").toString();

        if (m_accessToken.isEmpty() || m_userId.isEmpty()) {
            emit signInFailed("Unexpected response from Supabase Auth.");
            return;
        }

        emit signInSucceeded(m_userId);
    });
}

void SupabaseClient::fetchProfile(const QString &userId) {
    QUrl url(m_baseUrl + "/rest/v1/profiles");
    QUrlQuery query;
    query.addQueryItem("id", "eq." + userId);
    query.addQueryItem("select", "role,shop_id,full_name");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("apikey", m_anonKey.toUtf8());
    request.setRawHeader("Authorization", ("Bearer " + m_accessToken).toUtf8());

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray data = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            emit profileFetchFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        const QJsonArray rows = QJsonDocument::fromJson(data).array();
        if (rows.isEmpty()) {
            emit profileFetchFailed("No profile found for this account.");
            return;
        }

        const QJsonObject row = rows.first().toObject();
        Profile profile;
        profile.role = row.value("role").toString();
        profile.shopId = row.value("shop_id").toString();
        profile.fullName = row.value("full_name").toString();
        emit profileFetched(profile);
    });
}

void SupabaseClient::fetchProducts() {
    QUrl url(m_baseUrl + "/rest/v1/products");
    QUrlQuery query;
    query.addQueryItem("select", "id,name,sku,sell_price,cost_price");
    query.addQueryItem("order", "created_at.desc");
    query.addQueryItem("limit", "20");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("apikey", m_anonKey.toUtf8());
    request.setRawHeader("Authorization", ("Bearer " + m_accessToken).toUtf8());

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        const QByteArray data = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            emit productsFetchFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        QVector<Product> products;
        for (const QJsonValue &value : QJsonDocument::fromJson(data).array()) {
            const QJsonObject row = value.toObject();
            Product product;
            product.id = row.value("id").toString();
            product.name = row.value("name").toString();
            product.sku = row.value("sku").toString();
            product.sellPrice = row.value("sell_price").toDouble();
            product.costPrice = row.value("cost_price").toDouble();
            products.append(product);
        }
        emit productsFetched(products);
    });
}

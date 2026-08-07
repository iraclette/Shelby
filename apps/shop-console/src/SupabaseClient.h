#pragma once

#include <QObject>
#include <QString>
#include <QVector>

class QNetworkAccessManager;

struct Profile {
    QString role;
    QString shopId;
    QString fullName;
};

struct Product {
    QString id;
    QString name;
    QString sku;
    double sellPrice = 0;
    double costPrice = 0;
};

// Talks to Supabase's Auth and PostgREST HTTP APIs directly (no official
// C++ SDK exists for Supabase). Row Level Security on the database is what
// keeps this safe despite the client holding only the public anon key.
class SupabaseClient : public QObject {
    Q_OBJECT

public:
    SupabaseClient(QString baseUrl, QString anonKey, QObject *parent = nullptr);

    void signIn(const QString &email, const QString &password);
    void fetchProfile(const QString &userId);
    void fetchProducts();

signals:
    void signInSucceeded(const QString &userId);
    void signInFailed(const QString &message);
    void profileFetched(const Profile &profile);
    void profileFetchFailed(const QString &message);
    void productsFetched(const QVector<Product> &products);
    void productsFetchFailed(const QString &message);

private:
    QNetworkAccessManager *m_network;
    QString m_baseUrl;
    QString m_anonKey;
    QString m_accessToken;
    QString m_userId;
};

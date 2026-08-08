#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;
class QUrlQuery;

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
    int stockTotal = 0;
    QString mostlyInShop;
};

struct Category {
    QString id;
    QString name;
};

struct Shop {
    QString id;
    QString name;
};

struct ProductInput {
    QString name;
    QString description;
    QString categoryId;
    double costPrice = 0;
    double sellPrice = 0;
    QString sku;
    bool hasNativeBarcode = false;
};

struct InventoryLevelInput {
    QString shopId;
    int quantity = 0;
};

struct SaleItemInput {
    QString productId;
    int quantity = 0;
    double unitPrice = 0;
    double unitCost = 0;
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
    void createProduct(const ProductInput &input);

    void fetchCategories();
    void createCategory(const QString &name);

    void fetchShops();
    void upsertInventoryLevels(const QString &productId, const QVector<InventoryLevelInput> &levels);

    // Uploads the file to Storage, then records it in product_images.
    void uploadProductImage(const QString &productId, const QString &localFilePath, bool isPrimary);

    void lookupProductBySku(const QString &sku);

    // Records the sale + its line items, then decrements inventory_levels
    // for each item in the background (see comments on the emitted signal).
    void recordSale(const QString &shopId, const QVector<SaleItemInput> &items, double total);

signals:
    void signInSucceeded(const QString &userId);
    void signInFailed(const QString &message);
    void profileFetched(const Profile &profile);
    void profileFetchFailed(const QString &message);

    void productsFetched(const QVector<Product> &products);
    void productsFetchFailed(const QString &message);
    void productCreated(const QString &productId);
    void productCreateFailed(const QString &message);

    void categoriesFetched(const QVector<Category> &categories);
    void categoriesFetchFailed(const QString &message);
    void categoryCreated(const Category &category);
    void categoryCreateFailed(const QString &message);

    void shopsFetched(const QVector<Shop> &shops);
    void shopsFetchFailed(const QString &message);

    void inventoryLevelsUpserted();
    void inventoryLevelsUpsertFailed(const QString &message);

    void imageUploaded();
    void imageUploadFailed(const QString &message);

    void productLookedUp(const Product &product);
    void productLookupFailed(const QString &message);

    // Emitted once the sale + sale_items rows are written; inventory
    // decrements are fired at the same time but not waited on.
    void saleRecorded();
    void saleRecordFailed(const QString &message);

private:
    QNetworkAccessManager *m_network;
    QString m_baseUrl;
    QString m_anonKey;
    QString m_accessToken;
    QString m_userId;

    void authorizedGet(const QString &path, const QUrlQuery &query,
                        const std::function<void(QNetworkReply *)> &onFinished);
    void authorizedWrite(const QByteArray &verb, const QString &path, const QUrlQuery &query,
                          const QByteArray &body, const QMap<QByteArray, QByteArray> &extraHeaders,
                          const std::function<void(QNetworkReply *)> &onFinished);
};

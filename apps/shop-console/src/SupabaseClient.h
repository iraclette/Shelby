#pragma once

#include <QJsonValue>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVector>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;
class QUrlQuery;

struct Profile {
    QString id;
    QString email;
    QString role;
    QString shopId;
    QString fullName;
};

struct Product {
    QString id;
    QString name;
    QString sku;
    QString categoryId;
    double sellPrice = 0;
    double costPrice = 0;
    QMap<QString, int> stockByShop; // shop id -> quantity
};

struct Category {
    QString id;
    QString name;
};

struct ProductImage {
    QString id;
    QString storagePath;
    bool isPrimary = false;
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
    double unitPrice = 0; // price actually charged, may be discounted below listPrice
    double unitCost = 0;
    double listPrice = 0; // product's sell_price at the time it was added to the cart
};

struct SaleItemSummary {
    QString saleItemId;
    QString productId;
    QString productName;
    int quantity = 0;
    double unitPrice = 0;
};

struct SaleSummary {
    QString id;
    QString soldAt;
    double total = 0;
    QVector<SaleItemSummary> items;
};

struct ReturnItemInput {
    QString saleItemId;
    int quantity = 0;
};

// A sale that couldn't reach the server (offline) and is waiting to sync.
// clientGeneratedId is assigned up front and reused on every retry, so a
// retry that actually landed server-side but whose response got lost can't
// create a duplicate sale (see the record_sale DB function).
struct QueuedSale {
    QString clientGeneratedId;
    QString shopId;
    QVector<SaleItemInput> items;
    double total = 0;
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
    // field must be one of the products table's own column names
    // (name/sku/sell_price/cost_price) — always a fixed string from our own
    // code, never user-supplied, so there's no injection concern.
    void updateProductField(const QString &productId, const QString &field, const QJsonValue &value);

    void fetchCategories();
    void createCategory(const QString &name);

    void fetchShops();
    void upsertInventoryLevels(const QString &productId, const QVector<InventoryLevelInput> &levels);

    // Moves stock between two shops via the atomic transfer_stock DB
    // function — admin-only (see 0012_stock_transfers.sql's RLS policy),
    // rejected server-side if the source shop doesn't have enough.
    void transferStock(const QString &productId, const QString &fromShopId, const QString &toShopId, int quantity);

    void fetchProfiles();
    // field must be one of profiles' own column names (full_name/role/shop_id) —
    // always a fixed string from our own code, same guarantee as updateProductField.
    void updateProfileField(const QString &profileId, const QString &field, const QJsonValue &value);

    // Both go through the staff-admin Edge Function, not a direct table write —
    // creating/deleting a Supabase Auth user needs the service-role key, which
    // this app (shipped with only the public anon key) never holds. The function
    // re-checks the caller is an admin server-side before doing anything.
    void createStaffAccount(const QString &username, const QString &password, const QString &fullName,
                             const QString &role, const QString &shopId);
    void deleteStaffAccount(const QString &profileId);

    // Uploads the file to Storage, then records it in product_images.
    void uploadProductImage(const QString &productId, const QString &localFilePath, bool isPrimary);
    void fetchProductImages(const QString &productId);
    // Deletes both the Storage object and its product_images row.
    void deleteProductImage(const QString &imageId, const QString &storagePath);
    // The bucket is public, so this is just a URL — no auth needed to load it.
    QString publicImageUrl(const QString &storagePath) const;

    void lookupProductBySku(const QString &sku);

    // Records the sale via the atomic record_sale DB function. If the
    // request can't reach the server at all, the sale is queued to local
    // disk instead of failing outright, and synced automatically the next
    // time flushPendingSales() succeeds (see saleQueuedOffline).
    void recordSale(const QString &shopId, const QVector<SaleItemInput> &items, double total);

    // Recent sales for a shop, with nested line items — the picker list for
    // ReturnDialog. Not queued/retried offline like recordSale() — a return
    // is a deliberate, much less frequent action than a sale.
    void fetchRecentSales(const QString &shopId);
    // Records the return via the atomic record_return DB function (mirrors
    // recordSale/record_sale). saleId must be one of the shop's own sales;
    // unit prices come from the original sale_items server-side, never
    // trusted from here.
    void submitReturn(const QString &shopId, const QString &saleId, const QString &reason,
                       const QVector<ReturnItemInput> &items);

    // Attempts to resync queued offline sales, oldest first, stopping at
    // the first one that still can't reach the server. Safe to call
    // repeatedly (e.g. from a timer) — a no-op while empty or already
    // in-flight.
    void flushPendingSales();

    int pendingSalesCount() const { return m_pendingSales.size(); }

signals:
    void signInSucceeded(const QString &userId);
    void signInFailed(const QString &message);
    void profileFetched(const Profile &profile);
    void profileFetchFailed(const QString &message);

    void productsFetched(const QVector<Product> &products);
    void productsFetchFailed(const QString &message);
    void productCreated(const QString &productId);
    void productCreateFailed(const QString &message);
    void productUpdated();
    void productUpdateFailed(const QString &message);

    void categoriesFetched(const QVector<Category> &categories);
    void categoriesFetchFailed(const QString &message);
    void categoryCreated(const Category &category);
    void categoryCreateFailed(const QString &message);

    void shopsFetched(const QVector<Shop> &shops);
    void shopsFetchFailed(const QString &message);

    void profilesFetched(const QVector<Profile> &profiles);
    void profilesFetchFailed(const QString &message);
    void profileUpdated();
    void profileUpdateFailed(const QString &message);

    void staffAccountCreated();
    void staffAccountCreateFailed(const QString &message);
    void staffAccountDeleted();
    void staffAccountDeleteFailed(const QString &message);

    void inventoryLevelsUpserted();
    void inventoryLevelsUpsertFailed(const QString &message);

    void stockTransferred();
    // Also fires for a rejected transfer (e.g. "not enough stock at the
    // source shop") — a normal validation failure, not a crash.
    void stockTransferFailed(const QString &message);

    void imageUploaded();
    void imageUploadFailed(const QString &message);
    void productImagesFetched(const QVector<ProductImage> &images);
    void productImagesFetchFailed(const QString &message);
    void imageDeleted();
    void imageDeleteFailed(const QString &message);

    void productLookedUp(const Product &product);
    void productLookupFailed(const QString &message);

    void saleRecorded();
    void saleRecordFailed(const QString &message);
    // The server couldn't be reached, so the sale was saved locally instead.
    void saleQueuedOffline(int pendingCount);
    // Fires whenever the pending queue shrinks or grows, for a UI badge.
    void pendingSalesChanged(int pendingCount);

    void recentSalesFetched(const QVector<SaleSummary> &sales);
    void recentSalesFetchFailed(const QString &message);
    void returnRecorded();
    // Also fires for a rejected over-return (e.g. "only 1 left un-returned") —
    // that's a normal validation failure from record_return, not a crash.
    void returnRecordFailed(const QString &message);

private:
    QNetworkAccessManager *m_network;
    QString m_baseUrl;
    QString m_anonKey;
    QString m_accessToken;
    QString m_userId;

    QVector<QueuedSale> m_pendingSales;
    bool m_flushingPendingSales = false;

    void authorizedGet(const QString &path, const QUrlQuery &query,
                        const std::function<void(QNetworkReply *)> &onFinished);
    void authorizedWrite(const QByteArray &verb, const QString &path, const QUrlQuery &query,
                          const QByteArray &body, const QMap<QByteArray, QByteArray> &extraHeaders,
                          const std::function<void(QNetworkReply *)> &onFinished);

    void submitSale(const QString &shopId, const QString &clientGeneratedId,
                     const QVector<SaleItemInput> &items, double total, bool isRetryFromQueue);
    void enqueueOffline(const QueuedSale &sale);

    QString pendingSalesFilePath() const;
    void loadPendingSales();
    void savePendingSales() const;
};

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
    // Daily-wage pay config — unset (0) until an admin configures it on the
    // Salary page. See SalaryPage for how these turn into a period's earnings.
    double dailyRate = 0;
    double bonusThreshold = 0;
    double bonusAmount = 0;
};

struct ProductVariant {
    QString id;
    QString productId;
    QString name; // the shade/size label, e.g. "Red"
    QString sku;
    QMap<QString, int> stockByShop; // shop id -> quantity, same shape as Product::stockByShop
};

struct Product {
    QString id;
    QString name;
    QString sku;
    QString categoryId;
    QString supplierId;
    double sellPrice = 0;
    double costPrice = 0;
    int lowStockThreshold = 0; // 0 = disabled
    QMap<QString, int> stockByShop; // this product's own "no variant" bucket; shop id -> quantity
    QVector<ProductVariant> variants; // empty for a product with no variants

    // Set only by lookupProductBySku when the scanned barcode matched a
    // variant's own sku (not the product's) — lets PosWindow::addToCart
    // bypass the variant picker for a direct scan.
    QString selectedVariantId;
    QString selectedVariantName;
};

struct Category {
    QString id;
    QString name;
};

struct Supplier {
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
    QString supplierId;
    double costPrice = 0;
    double sellPrice = 0;
    QString sku;
    bool hasNativeBarcode = false;
    int lowStockThreshold = 0; // 0 = disabled
};

struct ProductVariantInput {
    QString name;
    QString sku; // empty = no barcode for this variant
};

struct InventoryLevelInput {
    QString shopId;
    int quantity = 0;
};

struct SaleItemInput {
    QString productId;
    QString variantId; // empty = no variant — omitted from the wire payload entirely
    int quantity = 0;
    double unitPrice = 0; // price actually charged, may be discounted below listPrice
    double unitCost = 0;
    double listPrice = 0; // product's sell_price at the time it was added to the cart
};

struct SaleItemSummary {
    QString saleItemId;
    QString productId;
    QString productName;
    QString variantName; // empty = no variant
    int quantity = 0;
    double unitPrice = 0;
};

struct SaleSummary {
    QString id;
    QString soldAt;
    QString shopId;
    QString staffId;
    double total = 0;
    QVector<SaleItemSummary> items;
};

// One audit_log row (see 0018_audit_log.sql) — product/staff/inventory
// edits that don't already have their own ledger (sales/returns/transfers
// do). actorName isn't stored on the row; pages resolve it against an
// already-fetched profiles list, same as SalaryPage resolves staff names.
struct AuditLogEntry {
    QString id;
    QString actorId;
    QString action;
    QString entityType;
    QString entityId;
    QString detail;
    QString createdAt;
};

// One stock_transfers row, for AuditLogPage's merged "what happened" view.
// Shop names are resolved client-side against an already-fetched shop list.
struct StockTransferSummary {
    QString id;
    QString productName;
    QString variantName; // empty = no variant
    QString fromShopId;
    QString toShopId;
    int quantity = 0;
    QString staffId;
    QString createdAt;
};

struct ReturnItemInput {
    QString saleItemId;
    int quantity = 0;
};

// One sale, stripped to just what SalaryPage needs to compute daily-wage
// earnings — who made it, when, and for how much. Unlike SaleSummary (used
// by ReturnDialog) this carries no line items.
struct SaleAttribution {
    QString staffId;
    QString soldAt;
    double total = 0;
};

// A logged payout from the salary_payments ledger (see SalaryPage).
struct SalaryPayment {
    QString id;
    QString staffId;
    double amount = 0;
    QString note;
    QString paidAt;
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

    void fetchSuppliers();
    void createSupplier(const QString &name);

    // Creates one or more variants for a product in a single insert,
    // mirroring how upsertInventoryLevels batches per-shop rows. Returns
    // the created rows (with ids) so the caller can seed a 0-stock
    // inventory row per (variant, shop) right after.
    void createVariants(const QString &productId, const QVector<ProductVariantInput> &variants);
    // field must be one of product_variants' own column names (name/sku).
    void updateVariantField(const QString &variantId, const QString &field, const QJsonValue &value);

    void fetchShops();
    // variantId empty = the product's own "no variant" bucket (unchanged
    // behavior). Non-empty = that variant's bucket.
    void upsertInventoryLevels(const QString &productId, const QVector<InventoryLevelInput> &levels,
                                const QString &variantId = QString());

    // Moves stock between two shops via the atomic transfer_stock DB
    // function — admin-only (see 0012_stock_transfers.sql's RLS policy),
    // rejected server-side if the source shop doesn't have enough.
    // variantId empty = the product's own bucket, same convention as above.
    void transferStock(const QString &productId, const QString &fromShopId, const QString &toShopId, int quantity,
                        const QString &variantId = QString());
    // Recent transfers for the Audit Log page's merged view — admin-only
    // per stock_transfers' own RLS policy.
    void fetchStockTransfers();

    void fetchProfiles();
    // field must be one of profiles' own column names (full_name/role/shop_id/
    // daily_rate/bonus_threshold/bonus_amount) — always a fixed string from
    // our own code, same guarantee as updateProductField.
    void updateProfileField(const QString &profileId, const QString &field, const QJsonValue &value);

    // Sales in [startDateIso, endDateIsoExclusive), stripped to staff_id/sold_at/
    // total for SalaryPage's earnings calculation. shopId empty = every shop
    // (only meaningful for an admin — RLS still scopes a non-admin to their own).
    void fetchSalesForPeriod(const QString &shopId, const QString &startDateIso, const QString &endDateIsoExclusive);

    // Payouts already logged for exactly this period (see recordSalaryPayment).
    void fetchSalaryPayments(const QString &periodStartIso, const QString &periodEndIso);
    // Logs a payout. paid_by is set server-side from auth.uid() (see
    // 0014_salary_tracking.sql), never trusted from here.
    void recordSalaryPayment(const QString &staffId, const QString &shopId, const QString &periodStartIso,
                              const QString &periodEndIso, double amount, const QString &note);

    // Full sales (with line items) in [dayStartIso, dayEndIsoExclusive) for
    // DailySummaryPage. shopId empty = every shop (admin-only meaningful,
    // same convention as fetchSalesForPeriod).
    void fetchSalesForDay(const QString &shopId, const QString &dayStartIso, const QString &dayEndIsoExclusive);

    // Best-effort activity trail entry (see 0018_audit_log.sql) — fire and
    // forget by design: a logging failure shouldn't block or complain about
    // the primary action that triggered it.
    void logAuditEvent(const QString &action, const QString &entityType, const QString &entityId,
                        const QString &detail);
    void fetchAuditLog();

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

    void suppliersFetched(const QVector<Supplier> &suppliers);
    void suppliersFetchFailed(const QString &message);
    void supplierCreated(const Supplier &supplier);
    void supplierCreateFailed(const QString &message);

    void variantsCreated(const QVector<ProductVariant> &variants);
    void variantsCreateFailed(const QString &message);
    void variantUpdated();
    void variantUpdateFailed(const QString &message);

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
    void stockTransfersFetched(const QVector<StockTransferSummary> &transfers);
    void stockTransfersFetchFailed(const QString &message);

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

    void salesForPeriodFetched(const QVector<SaleAttribution> &sales);
    void salesForPeriodFetchFailed(const QString &message);

    void salaryPaymentsFetched(const QVector<SalaryPayment> &payments);
    void salaryPaymentsFetchFailed(const QString &message);
    void salaryPaymentRecorded();
    void salaryPaymentRecordFailed(const QString &message);

    void dailySalesFetched(const QVector<SaleSummary> &sales);
    void dailySalesFetchFailed(const QString &message);

    void auditLogFetched(const QVector<AuditLogEntry> &entries);
    void auditLogFetchFailed(const QString &message);

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

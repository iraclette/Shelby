#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonValue>
#include <QMap>
#include <QObject>
#include <QString>
#include <QUrlQuery>
#include <QVector>
#include <functional>

#include "BusinessProfile.h"

class QNetworkAccessManager;
class QNetworkReply;
class QUrlQuery;

struct Profile {
    QString id;
    QString email;
    QString role;
    QString shopId;
    QString fullName;
    bool isDev = false; // set in the DB only; gets pre-release updates
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

    // Set when the scanned barcode was a variant's own sku.
    QString selectedVariantId;
    QString selectedVariantName;
};

struct Category {
    QString id;
    QString name;
    bool visibleOnStorefront = true;
    QString parentId; // empty for a top-level category
};

// Top-level categories, each followed by its subcategories (one level deep).
QVector<Category> categoriesInTreeOrder(const QVector<Category> &categories);
// "Perfumes › Niche Perfumes" for a subcategory, just the name otherwise.
QString categoryLabel(const Category &category, const QVector<Category> &categories);

struct CustomLeatherOrder {
    QString id;
    QString customerName;
    QString customerEmail;
    QString customerPhone;
    QString preferredShopId;
    QString itemDescription;
    QString status; // new/reviewed/quoted/in_progress/completed/cancelled
    QString createdAt;
};

// A storefront order paid through BOG: picked up at pickupShopId, or shipped from it with Inex.
struct OnlineOrderItem {
    QString productName;
    int quantity = 0;
    double unitPrice = 0;
};

struct OnlineOrder {
    QString id;
    QString customerName;
    QString customerEmail;
    QString customerPhone;
    QString pickupShopId;
    QString status; // pending/paid/failed/fulfilled/cancelled
    QString deliveryMethod; // pickup/inex_branch/inex_home
    double deliveryFee = 0; // included in total
    QString deliveryCity;
    QString deliveryAddress; // street (home) or preferred Inex branch (branch, optional)
    double total = 0;
    QString createdAt;
    QVector<OnlineOrderItem> items;
};

// A Contact Us submission from the storefront's /contact form.
struct ContactMessage {
    QString id;
    QString name;
    QString email;
    QString message;
    QString status; // unread/read
    bool emailSent = false;
    QString createdAt;
};

// A "Work with us" application from the storefront's /jobs page.
struct JobApplication {
    QString id;
    QString name;
    QString phone;
    QString email;
    QString preferredShopId; // empty = any shop
    QString availability; // full_time/part_time/either
    QStringList languages; // ka/ru/en
    QString about;
    QString status; // new/contacted/hired/rejected
    QString createdAt;
};

// A "let me know when it's back" request from an out-of-stock product on the storefront.
struct StockRequest {
    QString id;
    QString productName;
    QString contact; // phone or email
    QString status; // new/contacted/dismissed
    QString createdAt;
};

struct Supplier {
    QString id;
    QString name;
};

struct ProductImage {
    QString id;
    QString storagePath;
    bool isPrimary = false;
    QString thumbPath; // ~400px copy; empty for photos uploaded before 2.3.2
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

// actorName isn't stored; pages resolve it from the profiles list.
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

// Just what SalaryPage needs: who, when, how much.
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

// clientGeneratedId is reused on retries so a sale can't be recorded twice.
struct QueuedSale {
    QString clientGeneratedId;
    QString shopId;
    QVector<SaleItemInput> items;
    double total = 0;
};

// Talks to Supabase's REST APIs directly; RLS is what keeps the anon key safe.
class SupabaseClient : public QObject {
    Q_OBJECT

public:
    SupabaseClient(QString baseUrl, QString anonKey, QObject *parent = nullptr);

    void signIn(const QString &email, const QString &password);
    // Signs in with a saved refresh token instead of a password ("stay signed in", relaunch after an update).
    void restoreSession(const QString &refreshToken);
    // Revokes the session on the server and forgets it locally.
    void signOut();
    QString refreshToken() const { return m_refreshToken; }
    QString userId() const { return m_userId; } // the signed-in account, empty before sign-in
    void fetchProfile(const QString &userId);

    void fetchProducts();
    void createProduct(const ProductInput &input);
    // field is always a fixed column name from our own code, never user input.
    void updateProductField(const QString &productId, const QString &field, const QJsonValue &value);

    void fetchCategories();
    void createCategory(const QString &name, const QString &parentId = {});
    // field must be one of categories' own column names (currently just
    // visible_on_storefront) — same fixed-string guarantee as updateProductField.
    void updateCategoryField(const QString &categoryId, const QString &field, const QJsonValue &value);

    // Admin-only per custom_leather_orders' RLS policy (mirrors salary_payments).
    void fetchCustomLeatherOrders();

    void fetchOnlineOrders();
    void updateOnlineOrderStatus(const QString &orderId, const QString &status);
    // field must be one of custom_leather_orders' own column names
    // (currently just status).
    void updateCustomLeatherOrderField(const QString &orderId, const QString &field, const QJsonValue &value);

    // Admin-only per contact_messages' RLS policy.
    void fetchContactMessages();
    void fetchJobApplications();
    void updateJobApplicationStatus(const QString &applicationId, const QString &status);
    void fetchStockRequests();
    void updateStockRequestStatus(const QString &requestId, const QString &status);
    // field must be one of contact_messages' own column names (currently just status).
    void updateContactMessageField(const QString &messageId, const QString &field, const QJsonValue &value);

    void fetchSuppliers();
    void createSupplier(const QString &name);

    void createVariants(const QString &productId, const QVector<ProductVariantInput> &variants);
    // A variant for an existing product; the first one takes over the product's own stock
    // (add_product_variant in 0028_storefront_variants.sql).
    void addProductVariant(const QString &productId, const QString &name, const QString &sku);
    // field must be one of product_variants' own column names (name/sku).
    void updateVariantField(const QString &variantId, const QString &field, const QJsonValue &value);

    void fetchShops();
    // variantId empty = the product's own "no variant" bucket (unchanged
    // behavior). Non-empty = that variant's bucket.
    void upsertInventoryLevels(const QString &productId, const QVector<InventoryLevelInput> &levels,
                                const QString &variantId = QString());

    // Admin only; fails server-side if the source shop doesn't have enough.
    void transferStock(const QString &productId, const QString &fromShopId, const QString &toShopId, int quantity,
                        const QString &variantId = QString());
    // Recent transfers for the Audit Log page's merged view — admin-only
    // per stock_transfers' own RLS policy.
    void fetchStockTransfers();

    void fetchProfiles();
    // Same rule as updateProductField: field is a fixed column name.
    void updateProfileField(const QString &profileId, const QString &field, const QJsonValue &value);

    // shopId empty = every shop (admin only).
    void fetchSalesForPeriod(const QString &shopId, const QString &startDateIso, const QString &endDateIsoExclusive);

    // Payouts already logged for exactly this period (see recordSalaryPayment).
    void fetchSalaryPayments(const QString &periodStartIso, const QString &periodEndIso);
    // Logs a payout. paid_by is set server-side from auth.uid() (see
    // 0014_salary_tracking.sql), never trusted from here.
    void recordSalaryPayment(const QString &staffId, const QString &shopId, const QString &periodStartIso,
                              const QString &periodEndIso, double amount, const QString &note);

    // shopId empty = every shop (admin only).
    void fetchSalesForDay(const QString &shopId, const QString &dayStartIso, const QString &dayEndIsoExclusive);
    // Same query, but answered to the caller only — dailySalesFetched would also
    // repaint the Daily Summary page with someone else's date range.
    void fetchSalesBetween(const QString &startIso, const QString &endIsoExclusive,
                           std::function<void(const QVector<SaleSummary> &sales, const QString &error)> onDone);

    // Plain PostgREST access for small features that don't need a typed struct and signal
    // (desktop pop-ups, Telegram links). table is e.g. "telegram_links"; body is the JSON to send.
    void selectRows(const QString &table, const QUrlQuery &query,
                    std::function<void(const QJsonArray &rows, const QString &error)> onDone);
    void writeRows(const QByteArray &verb, const QString &table, const QUrlQuery &query, const QJsonValue &body,
                   std::function<void(const QString &error)> onDone);

    // Fire and forget: a logging failure never blocks the action.
    void logAuditEvent(const QString &action, const QString &entityType, const QString &entityId,
                        const QString &detail);
    void fetchAuditLog();

    // Via the staff-admin Edge Function, which holds the service-role key.
    void createStaffAccount(const QString &username, const QString &password, const QString &fullName,
                             const QString &role, const QString &shopId);
    void deleteStaffAccount(const QString &profileId);

    // Uploads the file to Storage, then records it in product_images.
    void uploadProductImage(const QString &productId, const QString &localFilePath, bool isPrimary);
    void fetchProductImages(const QString &productId);
    // Deletes both the Storage object and its product_images row.
    void deleteProductImage(const QString &imageId, const QString &storagePath, const QString &thumbPath);
    // The bucket is public, so this is just a URL — no auth needed to load it.
    QString publicImageUrl(const QString &storagePath) const;

    // business_settings (Settings → Business). The fetch needs no sign-in: the login screen
    // shows the business's name and logo.
    void fetchBusinessSettings();
    // fields are business_settings column names (name, currency_code, timezone, logo_path…).
    void updateBusinessSettings(const QJsonObject &fields);
    // Uploads to the branding bucket; businessLogoUploaded gives the new storage path.
    void uploadBusinessLogo(const QString &localFilePath);
    void deleteBusinessLogoFile(const QString &storagePath); // best-effort, no signal
    QString businessLogoUrl(const QString &storagePath) const; // empty for an empty path

    void lookupProductBySku(const QString &sku);

    // Queued locally if the server can't be reached; synced by flushPendingSales().
    void recordSale(const QString &shopId, const QVector<SaleItemInput> &items, double total);

    void fetchRecentSales(const QString &shopId);
    // Unit prices come from the original sale, server-side.
    void submitReturn(const QString &shopId, const QString &saleId, const QString &reason,
                       const QVector<ReturnItemInput> &items);

    // Stops at the first sale that still fails. Safe to call repeatedly.
    void flushPendingSales();

    int pendingSalesCount() const { return m_pendingSales.size(); }

signals:
    void signInSucceeded(const QString &userId);
    void signInFailed(const QString &message);
    // New tokens after sign-in or a refresh. Refresh tokens are single-use, so a saved one must be replaced.
    void sessionChanged(const QString &refreshToken);
    // The server rejected the refresh token (revoked, or the account was removed): sign in again.
    void sessionExpired();
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
    void categoryUpdated();
    void categoryUpdateFailed(const QString &message);

    void customLeatherOrdersFetched(const QVector<CustomLeatherOrder> &orders);
    void onlineOrdersFetched(const QVector<OnlineOrder> &orders);
    void onlineOrdersFetchFailed(const QString &message);
    void onlineOrderUpdated();
    void onlineOrderUpdateFailed(const QString &message);
    void customLeatherOrdersFetchFailed(const QString &message);
    void customLeatherOrderUpdated();
    void customLeatherOrderUpdateFailed(const QString &message);

    void contactMessagesFetched(const QVector<ContactMessage> &messages);
    void businessSettingsFetched(const BusinessSettings &settings, const QString &logoUrl);
    void businessSettingsFetchFailed(const QString &message);
    void businessSettingsSaved(const BusinessSettings &settings, const QString &logoUrl);
    void businessSettingsSaveFailed(const QString &message);
    void businessLogoUploaded(const QString &storagePath);
    void businessLogoUploadFailed(const QString &message);
    void jobApplicationsFetched(const QVector<JobApplication> &applications);
    void jobApplicationsFetchFailed(const QString &message);
    void jobApplicationUpdated();
    void jobApplicationUpdateFailed(const QString &message);
    void stockRequestsFetched(const QVector<StockRequest> &requests);
    void stockRequestsFetchFailed(const QString &message);
    void stockRequestUpdated();
    void stockRequestUpdateFailed(const QString &message);
    void contactMessagesFetchFailed(const QString &message);
    void contactMessageUpdated();
    void contactMessageUpdateFailed(const QString &message);

    void suppliersFetched(const QVector<Supplier> &suppliers);
    void suppliersFetchFailed(const QString &message);
    void supplierCreated(const Supplier &supplier);
    void supplierCreateFailed(const QString &message);

    void variantsCreated(const QVector<ProductVariant> &variants);
    void variantsCreateFailed(const QString &message);
    void variantAdded();
    void variantAddFailed(const QString &message);
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
    QString m_refreshToken;
    QDateTime m_accessTokenExpiresAt;
    QString m_userId;

    // Access tokens last about an hour, while a POS stays open all day. Every request
    // waits here until the token is fresh, refreshing first when it's about to expire.
    bool m_refreshing = false;
    QVector<std::function<void()>> m_waitingForToken;
    void withFreshToken(std::function<void()> send);
    void refreshAccessToken();
    void requestTokens(const QString &grantType, const QJsonObject &body);
    bool storeTokens(const QByteArray &data);

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

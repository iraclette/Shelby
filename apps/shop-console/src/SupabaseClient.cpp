#include "SupabaseClient.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>

SupabaseClient::SupabaseClient(QString baseUrl, QString anonKey, QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_baseUrl(std::move(baseUrl))
    , m_anonKey(std::move(anonKey)) {
    if (m_baseUrl.endsWith('/')) m_baseUrl.chop(1);
    loadPendingSales();
}

static bool isNetworkLayerError(QNetworkReply::NetworkError error) {
    // Qt groups "can't reach the server at all" errors into 1-99
    // (ConnectionRefusedError..UnknownNetworkError); everything above that
    // is a real response from the server (auth, validation, RLS, etc.) and
    // should surface as an error rather than be queued as "offline".
    return error >= QNetworkReply::ConnectionRefusedError && error <= QNetworkReply::UnknownNetworkError;
}

static QString extractErrorMessage(const QByteArray &body, const QString &fallback) {
    const QJsonObject obj = QJsonDocument::fromJson(body).object();
    for (const char *key : {"msg", "message", "error_description", "error"}) {
        const QString value = obj.value(key).toString();
        if (!value.isEmpty()) return value;
    }
    return fallback;
}

void SupabaseClient::authorizedGet(const QString &path, const QUrlQuery &query,
                                    const std::function<void(QNetworkReply *)> &onFinished) {
    QUrl url(m_baseUrl + path);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("apikey", m_anonKey.toUtf8());
    request.setRawHeader("Authorization", ("Bearer " + m_accessToken).toUtf8());

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [reply, onFinished]() {
        reply->deleteLater();
        onFinished(reply);
    });
}

void SupabaseClient::authorizedWrite(const QByteArray &verb, const QString &path, const QUrlQuery &query,
                                      const QByteArray &body, const QMap<QByteArray, QByteArray> &extraHeaders,
                                      const std::function<void(QNetworkReply *)> &onFinished) {
    QUrl url(m_baseUrl + path);
    if (!query.isEmpty()) url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("apikey", m_anonKey.toUtf8());
    request.setRawHeader("Authorization", ("Bearer " + m_accessToken).toUtf8());
    for (auto it = extraHeaders.constBegin(); it != extraHeaders.constEnd(); ++it) {
        request.setRawHeader(it.key(), it.value());
    }

    QNetworkReply *reply = m_network->sendCustomRequest(request, verb, body);
    connect(reply, &QNetworkReply::finished, this, [reply, onFinished]() {
        reply->deleteLater();
        onFinished(reply);
    });
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
    QUrlQuery query;
    query.addQueryItem("id", "eq." + userId);
    query.addQueryItem("select", "id,email,role,shop_id,full_name");

    authorizedGet("/rest/v1/profiles", query, [this](QNetworkReply *reply) {
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
        profile.id = row.value("id").toString();
        profile.email = row.value("email").toString();
        profile.role = row.value("role").toString();
        profile.shopId = row.value("shop_id").toString();
        profile.fullName = row.value("full_name").toString();
        emit profileFetched(profile);
    });
}

void SupabaseClient::fetchProfiles() {
    QUrlQuery query;
    query.addQueryItem("select", "id,email,role,shop_id,full_name,daily_rate,bonus_threshold,bonus_amount");
    query.addQueryItem("order", "full_name.asc");

    authorizedGet("/rest/v1/profiles", query, [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit profilesFetchFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        QVector<Profile> profiles;
        for (const QJsonValue &value : QJsonDocument::fromJson(data).array()) {
            const QJsonObject row = value.toObject();
            Profile profile;
            profile.id = row.value("id").toString();
            profile.email = row.value("email").toString();
            profile.role = row.value("role").toString();
            profile.shopId = row.value("shop_id").toString();
            profile.fullName = row.value("full_name").toString();
            profile.dailyRate = row.value("daily_rate").toDouble();
            profile.bonusThreshold = row.value("bonus_threshold").toDouble();
            profile.bonusAmount = row.value("bonus_amount").toDouble();
            profiles.append(profile);
        }
        emit profilesFetched(profiles);
    });
}

void SupabaseClient::updateProfileField(const QString &profileId, const QString &field, const QJsonValue &value) {
    QUrlQuery query;
    query.addQueryItem("id", "eq." + profileId);

    const QJsonObject body{{field, value}};
    const QMap<QByteArray, QByteArray> headers{{"Content-Type", "application/json"}};

    authorizedWrite("PATCH", "/rest/v1/profiles", query, QJsonDocument(body).toJson(), headers,
                    [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit profileUpdateFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }
        emit profileUpdated();
    });
}

void SupabaseClient::createStaffAccount(const QString &username, const QString &password, const QString &fullName,
                                         const QString &role, const QString &shopId) {
    const QJsonObject body{
        {"action", "create"},
        {"username", username},
        {"password", password},
        {"full_name", fullName},
        {"role", role},
        {"shop_id", shopId.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(shopId)},
    };
    const QMap<QByteArray, QByteArray> headers{{"Content-Type", "application/json"}};

    authorizedWrite("POST", "/functions/v1/staff-admin", {}, QJsonDocument(body).toJson(), headers,
                    [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit staffAccountCreateFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }
        emit staffAccountCreated();
    });
}

void SupabaseClient::deleteStaffAccount(const QString &profileId) {
    const QJsonObject body{
        {"action", "delete"},
        {"profile_id", profileId},
    };
    const QMap<QByteArray, QByteArray> headers{{"Content-Type", "application/json"}};

    authorizedWrite("POST", "/functions/v1/staff-admin", {}, QJsonDocument(body).toJson(), headers,
                    [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit staffAccountDeleteFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }
        emit staffAccountDeleted();
    });
}

void SupabaseClient::fetchProducts() {
    QUrlQuery query;
    // Embeds each product's per-shop stock rows via the inventory_levels
    // foreign key, so per-shop quantities are available in one round trip.
    query.addQueryItem("select", "id,name,sku,category_id,sell_price,cost_price,inventory_levels(shop_id,quantity)");
    query.addQueryItem("order", "created_at.desc");
    query.addQueryItem("limit", "50");

    authorizedGet("/rest/v1/products", query, [this](QNetworkReply *reply) {
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
            product.categoryId = row.value("category_id").toString();
            product.sellPrice = row.value("sell_price").toDouble();
            product.costPrice = row.value("cost_price").toDouble();

            for (const QJsonValue &levelValue : row.value("inventory_levels").toArray()) {
                const QJsonObject level = levelValue.toObject();
                product.stockByShop.insert(level.value("shop_id").toString(), level.value("quantity").toInt());
            }

            products.append(product);
        }
        emit productsFetched(products);
    });
}

void SupabaseClient::createProduct(const ProductInput &input) {
    QJsonObject body{
        {"name", input.name},
        {"description", input.description},
        {"cost_price", input.costPrice},
        {"sell_price", input.sellPrice},
        {"sku", input.sku},
        {"has_native_barcode", input.hasNativeBarcode},
    };
    if (!input.categoryId.isEmpty()) body["category_id"] = input.categoryId;

    const QMap<QByteArray, QByteArray> headers{
        {"Content-Type", "application/json"},
        {"Prefer", "return=representation"},
    };

    authorizedWrite("POST", "/rest/v1/products", {}, QJsonDocument(body).toJson(), headers,
                    [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit productCreateFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        const QJsonArray rows = QJsonDocument::fromJson(data).array();
        if (rows.isEmpty()) {
            emit productCreateFailed("Product was created but no id was returned.");
            return;
        }

        emit productCreated(rows.first().toObject().value("id").toString());
    });
}

void SupabaseClient::updateProductField(const QString &productId, const QString &field, const QJsonValue &value) {
    QUrlQuery query;
    query.addQueryItem("id", "eq." + productId);

    const QJsonObject body{{field, value}};
    const QMap<QByteArray, QByteArray> headers{{"Content-Type", "application/json"}};

    authorizedWrite("PATCH", "/rest/v1/products", query, QJsonDocument(body).toJson(), headers,
                    [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit productUpdateFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }
        emit productUpdated();
    });
}

void SupabaseClient::fetchCategories() {
    QUrlQuery query;
    query.addQueryItem("select", "id,name");
    query.addQueryItem("order", "name.asc");

    authorizedGet("/rest/v1/categories", query, [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit categoriesFetchFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        QVector<Category> categories;
        for (const QJsonValue &value : QJsonDocument::fromJson(data).array()) {
            const QJsonObject row = value.toObject();
            categories.append({row.value("id").toString(), row.value("name").toString()});
        }
        emit categoriesFetched(categories);
    });
}

void SupabaseClient::createCategory(const QString &name) {
    const QJsonObject body{{"name", name}};
    const QMap<QByteArray, QByteArray> headers{
        {"Content-Type", "application/json"},
        {"Prefer", "return=representation"},
    };

    authorizedWrite("POST", "/rest/v1/categories", {}, QJsonDocument(body).toJson(), headers,
                    [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit categoryCreateFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        const QJsonArray rows = QJsonDocument::fromJson(data).array();
        if (rows.isEmpty()) {
            emit categoryCreateFailed("Category was created but no id was returned.");
            return;
        }

        const QJsonObject row = rows.first().toObject();
        emit categoryCreated({row.value("id").toString(), row.value("name").toString()});
    });
}

void SupabaseClient::fetchShops() {
    QUrlQuery query;
    query.addQueryItem("select", "id,name");
    query.addQueryItem("order", "name.asc");

    authorizedGet("/rest/v1/shops", query, [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit shopsFetchFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        QVector<Shop> shops;
        for (const QJsonValue &value : QJsonDocument::fromJson(data).array()) {
            const QJsonObject row = value.toObject();
            shops.append({row.value("id").toString(), row.value("name").toString()});
        }
        emit shopsFetched(shops);
    });
}

void SupabaseClient::upsertInventoryLevels(const QString &productId, const QVector<InventoryLevelInput> &levels) {
    QJsonArray rows;
    for (const InventoryLevelInput &level : levels) {
        rows.append(QJsonObject{
            {"product_id", productId},
            {"shop_id", level.shopId},
            {"quantity", level.quantity},
        });
    }

    QUrlQuery query;
    query.addQueryItem("on_conflict", "product_id,shop_id");

    const QMap<QByteArray, QByteArray> headers{
        {"Content-Type", "application/json"},
        {"Prefer", "resolution=merge-duplicates"},
    };

    authorizedWrite("POST", "/rest/v1/inventory_levels", query, QJsonDocument(rows).toJson(), headers,
                    [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit inventoryLevelsUpsertFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }
        emit inventoryLevelsUpserted();
    });
}

void SupabaseClient::transferStock(const QString &productId, const QString &fromShopId, const QString &toShopId,
                                    int quantity) {
    const QJsonObject body{
        {"p_client_generated_id", QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {"p_product_id", productId},
        {"p_from_shop_id", fromShopId},
        {"p_to_shop_id", toShopId},
        {"p_quantity", quantity},
    };
    const QMap<QByteArray, QByteArray> headers{{"Content-Type", "application/json"}};

    authorizedWrite("POST", "/rest/v1/rpc/transfer_stock", {}, QJsonDocument(body).toJson(), headers,
                    [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit stockTransferFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }
        emit stockTransferred();
    });
}

void SupabaseClient::uploadProductImage(const QString &productId, const QString &localFilePath, bool isPrimary) {
    QFile file(localFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit imageUploadFailed("Could not open file: " + localFilePath);
        return;
    }
    const QByteArray fileBytes = file.readAll();
    file.close();

    const QFileInfo info(localFilePath);
    const QString storagePath = productId + "/" + QUuid::createUuid().toString(QUuid::WithoutBraces) +
                                 "." + info.suffix();
    const QString mimeType = QMimeDatabase().mimeTypeForFile(localFilePath).name();

    const QMap<QByteArray, QByteArray> headers{
        {"Content-Type", mimeType.toUtf8()},
    };

    authorizedWrite("POST", "/storage/v1/object/product-images/" + storagePath, {}, fileBytes, headers,
                    [this, productId, storagePath, isPrimary](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit imageUploadFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        const QJsonObject body{
            {"product_id", productId},
            {"storage_path", storagePath},
            {"is_primary", isPrimary},
        };
        const QMap<QByteArray, QByteArray> insertHeaders{{"Content-Type", "application/json"}};

        authorizedWrite("POST", "/rest/v1/product_images", {}, QJsonDocument(body).toJson(), insertHeaders,
                        [this](QNetworkReply *insertReply) {
            const QByteArray insertData = insertReply->readAll();
            if (insertReply->error() != QNetworkReply::NoError) {
                emit imageUploadFailed(extractErrorMessage(insertData, insertReply->errorString()));
                return;
            }
            emit imageUploaded();
        });
    });
}

void SupabaseClient::fetchProductImages(const QString &productId) {
    QUrlQuery query;
    query.addQueryItem("product_id", "eq." + productId);
    query.addQueryItem("select", "id,storage_path,is_primary");
    query.addQueryItem("order", "is_primary.desc,created_at.asc");

    authorizedGet("/rest/v1/product_images", query, [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit productImagesFetchFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        QVector<ProductImage> images;
        for (const QJsonValue &value : QJsonDocument::fromJson(data).array()) {
            const QJsonObject row = value.toObject();
            images.append({row.value("id").toString(), row.value("storage_path").toString(),
                            row.value("is_primary").toBool()});
        }
        emit productImagesFetched(images);
    });
}

void SupabaseClient::deleteProductImage(const QString &imageId, const QString &storagePath) {
    authorizedWrite("DELETE", "/storage/v1/object/product-images/" + storagePath, {}, {}, {},
                    [this, imageId](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit imageDeleteFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        QUrlQuery query;
        query.addQueryItem("id", "eq." + imageId);
        authorizedWrite("DELETE", "/rest/v1/product_images", query, {}, {},
                        [this](QNetworkReply *deleteReply) {
            const QByteArray deleteData = deleteReply->readAll();
            if (deleteReply->error() != QNetworkReply::NoError) {
                emit imageDeleteFailed(extractErrorMessage(deleteData, deleteReply->errorString()));
                return;
            }
            emit imageDeleted();
        });
    });
}

QString SupabaseClient::publicImageUrl(const QString &storagePath) const {
    return m_baseUrl + "/storage/v1/object/public/product-images/" + storagePath;
}

void SupabaseClient::lookupProductBySku(const QString &sku) {
    QUrlQuery query;
    query.addQueryItem("sku", "eq." + sku);
    query.addQueryItem("select", "id,name,sku,sell_price,cost_price");
    query.addQueryItem("limit", "1");

    authorizedGet("/rest/v1/products", query, [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit productLookupFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        const QJsonArray rows = QJsonDocument::fromJson(data).array();
        if (rows.isEmpty()) {
            emit productLookupFailed("No product found with that barcode.");
            return;
        }

        const QJsonObject row = rows.first().toObject();
        Product product;
        product.id = row.value("id").toString();
        product.name = row.value("name").toString();
        product.sku = row.value("sku").toString();
        product.sellPrice = row.value("sell_price").toDouble();
        product.costPrice = row.value("cost_price").toDouble();
        emit productLookedUp(product);
    });
}

void SupabaseClient::recordSale(const QString &shopId, const QVector<SaleItemInput> &items, double total) {
    const QString clientGeneratedId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    submitSale(shopId, clientGeneratedId, items, total, /*isRetryFromQueue=*/false);
}

void SupabaseClient::submitSale(const QString &shopId, const QString &clientGeneratedId,
                                  const QVector<SaleItemInput> &items, double total, bool isRetryFromQueue) {
    QJsonArray itemsJson;
    for (const SaleItemInput &item : items) {
        itemsJson.append(QJsonObject{
            {"product_id", item.productId},
            {"quantity", item.quantity},
            {"unit_price", item.unitPrice},
            {"unit_cost", item.unitCost},
            {"list_price", item.listPrice},
        });
    }

    const QJsonObject body{
        {"p_shop_id", shopId},
        {"p_client_generated_id", clientGeneratedId},
        {"p_items", itemsJson},
        {"p_total", total},
    };
    const QMap<QByteArray, QByteArray> headers{{"Content-Type", "application/json"}};

    authorizedWrite("POST", "/rest/v1/rpc/record_sale", {}, QJsonDocument(body).toJson(), headers,
                    [this, shopId, clientGeneratedId, items, total, isRetryFromQueue](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();

        if (reply->error() != QNetworkReply::NoError) {
            if (isRetryFromQueue) {
                // Still offline (or the server is down) — leave it queued
                // and stop; the next timer tick will try again.
                m_flushingPendingSales = false;
                return;
            }
            if (isNetworkLayerError(reply->error())) {
                enqueueOffline({clientGeneratedId, shopId, items, total});
                emit saleQueuedOffline(m_pendingSales.size());
            } else {
                emit saleRecordFailed(extractErrorMessage(data, reply->errorString()));
            }
            return;
        }

        if (isRetryFromQueue) {
            m_pendingSales.removeIf([&](const QueuedSale &sale) {
                return sale.clientGeneratedId == clientGeneratedId;
            });
            savePendingSales();
            emit pendingSalesChanged(m_pendingSales.size());
            m_flushingPendingSales = false;
            flushPendingSales(); // keep draining the queue
        } else {
            emit saleRecorded();
            flushPendingSales(); // this connection worked, so try any backlog too
        }
    });
}

void SupabaseClient::fetchRecentSales(const QString &shopId) {
    QUrlQuery query;
    query.addQueryItem("shop_id", "eq." + shopId);
    query.addQueryItem("select", "id,sold_at,total,sale_items(id,product_id,quantity,unit_price,products(name))");
    query.addQueryItem("order", "sold_at.desc");
    query.addQueryItem("limit", "50");

    authorizedGet("/rest/v1/sales", query, [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit recentSalesFetchFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        QVector<SaleSummary> sales;
        for (const QJsonValue &value : QJsonDocument::fromJson(data).array()) {
            const QJsonObject row = value.toObject();
            SaleSummary sale;
            sale.id = row.value("id").toString();
            sale.soldAt = row.value("sold_at").toString();
            sale.total = row.value("total").toDouble();

            for (const QJsonValue &itemValue : row.value("sale_items").toArray()) {
                const QJsonObject itemRow = itemValue.toObject();
                SaleItemSummary item;
                item.saleItemId = itemRow.value("id").toString();
                item.productId = itemRow.value("product_id").toString();
                item.quantity = itemRow.value("quantity").toInt();
                item.unitPrice = itemRow.value("unit_price").toDouble();
                item.productName = itemRow.value("products").toObject().value("name").toString();
                sale.items.append(item);
            }
            sales.append(sale);
        }
        emit recentSalesFetched(sales);
    });
}

void SupabaseClient::submitReturn(const QString &shopId, const QString &saleId, const QString &reason,
                                   const QVector<ReturnItemInput> &items) {
    QJsonArray itemsJson;
    for (const ReturnItemInput &item : items) {
        itemsJson.append(QJsonObject{
            {"sale_item_id", item.saleItemId},
            {"quantity", item.quantity},
        });
    }

    const QJsonObject body{
        {"p_shop_id", shopId},
        {"p_client_generated_id", QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {"p_sale_id", saleId},
        {"p_reason", reason},
        {"p_items", itemsJson},
    };
    const QMap<QByteArray, QByteArray> headers{{"Content-Type", "application/json"}};

    authorizedWrite("POST", "/rest/v1/rpc/record_return", {}, QJsonDocument(body).toJson(), headers,
                    [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit returnRecordFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }
        emit returnRecorded();
    });
}

void SupabaseClient::fetchSalesForPeriod(const QString &shopId, const QString &startDateIso,
                                          const QString &endDateIsoExclusive) {
    QUrlQuery query;
    query.addQueryItem("select", "staff_id,sold_at,total");
    query.addQueryItem("sold_at", "gte." + startDateIso);
    query.addQueryItem("sold_at", "lt." + endDateIsoExclusive);
    if (!shopId.isEmpty()) query.addQueryItem("shop_id", "eq." + shopId);
    query.addQueryItem("order", "sold_at.asc");

    authorizedGet("/rest/v1/sales", query, [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit salesForPeriodFetchFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        QVector<SaleAttribution> sales;
        for (const QJsonValue &value : QJsonDocument::fromJson(data).array()) {
            const QJsonObject row = value.toObject();
            sales.append({row.value("staff_id").toString(), row.value("sold_at").toString(),
                          row.value("total").toDouble()});
        }
        emit salesForPeriodFetched(sales);
    });
}

void SupabaseClient::fetchSalaryPayments(const QString &periodStartIso, const QString &periodEndIso) {
    QUrlQuery query;
    query.addQueryItem("select", "id,staff_id,amount,note,paid_at");
    query.addQueryItem("period_start", "eq." + periodStartIso);
    query.addQueryItem("period_end", "eq." + periodEndIso);

    authorizedGet("/rest/v1/salary_payments", query, [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit salaryPaymentsFetchFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }

        QVector<SalaryPayment> payments;
        for (const QJsonValue &value : QJsonDocument::fromJson(data).array()) {
            const QJsonObject row = value.toObject();
            SalaryPayment payment;
            payment.id = row.value("id").toString();
            payment.staffId = row.value("staff_id").toString();
            payment.amount = row.value("amount").toDouble();
            payment.note = row.value("note").toString();
            payment.paidAt = row.value("paid_at").toString();
            payments.append(payment);
        }
        emit salaryPaymentsFetched(payments);
    });
}

void SupabaseClient::recordSalaryPayment(const QString &staffId, const QString &shopId,
                                          const QString &periodStartIso, const QString &periodEndIso, double amount,
                                          const QString &note) {
    QJsonObject body{
        {"staff_id", staffId},
        {"shop_id", shopId},
        {"period_start", periodStartIso},
        {"period_end", periodEndIso},
        {"amount", amount},
    };
    if (!note.isEmpty()) body["note"] = note;
    const QMap<QByteArray, QByteArray> headers{{"Content-Type", "application/json"}};

    authorizedWrite("POST", "/rest/v1/salary_payments", {}, QJsonDocument(body).toJson(), headers,
                    [this](QNetworkReply *reply) {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit salaryPaymentRecordFailed(extractErrorMessage(data, reply->errorString()));
            return;
        }
        emit salaryPaymentRecorded();
    });
}

void SupabaseClient::enqueueOffline(const QueuedSale &sale) {
    m_pendingSales.append(sale);
    savePendingSales();
}

void SupabaseClient::flushPendingSales() {
    if (m_flushingPendingSales || m_pendingSales.isEmpty()) return;
    m_flushingPendingSales = true;

    const QueuedSale &sale = m_pendingSales.first();
    submitSale(sale.shopId, sale.clientGeneratedId, sale.items, sale.total, /*isRetryFromQueue=*/true);
}

QString SupabaseClient::pendingSalesFilePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/pending_sales.json";
}

void SupabaseClient::loadPendingSales() {
    QFile file(pendingSalesFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;

    for (const QJsonValue &value : QJsonDocument::fromJson(file.readAll()).array()) {
        const QJsonObject obj = value.toObject();
        QueuedSale sale;
        sale.clientGeneratedId = obj.value("client_generated_id").toString();
        sale.shopId = obj.value("shop_id").toString();
        sale.total = obj.value("total").toDouble();
        for (const QJsonValue &itemValue : obj.value("items").toArray()) {
            const QJsonObject itemObj = itemValue.toObject();
            sale.items.append({
                itemObj.value("product_id").toString(),
                itemObj.value("quantity").toInt(),
                itemObj.value("unit_price").toDouble(),
                itemObj.value("unit_cost").toDouble(),
                // Sales queued offline before this field existed fall back to
                // unit_price, same as record_sale's own coalesce.
                itemObj.contains("list_price") ? itemObj.value("list_price").toDouble()
                                                : itemObj.value("unit_price").toDouble(),
            });
        }
        m_pendingSales.append(sale);
    }
}

void SupabaseClient::savePendingSales() const {
    QJsonArray arr;
    for (const QueuedSale &sale : m_pendingSales) {
        QJsonArray itemsArr;
        for (const SaleItemInput &item : sale.items) {
            itemsArr.append(QJsonObject{
                {"product_id", item.productId},
                {"quantity", item.quantity},
                {"unit_price", item.unitPrice},
                {"unit_cost", item.unitCost},
                {"list_price", item.listPrice},
            });
        }
        arr.append(QJsonObject{
            {"client_generated_id", sale.clientGeneratedId},
            {"shop_id", sale.shopId},
            {"items", itemsArr},
            {"total", sale.total},
        });
    }

    const QString path = pendingSalesFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(arr).toJson());
    }
}

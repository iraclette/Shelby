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
    query.addQueryItem("select", "role,shop_id,full_name");

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
        profile.role = row.value("role").toString();
        profile.shopId = row.value("shop_id").toString();
        profile.fullName = row.value("full_name").toString();
        emit profileFetched(profile);
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

#include "ProductSpreadsheet.h"
#include "Csv.h"
#include "ProductDialog.h"
#include "Theme.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHash>
#include <QJsonObject>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSet>
#include <QUrlQuery>
#include <QVBoxLayout>
#include <memory>
#include <optional>

namespace {

enum Column {
    ColSku,
    ColName,
    ColVariant,
    ColVariantSku,
    ColCategory,
    ColSupplier,
    ColCostPrice,
    ColSellPrice,
    ColLowStock,
    ColDescriptionKa,
    ColDescriptionEn,
    ColDescriptionRu,
    ColumnCount,
};

// The import also accepts the English headings whatever language the console is in.
const char *const englishColumnNames[ColumnCount] = {
    "Barcode / SKU", "Name",       "Variant",         "Variant barcode",        "Category",
    "Supplier",      "Cost price", "Sell price",      "Low stock alert",        "Description (Georgian)",
    "Description (English)",       "Description (Russian)",
};
const char *const englishStockPrefix = "Stock: ";

const char *const descriptionLanguages[] = {"ka", "en", "ru"};

constexpr int batchSize = 100; // products per import_products call; each call has ~8 s

QString normalized(const QString &header) {
    return header.simplified().toLower();
}

QString moneyText(double value) {
    return QString::number(value, 'f', 2);
}

// "12.50", "12,50", "₾ 12.50" -> 12.5; empty -> no value; anything else -> ok = false.
std::optional<double> parseNumber(QString text, bool &ok) {
    ok = true;
    text.remove(QChar(0x20BE)).remove("GEL", Qt::CaseInsensitive).remove(' ').remove(QChar(0xA0));
    if (text.isEmpty()) return std::nullopt;
    text.replace(',', '.');
    const double value = text.toDouble(&ok);
    if (!ok || value < 0) {
        ok = false;
        return std::nullopt;
    }
    return value;
}

// The catalog both directions need, read in one go.
struct Catalog {
    QJsonArray shops, categories, suppliers, products;
};

void loadCatalog(SupabaseClient *client, std::function<void(const Catalog &, const QString &error)> onDone) {
    auto catalog = std::make_shared<Catalog>();
    QUrlQuery shops;
    shops.addQueryItem("select", "id,name");
    shops.addQueryItem("order", "name.asc");
    client->selectAllRows("shops", shops, [=](const QJsonArray &rows, const QString &error) {
        if (!error.isEmpty()) return onDone({}, error);
        catalog->shops = rows;
        QUrlQuery categories;
        categories.addQueryItem("select", "id,name,parent_id");
        categories.addQueryItem("order", "name.asc");
        client->selectAllRows("categories", categories, [=](const QJsonArray &rows, const QString &error) {
            if (!error.isEmpty()) return onDone({}, error);
            catalog->categories = rows;
            QUrlQuery suppliers;
            suppliers.addQueryItem("select", "id,name");
            suppliers.addQueryItem("order", "name.asc");
            client->selectAllRows("suppliers", suppliers, [=](const QJsonArray &rows, const QString &error) {
                if (!error.isEmpty()) return onDone({}, error);
                catalog->suppliers = rows;
                QUrlQuery products;
                products.addQueryItem("select",
                                      "id,name,sku,cost_price,sell_price,low_stock_threshold,category_id,supplier_id,"
                                      "descriptions,product_variants(id,name,sku),"
                                      "inventory_levels(shop_id,variant_id,quantity)");
                products.addQueryItem("order", "name.asc,id.asc");
                client->selectAllRows("products", products, [=](const QJsonArray &rows, const QString &error) {
                    if (!error.isEmpty()) return onDone({}, error);
                    catalog->products = rows;
                    onDone(*catalog, {});
                });
            });
        });
    });
}

// id -> "Parent / Child" (or just the name for a top-level category)
QHash<QString, QString> categoryPaths(const QJsonArray &categories) {
    QHash<QString, QString> names;
    for (const QJsonValue &value : categories) names[value["id"].toString()] = value["name"].toString();
    QHash<QString, QString> paths;
    for (const QJsonValue &value : categories) {
        const QString parent = value["parent_id"].toString();
        paths[value["id"].toString()] =
            parent.isEmpty() ? value["name"].toString() : names.value(parent) + " / " + value["name"].toString();
    }
    return paths;
}

} // namespace

QString ProductImportDialog::columnName(int column) {
    switch (column) {
    case ColSku: return tr("Barcode / SKU");
    case ColName: return tr("Name");
    case ColVariant: return tr("Variant");
    case ColVariantSku: return tr("Variant barcode");
    case ColCategory: return tr("Category");
    case ColSupplier: return tr("Supplier");
    case ColCostPrice: return tr("Cost price");
    case ColSellPrice: return tr("Sell price");
    case ColLowStock: return tr("Low stock alert");
    case ColDescriptionKa: return tr("Description (Georgian)");
    case ColDescriptionEn: return tr("Description (English)");
    case ColDescriptionRu: return tr("Description (Russian)");
    }
    return {};
}

QString ProductImportDialog::stockColumnName(const QString &shopName) {
    return tr("Stock: %1").arg(shopName);
}

void exportProducts(SupabaseClient *client, QWidget *parent) {
    const QString path = QFileDialog::getSaveFileName(
        parent, ProductImportDialog::tr("Export products"),
        QDir::home().filePath(QString("shelby-products-%1.csv").arg(QDate::currentDate().toString(Qt::ISODate))),
        ProductImportDialog::tr("Spreadsheet (*.csv)"));
    if (path.isEmpty()) return;

    loadCatalog(client, [parent, path](const Catalog &catalog, const QString &error) {
        if (!error.isEmpty()) {
            QMessageBox::warning(parent, ProductImportDialog::tr("Export products"),
                                 ProductImportDialog::tr("Couldn't read the products: ") + error);
            return;
        }

        QStringList header;
        for (int column = 0; column < ColumnCount; ++column) header << ProductImportDialog::columnName(column);
        QStringList shopIds;
        for (const QJsonValue &shop : catalog.shops) {
            header << ProductImportDialog::stockColumnName(shop["name"].toString());
            shopIds << shop["id"].toString();
        }

        const QHash<QString, QString> categories = categoryPaths(catalog.categories);
        QHash<QString, QString> suppliers;
        for (const QJsonValue &supplier : catalog.suppliers)
            suppliers[supplier["id"].toString()] = supplier["name"].toString();

        QVector<QStringList> rows{header};
        for (const QJsonValue &value : catalog.products) {
            const QJsonObject product = value.toObject();
            const QJsonObject descriptions = product["descriptions"].toObject();
            QStringList base(ColumnCount);
            base[ColSku] = product["sku"].toString();
            base[ColName] = product["name"].toString();
            base[ColCategory] = categories.value(product["category_id"].toString());
            base[ColSupplier] = suppliers.value(product["supplier_id"].toString());
            base[ColCostPrice] = moneyText(product["cost_price"].toDouble());
            base[ColSellPrice] = moneyText(product["sell_price"].toDouble());
            base[ColLowStock] = QString::number(product["low_stock_threshold"].toInt());
            for (int i = 0; i < 3; ++i)
                base[ColDescriptionKa + i] = descriptions[descriptionLanguages[i]].toObject()["text"].toString();

            // shop id -> quantity, for the product itself ("") or one variant
            QHash<QString, QHash<QString, int>> stock;
            for (const QJsonValue &level : product["inventory_levels"].toArray())
                stock[level["variant_id"].toString()][level["shop_id"].toString()] = level["quantity"].toInt();
            auto withStock = [&](QStringList row, const QString &variantId) {
                for (const QString &shopId : shopIds) row << QString::number(stock[variantId].value(shopId, 0));
                return row;
            };

            QVector<QJsonObject> variants;
            for (const QJsonValue &variant : product["product_variants"].toArray()) variants.append(variant.toObject());
            if (variants.isEmpty()) {
                rows.append(withStock(base, {}));
                continue;
            }
            std::sort(variants.begin(), variants.end(), [](const QJsonObject &a, const QJsonObject &b) {
                return QString::localeAwareCompare(a["name"].toString(), b["name"].toString()) < 0;
            });
            for (const QJsonObject &variant : variants) {
                QStringList row = base;
                row[ColVariant] = variant["name"].toString();
                row[ColVariantSku] = variant["sku"].toString();
                rows.append(withStock(row, variant["id"].toString()));
            }
        }

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(csv::write(rows)) < 0) {
            QMessageBox::warning(parent, ProductImportDialog::tr("Export products"),
                                 ProductImportDialog::tr("Couldn't save %1: %2").arg(path, file.errorString()));
            return;
        }
        QMessageBox::information(parent, ProductImportDialog::tr("Export products"),
                                 ProductImportDialog::tr("Saved %n products to %1.", nullptr, int(catalog.products.size()))
                                     .arg(QDir::toNativeSeparators(path)));
    });
}

ProductImportDialog::ProductImportDialog(SupabaseClient *client, QVector<QStringList> rows, QWidget *parent)
    : QDialog(parent), m_client(client), m_rows(std::move(rows)) {
    setWindowTitle(tr("Import products"));
    resize(720, 520);

    m_summary = new QLabel(tr("Checking the file…"), this);
    m_summary->setWordWrap(true);
    m_details = new QPlainTextEdit(this);
    m_details->setReadOnly(true);
    m_importButton = new QPushButton(tr("Import"), this);
    m_importButton->setObjectName("primaryButton");
    m_importButton->setEnabled(false);
    m_closeButton = new QPushButton(tr("Cancel"), this);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(m_importButton);
    buttons->addWidget(m_closeButton);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_summary);
    layout->addWidget(m_details, 1);
    layout->addLayout(buttons);

    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_importButton, &QPushButton::clicked, this, [this]() {
        m_importButton->setEnabled(false);
        m_closeButton->setEnabled(false);
        runBatch(0);
    });
    connect(m_client, &SupabaseClient::descriptionTranslated, this, [this](const QString &productId) {
        if (m_toTranslate.isEmpty() || productId != m_toTranslate.first()) return;
        ++m_translated;
        m_toTranslate.removeFirst();
        translateNext();
    });
    connect(m_client, &SupabaseClient::descriptionTranslateFailed, this, [this](const QString &productId, const QString &) {
        if (m_toTranslate.isEmpty() || productId != m_toTranslate.first()) return;
        ++m_translateFailed;
        m_toTranslate.removeFirst();
        translateNext();
    });

    loadCatalog(m_client, [this](const Catalog &catalog, const QString &error) {
        if (!error.isEmpty()) {
            m_summary->setText(tr("Couldn't read the current products: ") + error);
            return;
        }
        check(catalog.shops, catalog.categories, catalog.suppliers, catalog.products);
    });
}

void ProductImportDialog::check(const QJsonArray &shops, const QJsonArray &categories, const QJsonArray &suppliers,
                                const QJsonArray &products) {
    QStringList problems;
    QStringList notes;

    if (m_rows.size() < 2) {
        m_summary->setText(tr("The file has no products in it."));
        return;
    }

    // Header -> column.
    QHash<QString, int> known;
    for (int column = 0; column < ColumnCount; ++column) {
        known[normalized(englishColumnNames[column])] = column;
        known[normalized(columnName(column))] = column;
    }
    QHash<QString, QString> shopByHeader; // normalized header -> shop id
    for (const QJsonValue &shop : shops) {
        const QString name = shop["name"].toString();
        shopByHeader[normalized(englishStockPrefix + name)] = shop["id"].toString();
        shopByHeader[normalized(stockColumnName(name))] = shop["id"].toString();
    }
    QVector<int> columnOf(ColumnCount, -1);
    QMap<int, QString> stockColumns; // file column -> shop id
    const QStringList header = m_rows.first();
    for (int i = 0; i < header.size(); ++i) {
        const QString key = normalized(header[i]);
        if (key.isEmpty()) continue;
        if (known.contains(key)) columnOf[known[key]] = i;
        else if (shopByHeader.contains(key)) stockColumns[i] = shopByHeader[key];
        else notes << tr("Column \"%1\" isn't one the import knows, so it's ignored.").arg(header[i]);
    }
    if (columnOf[ColSku] < 0 && columnOf[ColName] < 0) {
        m_summary->setText(tr("The file needs a \"%1\" or \"%2\" column. Export the products first to get "
                              "a file with the right columns.")
                               .arg(columnName(ColSku), columnName(ColName)));
        return;
    }

    // What exists now.
    QHash<QString, QJsonObject> productBySku;
    QHash<QString, QStringList> skusByName; // lower-case name -> skus
    for (const QJsonValue &value : products) {
        const QJsonObject product = value.toObject();
        productBySku[product["sku"].toString()] = product;
        skusByName[product["name"].toString().trimmed().toLower()] << product["sku"].toString();
    }
    QSet<QString> categoryNames, supplierNames;
    for (const QJsonValue &c : categories) categoryNames << c["name"].toString().toLower();
    for (const QJsonValue &s : suppliers) supplierNames << s["name"].toString().toLower();

    struct Group {
        int line = 0;
        QString sku;
        bool newProduct = false;
        bool nativeBarcode = true;
        QJsonObject item;
        QStringList variantOrder;
        QHash<QString, QJsonObject> variants; // lower-case name -> {name, sku, stock}
        bool ownStock = false;
    };
    QVector<Group> groups;
    QHash<QString, int> groupByKey;
    QSet<QString> newCategories, newSuppliers;
    QHash<QString, int> variantSkuLine;

    for (int r = 1; r < m_rows.size(); ++r) {
        const QStringList &row = m_rows[r];
        const int line = r + 1; // as the spreadsheet numbers it
        auto cell = [&](int column) {
            const int index = columnOf[column];
            return index >= 0 && index < row.size() ? row[index].trimmed() : QString();
        };
        if (row.join(QString()).trimmed().isEmpty()) continue;

        QString sku = cell(ColSku);
        const QString name = cell(ColName);
        bool nativeBarcode = true;
        if (sku.isEmpty()) {
            if (name.isEmpty()) {
                problems << tr("Line %1: needs a barcode or a name.").arg(line);
                continue;
            }
            const QStringList matches = skusByName.value(name.toLower());
            if (matches.size() == 1) {
                sku = matches.first(); // re-importing a file without barcodes updates, not duplicates
            } else if (matches.size() > 1) {
                problems << tr("Line %1: there are several products called \"%2\"; add the barcode to say which.")
                                .arg(line)
                                .arg(name);
                continue;
            } else {
                nativeBarcode = false; // an internal code is made below
            }
        }
        const QString key = sku.isEmpty() ? "new:" + name.toLower() : sku;

        if (!groupByKey.contains(key)) {
            Group group;
            group.line = line;
            group.sku = sku;
            group.nativeBarcode = nativeBarcode;
            group.newProduct = sku.isEmpty() || !productBySku.contains(sku);
            groupByKey[key] = groups.size();
            groups.append(group);
        }
        Group &group = groups[groupByKey[key]];
        QJsonObject &item = group.item;

        // Product columns: the first non-empty value counts; a different one later is a mistake.
        auto takeText = [&](int column, const char *field) {
            const QString value = cell(column);
            if (value.isEmpty()) return;
            if (item.contains(field) && item[field].toString() != value) {
                problems << tr("Line %1: %2 differs from line %3 for the same product.")
                                .arg(line)
                                .arg(columnName(column))
                                .arg(group.line);
                return;
            }
            item[field] = value;
        };
        auto takeNumber = [&](int column, const char *field, bool whole) {
            bool ok = true;
            const std::optional<double> value = parseNumber(cell(column), ok);
            if (!ok || (whole && value && *value != qRound(*value))) {
                problems << tr("Line %1: %2 \"%3\" isn't a number.").arg(line).arg(columnName(column), cell(column));
                return;
            }
            if (!value) return;
            if (item.contains(field) && item[field].toDouble() != *value) {
                problems << tr("Line %1: %2 differs from line %3 for the same product.")
                                .arg(line)
                                .arg(columnName(column))
                                .arg(group.line);
                return;
            }
            item[field] = whole ? QJsonValue(qRound(*value)) : QJsonValue(*value);
        };
        takeText(ColName, "name");
        takeText(ColCategory, "category");
        takeText(ColSupplier, "supplier");
        takeNumber(ColCostPrice, "cost_price", false);
        takeNumber(ColSellPrice, "sell_price", false);
        takeNumber(ColLowStock, "low_stock_threshold", true);
        QJsonObject descriptions = item["descriptions"].toObject();
        for (int i = 0; i < 3; ++i) {
            const QString text = cell(ColDescriptionKa + i);
            if (!text.isEmpty() && !descriptions.contains(descriptionLanguages[i]))
                descriptions[descriptionLanguages[i]] = text;
        }
        if (!descriptions.isEmpty()) item["descriptions"] = descriptions;

        // Stock: whole numbers, empty = leave as it is.
        QJsonObject stock;
        for (auto it = stockColumns.constBegin(); it != stockColumns.constEnd(); ++it) {
            const QString text = it.key() < row.size() ? row[it.key()].trimmed() : QString();
            if (text.isEmpty()) continue;
            bool ok = false;
            const int quantity = text.toInt(&ok);
            if (!ok || quantity < 0) {
                problems << tr("Line %1: stock \"%2\" in %3 isn't a whole number.").arg(line).arg(text, header[it.key()]);
                continue;
            }
            stock[it.value()] = quantity;
        }

        const QString variantName = cell(ColVariant);
        if (variantName.isEmpty()) {
            if (!stock.isEmpty()) {
                group.ownStock = true;
                item["stock"] = stock;
            }
        } else {
            const QString variantKey = variantName.toLower();
            if (group.variants.contains(variantKey)) {
                problems << tr("Line %1: variant \"%2\" appears twice for the same product.").arg(line).arg(variantName);
                continue;
            }
            QJsonObject variant{{"name", variantName}};
            const QString variantSku = cell(ColVariantSku);
            if (!variantSku.isEmpty()) {
                if (variantSkuLine.contains(variantSku)) {
                    problems << tr("Line %1: variant barcode %2 is also on line %3.")
                                    .arg(line)
                                    .arg(variantSku)
                                    .arg(variantSkuLine[variantSku]);
                }
                variantSkuLine[variantSku] = line;
                variant["sku"] = variantSku;
            }
            if (!stock.isEmpty()) variant["stock"] = stock;
            group.variantOrder << variantKey;
            group.variants[variantKey] = variant;
        }
    }

    // Per product checks, then the items import_products receives.
    int created = 0, updated = 0, variantsCreated = 0, withStock = 0;
    m_items = {};
    for (Group &group : groups) {
        QJsonObject &item = group.item;
        const QJsonObject existing = productBySku.value(group.sku);
        const bool hasVariants = !group.variants.isEmpty() || !existing["product_variants"].toArray().isEmpty();
        if (group.ownStock && hasVariants) {
            problems << tr("Line %1: this product has variants, so its stock goes on the variant rows.").arg(group.line);
        }
        if (group.newProduct) {
            if (!item.contains("name"))
                problems << tr("Line %1: a new product needs a name.").arg(group.line);
            if (!item.contains("sell_price"))
                problems << tr("Line %1: a new product needs a sell price.").arg(group.line);
        }

        const QString category = item["category"].toString();
        for (const QString &part : category.split('/', Qt::SkipEmptyParts)) {
            if (!categoryNames.contains(part.trimmed().toLower())) newCategories << part.trimmed();
        }
        const QString supplier = item["supplier"].toString();
        if (!supplier.isEmpty() && !supplierNames.contains(supplier.toLower())) newSuppliers << supplier;

        QSet<QString> existingVariants;
        for (const QJsonValue &variant : existing["product_variants"].toArray())
            existingVariants << variant["name"].toString().toLower();
        QJsonArray variants;
        for (const QString &key : group.variantOrder) {
            variants.append(group.variants[key]);
            if (!existingVariants.contains(key)) ++variantsCreated;
        }
        if (!variants.isEmpty()) item["variants"] = variants;
        bool setsStock = item.contains("stock");
        for (const QJsonValue &variant : variants) setsStock = setsStock || variant.toObject().contains("stock");
        if (setsStock && !group.newProduct) ++withStock;

        item["sku"] = group.sku.isEmpty() ? generateInternalSku() : group.sku;
        item["native_barcode"] = group.nativeBarcode;
        m_items.append(item);
        group.newProduct ? ++created : ++updated;
    }

    QStringList lines;
    if (!newCategories.isEmpty())
        lines << tr("New categories that will be created: %1").arg(QStringList(newCategories.values()).join(", "));
    if (!newSuppliers.isEmpty())
        lines << tr("New suppliers that will be created: %1").arg(QStringList(newSuppliers.values()).join(", "));
    if (withStock > 0)
        lines << tr("Stock of %n existing products will be set to the numbers in the file. Sales made since "
                    "the file was exported would be overwritten; empty the stock cells to leave stock alone.",
                    nullptr, withStock);
    lines << notes;

    if (!problems.isEmpty()) {
        m_summary->setText(tr("%n problems in the file. Fix them in the spreadsheet and import it again; "
                              "nothing has been saved.",
                              nullptr, int(problems.size())));
        m_summary->setStyleSheet(theme::css(theme::palette().danger));
        m_details->setPlainText((problems + QStringList{QString()} + lines).join('\n'));
        return;
    }
    m_summary->setText(tr("Ready: %1 new, %2 updated, %3 new variants. Empty cells leave things as they are, "
                          "and nothing is deleted.")
                           .arg(created)
                           .arg(updated)
                           .arg(variantsCreated));
    m_details->setPlainText(lines.join('\n'));
    m_importButton->setEnabled(!m_items.isEmpty());
}

void ProductImportDialog::runBatch(int start) {
    if (start >= m_items.size()) {
        emit imported();
        // Saving is done; translating can be left to "Translate missing languages" on the website.
        m_closeButton->setText(tr("Close"));
        m_closeButton->setEnabled(true);
        translateNext();
        return;
    }
    m_summary->setText(tr("Saving… %1 of %2 products").arg(start).arg(m_items.size()));

    QJsonArray batch;
    for (int i = start; i < qMin(start + batchSize, int(m_items.size())); ++i) batch.append(m_items[i]);
    m_client->callRpc("import_products", {{"items", batch}}, [this, start](const QJsonValue &result, const QString &error) {
        if (!error.isEmpty()) {
            if (start > 0) emit imported();
            finish(tr("Saving stopped: %1\n\nThe first %2 products were saved. Fix the file and import it "
                      "again: products that are already saved just get updated.")
                       .arg(error)
                       .arg(start));
            return;
        }
        const QJsonObject summary = result.toObject();
        m_created += summary["created"].toInt();
        m_updated += summary["updated"].toInt();
        m_variantsCreated += summary["variants_created"].toInt();
        for (const QJsonValue &id : summary["translate"].toArray()) m_toTranslate << id.toString();
        runBatch(start + batchSize);
    });
}

// Descriptions that changed get their other languages, one product at a time
// (the free translator also limits characters per minute).
void ProductImportDialog::translateNext() {
    const QString saved = tr("Saved: %1 new, %2 updated, %3 new variants.")
                              .arg(m_created)
                              .arg(m_updated)
                              .arg(m_variantsCreated);
    if (m_toTranslate.isEmpty()) {
        QString message = saved;
        if (m_translated + m_translateFailed > 0)
            message += "\n" + tr("Descriptions translated: %1. Failed: %2 (use \"Translate missing languages\" on "
                                 "the website later).")
                                  .arg(m_translated)
                                  .arg(m_translateFailed);
        finish(message);
        return;
    }
    m_summary->setText(saved + "\n" +
                       tr("Translating descriptions… %1 left. You can close this: the rest can be translated "
                          "later with \"Translate missing languages\" on the website.")
                           .arg(m_toTranslate.size()));
    m_client->translateDescription(m_toTranslate.first());
}

void ProductImportDialog::finish(const QString &message) {
    m_summary->setStyleSheet({});
    m_summary->setText(message);
    m_closeButton->setText(tr("Close"));
    m_closeButton->setEnabled(true);
}

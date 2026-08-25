#include "PosWindow.h"
#include "DiscountDialog.h"
#include "HeldSalesDialog.h"
#include "ReturnDialog.h"

#include <QAction>
#include <QButtonGroup>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

namespace {
constexpr int kTileColumns = 4;

// Removes and deletes every item (and its widget, if any) from a layout —
// Qt has no one-line "clear" for this.
void clearLayout(QLayout *layout) {
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}
} // namespace

PosWindow::PosWindow(SupabaseClient *client, QString shopId, QWidget *parent)
    : QMainWindow(parent)
    , m_client(client)
    , m_shopId(std::move(shopId)) {
    setWindowTitle("Shop Console — Checkout");
    resize(1100, 700);

    auto *toolbar = addToolBar("main");
    toolbar->setMovable(false);
    auto *title = new QLabel("Checkout", this);
    title->setObjectName("pageTitle");
    toolbar->addWidget(title);
    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
    m_pendingBadge = new QLabel(this);
    m_pendingBadge->setStyleSheet("color: #f59e0b; font-weight: 600; padding: 0 8px;");
    m_pendingBadge->hide();
    toolbar->addWidget(m_pendingBadge);
    m_updateBanner = new QLabel(this);
    m_updateBanner->setOpenExternalLinks(true);
    m_updateBanner->setStyleSheet("padding: 0 8px;");
    m_updateBanner->hide();
    toolbar->addWidget(m_updateBanner);
    m_heldButton = new QPushButton(this);
    m_heldButton->hide();
    toolbar->addWidget(m_heldButton);
    connect(m_heldButton, &QPushButton::clicked, this, &PosWindow::openHeldSalesDialog);
    auto *returnsButton = new QPushButton("Returns", this);
    toolbar->addWidget(returnsButton);
    connect(returnsButton, &QPushButton::clicked, this, [this]() {
        ReturnDialog dialog(m_client, m_shopId, this);
        dialog.exec();
    });
    auto *signOutButton = new QPushButton("Sign out", this);
    toolbar->addWidget(signOutButton);
    connect(signOutButton, &QPushButton::clicked, this, &PosWindow::signedOut);

    auto *central = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(central);

    // --- Left: category chips + product tile grid, like the POS the shops already use ---
    auto *browseLayout = new QVBoxLayout;

    m_categoryButtonGroup = new QButtonGroup(this);
    m_categoryButtonGroup->setExclusive(true);
    auto *chipsWidget = new QWidget(this);
    m_categoryChipsLayout = new QHBoxLayout(chipsWidget);
    m_categoryChipsLayout->setContentsMargins(0, 0, 0, 0);
    auto *chipsScroll = new QScrollArea(this);
    chipsScroll->setWidget(chipsWidget);
    chipsScroll->setWidgetResizable(true);
    chipsScroll->setFixedHeight(48);
    chipsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    browseLayout->addWidget(chipsScroll);

    auto *gridContainer = new QWidget(this);
    m_productGridLayout = new QGridLayout(gridContainer);
    m_productGridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    auto *gridScroll = new QScrollArea(this);
    gridScroll->setWidget(gridContainer);
    gridScroll->setWidgetResizable(true);
    browseLayout->addWidget(gridScroll, 1);

    rootLayout->addLayout(browseLayout, 2);

    // --- Right: barcode fallback + cart + total ---
    auto *cartLayout = new QVBoxLayout;

    auto *barcodeRow = new QHBoxLayout;
    barcodeRow->addWidget(new QLabel("Scan barcode:", this));
    m_barcodeInput = new QLineEdit(this);
    m_barcodeInput->setPlaceholderText("Barcode / SKU");
    barcodeRow->addWidget(m_barcodeInput, 1);
    cartLayout->addLayout(barcodeRow);

    m_cartTable = new QTableWidget(this);
    m_cartTable->setColumnCount(5);
    m_cartTable->setHorizontalHeaderLabels({"Product", "Qty", "Unit price", "Line total", ""});
    m_cartTable->horizontalHeader()->setStretchLastSection(true);
    m_cartTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cartTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cartTable->setAlternatingRowColors(true);
    cartLayout->addWidget(m_cartTable, 1);

    auto *cartButtonsRow = new QHBoxLayout;
    auto *removeButton = new QPushButton("Remove selected", this);
    cartButtonsRow->addWidget(removeButton);
    m_holdButton = new QPushButton("Hold", this);
    m_holdButton->setEnabled(false);
    cartButtonsRow->addWidget(m_holdButton);
    cartButtonsRow->addStretch();
    cartLayout->addLayout(cartButtonsRow);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #ef4444;");
    m_statusLabel->setWordWrap(true);
    cartLayout->addWidget(m_statusLabel);

    m_totalLabel = new QLabel("Total: GEL 0.00", this);
    m_totalLabel->setStyleSheet("font-size: 18px; font-weight: 600;");
    cartLayout->addWidget(m_totalLabel);

    m_completeSaleButton = new QPushButton("Complete sale", this);
    m_completeSaleButton->setObjectName("primaryButton");
    m_completeSaleButton->setMinimumHeight(40);
    cartLayout->addWidget(m_completeSaleButton);

    rootLayout->addLayout(cartLayout, 1);

    setCentralWidget(central);

    connect(m_barcodeInput, &QLineEdit::returnPressed, this, &PosWindow::handleBarcodeEntered);
    connect(removeButton, &QPushButton::clicked, this, &PosWindow::removeSelectedLine);
    connect(m_holdButton, &QPushButton::clicked, this, &PosWindow::holdCart);
    connect(m_completeSaleButton, &QPushButton::clicked, this, &PosWindow::completeSale);

    connect(m_client, &SupabaseClient::productLookedUp, this, &PosWindow::addToCart);
    connect(m_client, &SupabaseClient::productLookupFailed, this, [this](const QString &message) {
        m_statusLabel->setText(message);
        m_barcodeInput->clear();
    });

    connect(m_client, &SupabaseClient::categoriesFetched, this, [this](const QVector<Category> &categories) {
        m_categories = categories;
        rebuildCategoryChips();
    });
    m_client->fetchCategories();

    connect(m_client, &SupabaseClient::productsFetched, this, [this](const QVector<Product> &products) {
        m_allProducts = products;
        rebuildProductGrid();
    });
    connect(m_client, &SupabaseClient::productsFetchFailed, this, [this](const QString &message) {
        m_statusLabel->setText(message);
    });
    m_client->fetchProducts();

    connect(m_client, &SupabaseClient::saleRecorded, this, [this]() {
        m_cart.clear();
        refreshCartTable();
        m_completeSaleButton->setEnabled(true);
        m_statusLabel->setStyleSheet("color: #22c55e;");
        m_statusLabel->setText("Sale recorded.");
        m_barcodeInput->setFocus();
    });
    connect(m_client, &SupabaseClient::saleRecordFailed, this, [this](const QString &message) {
        m_completeSaleButton->setEnabled(true);
        m_statusLabel->setStyleSheet("color: #ef4444;");
        m_statusLabel->setText(message);
    });
    connect(m_client, &SupabaseClient::saleQueuedOffline, this, [this](int pendingCount) {
        // Treated like a success from the cashier's point of view — the
        // sale is safely saved, it just hasn't reached the server yet.
        m_cart.clear();
        refreshCartTable();
        m_completeSaleButton->setEnabled(true);
        m_statusLabel->setStyleSheet("color: #f59e0b;");
        m_statusLabel->setText(
            QString("No connection — sale saved offline, will sync automatically (%1 pending).")
                .arg(pendingCount));
        m_barcodeInput->setFocus();
        updatePendingBadge(pendingCount);
    });
    connect(m_client, &SupabaseClient::pendingSalesChanged, this, &PosWindow::updatePendingBadge);

    updatePendingBadge(m_client->pendingSalesCount());
    updateHeldButton();

    auto *syncTimer = new QTimer(this);
    connect(syncTimer, &QTimer::timeout, this, [this]() { m_client->flushPendingSales(); });
    syncTimer->start(20000);
    m_client->flushPendingSales();

    m_barcodeInput->setFocus();
}

void PosWindow::showUpdateBanner(const QString &version, const QString &releaseUrl) {
    m_updateBanner->setText(
        QString("<a href=\"%1\" style=\"color:#4f8cff;\">Update available — v%2</a>").arg(releaseUrl, version));
    m_updateBanner->show();
}

void PosWindow::updatePendingBadge(int count) {
    if (count <= 0) {
        m_pendingBadge->hide();
        return;
    }
    m_pendingBadge->setText(QString("⏳ %1 pending sync").arg(count));
    m_pendingBadge->show();
}

void PosWindow::rebuildCategoryChips() {
    clearLayout(m_categoryChipsLayout);

    auto addChip = [this](const QString &label, const QString &categoryId) {
        auto *chip = new QPushButton(label, this);
        chip->setObjectName("categoryChip");
        chip->setCheckable(true);
        chip->setChecked(categoryId == m_selectedCategoryId);
        m_categoryButtonGroup->addButton(chip);
        connect(chip, &QPushButton::clicked, this, [this, categoryId]() {
            m_selectedCategoryId = categoryId;
            rebuildProductGrid();
        });
        m_categoryChipsLayout->addWidget(chip);
    };

    addChip("All", "");
    for (const Category &category : m_categories) {
        addChip(category.name, category.id);
    }
    m_categoryChipsLayout->addStretch();
}

void PosWindow::rebuildProductGrid() {
    clearLayout(m_productGridLayout);

    int index = 0;
    for (const Product &product : m_allProducts) {
        if (!m_selectedCategoryId.isEmpty() && product.categoryId != m_selectedCategoryId) continue;

        auto *tile = new QPushButton(
            QString("%1\nGEL %2").arg(product.name).arg(product.sellPrice, 0, 'f', 2), this);
        tile->setObjectName("productTile");
        tile->setMinimumSize(140, 84);
        tile->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(tile, &QPushButton::clicked, this, [this, product, tile]() {
            if (product.variants.isEmpty()) {
                addToCart(product);
            } else {
                promptForVariant(product, tile);
            }
        });

        m_productGridLayout->addWidget(tile, index / kTileColumns, index % kTileColumns);
        ++index;
    }
}

void PosWindow::handleBarcodeEntered() {
    const QString sku = m_barcodeInput->text().trimmed();
    if (sku.isEmpty()) return;

    m_statusLabel->clear();
    m_client->lookupProductBySku(sku);
}

void PosWindow::promptForVariant(const Product &product, QWidget *anchor) {
    QMenu menu(this);
    for (const ProductVariant &variant : product.variants) {
        QAction *action = menu.addAction(variant.name);
        connect(action, &QAction::triggered, this, [this, product, variant]() {
            Product selected = product;
            selected.selectedVariantId = variant.id;
            selected.selectedVariantName = variant.name;
            addToCart(selected);
        });
    }
    menu.exec(anchor->mapToGlobal(QPoint(0, anchor->height())));
}

void PosWindow::addToCart(const Product &product) {
    // A product that HAS variants but wasn't scanned/picked via a specific
    // variant has no sellable "no variant" stock bucket — ProductDialog
    // deliberately never seeds one (see 0015_product_variants.sql notes).
    // Ask which variant instead of silently recording a sale against a
    // bucket that doesn't exist.
    if (product.selectedVariantId.isEmpty() && !product.variants.isEmpty()) {
        promptForVariant(product, m_barcodeInput);
        return;
    }

    m_barcodeInput->clear();
    m_barcodeInput->setFocus();

    const QString &variantId = product.selectedVariantId;
    const QString lineName = variantId.isEmpty()
        ? product.name
        : QString("%1 (%2)").arg(product.name, product.selectedVariantName);

    for (CartLine &line : m_cart) {
        if (line.productId == product.id && line.variantId == variantId) {
            line.quantity += 1;
            refreshCartTable();
            return;
        }
    }

    m_cart.append({product.id, variantId, lineName, 1, product.sellPrice, product.sellPrice, product.costPrice});
    refreshCartTable();
}

void PosWindow::removeSelectedLine() {
    const int row = m_cartTable->currentRow();
    if (row < 0 || row >= m_cart.size()) return;
    m_cart.remove(row);
    refreshCartTable();
}

void PosWindow::openDiscountDialog(int row) {
    if (row < 0 || row >= m_cart.size()) return;
    CartLine &line = m_cart[row];

    DiscountDialog dialog(line.name, line.listPrice, line.unitPrice, this);
    if (dialog.exec() == QDialog::Accepted) {
        line.unitPrice = dialog.resultUnitPrice();
        refreshCartTable();
    }
}

void PosWindow::refreshCartTable() {
    m_holdButton->setEnabled(!m_cart.isEmpty());

    m_cartTable->setRowCount(m_cart.size());
    double total = 0;
    for (int row = 0; row < m_cart.size(); ++row) {
        const CartLine &line = m_cart[row];
        const double lineTotal = line.quantity * line.unitPrice;
        total += lineTotal;

        m_cartTable->setItem(row, 0, new QTableWidgetItem(line.name));
        m_cartTable->setItem(row, 1, new QTableWidgetItem(QString::number(line.quantity)));
        m_cartTable->setItem(row, 2, new QTableWidgetItem(QString::number(line.unitPrice, 'f', 2)));
        m_cartTable->setItem(row, 3, new QTableWidgetItem(QString::number(lineTotal, 'f', 2)));

        const bool discounted = line.unitPrice < line.listPrice;
        auto *discountButton = new QPushButton(
            discounted ? QString("-%1%").arg(qRound((1.0 - line.unitPrice / line.listPrice) * 100.0))
                       : "Discount",
            this);
        if (discounted) discountButton->setStyleSheet("color: #22c55e;");
        connect(discountButton, &QPushButton::clicked, this, [this, row]() { openDiscountDialog(row); });
        m_cartTable->setCellWidget(row, 4, discountButton);
    }
    m_totalLabel->setText(QString("Total: GEL %1").arg(total, 0, 'f', 2));
}

void PosWindow::holdCart() {
    if (m_cart.isEmpty()) return;
    m_heldSales.append({QDateTime::currentDateTime(), m_cart});
    m_cart.clear();
    refreshCartTable();
    updateHeldButton();
    m_statusLabel->setStyleSheet("color: #9aa0ac;");
    m_statusLabel->setText("Sale held.");
}

void PosWindow::updateHeldButton() {
    if (m_heldSales.isEmpty()) {
        m_heldButton->hide();
        return;
    }
    m_heldButton->setText(QString("Held (%1)").arg(m_heldSales.size()));
    m_heldButton->show();
}

void PosWindow::openHeldSalesDialog() {
    // Passed by reference — both Resume and Discard mutate m_heldSales
    // directly inside the dialog, so there's nothing to reconcile here
    // afterward beyond refreshing the button/count and, on a resume,
    // loading the returned cart.
    HeldSalesDialog dialog(m_heldSales, !m_cart.isEmpty(), this);
    dialog.exec();
    updateHeldButton();
    if (dialog.didResume()) {
        m_cart = dialog.resumedCart();
        refreshCartTable();
    }
}

void PosWindow::completeSale() {
    m_statusLabel->clear();

    if (m_cart.isEmpty()) {
        m_statusLabel->setText("Cart is empty.");
        return;
    }
    if (m_shopId.isEmpty()) {
        m_statusLabel->setText("Your account isn't assigned to a shop yet — ask the owner to assign one.");
        return;
    }

    QVector<SaleItemInput> items;
    double total = 0;
    for (const CartLine &line : m_cart) {
        items.append({line.productId, line.variantId, line.quantity, line.unitPrice, line.unitCost, line.listPrice});
        total += line.quantity * line.unitPrice;
    }

    m_completeSaleButton->setEnabled(false);
    m_client->recordSale(m_shopId, items, total);
}

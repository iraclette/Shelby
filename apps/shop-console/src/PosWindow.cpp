#include "PosWindow.h"

#include <QButtonGroup>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
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
    title->setStyleSheet("font-weight: 600; padding: 0 8px;");
    toolbar->addWidget(title);
    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
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
    m_cartTable->setColumnCount(4);
    m_cartTable->setHorizontalHeaderLabels({"Product", "Qty", "Unit price", "Line total"});
    m_cartTable->horizontalHeader()->setStretchLastSection(true);
    m_cartTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cartTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    cartLayout->addWidget(m_cartTable, 1);

    auto *removeButton = new QPushButton("Remove selected", this);
    cartLayout->addWidget(removeButton, 0, Qt::AlignLeft);

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

    m_barcodeInput->setFocus();
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
        connect(tile, &QPushButton::clicked, this, [this, product]() { addToCart(product); });

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

void PosWindow::addToCart(const Product &product) {
    m_barcodeInput->clear();
    m_barcodeInput->setFocus();

    for (CartLine &line : m_cart) {
        if (line.productId == product.id) {
            line.quantity += 1;
            refreshCartTable();
            return;
        }
    }

    m_cart.append({product.id, product.name, 1, product.sellPrice, product.costPrice});
    refreshCartTable();
}

void PosWindow::removeSelectedLine() {
    const int row = m_cartTable->currentRow();
    if (row < 0 || row >= m_cart.size()) return;
    m_cart.remove(row);
    refreshCartTable();
}

void PosWindow::refreshCartTable() {
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
    }
    m_totalLabel->setText(QString("Total: GEL %1").arg(total, 0, 'f', 2));
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
        items.append({line.productId, line.quantity, line.unitPrice, line.unitCost});
        total += line.quantity * line.unitPrice;
    }

    m_completeSaleButton->setEnabled(false);
    m_client->recordSale(m_shopId, items, total);
}

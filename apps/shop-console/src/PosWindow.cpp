#include "PosWindow.h"
#include "SettingsPage.h"
#include "Theme.h"
#include "BusinessProfile.h"
#include "DiscountDialog.h"
#include "FlowLayout.h"
#include "HeldSalesDialog.h"
#include "PaymentDialog.h"
#include "ReturnDialog.h"

#include <QAction>
#include <QEvent>
#include <QButtonGroup>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

namespace {
constexpr int kMinTileWidth = 160;
constexpr int kTileSpacing = 8;

// One accent per category, cycled — the tile's top stripe, so a cashier can
// find a category's products by colour at a glance, like Aronium's coloured buttons.
const char *const kCategoryColors[] = {
    "#4f8cff", "#22c55e", "#f59e0b", "#ec4899", "#a855f7", "#14b8a6", "#ef4444", "#eab308",
};

// Removes and deletes every item (and its widget, if any) from a layout —
// Qt has no one-line "clear" for this.
void clearLayout(QLayout *layout) {
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}


// A label that lets clicks fall through to the tile button it sits on.
QLabel *tileLabel(const QString &text, const char *objectName, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setObjectName(objectName);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    return label;
}
} // namespace

PosWindow::PosWindow(SupabaseClient *client, QString shopId, QWidget *parent, bool embedded)
    : QMainWindow(parent)
    , m_client(client)
    , m_shopId(std::move(shopId)) {
    setWindowTitle(tr("Shop Console — Checkout"));
    resize(1280, 760);

    auto *toolbar = addToolBar("main");
    toolbar->setMovable(false);
    if (!embedded) {
        auto *title = new QLabel(tr("Checkout"), this);
        title->setObjectName("pageTitle");
        toolbar->addWidget(title);
    }
    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setStyleSheet("background: transparent;"); // the app-wide QWidget background would show as a box
    toolbar->addWidget(spacer);
    m_pendingBadge = new QLabel(this);
    m_pendingBadge->setStyleSheet(theme::css(theme::palette().warning) + " font-weight: 600; padding: 0 8px;");
    m_pendingBadge->hide();
    toolbar->addWidget(m_pendingBadge);
    m_updateBanner = new QLabel(this);
    m_updateBanner->setOpenExternalLinks(true);
    m_updateBanner->setStyleSheet("padding: 0 8px;");
    m_updateBanner->hide();
    toolbar->addWidget(m_updateBanner);
    auto *returnsButton = new QPushButton(tr("Returns  F7"), this);
    toolbar->addWidget(returnsButton);
    if (!embedded) {
        // Cashiers don't get the Settings page, but how their till looks is theirs to choose.
        auto *appearanceButton = new QPushButton(tr("Appearance"), this);
        toolbar->addWidget(appearanceButton);
        connect(appearanceButton, &QPushButton::clicked, this, [this]() {
            QDialog dialog(this);
            dialog.setWindowTitle(tr("Appearance"));
            dialog.resize(640, 440);
            auto *layout = new QVBoxLayout(&dialog);
            layout->addWidget(new AppearanceSettingsSection(m_client, &dialog));
            layout->addSpacing(16);
            layout->addWidget(new LanguageSettingsSection(m_client, &dialog));
            auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
            connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
            layout->addWidget(buttons);
            dialog.exec();
        });
        auto *signOutButton = new QPushButton(tr("Sign out"), this);
        toolbar->addWidget(signOutButton);
        connect(signOutButton, &QPushButton::clicked, this, &PosWindow::signedOut);
    }

    auto *central = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(12);

    // --- Left: the receipt — search, lines, line tools, totals, pay ---
    auto *receiptPanel = new QFrame(this);
    receiptPanel->setObjectName("posPanel");
    auto *receiptLayout = new QVBoxLayout(receiptPanel);
    receiptLayout->setContentsMargins(12, 12, 12, 12);
    receiptLayout->setSpacing(8);

    // One field for both: a scanner types a barcode and hits Enter, a cashier
    // types a name and the tiles on the right filter as they go.
    m_barcodeInput = new QLineEdit(this);
    m_barcodeInput->setObjectName("posSearch");
    m_barcodeInput->setPlaceholderText(tr("Scan barcode or search products…  (F3)"));
    m_barcodeInput->setClearButtonEnabled(true);
    receiptLayout->addWidget(m_barcodeInput);

    m_cartTable = new QTableWidget(this);
    m_cartTable->setObjectName("receiptTable");
    m_cartTable->setColumnCount(4);
    m_cartTable->setHorizontalHeaderLabels({tr("Product"), tr("Qty"), tr("Price"), tr("Total")});
    m_cartTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_cartTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_cartTable->horizontalHeader()->setHighlightSections(false);
    m_cartTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_cartTable->verticalHeader()->hide();
    m_cartTable->verticalHeader()->setDefaultSectionSize(38);
    m_cartTable->setShowGrid(false);
    m_cartTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cartTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cartTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_cartTable->setAlternatingRowColors(true);
    // Keyboard focus stays in the search field so a barcode scan always
    // lands there, even right after the cashier clicked a line.
    m_cartTable->setFocusPolicy(Qt::NoFocus);
    receiptLayout->addWidget(m_cartTable, 1);

    auto *lineToolsRow = new QHBoxLayout;
    lineToolsRow->setSpacing(6);
    m_minusButton = new QPushButton("−", this);
    m_plusButton = new QPushButton("+", this);
    for (QPushButton *button : {m_minusButton, m_plusButton}) {
        button->setObjectName("stepButton");
        button->setFixedWidth(44);
    }
    m_quantityButton = new QPushButton(tr("Qty  F4"), this);
    m_discountButton = new QPushButton(tr("Discount  F2"), this);
    m_removeButton = new QPushButton(tr("Delete  Del"), this);
    lineToolsRow->addWidget(m_minusButton);
    lineToolsRow->addWidget(m_plusButton);
    lineToolsRow->addWidget(m_quantityButton, 1);
    lineToolsRow->addWidget(m_discountButton, 1);
    lineToolsRow->addWidget(m_removeButton, 1);
    receiptLayout->addLayout(lineToolsRow);

    auto *totalsFrame = new QFrame(this);
    totalsFrame->setObjectName("totalsPanel");
    auto *totalsLayout = new QVBoxLayout(totalsFrame);
    totalsLayout->setContentsMargins(12, 10, 12, 10);
    totalsLayout->setSpacing(4);
    auto addTotalsRow = [&](const QString &key, QLabel *&valueLabel) {
        auto *row = new QWidget(this);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        auto *keyLabel = new QLabel(key, this);
        keyLabel->setObjectName("totalsKey");
        valueLabel = new QLabel(this);
        valueLabel->setObjectName("totalsValue");
        valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        rowLayout->addWidget(keyLabel);
        rowLayout->addWidget(valueLabel, 1);
        totalsLayout->addWidget(row);
        return row;
    };
    addTotalsRow(tr("Items"), m_itemsCountLabel);
    addTotalsRow(tr("Subtotal"), m_subtotalLabel);
    m_savingsRow = addTotalsRow(tr("Discount"), m_savingsLabel);
    m_savingsLabel->setStyleSheet(theme::css(theme::palette().success));

    auto *totalRow = new QHBoxLayout;
    auto *totalKey = new QLabel(tr("TOTAL"), this);
    totalKey->setObjectName("totalKey");
    m_totalLabel = new QLabel(this);
    m_totalLabel->setObjectName("totalValue");
    m_totalLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    totalRow->addWidget(totalKey, 0, Qt::AlignBottom);
    totalRow->addWidget(m_totalLabel, 1);
    totalsLayout->addLayout(totalRow);
    receiptLayout->addWidget(totalsFrame);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->hide();
    receiptLayout->addWidget(m_statusLabel);

    auto *saleButtonsRow = new QHBoxLayout;
    saleButtonsRow->setSpacing(6);
    m_holdButton = new QPushButton(tr("Hold  F9"), this);
    m_heldButton = new QPushButton(this);
    m_voidButton = new QPushButton(tr("Void  F8"), this);
    m_voidButton->setObjectName("dangerButton");
    saleButtonsRow->addWidget(m_holdButton, 1);
    saleButtonsRow->addWidget(m_heldButton, 1);
    saleButtonsRow->addWidget(m_voidButton, 1);
    receiptLayout->addLayout(saleButtonsRow);

    m_completeSaleButton = new QPushButton(tr("Pay  F12"), this);
    m_completeSaleButton->setObjectName("payButton");
    m_completeSaleButton->setMinimumHeight(56);
    receiptLayout->addWidget(m_completeSaleButton);

    rootLayout->addWidget(receiptPanel, 2);

    // --- Right: category chips + product tile grid ---
    auto *browseLayout = new QVBoxLayout;
    browseLayout->setSpacing(10);

    m_categoryButtonGroup = new QButtonGroup(this);
    m_categoryButtonGroup->setExclusive(true);
    // Chips wrap onto extra rows instead of a fixed-height strip, so every
    // category stays reachable however many there are.
    auto *chipsWidget = new QWidget(this);
    m_categoryChipsLayout = new FlowLayout(chipsWidget);
    m_categoryChipsLayout->setContentsMargins(0, 0, 0, 0);
    browseLayout->addWidget(chipsWidget);
    m_subcategoryChips = new QWidget(this);
    m_subcategoryChipsLayout = new FlowLayout(m_subcategoryChips);
    m_subcategoryChipsLayout->setContentsMargins(24, 0, 0, 0); // indented under the parent's chip
    browseLayout->addWidget(m_subcategoryChips);

    auto *gridContainer = new QWidget(this);
    m_productGridLayout = new QGridLayout(gridContainer);
    m_productGridLayout->setAlignment(Qt::AlignTop);
    m_productGridLayout->setContentsMargins(0, 0, 4, 0);
    m_productGridLayout->setSpacing(kTileSpacing);
    m_gridScroll = new QScrollArea(this);
    m_gridScroll->setWidget(gridContainer);
    m_gridScroll->setWidgetResizable(true);
    m_gridScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_gridScroll->viewport()->installEventFilter(this); // re-flow tiles on resize
    browseLayout->addWidget(m_gridScroll, 1);

    rootLayout->addLayout(browseLayout, 3);

    setCentralWidget(central);
    toolbar->setVisible(!embedded);

    connect(m_barcodeInput, &QLineEdit::returnPressed, this, &PosWindow::handleBarcodeEntered);
    connect(m_barcodeInput, &QLineEdit::textChanged, this, &PosWindow::rebuildProductGrid);
    connect(m_cartTable, &QTableWidget::itemSelectionChanged, this, &PosWindow::updateLineButtons);
    connect(m_cartTable, &QTableWidget::cellDoubleClicked, this, &PosWindow::promptForQuantity);
    connect(m_minusButton, &QPushButton::clicked, this, [this]() { changeSelectedQuantity(-1); });
    connect(m_plusButton, &QPushButton::clicked, this, [this]() { changeSelectedQuantity(+1); });
    connect(m_quantityButton, &QPushButton::clicked, this, &PosWindow::promptForQuantity);
    connect(m_discountButton, &QPushButton::clicked, this, [this]() { openDiscountDialog(selectedRow()); });
    connect(m_removeButton, &QPushButton::clicked, this, &PosWindow::removeSelectedLine);
    connect(m_holdButton, &QPushButton::clicked, this, &PosWindow::holdCart);
    connect(m_heldButton, &QPushButton::clicked, this, &PosWindow::openHeldSalesDialog);
    connect(m_voidButton, &QPushButton::clicked, this, &PosWindow::voidCart);
    connect(m_completeSaleButton, &QPushButton::clicked, this, &PosWindow::completeSale);
    auto openReturns = [this]() {
        ReturnDialog dialog(m_client, m_shopId, this);
        dialog.exec();
        m_barcodeInput->setFocus();
    };
    connect(returnsButton, &QPushButton::clicked, this, openReturns);

    // Function keys, Aronium-style. Buttons carry their key in the label.
    auto addShortcut = [this](const QKeySequence &key, auto handler) {
        auto *shortcut = new QShortcut(key, this);
        connect(shortcut, &QShortcut::activated, this, handler);
    };
    addShortcut(Qt::Key_F2, [this]() { m_discountButton->click(); });
    addShortcut(Qt::Key_F3, [this]() {
        m_barcodeInput->setFocus();
        m_barcodeInput->selectAll();
    });
    addShortcut(Qt::Key_F4, [this]() { m_quantityButton->click(); });
    addShortcut(Qt::Key_F7, openReturns);
    addShortcut(Qt::Key_F8, [this]() { m_voidButton->click(); });
    addShortcut(Qt::Key_F9, [this]() { m_holdButton->click(); });
    addShortcut(Qt::Key_F10, [this]() { m_heldButton->click(); });
    addShortcut(Qt::Key_F12, [this]() { m_completeSaleButton->click(); });
    addShortcut(Qt::Key_Delete, [this]() {
        // Focus lives in the search field, so Delete must still edit text there.
        if (!m_barcodeInput->text().isEmpty()) {
            m_barcodeInput->del();
        } else {
            m_removeButton->click();
        }
    });
    addShortcut(Qt::Key_Escape, [this]() { m_barcodeInput->clear(); });
    // Up/Down move through the receipt lines without leaving the search field.
    addShortcut(Qt::Key_Up, [this]() {
        if (m_cart.isEmpty()) return;
        m_cartTable->selectRow(qMax(0, selectedRow() < 0 ? m_cart.size() - 1 : selectedRow() - 1));
    });
    addShortcut(Qt::Key_Down, [this]() {
        if (m_cart.isEmpty()) return;
        m_cartTable->selectRow(qMin(int(m_cart.size()) - 1, selectedRow() + 1));
    });
    addShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus), [this]() { changeSelectedQuantity(+1); });
    addShortcut(QKeySequence(Qt::CTRL | Qt::Key_Equal), [this]() { changeSelectedQuantity(+1); });
    addShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus), [this]() { changeSelectedQuantity(-1); });

    connect(m_client, &SupabaseClient::productLookedUp, this, &PosWindow::addToCart);
    connect(m_client, &SupabaseClient::productLookupFailed, this, [this](const QString &message) {
        showStatus(message);
        m_barcodeInput->clear();
    });

    connect(m_client, &SupabaseClient::categoriesFetched, this, [this](const QVector<Category> &categories) {
        m_categories = categories;
        rebuildCategoryChips();
        rebuildProductGrid(); // tile colours follow category order
    });
    m_client->fetchCategories();

    connect(m_client, &SupabaseClient::productsFetched, this, [this](const QVector<Product> &products) {
        m_allProducts = products;
        rebuildProductGrid();
    });
    connect(m_client, &SupabaseClient::productsFetchFailed, this, [this](const QString &message) {
        showStatus(message);
    });
    m_client->fetchProducts();

    connect(m_client, &SupabaseClient::saleRecorded, this, [this]() {
        m_cart.clear();
        refreshCartTable();
        m_completeSaleButton->setEnabled(true);
        showStatus(tr("Sale recorded.") + m_lastPaymentNote, theme::hex(theme::palette().success));
        m_barcodeInput->setFocus();
    });
    connect(m_client, &SupabaseClient::saleRecordFailed, this, [this](const QString &message) {
        m_completeSaleButton->setEnabled(true);
        showStatus(message);
    });
    connect(m_client, &SupabaseClient::saleQueuedOffline, this, [this](int pendingCount) {
        // Treated like a success from the cashier's point of view — the
        // sale is safely saved, it just hasn't reached the server yet.
        m_cart.clear();
        refreshCartTable();
        m_completeSaleButton->setEnabled(true);
        showStatus(tr("No connection — sale saved offline, will sync automatically (%1 pending).%2")
                       .arg(pendingCount)
                       .arg(m_lastPaymentNote),
                   theme::hex(theme::palette().warning));
        m_barcodeInput->setFocus();
        updatePendingBadge(pendingCount);
    });
    connect(m_client, &SupabaseClient::pendingSalesChanged, this, &PosWindow::updatePendingBadge);

    updatePendingBadge(m_client->pendingSalesCount());
    updateHeldButton();
    refreshCartTable();

    auto *syncTimer = new QTimer(this);
    connect(syncTimer, &QTimer::timeout, this, [this]() { m_client->flushPendingSales(); });
    syncTimer->start(20000);
    m_client->flushPendingSales();

    m_barcodeInput->setFocus();
}

void PosWindow::showUpdateBanner(const QString &version, const QString &releaseUrl) {
    m_updateBanner->setText(
        QString("<a href=\"%1\" style=\"color:%2;\">%3</a>")
                .arg(releaseUrl, theme::hex(theme::palette().accent), tr("Update available — v%1").arg(version)));
    m_updateBanner->show();
}

void PosWindow::showStatus(const QString &message, const QString &color) {
    m_statusLabel->setStyleSheet(QString("color: %1;").arg(color));
    m_statusLabel->setText(message);
    m_statusLabel->setVisible(!message.isEmpty());
}

void PosWindow::updatePendingBadge(int count) {
    if (count <= 0) {
        m_pendingBadge->hide();
        return;
    }
    m_pendingBadge->setText(tr("⏳ %1 pending sync").arg(count));
    m_pendingBadge->show();
}

void PosWindow::rebuildCategoryChips() {
    clearLayout(m_categoryChipsLayout);
    clearLayout(m_subcategoryChipsLayout);

    // The parent's chip stays lit while one of its subcategories is picked.
    const QString selectedTop = topLevelCategoryId(m_selectedCategoryId);
    auto addChip = [this](FlowLayout *layout, const QString &label, const QString &categoryId, bool checked) {
        auto *chip = new QPushButton(label, this);
        chip->setObjectName("categoryChip");
        chip->setCheckable(true);
        chip->setChecked(checked);
        // The checked style is bold, which is wider than the regular text the
        // size hint was measured with — reserve room for it so it isn't clipped.
        chip->ensurePolished(); // pick up the stylesheet's font size before measuring
        QFont boldFont = chip->font();
        boldFont.setWeight(QFont::DemiBold);
        const int boldTextWidth = QFontMetrics(boldFont).horizontalAdvance(label);
        chip->setMinimumWidth(qMax(chip->sizeHint().width(), boldTextWidth + 2 * 14 + 2 + 4));
        // Each row is its own exclusive group: a lit parent and a lit subcategory together.
        if (layout == m_categoryChipsLayout) m_categoryButtonGroup->addButton(chip);
        connect(chip, &QPushButton::clicked, this, [this, categoryId]() {
            m_selectedCategoryId = categoryId;
            // Deferred: this chip is deleted by the rebuild.
            QMetaObject::invokeMethod(this, &PosWindow::rebuildCategoryChips, Qt::QueuedConnection);
            rebuildProductGrid();
            m_barcodeInput->setFocus();
        });
        layout->addWidget(chip);
    };

    addChip(m_categoryChipsLayout, tr("All"), "", m_selectedCategoryId.isEmpty());
    bool hasSubcategories = false;
    for (const Category &category : m_categories) {
        if (!category.parentId.isEmpty()) continue;
        addChip(m_categoryChipsLayout, category.name, category.id, category.id == selectedTop);
    }
    for (const Category &category : m_categories) {
        if (selectedTop.isEmpty() || category.parentId != selectedTop) continue;
        if (!hasSubcategories) {
            addChip(m_subcategoryChipsLayout, tr("All"), selectedTop, m_selectedCategoryId == selectedTop);
            hasSubcategories = true;
        }
        addChip(m_subcategoryChipsLayout, category.name, category.id, category.id == m_selectedCategoryId);
    }
    m_subcategoryChips->setVisible(hasSubcategories);
}

QString PosWindow::topLevelCategoryId(const QString &categoryId) const {
    for (const Category &category : m_categories) {
        if (category.id == categoryId && !category.parentId.isEmpty()) return category.parentId;
    }
    return categoryId;
}

// A parent category's chip also shows the products in its subcategories.
bool PosWindow::inSelectedCategory(const Product &product) const {
    if (m_selectedCategoryId.isEmpty() || product.categoryId == m_selectedCategoryId) return true;
    return topLevelCategoryId(product.categoryId) == m_selectedCategoryId;
}

QString PosWindow::categoryColor(const QString &categoryId) const {
    // Subcategories share their parent's colour; only top-level categories count.
    constexpr int colorCount = sizeof(kCategoryColors) / sizeof(kCategoryColors[0]);
    const QString topId = topLevelCategoryId(categoryId);
    int index = 0;
    for (const Category &category : m_categories) {
        if (!category.parentId.isEmpty()) continue;
        if (category.id == topId) return kCategoryColors[index % colorCount];
        ++index;
    }
    return theme::hex(theme::palette().border); // uncategorised: same as a plain border
}

bool PosWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_gridScroll->viewport() && event->type() == QEvent::Resize) {
        const int width = m_gridScroll->viewport()->width() - 4; // the grid's right margin
        const int columns = qMax(1, (width + kTileSpacing) / (kMinTileWidth + kTileSpacing));
        if (columns != m_tileColumns) {
            m_tileColumns = columns;
            rebuildProductGrid();
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void PosWindow::rebuildProductGrid() {
    clearLayout(m_productGridLayout);
    // Equal columns even when a search leaves fewer tiles than a full row.
    for (int column = 0; column < m_productGridLayout->columnCount(); ++column) {
        m_productGridLayout->setColumnStretch(column, 0);
    }
    for (int column = 0; column < m_tileColumns; ++column) m_productGridLayout->setColumnStretch(column, 1);
    m_visibleProducts.clear();

    // A search looks through every category, not just the selected chip.
    const QString search = m_barcodeInput->text().trimmed();
    auto matchesSearch = [&search](const Product &product) {
        if (search.isEmpty()) return true;
        if (product.name.contains(search, Qt::CaseInsensitive)) return true;
        if (product.sku.contains(search, Qt::CaseInsensitive)) return true;
        for (const ProductVariant &variant : product.variants) {
            if (variant.name.contains(search, Qt::CaseInsensitive)) return true;
            if (variant.sku.contains(search, Qt::CaseInsensitive)) return true;
        }
        return false;
    };

    int index = 0;
    for (const Product &product : m_allProducts) {
        if (search.isEmpty() && !inSelectedCategory(product)) continue;
        if (!matchesSearch(product)) continue;
        if (!productHasStockAtShop(product)) continue;
        m_visibleProducts.append(product);

        auto *tile = new QPushButton(this);
        tile->setObjectName("productTile");
        tile->setMinimumSize(kMinTileWidth, 112);
        tile->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        tile->setStyleSheet(
            QString("QPushButton#productTile { border-top: 4px solid %1; }").arg(categoryColor(product.categoryId)));
        tile->setToolTip(product.sku.isEmpty() ? product.name : tr("%1\nSKU %2").arg(product.name, product.sku));

        auto *tileLayout = new QVBoxLayout(tile);
        tileLayout->setContentsMargins(10, 8, 10, 8);
        tileLayout->setSpacing(2);
        auto *nameLabel = tileLabel(product.name, "tileName", tile);
        nameLabel->setWordWrap(true);
        nameLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        tileLayout->addWidget(nameLabel, 1);

        // Price and stock on lines of their own: side by side they collided in Georgian and Russian.
        auto *priceRow = new QHBoxLayout;
        priceRow->addWidget(tileLabel(money(product.sellPrice), "tilePrice", tile));
        priceRow->addStretch();
        if (!product.variants.isEmpty())
            priceRow->addWidget(tileLabel(tr("%n variants", nullptr, int(product.variants.size())), "tileStock", tile));
        tileLayout->addLayout(priceRow);
        const int stock = stockAtShop(product);
        auto *stockLabel = tileLabel(tr("%n left", nullptr, stock), "tileStock", tile);
        if (product.lowStockThreshold > 0 && stock <= product.lowStockThreshold) {
            stockLabel->setStyleSheet(theme::css(theme::palette().warning));
        }
        tileLayout->addWidget(stockLabel);

        connect(tile, &QPushButton::clicked, this, [this, product, tile]() {
            if (product.variants.isEmpty()) {
                addToCart(product);
            } else {
                promptForVariant(product, tile);
            }
        });

        m_productGridLayout->addWidget(tile, index / m_tileColumns, index % m_tileColumns);
        ++index;
    }

    if (index == 0 && !m_allProducts.isEmpty()) {
        auto *empty = new QLabel(search.isEmpty() ? tr("Nothing in stock in this category.")
                                                  : tr("No products match \"%1\".").arg(search),
                                 this);
        empty->setObjectName("totalsKey");
        empty->setAlignment(Qt::AlignCenter);
        m_productGridLayout->addWidget(empty, 0, 0, 1, m_tileColumns);
    }
}

void PosWindow::handleBarcodeEntered() {
    const QString text = m_barcodeInput->text().trimmed();
    if (text.isEmpty()) return;
    showStatus({});

    // Exact barcode match in what's already loaded: no round trip, and it
    // still works while offline.
    for (const Product &product : m_allProducts) {
        if (!product.sku.isEmpty() && product.sku.compare(text, Qt::CaseInsensitive) == 0) {
            // A product with variants is sold as one of them, same as tapping its tile.
            if (product.variants.isEmpty()) {
                addToCart(product);
            } else {
                promptForVariant(product, m_barcodeInput);
            }
            return;
        }
        for (const ProductVariant &variant : product.variants) {
            if (!variant.sku.isEmpty() && variant.sku.compare(text, Qt::CaseInsensitive) == 0) {
                Product selected = product;
                selected.selectedVariantId = variant.id;
                selected.selectedVariantName = variant.name;
                addToCart(selected);
                return;
            }
        }
    }

    // A name search narrowed down to one tile: Enter adds it.
    if (m_visibleProducts.size() == 1) {
        const Product product = m_visibleProducts.first();
        if (product.variants.isEmpty()) {
            addToCart(product);
        } else {
            promptForVariant(product, m_barcodeInput);
        }
        return;
    }

    m_client->lookupProductBySku(text);
}

void PosWindow::promptForVariant(const Product &product, QWidget *anchor) {
    QMenu menu(this);
    for (const ProductVariant &variant : product.variants) {
        const int stock = variant.stockByShop.value(m_shopId, 0);
        if (stock <= 0) continue; // out of stock at this shop
        QAction *action = menu.addAction(tr("%1   (%2 left)").arg(variant.name).arg(stock));
        connect(action, &QAction::triggered, this, [this, product, variant]() {
            Product selected = product;
            selected.selectedVariantId = variant.id;
            selected.selectedVariantName = variant.name;
            addToCart(selected);
        });
    }
    if (menu.isEmpty()) {
        showStatus(tr("%1 has no variants in stock at this shop.").arg(product.name));
        return;
    }
    menu.exec(anchor->mapToGlobal(QPoint(0, anchor->height())));
    m_barcodeInput->setFocus();
}

bool PosWindow::productHasStockAtShop(const Product &product) const {
    if (product.variants.isEmpty()) return product.stockByShop.value(m_shopId, 0) > 0;
    for (const ProductVariant &variant : product.variants) {
        if (variant.stockByShop.value(m_shopId, 0) > 0) return true;
    }
    return false;
}

int PosWindow::stockAtShop(const Product &product) const {
    if (product.variants.isEmpty()) return product.stockByShop.value(m_shopId, 0);
    int total = 0;
    for (const ProductVariant &variant : product.variants) {
        total += qMax(0, variant.stockByShop.value(m_shopId, 0));
    }
    return total;
}

int PosWindow::availableStock(const QString &productId, const QString &variantId) const {
    for (const Product &product : m_allProducts) {
        if (product.id != productId) continue;
        if (variantId.isEmpty()) return product.stockByShop.value(m_shopId, 0);
        for (const ProductVariant &variant : product.variants) {
            if (variant.id == variantId) return variant.stockByShop.value(m_shopId, 0);
        }
    }
    return 0;
}

void PosWindow::addToCart(const Product &product) {
    // Variant products have no "no variant" stock, so make the cashier pick one.
    if (product.selectedVariantId.isEmpty() && !product.variants.isEmpty()) {
        promptForVariant(product, m_barcodeInput);
        return;
    }

    // Checked here too so a barcode scan can't add out-of-stock items. The
    // product passed in is fresher than m_allProducts when it came from a server lookup.
    int stock = 0;
    if (product.selectedVariantId.isEmpty()) {
        stock = product.stockByShop.value(m_shopId, 0);
    } else {
        for (const ProductVariant &variant : product.variants) {
            if (variant.id == product.selectedVariantId) {
                stock = variant.stockByShop.value(m_shopId, 0);
                break;
            }
        }
    }

    const QString &variantId = product.selectedVariantId;
    int alreadyInCart = 0;
    for (const CartLine &line : m_cart) {
        if (line.productId == product.id && line.variantId == variantId) {
            alreadyInCart = line.quantity;
            break;
        }
    }

    m_barcodeInput->clear();
    m_barcodeInput->setFocus();

    if (stock <= alreadyInCart) {
        showStatus(tr("%1 is out of stock at this shop.").arg(product.name));
        return;
    }
    showStatus({});

    const QString lineName = variantId.isEmpty()
        ? product.name
        : QString("%1 (%2)").arg(product.name, product.selectedVariantName);

    for (int row = 0; row < m_cart.size(); ++row) {
        CartLine &line = m_cart[row];
        if (line.productId == product.id && line.variantId == variantId) {
            line.quantity += 1;
            refreshCartTable(row);
            return;
        }
    }

    m_cart.append({product.id, variantId, lineName, 1, product.sellPrice, product.sellPrice, product.costPrice});
    refreshCartTable(m_cart.size() - 1);
}

int PosWindow::selectedRow() const {
    const auto rows = m_cartTable->selectionModel()->selectedRows();
    if (rows.isEmpty()) return -1;
    const int row = rows.first().row();
    return row < m_cart.size() ? row : -1;
}

void PosWindow::removeSelectedLine() {
    const int row = selectedRow();
    if (row < 0) return;
    m_cart.remove(row);
    refreshCartTable(qMin(row, int(m_cart.size()) - 1));
}

void PosWindow::changeSelectedQuantity(int delta) {
    const int row = selectedRow();
    if (row < 0) return;
    CartLine &line = m_cart[row];
    const int newQuantity = line.quantity + delta;
    if (newQuantity <= 0) {
        removeSelectedLine();
        return;
    }
    if (delta > 0 && newQuantity > availableStock(line.productId, line.variantId)) {
        showStatus(tr("Only %1 of %2 in stock at this shop.")
                       .arg(availableStock(line.productId, line.variantId))
                       .arg(line.name));
        return;
    }
    line.quantity = newQuantity;
    refreshCartTable(row);
}

void PosWindow::promptForQuantity() {
    const int row = selectedRow();
    if (row < 0) return;
    CartLine &line = m_cart[row];
    // Never cap below what's already in the cart, in case stock was edited meanwhile.
    const int maxQuantity = qMax(line.quantity, availableStock(line.productId, line.variantId));
    bool ok = false;
    const int quantity = QInputDialog::getInt(
        this, "Quantity", tr("%1\n(%2 in stock)").arg(line.name).arg(maxQuantity), line.quantity, 1,
        maxQuantity, 1, &ok);
    if (ok) {
        line.quantity = quantity;
        refreshCartTable(row);
    }
    m_barcodeInput->setFocus();
}

void PosWindow::openDiscountDialog(int row) {
    if (row < 0 || row >= m_cart.size()) return;
    CartLine &line = m_cart[row];

    DiscountDialog dialog(line.name, line.listPrice, line.unitPrice, this);
    if (dialog.exec() == QDialog::Accepted) {
        line.unitPrice = dialog.resultUnitPrice();
        refreshCartTable(row);
    }
    m_barcodeInput->setFocus();
}

void PosWindow::voidCart() {
    if (m_cart.isEmpty()) return;
    const auto answer = QMessageBox::question(this, tr("Void sale"), tr("Remove every item from this sale?"));
    if (answer == QMessageBox::Yes) {
        m_cart.clear();
        refreshCartTable();
        showStatus(tr("Sale voided."), theme::hex(theme::palette().textMuted));
    }
    m_barcodeInput->setFocus();
}

void PosWindow::refreshCartTable(int selectRow) {
    if (selectRow < 0) selectRow = selectedRow();

    m_cartTable->setRowCount(m_cart.size());
    double total = 0;
    double savings = 0;
    int itemCount = 0;
    for (int row = 0; row < m_cart.size(); ++row) {
        const CartLine &line = m_cart[row];
        const double lineTotal = line.quantity * line.unitPrice;
        total += lineTotal;
        savings += line.quantity * (line.listPrice - line.unitPrice);
        itemCount += line.quantity;

        auto *nameItem = new QTableWidgetItem(line.name);
        auto *quantityItem = new QTableWidgetItem(QString::number(line.quantity));
        quantityItem->setTextAlignment(Qt::AlignCenter);
        const bool discounted = line.unitPrice < line.listPrice;
        auto *priceItem = new QTableWidgetItem(
            discounted ? QString("%1  -%2%")
                             .arg(line.unitPrice, 0, 'f', 2)
                             .arg(qRound((1.0 - line.unitPrice / line.listPrice) * 100.0))
                       : QString::number(line.unitPrice, 'f', 2));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (discounted) priceItem->setForeground(theme::palette().success);
        auto *totalItem = new QTableWidgetItem(QString::number(lineTotal, 'f', 2));
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QFont bold = totalItem->font();
        bold.setWeight(QFont::DemiBold);
        totalItem->setFont(bold);

        m_cartTable->setItem(row, 0, nameItem);
        m_cartTable->setItem(row, 1, quantityItem);
        m_cartTable->setItem(row, 2, priceItem);
        m_cartTable->setItem(row, 3, totalItem);
    }

    if (selectRow >= 0 && selectRow < m_cart.size()) {
        m_cartTable->selectRow(selectRow);
        m_cartTable->scrollToItem(m_cartTable->item(selectRow, 0));
    } else {
        m_cartTable->clearSelection();
    }

    m_itemsCountLabel->setText(QString::number(itemCount));
    m_subtotalLabel->setText(money(total + savings));
    m_savingsLabel->setText("−" + money(savings));
    m_savingsRow->setVisible(savings > 0.004);
    m_totalLabel->setText(money(total));

    const bool hasItems = !m_cart.isEmpty();
    m_holdButton->setEnabled(hasItems);
    m_voidButton->setEnabled(hasItems);
    m_completeSaleButton->setText(hasItems ? tr("Pay  %1   ·   F12").arg(money(total)) : tr("Pay  F12"));
    updateLineButtons();
}

void PosWindow::updateLineButtons() {
    const bool lineSelected = selectedRow() >= 0;
    for (QPushButton *button : {m_minusButton, m_plusButton, m_quantityButton, m_discountButton, m_removeButton}) {
        button->setEnabled(lineSelected);
    }
}

void PosWindow::holdCart() {
    if (m_cart.isEmpty()) return;
    m_heldSales.append({QDateTime::currentDateTime(), m_cart});
    m_cart.clear();
    refreshCartTable();
    updateHeldButton();
    showStatus(tr("Sale held."), theme::hex(theme::palette().textMuted));
    m_barcodeInput->setFocus();
}

void PosWindow::updateHeldButton() {
    m_heldButton->setText(tr("Held (%1)  F10").arg(m_heldSales.size()));
    m_heldButton->setEnabled(!m_heldSales.isEmpty());
}

void PosWindow::openHeldSalesDialog() {
    if (m_heldSales.isEmpty()) return;
    HeldSalesDialog dialog(m_heldSales, m_cart.isEmpty(), this);
    dialog.exec();
    updateHeldButton();
    if (dialog.didResume()) {
        m_cart = dialog.resumedCart();
        refreshCartTable();
    }
    m_barcodeInput->setFocus();
}

void PosWindow::completeSale() {
    showStatus({});

    if (m_cart.isEmpty()) {
        showStatus(tr("Cart is empty."));
        return;
    }
    if (m_shopId.isEmpty()) {
        showStatus(tr("Your account isn't assigned to a shop yet — ask the owner to assign one."));
        return;
    }

    QVector<SaleItemInput> items;
    double total = 0;
    for (const CartLine &line : m_cart) {
        items.append({line.productId, line.variantId, line.quantity, line.unitPrice, line.unitCost, line.listPrice});
        total += line.quantity * line.unitPrice;
    }

    PaymentDialog payment(total, this);
    const bool paid = payment.exec() == QDialog::Accepted;
    m_barcodeInput->setFocus();
    if (!paid) return;
    m_lastPaymentNote = payment.paidByCard()   ? " " + tr("Paid by card.")
                        : payment.change() > 0 ? " " + tr("Change: %1").arg(money(payment.change()))
                                               : QString();

    if (m_practiceMode) {
        m_cart.clear();
        refreshCartTable();
        showStatus(tr("Practice sale — nothing recorded.") + m_lastPaymentNote, theme::hex(theme::palette().info));
        return;
    }

    m_completeSaleButton->setEnabled(false);
    m_client->recordSale(m_shopId, items, total);
}

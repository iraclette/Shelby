#include "AdminWindow.h"
#include "AuditLogPage.h"
#include "CategoriesPage.h"
#include "CheckoutPage.h"
#include "CustomOrdersPage.h"
#include "DailySummaryPage.h"
#include "EmployeesPage.h"
#include "InventoryPage.h"
#include "MessagesPage.h"
#include "ApplicationsPage.h"
#include "StockRequestsPage.h"
#include "OnlineOrdersPage.h"
#include "OverviewPage.h"
#include "SalaryPage.h"
#include "SoftwarePage.h"
#include "SupabaseClient.h"
#include "UpdateChecker.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QPushButton>
#include <QSize>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include <type_traits>

namespace {
constexpr int kPageRole = Qt::UserRole;        // index into the page stack; absent on group headers
constexpr int kHeaderRole = Qt::UserRole + 1;  // true on the small "SALES" / "CATALOG" captions
constexpr int kBadgeRole = Qt::UserRole + 2;   // count shown as a pill on the right, 0 = none

// Pages build their tables independently; give them one consistent look here:
// no row-number gutter and no grid, like the checkout receipt.
void tidyTables(QWidget *page) {
    for (QTableView *table : page->findChildren<QTableView *>()) {
        table->verticalHeader()->hide();
        table->verticalHeader()->setDefaultSectionSize(34);
        table->setShowGrid(false);
        table->setAlternatingRowColors(true);
        table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        table->horizontalHeader()->setHighlightSections(false);
    }
}

// Group captions and count badges — neither has a stylesheet equivalent.
class SidebarDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        if (index.data(kHeaderRole).toBool()) {
            painter->save();
            QFont font = option.font;
            font.setPixelSize(10);
            font.setWeight(QFont::Bold);
            font.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
            painter->setFont(font);
            painter->setPen(QColor("#6b7280"));
            painter->drawText(option.rect.adjusted(24, 0, -8, -6), Qt::AlignLeft | Qt::AlignBottom,
                              index.data(Qt::DisplayRole).toString().toUpper());
            painter->restore();
            return;
        }

        QStyledItemDelegate::paint(painter, option, index);

        const int badge = index.data(kBadgeRole).toInt();
        if (badge <= 0) return;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        QFont font = option.font;
        font.setPixelSize(11);
        font.setWeight(QFont::Bold);
        painter->setFont(font);
        const QString text = badge > 99 ? "99+" : QString::number(badge);
        const int width = qMax(20, painter->fontMetrics().horizontalAdvance(text) + 12);
        const QRect pill(option.rect.right() - 16 - width, option.rect.center().y() - 9, width, 18);
        const bool selected = option.state & QStyle::State_Selected;
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(selected ? "#ffffff" : "#4f8cff"));
        painter->drawRoundedRect(pill, 9, 9);
        painter->setPen(QColor(selected ? "#4f8cff" : "#ffffff"));
        painter->drawText(pill, Qt::AlignCenter, text);
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        if (index.data(kHeaderRole).toBool()) return {option.rect.width(), 26};
        return QStyledItemDelegate::sizeHint(option, index);
    }
};

QString initials(const QString &name) {
    QString result;
    for (const QString &part : name.split(' ', Qt::SkipEmptyParts)) {
        result += part.front().toUpper();
        if (result.size() == 2) break;
    }
    return result.isEmpty() ? "?" : result;
}
} // namespace

AdminWindow::AdminWindow(SupabaseClient *client, UpdateChecker *updateChecker, const Profile &profile,
                         QWidget *parent)
    : QMainWindow(parent), m_updateChecker(updateChecker) {
    setWindowTitle("Shop Console — Admin");
    showMaximized();

    auto *toolbar = addToolBar("main");
    toolbar->setMovable(false);
    // Names the section that's open, so it's clear without reading the sidebar.
    auto *title = new QLabel(this);
    title->setObjectName("pageTitle");
    toolbar->addWidget(title);
    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setStyleSheet("background: transparent;"); // the app-wide QWidget background would show as a box
    toolbar->addWidget(spacer);
    m_updateButton = new QPushButton("Update now", this);
    m_updateButton->setObjectName("primaryButton");
    m_updateButton->hide();
    toolbar->addWidget(m_updateButton);
    connect(m_updateButton, &QPushButton::clicked, this, [this]() {
        m_updateButton->setEnabled(false);
        m_updateButton->setText("Updating…");
        m_updateChecker->downloadAndInstall(m_pendingAssetUrl);
    });
    connect(m_updateChecker, &UpdateChecker::installFailed, this, [this](const QString &) {
        m_updateButton->setEnabled(true);
        m_updateButton->setText("Update now");
    });
    connect(m_updateChecker, &UpdateChecker::downloadProgress, this, [this](int percent) {
        m_updateButton->setText(QString("Updating… %1%").arg(percent));
    });

    // --- Sidebar: brand, grouped sections, signed-in user ---
    auto *sidebarPanel = new QWidget(this);
    sidebarPanel->setObjectName("sidebarPanel");
    sidebarPanel->setFixedWidth(220);
    auto *sidebarLayout = new QVBoxLayout(sidebarPanel);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    auto *brand = new QWidget(sidebarPanel);
    brand->setObjectName("sidebarBrand");
    auto *brandLayout = new QHBoxLayout(brand);
    brandLayout->setContentsMargins(18, 16, 18, 14);
    brandLayout->setSpacing(10);
    auto *brandMark = new QLabel("S", brand);
    brandMark->setObjectName("brandMark");
    brandMark->setFixedSize(34, 34);
    brandMark->setAlignment(Qt::AlignCenter);
    auto *brandText = new QVBoxLayout;
    brandText->setSpacing(0);
    auto *brandName = new QLabel("Shelby", brand);
    brandName->setObjectName("brandName");
    auto *brandSub = new QLabel("Shop Console", brand);
    brandSub->setObjectName("brandSub");
    brandText->addWidget(brandName);
    brandText->addWidget(brandSub);
    brandLayout->addWidget(brandMark);
    brandLayout->addLayout(brandText, 1);
    sidebarLayout->addWidget(brand);

    auto *sidebar = new QListWidget(sidebarPanel);
    sidebar->setObjectName("adminSidebar");
    sidebar->setIconSize(QSize(18, 18));
    sidebar->setFocusPolicy(Qt::NoFocus);
    sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebar->setItemDelegate(new SidebarDelegate(sidebar));
    sidebarLayout->addWidget(sidebar, 1);

    auto *footer = new QWidget(sidebarPanel);
    footer->setObjectName("sidebarFooter");
    auto *footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(14, 12, 14, 12);
    footerLayout->setSpacing(10);
    auto *userRow = new QHBoxLayout;
    userRow->setSpacing(10);
    const QString displayName = profile.fullName.isEmpty() ? profile.email : profile.fullName;
    auto *avatar = new QLabel(initials(displayName), footer);
    avatar->setObjectName("avatar");
    avatar->setFixedSize(32, 32);
    avatar->setAlignment(Qt::AlignCenter);
    auto *userText = new QVBoxLayout;
    userText->setSpacing(0);
    auto *userName = new QLabel(displayName, footer);
    userName->setObjectName("userName");
    QString roleText = profile.role.isEmpty() ? QString() : profile.role.front().toUpper() + profile.role.mid(1);
    if (profile.isDev) roleText += " · Dev";
    auto *userRole = new QLabel(roleText, footer);
    userRole->setObjectName("brandSub");
    userText->addWidget(userName);
    userText->addWidget(userRole);
    userRow->addWidget(avatar);
    userRow->addLayout(userText, 1);
    footerLayout->addLayout(userRow);
    auto *signOutButton = new QPushButton("Sign out", footer);
    footerLayout->addWidget(signOutButton);
    sidebarLayout->addWidget(footer);
    connect(signOutButton, &QPushButton::clicked, this, &AdminWindow::signedOut);

    auto *pages = new QStackedWidget(this);

    auto addGroup = [&](const QString &caption) {
        auto *item = new QListWidgetItem(caption);
        item->setFlags(Qt::NoItemFlags);
        item->setData(kHeaderRole, true);
        sidebar->addItem(item);
    };
    auto addSection = [&](const QString &icon, const QString &label, QWidget *page) {
        auto *item = new QListWidgetItem(QIcon(":/icons/" + icon + ".svg"), label);
        item->setData(kPageRole, pages->count());
        sidebar->addItem(item);
        tidyTables(page);
        pages->addWidget(page);
        return item;
    };
    auto forwardStatus = [this](auto *page) {
        connect(page, &std::remove_pointer_t<decltype(page)>::statusMessage, this,
                [this](const QString &message) { statusBar()->showMessage(message, 8000); });
        return page;
    };

    const QString firstName = profile.fullName.section(' ', 0, 0);
    auto *overviewPage = forwardStatus(new OverviewPage(client, firstName, this));
    QListWidgetItem *overviewItem = addSection("overview", "Overview", overviewPage);
    addGroup("Sales");
    auto *onlineOrdersPage = forwardStatus(new OnlineOrdersPage(client, this));
    QListWidgetItem *onlineOrdersItem = addSection("online-orders", "Online Orders", onlineOrdersPage);
    addSection("daily-summary", "Daily Summary", forwardStatus(new DailySummaryPage(client, this)));
    if (profile.isDev) addSection("checkout", "Checkout", new CheckoutPage(client, this));
    addGroup("Catalog");
    addSection("inventory", "Inventory", forwardStatus(new InventoryPage(client, this)));
    addSection("categories", "Categories", forwardStatus(new CategoriesPage(client, this)));
    addGroup("People");
    addSection("employees", "Employees", forwardStatus(new EmployeesPage(client, this)));
    addSection("salary", "Salary", forwardStatus(new SalaryPage(client, this)));
    auto *applicationsPage = forwardStatus(new ApplicationsPage(client, this));
    QListWidgetItem *applicationsItem = addSection("applications", "Applications", applicationsPage);
    addGroup("Storefront");
    QListWidgetItem *ordersItem =
        addSection("orders", "Custom Orders", forwardStatus(new CustomOrdersPage(client, this)));
    // Storefront "Contact us" submissions.
    QListWidgetItem *messagesItem = addSection("help", "Messages", forwardStatus(new MessagesPage(client, this)));
    auto *stockRequestsPage = forwardStatus(new StockRequestsPage(client, this));
    QListWidgetItem *stockRequestsItem = addSection("stock-requests", "Stock Requests", stockRequestsPage);
    addGroup("System");
    addSection("audit-log", "Audit Log", forwardStatus(new AuditLogPage(client, this)));
    m_softwarePage = new SoftwarePage(updateChecker, this);
    addSection("software", "Software", m_softwarePage);

    connect(sidebar, &QListWidget::currentItemChanged, this, [pages, title](QListWidgetItem *item) {
        if (!item || item->data(kHeaderRole).toBool()) return;
        pages->setCurrentIndex(item->data(kPageRole).toInt());
        title->setText(item->text());
    });
    sidebar->setCurrentItem(overviewItem);

    connect(overviewPage, &OverviewPage::openSection, this, [sidebar](const QString &section) {
        const auto matches = sidebar->findItems(section, Qt::MatchExactly);
        if (!matches.isEmpty()) sidebar->setCurrentItem(matches.first());
    });
    connect(overviewPage, &OverviewPage::unreadMessagesChanged, this, [messagesItem](int count) {
        messagesItem->setData(kBadgeRole, count);
    });
    connect(overviewPage, &OverviewPage::newOrdersChanged, this, [ordersItem](int count) {
        ordersItem->setData(kBadgeRole, count);
    });
    connect(onlineOrdersPage, &OnlineOrdersPage::toPrepareChanged, this, [onlineOrdersItem](int count) {
        onlineOrdersItem->setData(kBadgeRole, count);
    });
    connect(applicationsPage, &ApplicationsPage::newCountChanged, this, [applicationsItem](int count) {
        applicationsItem->setData(kBadgeRole, count);
    });
    connect(stockRequestsPage, &StockRequestsPage::newCountChanged, this, [stockRequestsItem](int count) {
        stockRequestsItem->setData(kBadgeRole, count);
    });

    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(sidebarPanel);
    layout->addWidget(pages, 1);
    setCentralWidget(central);
}

void AdminWindow::reportUpdateCheckResult(bool ok, bool isNewer, const QString &version, const QString &releaseUrl,
                                           const QString &assetUrl, const QString &errorMessage) {
    m_pendingAssetUrl = assetUrl;
    if (ok && isNewer && !assetUrl.isEmpty()) {
        m_updateButton->setText(QString("Update now — v%1").arg(version));
        m_updateButton->setEnabled(true);
        m_updateButton->show();
    } else {
        m_updateButton->hide();
    }
    m_softwarePage->reportUpdateCheckResult(ok, isNewer, version, releaseUrl, assetUrl, errorMessage);
}

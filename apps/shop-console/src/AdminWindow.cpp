#include "AdminWindow.h"
#include "Theme.h"
#include "BusinessProfile.h"
#include "AuditLogPage.h"
#include "CategoriesPage.h"
#include "CheckoutPage.h"
#include "CustomOrdersPage.h"
#include "DailySummaryPage.h"
#include "DesktopNotifier.h"
#include "EmployeeDialog.h"
#include "EmployeesPage.h"
#include "InventoryPage.h"
#include "MessagesPage.h"
#include "ApplicationsPage.h"
#include "StockRequestsPage.h"
#include "OnlineOrdersPage.h"
#include "OverviewPage.h"
#include "SalaryPage.h"
#include "SettingsPage.h"
#include "SoftwarePage.h"
#include "SupabaseClient.h"
#include "UpdateChecker.h"

#include <QApplication>
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
#include <QHash>
#include <algorithm>
#include <QStyle>
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
            painter->setPen(theme::palette().textDisabled);
            painter->drawText(option.rect.adjusted(24, 0, -8, -6), Qt::AlignLeft | Qt::AlignBottom,
                              index.data(Qt::DisplayRole).toString().toUpper());
            painter->restore();
            return;
        }

        const int badge = index.data(kBadgeRole).toInt();
        QFont badgeFont = option.font;
        badgeFont.setPixelSize(11);
        badgeFont.setWeight(QFont::Bold);
        const QString text = badge > 99 ? "99+" : QString::number(badge);
        const int width = qMax(20, QFontMetrics(badgeFont).horizontalAdvance(text) + 12);
        const QRect pill(option.rect.right() - 16 - width, option.rect.center().y() - 9, width, 18);

        // A label too long for the row (Georgian, Russian) ends in "…" before the badge
        // instead of running under it; the tooltip has the whole name.
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        const QWidget *view = opt.widget;
        QStyle *style = view ? view->style() : QApplication::style();
        const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, view);
        const int textRight = badge > 0 ? pill.left() - 6 : textRect.right();
        opt.text = opt.fontMetrics.elidedText(opt.text, Qt::ElideRight, qMax(0, textRight - textRect.left()));
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, view);

        if (badge <= 0) return;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setFont(badgeFont);
        const bool selected = option.state & QStyle::State_Selected;
        painter->setPen(Qt::NoPen);
        painter->setBrush((selected ? theme::palette().onAccent : theme::palette().accent));
        painter->drawRoundedRect(pill, 9, 9);
        painter->setPen((selected ? theme::palette().accent : theme::palette().onAccent));
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
    setWindowTitle(tr("Shop Console — Admin"));
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
    m_updateButton = new QPushButton(tr("Update now"), this);
    m_updateButton->setObjectName("primaryButton");
    m_updateButton->hide();
    toolbar->addWidget(m_updateButton);
    connect(m_updateButton, &QPushButton::clicked, this, [this]() {
        m_updateButton->setEnabled(false);
        m_updateButton->setText(tr("Updating…"));
        m_updateChecker->downloadAndInstall(m_pendingAssetUrl);
    });
    connect(m_updateChecker, &UpdateChecker::installFailed, this, [this](const QString &) {
        m_updateButton->setEnabled(true);
        m_updateButton->setText(tr("Update now"));
    });
    connect(m_updateChecker, &UpdateChecker::downloadProgress, this, [this](int percent) {
        m_updateButton->setText(tr("Updating… %1%").arg(percent));
    });

    // --- Sidebar: brand, grouped sections, signed-in user ---
    auto *sidebarPanel = new QWidget(this);
    sidebarPanel->setObjectName("sidebarPanel");
    sidebarPanel->setFixedWidth(232);
    auto *sidebarLayout = new QVBoxLayout(sidebarPanel);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    auto *brand = new QWidget(sidebarPanel);
    brand->setObjectName("sidebarBrand");
    auto *brandLayout = new QHBoxLayout(brand);
    brandLayout->setContentsMargins(18, 16, 18, 14);
    brandLayout->setSpacing(10);
    auto *brandMark = new QLabel(brand);
    brandMark->setObjectName("brandMark");
    brandMark->setFixedSize(34, 34);
    brandMark->setAlignment(Qt::AlignCenter);
    auto *brandText = new QVBoxLayout;
    brandText->setSpacing(0);
    auto *brandName = new QLabel(brand);
    brandName->setObjectName("brandName");
    auto *brandSub = new QLabel(tr("Shop Console"), brand);
    brandSub->setObjectName("brandSub");
    brandText->addWidget(brandName);
    brandText->addWidget(brandSub);
    brandLayout->addWidget(brandMark);
    brandLayout->addLayout(brandText, 1);
    sidebarLayout->addWidget(brand);

    // The business's name and logo (Settings → Business); the first letter when there's no logo.
    auto refreshBrand = [brandMark, brandName]() {
        const BusinessProfile *business = BusinessProfile::instance();
        brandName->setText(business->settings().name);
        const QPixmap logo = business->roundLogo(brandMark->width(), brandMark->devicePixelRatioF());
        if (logo.isNull()) {
            brandMark->setObjectName("brandMark");
            brandMark->setPixmap({});
            brandMark->setText(business->settings().name.left(1).toUpper());
        } else {
            brandMark->setObjectName("brandLogo");
            brandMark->setPixmap(logo);
        }
        brandMark->style()->unpolish(brandMark);
        brandMark->style()->polish(brandMark);
    };
    refreshBrand();
    connect(BusinessProfile::instance(), &BusinessProfile::changed, brand, refreshBrand);

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
    QString roleText = roleLabel(profile.role);
    if (profile.isDev) roleText += " · Dev";
    auto *userRole = new QLabel(roleText, footer);
    userRole->setObjectName("brandSub");
    userText->addWidget(userName);
    userText->addWidget(userRole);
    userRow->addWidget(avatar);
    userRow->addLayout(userText, 1);
    footerLayout->addLayout(userRow);
    auto *signOutButton = new QPushButton(tr("Sign out"), footer);
    footerLayout->addWidget(signOutButton);
    sidebarLayout->addWidget(footer);
    connect(signOutButton, &QPushButton::clicked, this, &AdminWindow::signedOut);

    auto *pages = new QStackedWidget(this);

    // Each group heading and the sections under it, so a heading can hide with its sections.
    QList<QPair<QListWidgetItem *, QList<QListWidgetItem *>>> groups;
    auto addGroup = [&](const QString &caption) {
        auto *item = new QListWidgetItem(caption);
        item->setFlags(Qt::NoItemFlags);
        item->setData(kHeaderRole, true);
        sidebar->addItem(item);
        groups.append(qMakePair(item, QList<QListWidgetItem *>()));
    };
    auto addSection = [&](const QString &icon, const QString &label, QWidget *page) {
        auto *item = new QListWidgetItem(QIcon(":/icons/" + icon + ".svg"), label);
        item->setToolTip(label);
        item->setData(kPageRole, pages->count());
        sidebar->addItem(item);
        if (!groups.isEmpty()) groups.last().second.append(item);
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
    QListWidgetItem *overviewItem = addSection("overview", tr("Overview"), overviewPage);
    addGroup(tr("Sales"));
    auto *onlineOrdersPage = forwardStatus(new OnlineOrdersPage(client, this));
    QListWidgetItem *onlineOrdersItem = addSection("online-orders", tr("Online Orders"), onlineOrdersPage);
    QHash<QString, QListWidgetItem *> moduleItems; // Settings → Modules key -> sidebar item
    moduleItems.insert("online_orders", onlineOrdersItem);
    addSection("daily-summary", tr("Daily Summary"), forwardStatus(new DailySummaryPage(client, this)));
    if (profile.isDev) addSection("checkout", tr("Checkout"), new CheckoutPage(client, this));
    addGroup(tr("Catalog"));
    addSection("inventory", tr("Inventory"), forwardStatus(new InventoryPage(client, this)));
    addSection("categories", tr("Categories"), forwardStatus(new CategoriesPage(client, this)));
    addGroup(tr("People"));
    addSection("employees", tr("Employees"), forwardStatus(new EmployeesPage(client, this)));
    moduleItems.insert("salary", addSection("salary", tr("Salary"), forwardStatus(new SalaryPage(client, this))));
    auto *applicationsPage = forwardStatus(new ApplicationsPage(client, this));
    QListWidgetItem *applicationsItem = addSection("applications", tr("Applications"), applicationsPage);
    moduleItems.insert("job_applications", applicationsItem);
    addGroup(tr("Storefront"));
    QListWidgetItem *ordersItem =
        addSection("orders", tr("Custom Orders"), forwardStatus(new CustomOrdersPage(client, this)));
    // Storefront "Contact us" submissions.
    moduleItems.insert("custom_orders", ordersItem);
    QListWidgetItem *messagesItem = addSection("help", tr("Messages"), forwardStatus(new MessagesPage(client, this)));
    moduleItems.insert("messages", messagesItem);
    auto *stockRequestsPage = forwardStatus(new StockRequestsPage(client, this));
    QListWidgetItem *stockRequestsItem = addSection("stock-requests", tr("Stock Requests"), stockRequestsPage);
    moduleItems.insert("stock_requests", stockRequestsItem);
    addGroup(tr("System"));
    addSection("audit-log", tr("Audit Log"), forwardStatus(new AuditLogPage(client, this)));
    m_softwarePage = new SoftwarePage(updateChecker, this);
    addSection("settings", tr("Settings"), forwardStatus(new SettingsPage(client, m_softwarePage, this)));

    connect(sidebar, &QListWidget::currentItemChanged, this, [pages, title](QListWidgetItem *item) {
        if (!item || item->data(kHeaderRole).toBool()) return;
        pages->setCurrentIndex(item->data(kPageRole).toInt());
        title->setText(item->text());
    });
    sidebar->setCurrentItem(overviewItem);

    // Settings → Modules: hide what the business switched off, and a heading left with nothing under it.
    auto applyModules = [sidebar, overviewItem, moduleItems, groups]() {
        const QStringList disabled = BusinessProfile::instance()->settings().disabledModules;
        for (auto it = moduleItems.cbegin(); it != moduleItems.cend(); ++it) it.value()->setHidden(disabled.contains(it.key()));
        for (const auto &[header, items] : groups) {
            header->setHidden(std::all_of(items.begin(), items.end(), [](QListWidgetItem *item) { return item->isHidden(); }));
        }
        if (sidebar->currentItem() && sidebar->currentItem()->isHidden()) sidebar->setCurrentItem(overviewItem);
    };
    applyModules();
    connect(BusinessProfile::instance(), &BusinessProfile::changed, sidebar, applyModules);

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

    // Settings → Notifications: pop-ups for what comes in from the website, and the lists they're
    // about reload so the counts stay current without pressing Refresh.
    auto *notifier = new DesktopNotifier(client, this);
    connect(notifier, &DesktopNotifier::arrived, this, [client](const QString &event) {
        if (event == "contact_message") client->fetchContactMessages();
        else if (event == "stock_request") client->fetchStockRequests();
        else if (event == "custom_order") client->fetchCustomLeatherOrders();
        else if (event == "job_application") client->fetchJobApplications();
        else if (event == "online_order") client->fetchOnlineOrders();
    });
    connect(notifier, &DesktopNotifier::openRequested, this, [sidebar, moduleItems](const QString &event) {
        QListWidgetItem *item = moduleItems.value(DesktopNotifier::moduleKey(event));
        if (item && !item->isHidden()) sidebar->setCurrentItem(item);
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
        m_updateButton->setText(tr("Update now — v%1").arg(version));
        m_updateButton->setEnabled(true);
        m_updateButton->show();
    } else {
        m_updateButton->hide();
    }
    m_softwarePage->reportUpdateCheckResult(ok, isNewer, version, releaseUrl, assetUrl, errorMessage);
}

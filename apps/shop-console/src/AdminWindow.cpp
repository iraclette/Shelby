#include "AdminWindow.h"
#include "AuditLogPage.h"
#include "CheckoutPage.h"
#include "CategoriesPage.h"
#include "CustomOrdersPage.h"
#include "DailySummaryPage.h"
#include "EmployeesPage.h"
#include "InventoryPage.h"
#include "MessagesPage.h"
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
#include <QPushButton>
#include <QSize>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableView>
#include <QToolBar>
#include <QWidget>

#include <type_traits>

namespace {
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
} // namespace

AdminWindow::AdminWindow(SupabaseClient *client, UpdateChecker *updateChecker, bool isDev, QWidget *parent)
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
    auto *signOutButton = new QPushButton("Sign out", this);
    toolbar->addWidget(signOutButton);
    connect(signOutButton, &QPushButton::clicked, this, &AdminWindow::signedOut);
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

    auto *sidebar = new QListWidget(this);
    sidebar->setObjectName("adminSidebar");
    sidebar->setFixedWidth(180);
    sidebar->setIconSize(QSize(18, 18));
    sidebar->setFocusPolicy(Qt::NoFocus);
    sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *pages = new QStackedWidget(this);

    // Sidebar row i shows page i, so both are added together.
    auto addSection = [&](const QString &icon, const QString &label, QWidget *page) {
        sidebar->addItem(new QListWidgetItem(QIcon(":/icons/" + icon + ".svg"), label));
        tidyTables(page);
        pages->addWidget(page);
    };
    auto forwardStatus = [this](auto *page) {
        connect(page, &std::remove_pointer_t<decltype(page)>::statusMessage, this,
                [this](const QString &message) { statusBar()->showMessage(message); });
        return page;
    };

    addSection("inventory", "Inventory", forwardStatus(new InventoryPage(client, this)));
    addSection("employees", "Employees", forwardStatus(new EmployeesPage(client, this)));
    addSection("salary", "Salary", forwardStatus(new SalaryPage(client, this)));
    addSection("daily-summary", "Daily Summary", forwardStatus(new DailySummaryPage(client, this)));
    addSection("audit-log", "Audit Log", forwardStatus(new AuditLogPage(client, this)));
    addSection("categories", "Categories", forwardStatus(new CategoriesPage(client, this)));
    addSection("orders", "Custom Orders", forwardStatus(new CustomOrdersPage(client, this)));
    // Storefront "Contact us" submissions.
    addSection("help", "Messages", forwardStatus(new MessagesPage(client, this)));
    if (isDev) addSection("checkout", "Checkout", new CheckoutPage(client, this));
    m_softwarePage = new SoftwarePage(updateChecker, this);
    addSection("software", "Software", m_softwarePage);

    connect(sidebar, &QListWidget::currentRowChanged, this, [pages, sidebar, title](int row) {
        pages->setCurrentIndex(row);
        if (row >= 0) title->setText(sidebar->item(row)->text());
    });
    sidebar->setCurrentRow(0);

    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(sidebar);
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

#include "AdminWindow.h"
#include "AuditLogPage.h"
#include "DailySummaryPage.h"
#include "EmployeesPage.h"
#include "InventoryPage.h"
#include "SalaryPage.h"
#include "SoftwarePage.h"
#include "SupabaseClient.h"
#include "UpdateChecker.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSize>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolBar>
#include <QWidget>

AdminWindow::AdminWindow(SupabaseClient *client, UpdateChecker *updateChecker, QWidget *parent)
    : QMainWindow(parent), m_updateChecker(updateChecker) {
    setWindowTitle("Shop Console — Admin");
    showMaximized();

    auto *toolbar = addToolBar("main");
    toolbar->setMovable(false);
    auto *title = new QLabel("Shop Console", this);
    title->setObjectName("pageTitle");
    toolbar->addWidget(title);
    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
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
    sidebar->setFixedWidth(170);
    sidebar->setIconSize(QSize(18, 18));
    sidebar->setFocusPolicy(Qt::NoFocus);
    sidebar->addItem(new QListWidgetItem(QIcon(":/icons/inventory.svg"), "Inventory"));
    sidebar->addItem(new QListWidgetItem(QIcon(":/icons/employees.svg"), "Employees"));
    sidebar->addItem(new QListWidgetItem(QIcon(":/icons/salary.svg"), "Salary"));
    sidebar->addItem(new QListWidgetItem(QIcon(":/icons/daily-summary.svg"), "Daily Summary"));
    sidebar->addItem(new QListWidgetItem(QIcon(":/icons/audit-log.svg"), "Audit Log"));
    sidebar->addItem(new QListWidgetItem(QIcon(":/icons/software.svg"), "Software"));

    auto *pages = new QStackedWidget(this);
    auto *inventoryPage = new InventoryPage(client, this);
    auto *employeesPage = new EmployeesPage(client, this);
    auto *salaryPage = new SalaryPage(client, this);
    auto *dailySummaryPage = new DailySummaryPage(client, this);
    auto *auditLogPage = new AuditLogPage(client, this);
    m_softwarePage = new SoftwarePage(updateChecker, this);
    pages->addWidget(inventoryPage);
    pages->addWidget(employeesPage);
    pages->addWidget(salaryPage);
    pages->addWidget(dailySummaryPage);
    pages->addWidget(auditLogPage);
    pages->addWidget(m_softwarePage);

    connect(sidebar, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
    sidebar->setCurrentRow(0);

    connect(inventoryPage, &InventoryPage::statusMessage, this,
            [this](const QString &message) { statusBar()->showMessage(message); });
    connect(employeesPage, &EmployeesPage::statusMessage, this,
            [this](const QString &message) { statusBar()->showMessage(message); });
    connect(salaryPage, &SalaryPage::statusMessage, this,
            [this](const QString &message) { statusBar()->showMessage(message); });
    connect(dailySummaryPage, &DailySummaryPage::statusMessage, this,
            [this](const QString &message) { statusBar()->showMessage(message); });
    connect(auditLogPage, &AuditLogPage::statusMessage, this,
            [this](const QString &message) { statusBar()->showMessage(message); });

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

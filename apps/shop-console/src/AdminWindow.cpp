#include "AdminWindow.h"
#include "EmployeesPage.h"
#include "InventoryPage.h"
#include "SoftwarePage.h"
#include "SupabaseClient.h"

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

AdminWindow::AdminWindow(SupabaseClient *client, QWidget *parent)
    : QMainWindow(parent) {
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
    m_updateBanner = new QLabel(this);
    m_updateBanner->setOpenExternalLinks(true);
    m_updateBanner->setStyleSheet("padding: 0 8px;");
    m_updateBanner->hide();
    toolbar->addWidget(m_updateBanner);
    auto *signOutButton = new QPushButton("Sign out", this);
    toolbar->addWidget(signOutButton);
    connect(signOutButton, &QPushButton::clicked, this, &AdminWindow::signedOut);

    auto *sidebar = new QListWidget(this);
    sidebar->setObjectName("adminSidebar");
    sidebar->setFixedWidth(170);
    sidebar->setIconSize(QSize(18, 18));
    sidebar->setFocusPolicy(Qt::NoFocus);
    sidebar->addItem(new QListWidgetItem(QIcon(":/icons/inventory.svg"), "Inventory"));
    sidebar->addItem(new QListWidgetItem(QIcon(":/icons/employees.svg"), "Employees"));
    sidebar->addItem(new QListWidgetItem(QIcon(":/icons/software.svg"), "Software"));

    auto *pages = new QStackedWidget(this);
    auto *inventoryPage = new InventoryPage(client, this);
    auto *employeesPage = new EmployeesPage(client, this);
    m_softwarePage = new SoftwarePage(this);
    pages->addWidget(inventoryPage);
    pages->addWidget(employeesPage);
    pages->addWidget(m_softwarePage);

    connect(sidebar, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
    sidebar->setCurrentRow(0);

    connect(inventoryPage, &InventoryPage::statusMessage, this,
            [this](const QString &message) { statusBar()->showMessage(message); });
    connect(employeesPage, &EmployeesPage::statusMessage, this,
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
                                           const QString &errorMessage) {
    if (ok && isNewer) {
        m_updateBanner->setText(
            QString("<a href=\"%1\" style=\"color:#4f8cff;\">Update available — v%2</a>").arg(releaseUrl, version));
        m_updateBanner->show();
    } else {
        m_updateBanner->hide();
    }
    m_softwarePage->reportUpdateCheckResult(ok, isNewer, version, releaseUrl, errorMessage);
}

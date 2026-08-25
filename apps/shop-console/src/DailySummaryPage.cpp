#include "DailySummaryPage.h"
#include "SupabaseClient.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

DailySummaryPage::DailySummaryPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client), m_day(QDate::currentDate()) {
    auto *toolbar = new QHBoxLayout;
    auto *prevButton = new QPushButton("<", this);
    prevButton->setFixedWidth(32);
    toolbar->addWidget(prevButton);
    m_dayLabel = new QLabel(this);
    m_dayLabel->setStyleSheet("font-weight: 600; padding: 0 8px;");
    toolbar->addWidget(m_dayLabel);
    auto *nextButton = new QPushButton(">", this);
    nextButton->setFixedWidth(32);
    toolbar->addWidget(nextButton);
    auto *todayButton = new QPushButton("Today", this);
    toolbar->addWidget(todayButton);

    toolbar->addSpacing(16);
    toolbar->addWidget(new QLabel("Shop:", this));
    m_shopCombo = new QComboBox(this);
    m_shopCombo->addItem("All shops", QString());
    toolbar->addWidget(m_shopCombo);

    auto *refreshButton = new QPushButton("Refresh", this);
    toolbar->addWidget(refreshButton);
    toolbar->addStretch();
    m_statsLabel = new QLabel(this);
    m_statsLabel->setStyleSheet("color: #9aa0ac;");
    toolbar->addWidget(m_statsLabel);

    connect(prevButton, &QPushButton::clicked, this, [this]() { changeDay(-1); });
    connect(nextButton, &QPushButton::clicked, this, [this]() { changeDay(1); });
    connect(todayButton, &QPushButton::clicked, this, [this]() {
        m_day = QDate::currentDate();
        reload();
    });
    connect(m_shopCombo, &QComboBox::currentIndexChanged, this, [this]() { reload(); });
    connect(refreshButton, &QPushButton::clicked, this, [this]() { reload(); });

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"Time", "Shop", "Staff", "Items", "Total"});
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        m_shops = shops;
        for (const Shop &shop : m_shops) m_shopCombo->addItem(shop.name, shop.id);
        rebuildTable();
    });
    m_client->fetchShops();

    connect(m_client, &SupabaseClient::profilesFetched, this, [this](const QVector<Profile> &profiles) {
        m_profiles = profiles;
        rebuildTable();
    });
    m_client->fetchProfiles();

    connect(m_client, &SupabaseClient::dailySalesFetched, this, [this](const QVector<SaleSummary> &sales) {
        m_sales = sales;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::dailySalesFetchFailed, this, [this](const QString &message) {
        emit statusMessage("Failed to load sales: " + message);
    });

    reload();
}

void DailySummaryPage::changeDay(int deltaDays) {
    m_day = m_day.addDays(deltaDays);
    reload();
}

void DailySummaryPage::reload() {
    m_dayLabel->setText(m_day.toString("dddd, MMMM d yyyy"));

    const QString startIso = m_day.toString(Qt::ISODate);
    const QString endExclusiveIso = m_day.addDays(1).toString(Qt::ISODate);
    const QString shopId = m_shopCombo->currentData().toString();

    m_client->fetchSalesForDay(shopId, startIso, endExclusiveIso);
}

void DailySummaryPage::rebuildTable() {
    double total = 0;
    m_table->setRowCount(m_sales.size());

    for (int row = 0; row < m_sales.size(); ++row) {
        const SaleSummary &sale = m_sales[row];
        total += sale.total;

        QString shopName;
        for (const Shop &shop : m_shops) {
            if (shop.id == sale.shopId) {
                shopName = shop.name;
                break;
            }
        }
        QString staffName;
        for (const Profile &profile : m_profiles) {
            if (profile.id == sale.staffId) {
                staffName = profile.fullName;
                break;
            }
        }

        int itemCount = 0;
        for (const SaleItemSummary &item : sale.items) itemCount += item.quantity;

        m_table->setItem(row, 0, new QTableWidgetItem(sale.soldAt.mid(11, 8))); // HH:MM:SS out of an ISO timestamp
        m_table->setItem(row, 1, new QTableWidgetItem(shopName));
        m_table->setItem(row, 2, new QTableWidgetItem(staffName));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(itemCount)));
        m_table->setItem(row, 4, new QTableWidgetItem(QString("GEL %1").arg(sale.total, 0, 'f', 2)));
    }

    m_statsLabel->setText(QString("%1 sale%2 · GEL %3 total")
                               .arg(m_sales.size())
                               .arg(m_sales.size() == 1 ? "" : "s")
                               .arg(total, 0, 'f', 2));
    emit statusMessage(QString("Loaded %1 sale(s) for %2.").arg(m_sales.size()).arg(m_day.toString(Qt::ISODate)));
}

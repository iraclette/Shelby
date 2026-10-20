#include "SalaryPage.h"
#include "ShopTime.h"
#include "PaySalaryDialog.h"
#include "SupabaseClient.h"

#include <QLocale>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

SalaryPage::SalaryPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    const QDate today = shoptime::today();
    m_periodStart = QDate(today.year(), today.month(), 1);

    auto *toolbar = new QHBoxLayout;
    auto *prevButton = new QPushButton("<", this);
    prevButton->setObjectName("compactButton");
    prevButton->setFixedWidth(32);
    toolbar->addWidget(prevButton);
    m_periodLabel = new QLabel(this);
    m_periodLabel->setStyleSheet("font-weight: 600; padding: 0 8px;");
    toolbar->addWidget(m_periodLabel);
    auto *nextButton = new QPushButton(">", this);
    nextButton->setObjectName("compactButton");
    nextButton->setFixedWidth(32);
    toolbar->addWidget(nextButton);

    toolbar->addSpacing(16);
    toolbar->addWidget(new QLabel(tr("Shop:"), this));
    m_shopCombo = new QComboBox(this);
    m_shopCombo->addItem(tr("All shops"), QString());
    toolbar->addWidget(m_shopCombo);

    auto *refreshButton = new QPushButton(tr("Refresh"), this);
    toolbar->addWidget(refreshButton);
    toolbar->addStretch();

    connect(prevButton, &QPushButton::clicked, this, [this]() { changeMonth(-1); });
    connect(nextButton, &QPushButton::clicked, this, [this]() { changeMonth(1); });
    connect(m_shopCombo, &QComboBox::currentIndexChanged, this, [this]() { reload(); });
    connect(refreshButton, &QPushButton::clicked, this, [this]() { reload(); });

    m_table = new QTableWidget(this);
    m_table->setColumnCount(11);
    m_table->setHorizontalHeaderLabels({tr("Employee"), tr("Shop"), tr("Daily rate"), tr("Bonus threshold"), tr("Bonus amount"),
                                         tr("Days worked"), tr("Bonus days"), tr("Earned"), tr("Paid"), tr("Balance"), ""});
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::itemChanged, this, &SalaryPage::handleItemChanged);

    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        m_shops = shops;
        // shopsFetched fires for every page's fetchShops(), so clear before re-adding.
        const QString previouslySelected = m_shopCombo->currentData().toString();
        m_shopCombo->clear();
        m_shopCombo->addItem(tr("All shops"), QString());
        for (const Shop &shop : m_shops) m_shopCombo->addItem(shop.name, shop.id);
        const int index = m_shopCombo->findData(previouslySelected);
        if (index >= 0) m_shopCombo->setCurrentIndex(index);
        rebuildTable();
    });
    m_client->fetchShops();

    connect(m_client, &SupabaseClient::profilesFetched, this, [this](const QVector<Profile> &profiles) {
        m_profiles = profiles;
        recompute();
    });
    connect(m_client, &SupabaseClient::profilesFetchFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Failed to load employees: ") + message);
    });

    connect(m_client, &SupabaseClient::salesForPeriodFetched, this, [this](const QVector<SaleAttribution> &sales) {
        m_sales = sales;
        recompute();
    });
    connect(m_client, &SupabaseClient::salesForPeriodFetchFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Failed to load sales: ") + message);
    });

    connect(m_client, &SupabaseClient::salaryPaymentsFetched, this, [this](const QVector<SalaryPayment> &payments) {
        m_payments = payments;
        recompute();
    });
    connect(m_client, &SupabaseClient::salaryPaymentsFetchFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Failed to load payments: ") + message);
    });

    connect(m_client, &SupabaseClient::profileUpdateFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Update failed: ") + message);
        m_client->fetchProfiles();
    });
    connect(m_client, &SupabaseClient::profileUpdated, this, [this]() { m_client->fetchProfiles(); });

    reload();
}

void SalaryPage::changeMonth(int delta) {
    m_periodStart = m_periodStart.addMonths(delta);
    reload();
}

void SalaryPage::reload() {
    m_periodLabel->setText(QLocale().toString(m_periodStart, "LLLL yyyy"));

    const QString startIso = m_periodStart.toString(Qt::ISODate);
    const QString endInclusiveIso = m_periodStart.addMonths(1).addDays(-1).toString(Qt::ISODate);
    const QString shopId = m_shopCombo->currentData().toString();

    m_client->fetchProfiles();
    m_client->fetchSalesForPeriod(shopId, shoptime::dayStartUtc(m_periodStart),
                                  shoptime::dayStartUtc(m_periodStart.addMonths(1)));
    m_client->fetchSalaryPayments(startIso, endInclusiveIso);
}

void SalaryPage::recompute() {
    m_earningsByStaffId.clear();

    // staff_id -> (day string "yyyy-MM-dd" -> summed sales total that day)
    QMap<QString, QMap<QString, double>> dailyTotalsByStaff;
    for (const SaleAttribution &sale : m_sales) {
        dailyTotalsByStaff[sale.staffId][shoptime::dayOf(sale.soldAt).toString(Qt::ISODate)] += sale.total;
    }

    for (const Profile &profile : m_profiles) {
        EmployeeEarnings earnings;
        const QMap<QString, double> days = dailyTotalsByStaff.value(profile.id);
        earnings.daysWorked = days.size();
        // An unconfigured threshold (0/unset) never grants a bonus — only an
        // admin explicitly setting one above zero turns the bonus on.
        if (profile.bonusThreshold > 0) {
            for (double dayTotal : days) {
                if (dayTotal > profile.bonusThreshold) ++earnings.bonusDays;
            }
        }
        earnings.earned = earnings.daysWorked * profile.dailyRate + earnings.bonusDays * profile.bonusAmount;
        m_earningsByStaffId.insert(profile.id, earnings);
    }

    for (const SalaryPayment &payment : m_payments) {
        auto it = m_earningsByStaffId.find(payment.staffId);
        if (it != m_earningsByStaffId.end()) it->paid += payment.amount;
    }

    rebuildTable();
}

void SalaryPage::rebuildTable() {
    m_populating = true;

    const QString filterShopId = m_shopCombo->currentData().toString();
    m_visibleProfiles.clear();
    for (const Profile &profile : m_profiles) {
        // Admins/owners aren't on the daily wage — only staff get paid here.
        if (profile.role != "staff") continue;
        if (!filterShopId.isEmpty() && profile.shopId != filterShopId) continue;
        m_visibleProfiles.append(profile);
    }

    m_table->setRowCount(m_visibleProfiles.size());
    for (int row = 0; row < m_visibleProfiles.size(); ++row) {
        const Profile &profile = m_visibleProfiles[row];
        const EmployeeEarnings earnings = m_earningsByStaffId.value(profile.id);
        const double balance = earnings.earned - earnings.paid;

        auto *nameItem = new QTableWidgetItem(profile.fullName);
        nameItem->setData(Qt::UserRole, profile.id);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColName, nameItem);

        QString shopName;
        for (const Shop &shop : m_shops) {
            if (shop.id == profile.shopId) {
                shopName = shop.name;
                break;
            }
        }
        auto *shopItem = new QTableWidgetItem(shopName);
        shopItem->setFlags(shopItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColShop, shopItem);

        m_table->setItem(row, kColDailyRate, new QTableWidgetItem(QString::number(profile.dailyRate, 'f', 2)));
        m_table->setItem(row, kColBonusThreshold,
                          new QTableWidgetItem(QString::number(profile.bonusThreshold, 'f', 2)));
        m_table->setItem(row, kColBonusAmount, new QTableWidgetItem(QString::number(profile.bonusAmount, 'f', 2)));

        auto addReadOnly = [this, row](int col, const QString &text) {
            auto *item = new QTableWidgetItem(text);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            m_table->setItem(row, col, item);
        };
        addReadOnly(kColDaysWorked, QString::number(earnings.daysWorked));
        addReadOnly(kColBonusDays, QString::number(earnings.bonusDays));
        addReadOnly(kColEarned, QString::number(earnings.earned, 'f', 2));
        addReadOnly(kColPaid, QString::number(earnings.paid, 'f', 2));
        addReadOnly(kColBalance, QString::number(balance, 'f', 2));

        auto *payButton = new QPushButton(balance > 0 ? tr("Pay") : tr("Record"), this);
        payButton->setObjectName("compactButton");
        connect(payButton, &QPushButton::clicked, this, [this, row]() { openPayDialog(row); });
        m_table->setCellWidget(row, kColPayButton, payButton);
    }

    m_populating = false;
    emit statusMessage(tr("%n employees for %1.", nullptr, int(m_visibleProfiles.size())).arg(m_periodLabel->text()));
}

void SalaryPage::handleItemChanged(QTableWidgetItem *item) {
    if (m_populating) return;

    const int row = item->row();
    const int col = item->column();
    if (col != kColDailyRate && col != kColBonusThreshold && col != kColBonusAmount) return;

    const QTableWidgetItem *nameItem = m_table->item(row, kColName);
    const QString profileId = nameItem ? nameItem->data(Qt::UserRole).toString() : QString();
    if (profileId.isEmpty()) return;

    const QString text = item->text().trimmed();
    bool ok = false;
    const double value = text.isEmpty() ? 0.0 : text.toDouble(&ok);
    if (!text.isEmpty() && (!ok || value < 0)) {
        emit statusMessage(tr("Invalid amount — must be a non-negative number."));
        m_client->fetchProfiles();
        return;
    }

    const QString field = col == kColDailyRate     ? "daily_rate"
                           : col == kColBonusThreshold ? "bonus_threshold"
                                                        : "bonus_amount";
    m_client->updateProfileField(profileId, field, value);
}

void SalaryPage::openPayDialog(int row) {
    if (row < 0 || row >= m_visibleProfiles.size()) return;
    const Profile &profile = m_visibleProfiles[row];
    if (profile.shopId.isEmpty()) {
        emit statusMessage(tr("Assign this employee to a shop before recording a payment."));
        return;
    }

    const EmployeeEarnings earnings = m_earningsByStaffId.value(profile.id);
    const double balance = earnings.earned - earnings.paid;

    const QString periodStartIso = m_periodStart.toString(Qt::ISODate);
    const QString periodEndIso = m_periodStart.addMonths(1).addDays(-1).toString(Qt::ISODate);

    PaySalaryDialog dialog(m_client, profile.id, profile.shopId, profile.fullName, periodStartIso, periodEndIso,
                            balance, this);
    if (dialog.exec() == QDialog::Accepted) {
        emit statusMessage(tr("Payment recorded."));
        reload();
    }
}

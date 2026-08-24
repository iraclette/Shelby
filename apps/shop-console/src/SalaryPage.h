#pragma once

#include <QDate>
#include <QMap>
#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QComboBox;
class QLabel;
class QTableWidget;
class QTableWidgetItem;

// Daily-wage earnings per employee for a selected month, plus a payout
// ledger. There's no clock-in system, so a "day worked" is inferred from
// sales.staff_id: any calendar day the employee has at least one recorded
// sale counts as a paid day (see SupabaseClient::fetchSalesForPeriod).
// Earnings are computed here client-side from that plus each employee's pay
// config (profiles.daily_rate/bonus_threshold/bonus_amount, editable inline
// like InventoryPage's price cells) — nothing about the calculation itself
// is persisted, only the actual payments an admin records via "Pay".
class SalaryPage : public QWidget {
    Q_OBJECT

public:
    explicit SalaryPage(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);

private:
    struct EmployeeEarnings {
        int daysWorked = 0;
        int bonusDays = 0;
        double earned = 0;
        double paid = 0;
    };

    void changeMonth(int delta);
    void reload();
    void recompute();
    void rebuildTable();
    void handleItemChanged(QTableWidgetItem *item);
    void openPayDialog(int row);

    SupabaseClient *m_client;

    QLabel *m_periodLabel;
    QComboBox *m_shopCombo;
    QTableWidget *m_table;

    QDate m_periodStart; // always the 1st of the displayed month

    QVector<Shop> m_shops;
    QVector<Profile> m_profiles;
    QVector<Profile> m_visibleProfiles; // m_profiles filtered by m_shopCombo, parallel to m_table rows
    QVector<SaleAttribution> m_sales;
    QVector<SalaryPayment> m_payments;
    QMap<QString, EmployeeEarnings> m_earningsByStaffId;
    bool m_populating = false; // guards against itemChanged firing during rebuildTable()

    static constexpr int kColName = 0;
    static constexpr int kColShop = 1;
    static constexpr int kColDailyRate = 2;
    static constexpr int kColBonusThreshold = 3;
    static constexpr int kColBonusAmount = 4;
    static constexpr int kColDaysWorked = 5;
    static constexpr int kColBonusDays = 6;
    static constexpr int kColEarned = 7;
    static constexpr int kColPaid = 8;
    static constexpr int kColBalance = 9;
    static constexpr int kColPayButton = 10;
};

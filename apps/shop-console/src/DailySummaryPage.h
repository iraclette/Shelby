#pragma once

#include <QDate>
#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QComboBox;
class QLabel;
class QTableWidget;

// "Counter for the day" — what sold, for how much, and who sold it. Same
// shell pattern as SalaryPage: a day nav (defaults to today) + shop filter,
// a small stat row, and a table of the day's sales.
class DailySummaryPage : public QWidget {
    Q_OBJECT

public:
    explicit DailySummaryPage(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);

private:
    void changeDay(int deltaDays);
    void reload();
    void rebuildTable();

    SupabaseClient *m_client;

    QLabel *m_dayLabel;
    QComboBox *m_shopCombo;
    QLabel *m_statsLabel;
    QTableWidget *m_table;

    QDate m_day;

    QVector<Shop> m_shops;
    QVector<Profile> m_profiles;
    QVector<SaleSummary> m_sales;
};

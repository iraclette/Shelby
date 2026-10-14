#pragma once

#include <QDate>
#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QComboBox;
class QLabel;
class QTableWidget;

// What sold on a given day, for how much, and by whom.
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

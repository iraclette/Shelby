#pragma once

#include <QVector>
#include <QWidget>

#include "SupabaseClient.h"

class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;

// "Work with us" applications from the storefront's /jobs page. The shop calls
// the applicant, then marks them contacted, hired or not a fit.
class ApplicationsPage : public QWidget {
    Q_OBJECT

public:
    explicit ApplicationsPage(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);
    void newCountChanged(int count); // not yet contacted: drives the sidebar badge

private:
    void rebuildTable();
    void showDetails();
    void setSelectedStatus(const QString &status);
    const JobApplication *selectedApplication() const;
    QString shopName(const QString &shopId) const;

    SupabaseClient *m_client;
    QComboBox *m_filter;
    QTableWidget *m_table;
    QLabel *m_details;
    QPushButton *m_contactedButton;
    QPushButton *m_hiredButton;
    QPushButton *m_rejectedButton;

    QVector<JobApplication> m_applications;
    QVector<int> m_visible; // table row -> index into m_applications
    QVector<Shop> m_shops;
};

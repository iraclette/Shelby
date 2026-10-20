#include "ApplicationsPage.h"
#include "Theme.h"
#include "ShopTime.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

namespace {
QString statusLabel(const QString &status) {
    if (status == "new") return QObject::tr("New");
    if (status == "contacted") return QObject::tr("Contacted");
    if (status == "hired") return QObject::tr("Hired");
    if (status == "rejected") return QObject::tr("Not a fit");
    return status;
}

QColor statusColor(const QString &status) {
    if (status == "new") return theme::palette().accent;
    if (status == "hired") return theme::palette().success;
    if (status == "rejected") return theme::palette().textDisabled;
    return theme::palette().warning;
}

QString availabilityLabel(const QString &availability) {
    if (availability == "full_time") return QObject::tr("Full time");
    if (availability == "part_time") return QObject::tr("Part time");
    return QObject::tr("Full or part time");
}

QString languagesLabel(const QStringList &languages) {
    QStringList names;
    for (const QString &lang : languages) {
        if (lang == "ka") names << QObject::tr("Georgian");
        else if (lang == "ru") names << QObject::tr("Russian");
        else if (lang == "en") names << QObject::tr("English");
    }
    return names.isEmpty() ? "—" : names.join(", ");
}
} // namespace

ApplicationsPage::ApplicationsPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton(tr("Refresh"), this);
    toolbar->addWidget(refreshButton);
    m_filter = new QComboBox(this);
    m_filter->addItem(tr("New"), "new");
    m_filter->addItem(tr("All applications"), "");
    m_filter->addItem(tr("Contacted"), "contacted");
    m_filter->addItem(tr("Hired"), "hired");
    m_filter->addItem(tr("Not a fit"), "rejected");
    toolbar->addWidget(m_filter);
    toolbar->addStretch();
    m_rejectedButton = new QPushButton(tr("Not a fit"), this);
    m_hiredButton = new QPushButton(tr("Hired"), this);
    m_contactedButton = new QPushButton(tr("Mark contacted"), this);
    m_contactedButton->setObjectName("primaryButton");
    for (QPushButton *button : {m_rejectedButton, m_hiredButton, m_contactedButton}) {
        button->setEnabled(false);
        toolbar->addWidget(button);
    }

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4); // shop, hours and languages are in the details panel
    m_table->setHorizontalHeaderLabels({tr("Received"), tr("Name"), tr("Phone"), tr("Status")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    auto *detailsPanel = new QFrame(this);
    detailsPanel->setObjectName("card");
    auto *detailsLayout = new QVBoxLayout(detailsPanel);
    detailsLayout->setContentsMargins(16, 14, 16, 14);
    m_details = new QLabel(detailsPanel);
    m_details->setObjectName("totalsKey");
    m_details->setWordWrap(true);
    m_details->setTextInteractionFlags(Qt::TextSelectableByMouse); // copy the phone number
    m_details->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    detailsLayout->addWidget(m_details, 1);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(m_table);
    splitter->addWidget(detailsPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(splitter, 1);

    connect(refreshButton, &QPushButton::clicked, this, [this]() { m_client->fetchJobApplications(); });
    connect(m_filter, &QComboBox::currentIndexChanged, this, &ApplicationsPage::rebuildTable);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ApplicationsPage::showDetails);
    connect(m_contactedButton, &QPushButton::clicked, this, [this]() { setSelectedStatus("contacted"); });
    connect(m_hiredButton, &QPushButton::clicked, this, [this]() { setSelectedStatus("hired"); });
    connect(m_rejectedButton, &QPushButton::clicked, this, [this]() { setSelectedStatus("rejected"); });

    connect(m_client, &SupabaseClient::shopsFetched, this, [this](const QVector<Shop> &shops) {
        m_shops = shops;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::jobApplicationsFetched, this,
            [this](const QVector<JobApplication> &applications) {
        m_applications = applications;
        rebuildTable();
        emit newCountChanged(std::count_if(applications.begin(), applications.end(),
                                           [](const JobApplication &a) { return a.status == "new"; }));
    });
    connect(m_client, &SupabaseClient::jobApplicationsFetchFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Failed to load applications: ") + message);
    });
    connect(m_client, &SupabaseClient::jobApplicationUpdated, this, [this]() {
        emit statusMessage(tr("Application updated."));
        m_client->fetchJobApplications();
    });
    connect(m_client, &SupabaseClient::jobApplicationUpdateFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Update failed: ") + message);
        m_client->fetchJobApplications();
    });
    m_client->fetchJobApplications();
}

QString ApplicationsPage::shopName(const QString &shopId) const {
    if (shopId.isEmpty()) return tr("Any shop");
    for (const Shop &shop : m_shops) {
        if (shop.id == shopId) return shop.name;
    }
    return {};
}

const JobApplication *ApplicationsPage::selectedApplication() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return nullptr;
    const int row = rows.first().row();
    return row < m_visible.size() ? &m_applications[m_visible[row]] : nullptr;
}

void ApplicationsPage::setSelectedStatus(const QString &status) {
    const JobApplication *application = selectedApplication();
    if (!application || application->status == status) return;
    for (QPushButton *button : {m_rejectedButton, m_hiredButton, m_contactedButton}) button->setEnabled(false);
    m_client->updateJobApplicationStatus(application->id, status);
}

void ApplicationsPage::rebuildTable() {
    const QString filter = m_filter->currentData().toString();
    m_visible.clear();
    for (int i = 0; i < m_applications.size(); ++i) {
        if (filter.isEmpty() || m_applications[i].status == filter) m_visible.append(i);
    }

    m_table->setRowCount(m_visible.size());
    for (int row = 0; row < m_visible.size(); ++row) {
        const JobApplication &application = m_applications[m_visible[row]];
        m_table->setItem(row, 0, new QTableWidgetItem(shoptime::format(application.createdAt, "yyyy-MM-dd HH:mm")));
        m_table->setItem(row, 1, new QTableWidgetItem(application.name));
        m_table->setItem(row, 2, new QTableWidgetItem(application.phone));
        auto *statusItem = new QTableWidgetItem(statusLabel(application.status));
        statusItem->setForeground(statusColor(application.status));
        m_table->setItem(row, 3, statusItem);
    }
    showDetails();
}

void ApplicationsPage::showDetails() {
    const JobApplication *application = selectedApplication();
    m_contactedButton->setEnabled(application && application->status == "new");
    m_hiredButton->setEnabled(application && application->status != "hired");
    m_rejectedButton->setEnabled(application && application->status != "rejected");
    if (!application) {
        m_details->setText(m_visible.isEmpty() ? tr("No applications here.") : tr("Select an application to read it."));
        return;
    }

    const QString strong = theme::hex(theme::palette().text);
    QString html = QString("<p style='font-size:16px; font-weight:700; color:%1;'>%2</p>"
                           "<p style='color:%3; font-weight:600;'>%4</p>")
                       .arg(theme::hex(theme::palette().textStrong), application->name.toHtmlEscaped(),
                            statusColor(application->status).name(), statusLabel(application->status));
    html += QString("<p><b style='color:%1;'>%2</b>%3</p>")
                .arg(strong, application->phone.toHtmlEscaped(),
                     application->email.isEmpty() ? QString() : "<br>" + application->email.toHtmlEscaped());
    html += QString("<p>%1 <b style='color:%2;'>%3</b><br>%4 <b style='color:%2;'>%5</b><br>"
                    "%6 <b style='color:%2;'>%7</b></p>")
                .arg(tr("Shop:"), strong, shopName(application->preferredShopId), tr("Hours:"),
                     availabilityLabel(application->availability), tr("Languages:"),
                     languagesLabel(application->languages));
    html += QString("<p style='color:%1;'>%2</p>")
                .arg(strong, application->about.toHtmlEscaped().replace('\n', "<br>"));
    m_details->setText(html);
}

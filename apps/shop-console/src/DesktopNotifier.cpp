#include "DesktopNotifier.h"
#include "BusinessProfile.h"
#include "SupabaseClient.h"
#include "Theme.h"

#include <QApplication>
#include <QJsonObject>
#include <QPainter>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrlQuery>
#include <QWidget>

namespace {
constexpr int kCheckIntervalMs = 60 * 1000;
constexpr int kRowsPerCheck = 20; // far more than ever comes in within a minute

QSettings settings() {
    return QSettings("Shelby", "ShopConsole");
}

QString settingsKey(const QString &userId, const QString &event) {
    return QString("notifications/%1/popup_%2").arg(userId, event);
}

// The business logo, or its first letter on the accent colour, for the taskbar corner.
QIcon trayIcon() {
    const QPixmap logo = BusinessProfile::instance()->roundLogo(32, 1.0);
    if (!logo.isNull()) return QIcon(logo);
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(theme::palette().accent);
    painter.drawRoundedRect(0, 0, 32, 32, 7, 7);
    QFont font = painter.font();
    font.setPixelSize(19);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(theme::palette().onAccent);
    const QString name = BusinessProfile::instance()->settings().name;
    painter.drawText(pixmap.rect(), Qt::AlignCenter, name.isEmpty() ? "S" : name.left(1).toUpper());
    return QIcon(pixmap);
}
} // namespace

QStringList DesktopNotifier::events() {
    return {"contact_message", "stock_request", "custom_order", "job_application", "online_order"};
}

QString DesktopNotifier::eventName(const QString &event) {
    if (event == "contact_message") return tr("Contact form messages");
    if (event == "stock_request") return tr("\"Let me know when it's back\" requests");
    if (event == "custom_order") return tr("Custom order requests");
    if (event == "job_application") return tr("New candidates");
    if (event == "online_order") return tr("Paid online orders");
    return event;
}

QString DesktopNotifier::moduleKey(const QString &event) {
    if (event == "contact_message") return "messages";
    if (event == "stock_request") return "stock_requests";
    if (event == "custom_order") return "custom_orders";
    if (event == "job_application") return "job_applications";
    if (event == "online_order") return "online_orders";
    return event;
}

bool DesktopNotifier::isEnabled(const QString &userId, const QString &event) {
    return settings().value(settingsKey(userId, event), true).toBool();
}

void DesktopNotifier::setEnabled(const QString &userId, const QString &event, bool enabled) {
    QSettings s = settings();
    s.setValue(settingsKey(userId, event), enabled);
}

DesktopNotifier::DesktopNotifier(SupabaseClient *client, QWidget *window)
    : QObject(window), m_client(client), m_window(window) {
    m_tray = new QSystemTrayIcon(trayIcon(), this);
    m_tray->setToolTip(BusinessProfile::instance()->settings().name);
    m_tray->show();
    connect(BusinessProfile::instance(), &BusinessProfile::changed, this, [this]() {
        m_tray->setIcon(trayIcon());
        m_tray->setToolTip(BusinessProfile::instance()->settings().name);
    });
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) bringWindowUp();
    });
    connect(m_tray, &QSystemTrayIcon::messageClicked, this, [this]() {
        bringWindowUp();
        if (!m_lastShown.isEmpty()) emit openRequested(m_lastShown);
    });

    m_timer = new QTimer(this);
    m_timer->setInterval(kCheckIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &DesktopNotifier::check);
    m_timer->start();
    check();
}

void DesktopNotifier::check() {
    for (const QString &event : events()) checkEvent(event);
}

void DesktopNotifier::checkEvent(const QString &event) {
    if (m_inFlight.contains(event)) return; // the last check is still waiting on a slow network
    // A section the business switched off: nothing to show, and its page may not be there.
    if (BusinessProfile::instance()->settings().disabledModules.contains(moduleKey(event))) return;

    QString table;
    QUrlQuery query;
    if (event == "contact_message") {
        table = "contact_messages";
        query.addQueryItem("select", "id,name");
    } else if (event == "stock_request") {
        table = "stock_requests";
        query.addQueryItem("select", "id,products(name),product_variants(name)");
    } else if (event == "custom_order") {
        table = "custom_leather_orders";
        query.addQueryItem("select", "id,customer_name");
    } else if (event == "job_application") {
        table = "job_applications";
        query.addQueryItem("select", "id,name");
    } else if (event == "online_order") {
        // An order is created when checkout starts and turns paid when the bank confirms it.
        table = "orders";
        query.addQueryItem("select", "id,customer_name,total");
        query.addQueryItem("status", "eq.paid");
    } else {
        return;
    }
    query.addQueryItem("order", event == "online_order" ? "updated_at.desc" : "created_at.desc");
    query.addQueryItem("limit", QString::number(kRowsPerCheck));

    m_inFlight.insert(event);
    m_client->selectRows(table, query, [this, event](const QJsonArray &rows, const QString &error) {
        m_inFlight.remove(event);
        if (!error.isEmpty()) {
            qWarning("Desktop notifications: checking %s failed: %s", qPrintable(event), qPrintable(error));
            return;
        }
        QVector<Item> items;
        for (const QJsonValue &value : rows) {
            const QJsonObject row = value.toObject();
            Item item{row.value("id").toString(), {}};
            if (event == "contact_message" || event == "job_application") {
                item.label = row.value("name").toString();
            } else if (event == "custom_order") {
                item.label = row.value("customer_name").toString();
            } else if (event == "stock_request") {
                item.label = row.value("products").toObject().value("name").toString();
                const QString variant = row.value("product_variants").toObject().value("name").toString();
                if (!variant.isEmpty()) item.label += " — " + variant;
            } else if (event == "online_order") {
                item.label = QString("%1 · %2").arg(row.value("customer_name").toString(),
                                                     money(row.value("total").toDouble()));
            }
            items.append(item);
        }
        handle(event, items);
    });
}

void DesktopNotifier::handle(const QString &event, const QVector<Item> &items) {
    const bool firstCheck = !m_seen.contains(event);
    QSet<QString> &seen = m_seen[event];
    QStringList fresh;
    // Oldest first, so the pop-up lists them in the order they came in.
    for (auto it = items.crbegin(); it != items.crend(); ++it) {
        if (seen.contains(it->id)) continue;
        seen.insert(it->id);
        if (!firstCheck) fresh.append(it->label);
    }
    if (fresh.isEmpty()) return;

    emit arrived(event);
    if (isEnabled(m_client->userId(), event)) show(event, fresh);
}

void DesktopNotifier::show(const QString &event, const QStringList &labels) {
    const int count = labels.size();
    QString title;
    if (event == "contact_message") {
        title = count == 1 ? tr("New message") : tr("%n new messages", "", count);
    } else if (event == "stock_request") {
        title = count == 1 ? tr("Someone is waiting for a product") : tr("%n new stock requests", "", count);
    } else if (event == "custom_order") {
        title = count == 1 ? tr("New custom order") : tr("%n new custom orders", "", count);
    } else if (event == "job_application") {
        title = count == 1 ? tr("New candidate") : tr("%n new candidates", "", count);
    } else if (event == "online_order") {
        title = count == 1 ? tr("New paid online order") : tr("%n new paid online orders", "", count);
    }

    QStringList shown = labels.mid(0, 3);
    shown.removeAll(QString());
    QString body = shown.join(", ");
    if (count > 3) body += "…";
    if (body.isEmpty()) body = tr("Open the console to see it.");

    m_lastShown = event;
    m_tray->showMessage(title, body, m_tray->icon(), 10000);
    QApplication::alert(m_window); // flashes the taskbar button too
}

void DesktopNotifier::bringWindowUp() {
    if (m_window->isMinimized()) m_window->showNormal();
    m_window->raise();
    m_window->activateWindow();
}

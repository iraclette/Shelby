#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QStringList>

class QSystemTrayIcon;
class QTimer;
class QWidget;
class SupabaseClient;

// Pop-ups in the corner of the screen when something new comes in from the website while the
// console is open (Settings → Notifications). Checks once a minute. The events use the same
// keys as the Telegram alerts: contact_message, stock_request, custom_order, job_application,
// online_order.
class DesktopNotifier : public QObject {
    Q_OBJECT

public:
    DesktopNotifier(SupabaseClient *client, QWidget *window);

    static QStringList events();
    static QString eventName(const QString &event); // "Contact form messages"…, translated
    // The Settings → Modules key of the section that shows the event ("messages"…).
    static QString moduleKey(const QString &event);

    // Per account, on this computer. Everything is on until switched off.
    static bool isEnabled(const QString &userId, const QString &event);
    static void setEnabled(const QString &userId, const QString &event, bool enabled);

signals:
    void arrived(const QString &event);     // something new came in: its page can reload
    void openRequested(const QString &event); // a pop-up was clicked

private:
    struct Item {
        QString id;
        QString label; // who or what, for the pop-up text
    };

    void check();
    void checkEvent(const QString &event);
    void handle(const QString &event, const QVector<Item> &items);
    void show(const QString &event, const QStringList &labels);
    void bringWindowUp();

    SupabaseClient *m_client;
    QWidget *m_window;
    QSystemTrayIcon *m_tray;
    QTimer *m_timer;
    // Each check reads the latest rows and pops up the ones not seen before. The first check
    // only notes what's already there, so opening the console doesn't pop up old things.
    QHash<QString, QSet<QString>> m_seen; // event -> ids seen
    QSet<QString> m_inFlight;
    QString m_lastShown; // the event the latest pop-up is about
};

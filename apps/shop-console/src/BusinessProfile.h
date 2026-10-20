#pragma once

#include <QObject>
#include <QPixmap>
#include <QString>
#include <QList>
#include <QStringList>

class QNetworkAccessManager;

// The business's own details (Settings → Business, the business_settings table): name and
// logo for the login screen and sidebar, how money is written, and the time zone that
// decides what "today" is. Cached on this computer so the login screen and offline starts
// have them too; refreshed from Supabase at start-up and after every save.
struct BusinessSettings {
    QString name = "Shop Console"; // until the first fetch on a brand-new computer
    QString logoPath; // in the branding bucket; empty = the name's first letter instead
    QString currencyCode = "GEL";
    QString currencySymbol = "₾";
    QString currencyStyle = "code_before"; // code_before ("GEL 12.50"), symbol_before ("₾12.50"), symbol_after ("12.50 ₾")
    QString timezone = "Asia/Tbilisi";
    // Settings → Modules: console sections this business switched off (see kOptionalModules).
    QStringList disabledModules;
    // Settings → Checkout.
    QStringList paymentMethods{"cash", "card"}; // what the till offers, at least one
    QString defaultPaymentMethod = "cash";      // selected when the payment window opens
    int defaultLowStockThreshold = 0;           // for new products; 0 = off
    QString skuPrefix = "INT";                  // codes for products without a barcode: INT-XXXXXX
    QList<double> quickCashAmounts{5, 10, 20, 50, 100, 200}; // one-tap buttons in the cash payment window
};

// The console sections a business can switch off, in sidebar order: key, name, what it's for.
// The names are marked for translation and translated where they're shown (QObject::tr).
struct OptionalModule {
    const char *key;
    const char *name;
    const char *description;
};
inline constexpr OptionalModule kOptionalModules[] = {
    {"online_orders", QT_TR_NOOP("Online Orders"), QT_TR_NOOP("Orders paid on the website, to prepare and hand over.")},
    {"salary", QT_TR_NOOP("Salary"), QT_TR_NOOP("Track and pay employees' salaries.")},
    {"job_applications", QT_TR_NOOP("Job Applications"), QT_TR_NOOP("Applications sent from the website's \"work with us\" page.")},
    {"custom_orders", QT_TR_NOOP("Custom Orders"), QT_TR_NOOP("Made-to-order requests from the website (handmade leather).")},
    {"messages", QT_TR_NOOP("Messages"), QT_TR_NOOP("Messages sent through the website's contact form.")},
    {"stock_requests", QT_TR_NOOP("Stock Requests"), QT_TR_NOOP("\"Let me know when it's back\" requests from the website.")},
};

class BusinessProfile : public QObject {
    Q_OBJECT

public:
    static BusinessProfile *instance();

    const BusinessSettings &settings() const { return m_settings; }
    // Null when the business has no logo (or it hasn't downloaded yet).
    QPixmap logo() const { return m_logo; }
    // The logo filling a circle of `size` pixels (crisp on high-DPI screens); null without a logo.
    QPixmap roundLogo(int size, qreal devicePixelRatio) const;

    // New settings from Supabase. logoUrl is the logo's public URL (empty when there's none).
    void apply(const BusinessSettings &settings, const QString &logoUrl);

signals:
    void changed();

private:
    BusinessProfile();
    void saveCache() const;
    void loadLogo(const QString &logoUrl);

    BusinessSettings m_settings;
    QPixmap m_logo;
    QNetworkAccessManager *m_network = nullptr;
};

// An amount as the business writes it: "GEL 12.50", "₾12.50" or "12.50 ₾".
QString money(double amount);
// For spin boxes, which show the currency around the number: e.g. "GEL " and "", or "" and " ₾".
QString moneyPrefix();
QString moneySuffix();
// The currency on its own, for labels like "Sales (GEL)".
QString currencyLabel();

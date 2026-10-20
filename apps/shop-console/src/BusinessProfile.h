#pragma once

#include <QObject>
#include <QPixmap>
#include <QString>

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

#include "BusinessProfile.h"

#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QStandardPaths>

namespace {
QSettings cache() {
    return QSettings("Shelby", "ShopConsole");
}

// The downloaded logo, so it shows on the login screen without waiting for the network.
QString logoCacheFile() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return QDir(dir).filePath("business-logo");
}
} // namespace

BusinessProfile *BusinessProfile::instance() {
    static BusinessProfile *profile = new BusinessProfile;
    return profile;
}

BusinessProfile::BusinessProfile() {
    QSettings s = cache();
    s.beginGroup("business");
    m_settings.name = s.value("name", m_settings.name).toString();
    m_settings.logoPath = s.value("logoPath").toString();
    m_settings.currencyCode = s.value("currencyCode", m_settings.currencyCode).toString();
    m_settings.currencySymbol = s.value("currencySymbol", m_settings.currencySymbol).toString();
    m_settings.currencyStyle = s.value("currencyStyle", m_settings.currencyStyle).toString();
    m_settings.timezone = s.value("timezone", m_settings.timezone).toString();
    s.endGroup();
    if (!m_settings.logoPath.isEmpty()) m_logo.load(logoCacheFile());
}

void BusinessProfile::apply(const BusinessSettings &settings, const QString &logoUrl) {
    const bool logoChanged = settings.logoPath != m_settings.logoPath || (m_logo.isNull() && !logoUrl.isEmpty());
    m_settings = settings;
    saveCache();
    if (logoChanged) {
        m_logo = QPixmap();
        QFile::remove(logoCacheFile());
        if (!logoUrl.isEmpty()) loadLogo(logoUrl);
    }
    emit changed();
}

QPixmap BusinessProfile::roundLogo(int size, qreal devicePixelRatio) const {
    if (m_logo.isNull()) return {};
    const int pixels = qRound(size * devicePixelRatio);
    // Fill the circle (crop the overflow) rather than letterbox a non-square logo.
    const QPixmap scaled = m_logo.scaled(pixels, pixels, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPixmap round(pixels, pixels);
    round.fill(Qt::transparent);
    QPainter painter(&round);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath circle;
    circle.addEllipse(0, 0, pixels, pixels);
    painter.setClipPath(circle);
    painter.drawPixmap((pixels - scaled.width()) / 2, (pixels - scaled.height()) / 2, scaled);
    painter.end();
    round.setDevicePixelRatio(devicePixelRatio);
    return round;
}

void BusinessProfile::saveCache() const {
    QSettings s = cache();
    s.beginGroup("business");
    s.setValue("name", m_settings.name);
    s.setValue("logoPath", m_settings.logoPath);
    s.setValue("currencyCode", m_settings.currencyCode);
    s.setValue("currencySymbol", m_settings.currencySymbol);
    s.setValue("currencyStyle", m_settings.currencyStyle);
    s.setValue("timezone", m_settings.timezone);
    s.endGroup();
}

void BusinessProfile::loadLogo(const QString &logoUrl) {
    if (!m_network) m_network = new QNetworkAccessManager(this);
    const QString expectedPath = m_settings.logoPath;
    QNetworkReply *reply = m_network->get(QNetworkRequest(QUrl(logoUrl)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, expectedPath]() {
        reply->deleteLater();
        // Ignore a download that finished after the logo was changed again.
        if (reply->error() != QNetworkReply::NoError || expectedPath != m_settings.logoPath) return;
        const QByteArray bytes = reply->readAll();
        QPixmap logo;
        if (!logo.loadFromData(bytes)) return;
        m_logo = logo;
        QFile file(logoCacheFile());
        if (file.open(QIODevice::WriteOnly)) file.write(bytes);
        emit changed();
    });
}

QString money(double amount) {
    const BusinessSettings &s = BusinessProfile::instance()->settings();
    const QString number = QString::number(amount, 'f', 2);
    if (s.currencyStyle == "symbol_before") return s.currencySymbol + number;
    if (s.currencyStyle == "symbol_after") return number + " " + s.currencySymbol;
    return s.currencyCode + " " + number;
}

QString moneyPrefix() {
    const BusinessSettings &s = BusinessProfile::instance()->settings();
    if (s.currencyStyle == "symbol_before") return s.currencySymbol;
    if (s.currencyStyle == "symbol_after") return {};
    return s.currencyCode + " ";
}

QString moneySuffix() {
    const BusinessSettings &s = BusinessProfile::instance()->settings();
    return s.currencyStyle == "symbol_after" ? " " + s.currencySymbol : QString();
}

QString currencyLabel() {
    const BusinessSettings &s = BusinessProfile::instance()->settings();
    return s.currencyStyle == "code_before" ? s.currencyCode : s.currencySymbol;
}

#include "Translations.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QSettings>

namespace translations {
namespace {
QSettings localSettings() {
    return QSettings("Shelby", "ShopConsole");
}

QString g_startupLanguage = "en";
} // namespace

QStringList languageCodes() {
    return {"en", "ka", "ru"};
}

QString languageName(const QString &code) {
    if (code == "ka") return QString::fromUtf8("ქართული");
    if (code == "ru") return QString::fromUtf8("Русский");
    return "English";
}

QString currentLanguage() {
    const QString code = localSettings().value("appearance/language", "en").toString();
    return languageCodes().contains(code) ? code : "en";
}

void setLanguage(const QString &code) {
    localSettings().setValue("appearance/language", languageCodes().contains(code) ? code : "en");
}

QString startupLanguage() {
    return g_startupLanguage;
}

void install() {
    g_startupLanguage = currentLanguage();
    QLocale::setDefault(QLocale(g_startupLanguage == "ka"   ? QLocale::Georgian
                                : g_startupLanguage == "ru" ? QLocale::Russian
                                                            : QLocale::English));
    // English too: its file holds the plural forms ("%n sale(s)" -> "1 sale" / "2 sales").
    static JsonTranslator translator;
    if (translator.loadLanguage(g_startupLanguage)) QCoreApplication::installTranslator(&translator);
}

} // namespace translations

bool JsonTranslator::loadLanguage(const QString &code) {
    QFile file(":/translations/" + code + ".json");
    if (!file.open(QIODevice::ReadOnly)) return false;
    const QJsonObject entries = QJsonDocument::fromJson(file.readAll()).object();
    m_language = code;
    m_entries.clear();
    for (auto it = entries.begin(); it != entries.end(); ++it) m_entries.insert(it.key(), it.value());
    return !m_entries.isEmpty();
}

QString JsonTranslator::translate(const char *, const char *sourceText, const char *, int n) const {
    const QJsonValue value = m_entries.value(QString::fromUtf8(sourceText));
    if (value.isString()) return value.toString();
    if (!value.isObject()) return {}; // not translated: Qt shows the English source

    const QJsonObject forms = value.toObject();
    QString form = "other";
    if (m_language == "ru") {
        // Russian: 1, 21, 31… / 2–4, 22–24… / everything else.
        const int last = n % 10;
        const int lastTwo = n % 100;
        if (last == 1 && lastTwo != 11) form = "one";
        else if (last >= 2 && last <= 4 && (lastTwo < 12 || lastTwo > 14)) form = "few";
        else form = "many";
    } else if (m_language == "en") {
        form = n == 1 ? "one" : "other";
    }
    // Georgian keeps the noun singular after a number, so it only needs "other".
    return forms.value(form).toString(forms.value("other").toString());
}

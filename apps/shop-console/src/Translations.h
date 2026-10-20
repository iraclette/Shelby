#pragma once

#include <QHash>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <QTranslator>

// The console's languages. Text in the code is English, wrapped in tr(); the other
// languages live in resources/translations/<code>.json as {"English text": "translation"}.
// A plural (a tr() call with a count, "%n …") maps to {"one": …, "few": …, "many": …,
// "other": …} with the forms that language uses. Missing translations fall back to English.
namespace translations {

// "en", "ka", "ru"
QStringList languageCodes();
QString languageName(const QString &code); // in the language itself: English, ქართული, Русский

// This computer's language (Settings → Language). Read at start-up; a change needs a restart.
QString currentLanguage();
void setLanguage(const QString &code);
QString startupLanguage(); // what this run uses

// Installs the translator and sets the default locale (dates, numbers). Call once, after
// QApplication exists and before any window is created.
void install();

} // namespace translations

class JsonTranslator : public QTranslator {
public:
    bool loadLanguage(const QString &code);
    bool isEmpty() const override { return m_entries.isEmpty(); }
    QString translate(const char *context, const char *sourceText, const char *disambiguation = nullptr,
                      int n = -1) const override;

private:
    QString m_language;
    QHash<QString, QJsonValue> m_entries;
};

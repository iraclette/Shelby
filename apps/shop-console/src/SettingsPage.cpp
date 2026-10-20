#include "SettingsPage.h"
#include "BusinessProfile.h"
#include "Config.h"
#include "DesktopNotifier.h"
#include "ProductDialog.h"
#include "SessionStore.h"
#include "SupabaseClient.h"
#include "Theme.h"
#include "Translations.h"

#include <QLocale>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QJsonArray>
#include <QCompleter>
#include <QDateTime>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QProcess>
#include <QRandomGenerator>
#include <QRegularExpressionValidator>
#include <QSpinBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimeZone>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>

#include "qrcodegen.hpp"

namespace {
// Common currencies and their symbols; anything else can be typed in.
const QList<QPair<QString, QString>> kCurrencies{
    {"GEL", "₾"}, {"USD", "$"}, {"EUR", "€"}, {"GBP", "£"}, {"TRY", "₺"}, {"AMD", "֏"}, {"AZN", "₼"},
    {"UAH", "₴"}, {"RUB", "₽"}, {"KZT", "₸"}, {"PLN", "zł"}, {"CHF", "Fr"}, {"CAD", "$"}, {"AUD", "$"},
};

const QStringList kStyles{"code_before", "symbol_before", "symbol_after"};

QWidget *sectionHeader(const QString &title, const QString &subtitle, QWidget *parent) {
    auto *header = new QWidget(parent);
    auto *layout = new QVBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 8);
    layout->setSpacing(2);
    auto *titleLabel = new QLabel(title, header);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: 600;");
    auto *subtitleLabel = new QLabel(subtitle, header);
    subtitleLabel->setObjectName("brandSub");
    subtitleLabel->setWordWrap(true);
    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    return header;
}
} // namespace

SettingsPage::SettingsPage(SupabaseClient *client, QWidget *softwareSection, QWidget *parent) : QWidget(parent) {
    auto *sections = new QListWidget(this);
    sections->setObjectName("settingsSections");
    sections->setFixedWidth(170);
    sections->setFocusPolicy(Qt::NoFocus);
    auto *stack = new QStackedWidget(this);

    auto addSection = [&](const QString &name, QWidget *widget) {
        sections->addItem(name);
        stack->addWidget(widget);
    };

    auto *business = new BusinessSettingsSection(client, this);
    connect(business, &BusinessSettingsSection::statusMessage, this, &SettingsPage::statusMessage);
    addSection(tr("Business"), business);
    addSection(tr("Modules"), new ModulesSettingsSection(client, this));
    addSection(tr("Checkout"), new CheckoutSettingsSection(client, this));
    addSection(tr("Appearance"), new AppearanceSettingsSection(client, this));
    addSection(tr("Language"), new LanguageSettingsSection(client, this));
    addSection(tr("Notifications"), new NotificationsSettingsSection(client, this));
    addSection(tr("Software"), softwareSection);

    connect(sections, &QListWidget::currentRowChanged, stack, &QStackedWidget::setCurrentIndex);
    sections->setCurrentRow(0);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(24);
    layout->addWidget(sections);
    layout->addWidget(stack, 1);
}

BusinessSettingsSection::BusinessSettingsSection(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    m_logoPreview = new QLabel(this);
    m_logoPreview->setObjectName("brandMark");
    m_logoPreview->setFixedSize(64, 64);
    m_logoPreview->setAlignment(Qt::AlignCenter);
    auto *uploadLogoButton = new QPushButton(tr("Upload logo…"), this);
    m_removeLogoButton = new QPushButton(tr("Remove"), this);
    auto *logoButtons = new QVBoxLayout;
    logoButtons->addWidget(uploadLogoButton);
    logoButtons->addWidget(m_removeLogoButton);
    logoButtons->addStretch();
    auto *logoRow = new QHBoxLayout;
    logoRow->addWidget(m_logoPreview);
    logoRow->addLayout(logoButtons);
    logoRow->addStretch();

    m_name = new QLineEdit(this);
    m_name->setMaxLength(60);

    m_currency = new QComboBox(this);
    m_currency->setEditable(true);
    m_currency->setInsertPolicy(QComboBox::NoInsert);
    for (const auto &[code, symbol] : kCurrencies) m_currency->addItem(QString("%1  %2").arg(code, symbol), code);
    m_symbol = new QLineEdit(this);
    m_symbol->setMaxLength(4);
    m_symbol->setFixedWidth(70);
    auto *currencyRow = new QHBoxLayout;
    currencyRow->addWidget(m_currency, 1);
    currencyRow->addWidget(new QLabel(tr("Symbol"), this));
    currencyRow->addWidget(m_symbol);

    m_style = new QComboBox(this);

    m_timezone = new QComboBox(this);
    m_timezone->setEditable(true);
    m_timezone->setInsertPolicy(QComboBox::NoInsert);
    QStringList zones;
    for (const QByteArray &id : QTimeZone::availableTimeZoneIds()) zones << QString::fromUtf8(id);
    zones.sort();
    m_timezone->addItems(zones);
    m_timezone->completer()->setFilterMode(Qt::MatchContains);
    m_timezone->completer()->setCaseSensitivity(Qt::CaseInsensitive);
    m_timePreview = new QLabel(this);
    m_timePreview->setObjectName("brandSub");

    auto *form = new QFormLayout;
    form->setSpacing(12);
    form->addRow(tr("Logo"), logoRow);
    form->addRow(tr("Business name"), m_name);
    form->addRow(tr("Currency"), currencyRow);
    form->addRow(tr("Amounts look like"), m_style);
    form->addRow(tr("Time zone"), m_timezone);
    form->addRow("", m_timePreview);

    m_saveButton = new QPushButton(tr("Save"), this);
    m_saveButton->setObjectName("primaryButton");
    m_status = new QLabel(this);
    m_status->setObjectName("brandSub");
    m_status->setWordWrap(true);
    auto *saveRow = new QHBoxLayout;
    saveRow->addWidget(m_saveButton);
    saveRow->addWidget(m_status, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(sectionHeader(
        tr("Business"), tr("Shared by every till and shop. Only owners and admins can change these."), this));
    layout->addLayout(form);
    layout->addSpacing(12);
    layout->addLayout(saveRow);
    layout->addStretch();

    connect(uploadLogoButton, &QPushButton::clicked, this, &BusinessSettingsSection::chooseLogo);
    connect(m_removeLogoButton, &QPushButton::clicked, this, &BusinessSettingsSection::removeLogo);
    connect(m_saveButton, &QPushButton::clicked, this, &BusinessSettingsSection::save);
    connect(m_name, &QLineEdit::textChanged, this, &BusinessSettingsSection::refreshLogo);

    // Picking a known currency fills in its symbol; a typed code keeps whatever symbol is there.
    connect(m_currency, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0 && index < kCurrencies.size()) m_symbol->setText(kCurrencies[index].second);
        refreshPreviews();
    });
    connect(m_currency, &QComboBox::currentTextChanged, this, &BusinessSettingsSection::refreshPreviews);
    connect(m_symbol, &QLineEdit::textChanged, this, &BusinessSettingsSection::refreshPreviews);
    connect(m_timezone, &QComboBox::currentTextChanged, this, &BusinessSettingsSection::refreshPreviews);

    connect(m_client, &SupabaseClient::businessSettingsSaved, this, [this]() {
        if (!m_saving) return;
        m_saving = false;
        m_saveButton->setEnabled(true);
        m_client->deleteBusinessLogoFile(m_oldLogoPath);
        m_oldLogoPath.clear();
        m_status->setText(tr("Saved. Other tills pick it up the next time they start."));
    });
    connect(m_client, &SupabaseClient::businessSettingsSaveFailed, this, [this](const QString &message) {
        if (!m_saving) return;
        m_saving = false;
        m_saveButton->setEnabled(true);
        m_status->setText(tr("Couldn't save: ") + message);
    });
    connect(m_client, &SupabaseClient::businessLogoUploaded, this, [this](const QString &storagePath) {
        m_oldLogoPath = BusinessProfile::instance()->settings().logoPath;
        m_saving = true;
        m_client->updateBusinessSettings({{"logo_path", storagePath}});
    });
    connect(m_client, &SupabaseClient::businessLogoUploadFailed, this, [this](const QString &message) {
        m_saving = false;
        m_saveButton->setEnabled(true);
        m_status->setText(tr("Couldn't upload the logo: ") + message);
    });

    connect(BusinessProfile::instance(), &BusinessProfile::changed, this, &BusinessSettingsSection::refreshLogo);
    loadFromProfile();
}

void BusinessSettingsSection::loadFromProfile() {
    const BusinessSettings &s = BusinessProfile::instance()->settings();
    m_name->setText(s.name);
    const int known = m_currency->findData(s.currencyCode);
    if (known >= 0) {
        m_currency->setCurrentIndex(known);
    } else {
        m_currency->setEditText(s.currencyCode);
    }
    m_symbol->setText(s.currencySymbol);
    m_timezone->setCurrentText(s.timezone);
    refreshPreviews();
    m_style->setCurrentIndex(qMax(0, kStyles.indexOf(s.currencyStyle)));
    refreshLogo();
}

// The logo, or the name's first letter like the sidebar shows when there's none.
void BusinessSettingsSection::refreshLogo() {
    const QPixmap logo = BusinessProfile::instance()->logo();
    if (!logo.isNull()) {
        m_logoPreview->setPixmap(logo.scaled(m_logoPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_logoPreview->setPixmap({});
        m_logoPreview->setText(m_name->text().trimmed().left(1).toUpper());
    }
    m_removeLogoButton->setEnabled(!BusinessProfile::instance()->settings().logoPath.isEmpty());
}

// The currency code typed or picked: "GEL" from "GEL  ₾".
static QString currencyCode(const QComboBox *combo) {
    return combo->currentText().section(' ', 0, 0).trimmed().toUpper();
}

void BusinessSettingsSection::refreshPreviews() {
    const QString code = currencyCode(m_currency);
    const QString symbol = m_symbol->text().trimmed();
    const int style = m_style->currentIndex();
    m_style->blockSignals(true);
    m_style->clear();
    m_style->addItem(code + " 12.50");
    m_style->addItem(symbol + "12.50");
    m_style->addItem("12.50 " + symbol);
    m_style->setCurrentIndex(qMax(0, style));
    m_style->blockSignals(false);

    const QTimeZone zone(m_timezone->currentText().toUtf8());
    m_timePreview->setText(zone.isValid()
                               ? tr("It's %1 there now.").arg(QLocale().toString(QDateTime::currentDateTimeUtc().toTimeZone(zone), "dddd HH:mm"))
                               : tr("Pick a time zone from the list."));
}

void BusinessSettingsSection::save() {
    const QString name = m_name->text().trimmed();
    const QString code = currencyCode(m_currency);
    const QString symbol = m_symbol->text().trimmed();
    const QString timezone = m_timezone->currentText().trimmed();
    if (name.isEmpty()) return m_status->setText(tr("The business needs a name."));
    if (code.size() < 2 || code.size() > 4) return m_status->setText(tr("Currency should be a code like GEL or USD."));
    if (symbol.isEmpty()) return m_status->setText(tr("Give the currency a symbol (it can be the code itself)."));
    if (!QTimeZone(timezone.toUtf8()).isValid()) return m_status->setText(tr("Pick a time zone from the list."));

    m_saving = true;
    m_saveButton->setEnabled(false);
    m_status->setText(tr("Saving…"));
    m_client->updateBusinessSettings({
        {"name", name},
        {"currency_code", code},
        {"currency_symbol", symbol},
        {"currency_style", kStyles.value(m_style->currentIndex(), "code_before")},
        {"timezone", timezone},
    });
}

void BusinessSettingsSection::chooseLogo() {
    const QString path =
        QFileDialog::getOpenFileName(this, tr("Choose a logo"), QString(), tr("Images (*.png *.jpg *.jpeg *.webp)"));
    if (path.isEmpty()) return;
    m_saving = true;
    m_saveButton->setEnabled(false);
    m_status->setText(tr("Uploading the logo…"));
    m_client->uploadBusinessLogo(path);
}

void BusinessSettingsSection::removeLogo() {
    m_oldLogoPath = BusinessProfile::instance()->settings().logoPath;
    m_saving = true;
    m_saveButton->setEnabled(false);
    m_status->setText(tr("Removing the logo…"));
    m_client->updateBusinessSettings({{"logo_path", QJsonValue(QJsonValue::Null)}});
}

ModulesSettingsSection::ModulesSettingsSection(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(sectionHeader(
        tr("Modules"),
        tr("Switch off the parts of the console your business doesn't use; they disappear from the sidebar on "
        "every till. Nothing is deleted, and switching one back on brings everything back."),
        this));

    for (const OptionalModule &module : kOptionalModules) {
        auto *toggle = new QCheckBox(QObject::tr(module.name), this);
        toggle->setStyleSheet("font-weight: 600;");
        auto *description = new QLabel(QObject::tr(module.description), this);
        description->setObjectName("brandSub");
        description->setContentsMargins(26, 0, 0, 6);
        layout->addWidget(toggle);
        layout->addWidget(description);
        m_switches.append({module.key, toggle});
    }

    m_saveButton = new QPushButton(tr("Save"), this);
    m_saveButton->setObjectName("primaryButton");
    m_status = new QLabel(this);
    m_status->setObjectName("brandSub");
    m_status->setWordWrap(true);
    auto *saveRow = new QHBoxLayout;
    saveRow->addWidget(m_saveButton);
    saveRow->addWidget(m_status, 1);
    layout->addSpacing(12);
    layout->addLayout(saveRow);
    layout->addStretch();

    connect(m_saveButton, &QPushButton::clicked, this, &ModulesSettingsSection::save);
    connect(m_client, &SupabaseClient::businessSettingsSaved, this, [this]() {
        if (!m_saving) return;
        m_saving = false;
        m_saveButton->setEnabled(true);
        m_status->setText(tr("Saved. Other tills pick it up the next time they start."));
    });
    connect(m_client, &SupabaseClient::businessSettingsSaveFailed, this, [this](const QString &message) {
        if (!m_saving) return;
        m_saving = false;
        m_saveButton->setEnabled(true);
        m_status->setText(tr("Couldn't save: ") + message);
    });
    connect(BusinessProfile::instance(), &BusinessProfile::changed, this, [this]() {
        if (!m_saving) loadFromProfile();
    });
    loadFromProfile();
}

void ModulesSettingsSection::loadFromProfile() {
    const QStringList disabled = BusinessProfile::instance()->settings().disabledModules;
    for (const auto &[key, toggle] : m_switches) toggle->setChecked(!disabled.contains(key));
}

void ModulesSettingsSection::save() {
    QJsonArray disabled;
    for (const auto &[key, toggle] : m_switches) {
        if (!toggle->isChecked()) disabled.append(key);
    }
    m_saving = true;
    m_saveButton->setEnabled(false);
    m_status->setText(tr("Saving…"));
    m_client->updateBusinessSettings({{"disabled_modules", disabled}});
}

namespace {
// A theme's colours as a little strip: background, panel, accent, text.
QIcon themeSwatch(const QString &themeId) {
    const theme::Palette p = theme::themePalette(themeId);
    QPixmap strip(88, 30);
    strip.fill(p.window);
    QPainter painter(&strip);
    painter.setPen(Qt::NoPen);
    painter.setBrush(p.surface);
    painter.drawRoundedRect(6, 6, 34, 18, 3, 3);
    painter.setBrush(p.accent);
    painter.drawRoundedRect(46, 6, 16, 18, 3, 3);
    painter.setBrush(p.text);
    painter.drawRoundedRect(68, 12, 14, 6, 2, 2);
    painter.setPen(p.border);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(0, 0, strip.width() - 1, strip.height() - 1);
    return QIcon(strip);
}

const QList<QColor> kAccentPresets{
    QColor("#4f8cff"), QColor("#c8963e"), QColor("#22c55e"), QColor("#14b8a6"),
    QColor("#a855f7"), QColor("#ec4899"), QColor("#ef4444"), QColor("#a3161c"),
};

// Relaunch, signed in as the same person (the same hand-over the updater uses).
void restartApp(SupabaseClient *client) {
    if (!client->refreshToken().isEmpty()) {
        sessionstore::setRefreshToken(client->refreshToken());
        sessionstore::setResumeAfterUpdate(true);
    }
    QProcess::startDetached(QCoreApplication::applicationFilePath(), {});
    qApp->quit();
}
} // namespace

AppearanceSettingsSection::AppearanceSettingsSection(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(sectionHeader(
        tr("Appearance"), tr("How the console looks on this computer. Changes show straight away."), this));

    auto *themeRow = new QHBoxLayout;
    auto *themeGroup = new QButtonGroup(this);
    for (const QString &id : theme::themeIds()) {
        auto *button = new QPushButton(theme::themeName(id), this);
        button->setObjectName("categoryChip");
        button->setCheckable(true);
        button->setIcon(themeSwatch(id));
        button->setIconSize(QSize(88, 30));
        button->setMinimumHeight(48);
        themeGroup->addButton(button);
        themeRow->addWidget(button);
        m_themeButtons.append({id, button});
        // Switching theme keeps a custom accent; "Theme default" goes back to the theme's own.
        connect(button, &QPushButton::clicked, this, [this, id]() {
            theme::setTheme(id, theme::customAccent());
            refresh();
        });
    }
    themeRow->addStretch();

    auto *accentRow = new QHBoxLayout;
    accentRow->setSpacing(8);
    m_defaultAccentButton = new QPushButton(tr("Theme default"), this);
    m_defaultAccentButton->setObjectName("categoryChip");
    m_defaultAccentButton->setCheckable(true);
    accentRow->addWidget(m_defaultAccentButton);
    connect(m_defaultAccentButton, &QPushButton::clicked, this, [this]() { setAccent({}); });
    for (const QColor &color : kAccentPresets) {
        auto *swatch = new QPushButton(this);
        swatch->setFixedSize(30, 30);
        swatch->setToolTip(color.name());
        accentRow->addWidget(swatch);
        m_accentButtons.append({color, swatch});
        connect(swatch, &QPushButton::clicked, this, [this, color]() { setAccent(color); });
    }
    m_customAccentButton = new QPushButton(tr("Custom…"), this);
    accentRow->addWidget(m_customAccentButton);
    connect(m_customAccentButton, &QPushButton::clicked, this, [this]() {
        const QColor start = theme::customAccent().isValid() ? theme::customAccent() : theme::palette().accent;
        const QColor picked = QColorDialog::getColor(start, this, tr("Accent colour"));
        if (picked.isValid()) setAccent(picked);
    });
    accentRow->addStretch();

    m_scale = new QComboBox(this);
    for (int percent : {90, 100, 110, 125, 150}) m_scale->addItem(QString("%1%").arg(percent), percent);
    m_scaleNote = new QLabel(this);
    m_scaleNote->setObjectName("hint");
    m_restartButton = new QPushButton(tr("Restart now"), this);
    auto *scaleRow = new QHBoxLayout;
    scaleRow->addWidget(m_scale);
    scaleRow->addWidget(m_scaleNote);
    scaleRow->addWidget(m_restartButton);
    scaleRow->addStretch();
    connect(m_scale, &QComboBox::activated, this, [this]() {
        theme::setInterfaceScale(m_scale->currentData().toInt());
        refresh();
    });
    connect(m_restartButton, &QPushButton::clicked, this, [this]() { restartApp(m_client); });

    auto *form = new QFormLayout;
    form->setSpacing(14);
    form->addRow(tr("Theme"), themeRow);
    form->addRow(tr("Accent colour"), accentRow);
    form->addRow(tr("Interface size"), scaleRow);
    layout->addLayout(form);
    layout->addStretch();

    refresh();
}

void AppearanceSettingsSection::setAccent(const QColor &accent) {
    theme::setTheme(theme::currentThemeId(), accent);
    refresh();
}

void AppearanceSettingsSection::refresh() {
    for (const auto &[id, button] : m_themeButtons) button->setChecked(id == theme::currentThemeId());

    const QColor custom = theme::customAccent();
    m_defaultAccentButton->setChecked(!custom.isValid());
    bool presetPicked = false;
    for (const auto &[color, swatch] : m_accentButtons) {
        const bool picked = custom.isValid() && custom.name() == color.name();
        presetPicked = presetPicked || picked;
        // The picked one gets a ring in the text colour.
        swatch->setStyleSheet(QString("background-color: %1; border-radius: 15px; border: 2px solid %2;")
                                  .arg(color.name(), picked ? theme::hex(theme::palette().textStrong) : "transparent"));
    }
    m_customAccentButton->setText(custom.isValid() && !presetPicked ? tr("Custom: %1").arg(custom.name()) : tr("Custom…"));

    m_scale->setCurrentIndex(qMax(0, m_scale->findData(theme::interfaceScale())));
    const bool needsRestart = theme::interfaceScale() != theme::startupInterfaceScale();
    m_scaleNote->setText(needsRestart ? tr("Takes effect after a restart.") : "");
    m_restartButton->setVisible(needsRestart);
}

LanguageSettingsSection::LanguageSettingsSection(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(sectionHeader(tr("Language"), tr("The console's language on this computer."), this));

    // Each language named in itself, so whoever needs it can find it.
    auto *row = new QHBoxLayout;
    auto *group = new QButtonGroup(this);
    for (const QString &code : translations::languageCodes()) {
        auto *button = new QPushButton(translations::languageName(code), this);
        button->setObjectName("categoryChip");
        button->setCheckable(true);
        button->setMinimumHeight(40);
        group->addButton(button);
        row->addWidget(button);
        m_languageButtons.append({code, button});
        connect(button, &QPushButton::clicked, this, [this, code]() {
            translations::setLanguage(code);
            refresh();
        });
    }
    row->addStretch();
    layout->addLayout(row);

    m_note = new QLabel(this);
    m_note->setObjectName("hint");
    m_restartButton = new QPushButton(tr("Restart now"), this);
    auto *restartRow = new QHBoxLayout;
    restartRow->addWidget(m_note);
    restartRow->addWidget(m_restartButton);
    restartRow->addStretch();
    layout->addSpacing(8);
    layout->addLayout(restartRow);
    layout->addStretch();
    connect(m_restartButton, &QPushButton::clicked, this, [this]() { restartApp(m_client); });

    refresh();
}

void LanguageSettingsSection::refresh() {
    for (const auto &[code, button] : m_languageButtons) button->setChecked(code == translations::currentLanguage());
    const bool needsRestart = translations::currentLanguage() != translations::startupLanguage();
    m_note->setText(needsRestart ? tr("Takes effect after a restart.") : QString());
    m_restartButton->setVisible(needsRestart);
}

namespace {
// The website's bot drops a link code after 30 minutes (storefront /api/telegram/webhook).
constexpr int kLinkCodeLifetimeMs = 30 * 60 * 1000;

// 24 URL-safe characters: Telegram's /start parameter allows A-Z, a-z, 0-9, _ and -.
QString randomLinkCode() {
    QByteArray bytes(18, Qt::Uninitialized);
    for (char &byte : bytes) byte = char(QRandomGenerator::system()->bounded(256));
    return QString::fromLatin1(bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

// Black on white with the quiet border scanners expect, whatever the theme.
QPixmap qrPixmap(const QString &text, int size, qreal devicePixelRatio) {
    const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(text.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM);
    const int modules = qr.getSize() + 8;
    QPixmap pixmap(QSize(size, size) * devicePixelRatio);
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    const qreal cell = qreal(size) / modules;
    for (int y = 0; y < qr.getSize(); ++y) {
        for (int x = 0; x < qr.getSize(); ++x) {
            if (qr.getModule(x, y)) painter.drawRect(QRectF((x + 4) * cell, (y + 4) * cell, cell + 0.02, cell + 0.02));
        }
    }
    return pixmap;
}

QLabel *groupTitle(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setStyleSheet("font-weight: 600;");
    return label;
}

QLabel *hintLabel(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setObjectName("brandSub");
    label->setWordWrap(true);
    return label;
}
} // namespace

NotificationsSettingsSection::NotificationsSettingsSection(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client), m_botUsername(Config::load().telegramBotUsername) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(sectionHeader(tr("Notifications"), tr("Alerts for what customers send through the website."), this));

    layout->addWidget(groupTitle(tr("Pop-ups on this computer"), this));
    layout->addWidget(hintLabel(
        tr("Shown in the corner of the screen while the console is open, for your account on this computer."), this));
    for (const QString &event : DesktopNotifier::events()) {
        auto *toggle = new QCheckBox(DesktopNotifier::eventName(event), this);
        toggle->setChecked(DesktopNotifier::isEnabled(m_client->userId(), event));
        connect(toggle, &QCheckBox::toggled, this, [this, event](bool on) {
            DesktopNotifier::setEnabled(m_client->userId(), event, on);
        });
        layout->addWidget(toggle);
    }

    layout->addSpacing(20);
    layout->addWidget(groupTitle(tr("Telegram"), this));
    layout->addWidget(hintLabel(tr("The same alerts on your phone, even when the console is closed."), this));
    if (m_botUsername.isEmpty()) {
        layout->addWidget(hintLabel(tr("Telegram isn't set up for this console."), this));
        layout->addStretch();
        return;
    }

    m_connectButton = new QPushButton(tr("Connect a phone"), this);
    m_connectButton->setObjectName("primaryButton");
    auto *connectRow = new QHBoxLayout;
    connectRow->addWidget(m_connectButton);
    connectRow->addStretch();
    layout->addLayout(connectRow);
    connect(m_connectButton, &QPushButton::clicked, this, &NotificationsSettingsSection::startLinking);

    m_qrPanel = new QWidget(this);
    m_qrPanel->setObjectName("card");
    auto *qrLayout = new QHBoxLayout(m_qrPanel);
    qrLayout->setContentsMargins(16, 16, 16, 16);
    qrLayout->setSpacing(20);
    m_qrCode = new QLabel(m_qrPanel);
    m_qrCode->setFixedSize(200, 200);
    qrLayout->addWidget(m_qrCode, 0, Qt::AlignTop);
    auto *qrText = new QVBoxLayout;
    qrText->setSpacing(8);
    qrText->addWidget(groupTitle(tr("Scan this code with your phone's camera"), m_qrPanel));
    qrText->addWidget(hintLabel(tr("Telegram opens on the %1 bot. Press Start, and this phone gets the alerts. "
                                   "The code works for 30 minutes.")
                                    .arg("@" + m_botUsername),
                                m_qrPanel));
    m_qrLink = new QLabel(m_qrPanel);
    m_qrLink->setObjectName("brandSub");
    m_qrLink->setOpenExternalLinks(true);
    m_qrLink->setWordWrap(true);
    qrText->addWidget(m_qrLink);
    auto *cancelButton = new QPushButton(tr("Cancel"), m_qrPanel);
    auto *cancelRow = new QHBoxLayout;
    cancelRow->addWidget(cancelButton);
    cancelRow->addStretch();
    qrText->addLayout(cancelRow);
    qrText->addStretch();
    qrLayout->addLayout(qrText, 1);
    m_qrPanel->hide();
    layout->addWidget(m_qrPanel);
    connect(theme::notifier(), &theme::Notifier::changed, this, &NotificationsSettingsSection::showQrLink);
    connect(cancelButton, &QPushButton::clicked, this, [this]() {
        stopLinking();
        m_telegramStatus->clear();
    });

    m_telegramStatus = hintLabel({}, this);
    layout->addWidget(m_telegramStatus);

    m_linksLayout = new QVBoxLayout;
    m_linksLayout->setSpacing(10);
    layout->addSpacing(6);
    layout->addLayout(m_linksLayout);
    layout->addStretch();

    m_linkPoll = new QTimer(this);
    m_linkPoll->setInterval(3000);
    connect(m_linkPoll, &QTimer::timeout, this, [this]() {
        if (m_linkStarted.msecsTo(QDateTime::currentDateTimeUtc()) > kLinkCodeLifetimeMs) {
            stopLinking();
            m_telegramStatus->setText(tr("The code ran out. Press Connect a phone for a new one."));
            return;
        }
        loadLinks();
    });

    loadLinks();
}

void NotificationsSettingsSection::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (!m_botUsername.isEmpty()) loadLinks();
}

void NotificationsSettingsSection::startLinking() {
    const QString code = randomLinkCode();
    m_connectButton->setEnabled(false);
    m_telegramStatus->clear();
    const QJsonObject row{{"code", code}, {"profile_id", m_client->userId()}, {"lang", translations::currentLanguage()}};
    m_client->writeRows("POST", "telegram_link_codes", {}, row, [this, code](const QString &error) {
        if (!error.isEmpty()) {
            m_connectButton->setEnabled(true);
            m_telegramStatus->setText(tr("Couldn't start: ") + error);
            return;
        }
        const QString url = QString("https://t.me/%1?start=%2").arg(m_botUsername, code);
        m_qrCode->setPixmap(qrPixmap(url, m_qrCode->width(), devicePixelRatioF()));
        m_qrUrl = url;
        showQrLink();
        m_qrPanel->show();
        m_linkStarted = QDateTime::currentDateTimeUtc();
        m_linkPoll->start();
    });
}

// Rich-text links ignore the stylesheet, so the link gets the accent colour inline.
void NotificationsSettingsSection::showQrLink() {
    if (m_qrUrl.isEmpty()) return;
    const QString text = tr("No camera? <a href=\"%1\">Open the link on this computer</a>.").arg(m_qrUrl);
    m_qrLink->setText(QString(text).replace(
        "<a href=", QString("<a style=\"color:%1;\" href=").arg(theme::hex(theme::palette().accent))));
}

void NotificationsSettingsSection::stopLinking() {
    m_linkPoll->stop();
    m_qrPanel->hide();
    m_qrCode->clear();
    m_qrUrl.clear();
    m_connectButton->setEnabled(true);
}

void NotificationsSettingsSection::loadLinks() {
    QUrlQuery query;
    query.addQueryItem("select", "id,chat_name,events");
    query.addQueryItem("order", "created_at");
    m_client->selectRows("telegram_links", query, [this](const QJsonArray &rows, const QString &error) {
        if (!error.isEmpty()) {
            if (!m_linkPoll->isActive()) m_telegramStatus->setText(tr("Couldn't load linked phones: ") + error);
            return;
        }
        QSet<QString> knownIds;
        for (const TelegramLink &link : m_links) knownIds.insert(link.id);
        QVector<TelegramLink> links;
        QString newChat;
        for (const QJsonValue &value : rows) {
            const QJsonObject row = value.toObject();
            TelegramLink link{row.value("id").toString(), row.value("chat_name").toString(), {}};
            for (const QJsonValue &event : row.value("events").toArray()) link.events.append(event.toString());
            if (!knownIds.contains(link.id)) newChat = link.chatName.isEmpty() ? "Telegram" : link.chatName;
            links.append(link);
        }
        const bool changed = links.size() != m_links.size() || !newChat.isEmpty();
        m_links = links;
        if (m_linkPoll->isActive() && !newChat.isEmpty()) {
            stopLinking();
            m_telegramStatus->setText(tr("Connected: %1. Telegram sent a message to say so.").arg(newChat));
        }
        if (changed || m_linksLayout->isEmpty()) showLinks();
    });
}

void NotificationsSettingsSection::showLinks() {
    while (QLayoutItem *item = m_linksLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    if (m_links.isEmpty()) {
        m_linksLayout->addWidget(hintLabel(tr("No phones connected yet."), this));
        return;
    }
    for (const TelegramLink &link : m_links) {
        auto *card = new QWidget(this);
        card->setObjectName("card");
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 12, 16, 12);
        auto *top = new QHBoxLayout;
        const QString name = link.chatName.isEmpty() ? "Telegram" : link.chatName;
        top->addWidget(groupTitle(name, card), 1);
        auto *disconnectButton = new QPushButton(tr("Disconnect"), card);
        disconnectButton->setObjectName("dangerButton");
        top->addWidget(disconnectButton);
        cardLayout->addLayout(top);
        cardLayout->addWidget(hintLabel(tr("Sends:"), card));
        for (const QString &event : DesktopNotifier::events()) {
            auto *toggle = new QCheckBox(DesktopNotifier::eventName(event), card);
            toggle->setChecked(link.events.contains(event));
            connect(toggle, &QCheckBox::toggled, this,
                    [this, id = link.id, event](bool on) { setLinkEvent(id, event, on); });
            cardLayout->addWidget(toggle);
        }
        connect(disconnectButton, &QPushButton::clicked, this,
                [this, id = link.id, name]() { disconnectLink(id, name); });
        m_linksLayout->addWidget(card);
    }
}

void NotificationsSettingsSection::setLinkEvent(const QString &linkId, const QString &event, bool on) {
    QJsonArray events;
    for (TelegramLink &link : m_links) {
        if (link.id != linkId) continue;
        link.events.removeAll(event);
        if (on) link.events.append(event);
        for (const QString &e : link.events) events.append(e);
    }
    QUrlQuery query;
    query.addQueryItem("id", "eq." + linkId);
    m_client->writeRows("PATCH", "telegram_links", query, QJsonObject{{"events", events}}, [this](const QString &error) {
        if (error.isEmpty()) return;
        m_telegramStatus->setText(tr("Couldn't save: ") + error);
        m_links.clear(); // shows the saved state again
        loadLinks();
    });
}

void NotificationsSettingsSection::disconnectLink(const QString &linkId, const QString &chatName) {
    if (QMessageBox::question(this, tr("Disconnect"), tr("Stop sending alerts to %1 on Telegram?").arg(chatName)) !=
        QMessageBox::Yes) {
        return;
    }
    QUrlQuery query;
    query.addQueryItem("id", "eq." + linkId);
    m_client->writeRows("DELETE", "telegram_links", query, QJsonValue(), [this](const QString &error) {
        m_telegramStatus->setText(error.isEmpty() ? QString() : tr("Couldn't disconnect: ") + error);
        loadLinks();
    });
}

CheckoutSettingsSection::CheckoutSettingsSection(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(sectionHeader(
        tr("Checkout"), tr("How the till takes payment, and what new products start with. Applies to every till."), this));

    m_cash = new QCheckBox(tr("Cash"), this);
    m_card = new QCheckBox(tr("Card"), this);
    auto *methodsRow = new QHBoxLayout;
    methodsRow->addWidget(m_cash);
    methodsRow->addWidget(m_card);
    methodsRow->addStretch();
    m_defaultMethod = new QComboBox(this);
    auto *defaultRow = new QHBoxLayout;
    defaultRow->addWidget(m_defaultMethod);
    defaultRow->addStretch();

    m_lowStock = new QSpinBox(this);
    m_lowStock->setRange(0, 999999);
    m_lowStock->setSpecialValueText(tr("Disabled"));
    auto *lowStockRow = new QHBoxLayout;
    lowStockRow->addWidget(m_lowStock);
    lowStockRow->addStretch();

    m_prefix = new QLineEdit(this);
    m_prefix->setMaxLength(6);
    m_prefix->setFixedWidth(110);
    m_prefix->setValidator(new QRegularExpressionValidator(QRegularExpression("[A-Za-z0-9]{1,6}"), m_prefix));
    m_prefixPreview = new QLabel(this);
    m_prefixPreview->setObjectName("brandSub");
    auto *prefixRow = new QHBoxLayout;
    prefixRow->addWidget(m_prefix);
    prefixRow->addWidget(m_prefixPreview);
    prefixRow->addStretch();

    auto *form = new QFormLayout;
    form->setSpacing(12);
    form->addRow(tr("Payment methods"), methodsRow);
    form->addRow(tr("Selected first"), defaultRow);
    form->addRow(tr("Low stock alert for new products"), lowStockRow);
    form->addRow("", hintLabel(tr("Each product can still have its own; products you already have keep theirs."), this));
    form->addRow(tr("Code prefix"), prefixRow);
    form->addRow("", hintLabel(tr("For products without a barcode, made here and on the website. Existing codes stay as "
                                  "they are."),
                               this));
    layout->addLayout(form);

    m_saveButton = new QPushButton(tr("Save"), this);
    m_saveButton->setObjectName("primaryButton");
    m_status = new QLabel(this);
    m_status->setObjectName("brandSub");
    m_status->setWordWrap(true);
    auto *saveRow = new QHBoxLayout;
    saveRow->addWidget(m_saveButton);
    saveRow->addWidget(m_status, 1);
    layout->addSpacing(12);
    layout->addLayout(saveRow);
    layout->addStretch();

    // At least one method stays ticked: unticking the last one ticks the other.
    connect(m_cash, &QCheckBox::toggled, this, [this](bool on) {
        if (!on && !m_card->isChecked()) m_card->setChecked(true);
        refreshDefaultMethods();
    });
    connect(m_card, &QCheckBox::toggled, this, [this](bool on) {
        if (!on && !m_cash->isChecked()) m_cash->setChecked(true);
        refreshDefaultMethods();
    });
    connect(m_prefix, &QLineEdit::textEdited, this, [this](const QString &text) {
        const int cursor = m_prefix->cursorPosition();
        m_prefix->setText(text.toUpper());
        m_prefix->setCursorPosition(cursor);
        refreshPrefixPreview();
    });
    connect(m_saveButton, &QPushButton::clicked, this, &CheckoutSettingsSection::save);
    connect(m_client, &SupabaseClient::businessSettingsSaved, this, [this]() {
        if (!m_saving) return;
        m_saving = false;
        m_saveButton->setEnabled(true);
        m_status->setText(tr("Saved. Other tills pick it up the next time they start."));
    });
    connect(m_client, &SupabaseClient::businessSettingsSaveFailed, this, [this](const QString &message) {
        if (!m_saving) return;
        m_saving = false;
        m_saveButton->setEnabled(true);
        m_status->setText(tr("Couldn't save: ") + message);
    });
    connect(BusinessProfile::instance(), &BusinessProfile::changed, this, [this]() {
        if (!m_saving) loadFromProfile();
    });
    loadFromProfile();
}

void CheckoutSettingsSection::loadFromProfile() {
    const BusinessSettings &settings = BusinessProfile::instance()->settings();
    m_cash->setChecked(settings.paymentMethods.contains("cash"));
    m_card->setChecked(settings.paymentMethods.contains("card"));
    refreshDefaultMethods();
    m_defaultMethod->setCurrentIndex(qMax(0, m_defaultMethod->findData(settings.defaultPaymentMethod)));
    m_lowStock->setValue(settings.defaultLowStockThreshold);
    m_prefix->setText(settings.skuPrefix);
    refreshPrefixPreview();
}

// "Selected first" only lists the methods that are on; with one, there's no choice to make.
void CheckoutSettingsSection::refreshDefaultMethods() {
    const QString current = m_defaultMethod->currentData().toString();
    m_defaultMethod->clear();
    if (m_cash->isChecked()) m_defaultMethod->addItem(tr("Cash"), "cash");
    if (m_card->isChecked()) m_defaultMethod->addItem(tr("Card"), "card");
    m_defaultMethod->setCurrentIndex(qMax(0, m_defaultMethod->findData(current)));
    m_defaultMethod->setEnabled(m_defaultMethod->count() > 1);
}

void CheckoutSettingsSection::refreshPrefixPreview() {
    const QString prefix = m_prefix->text().isEmpty() ? "INT" : m_prefix->text();
    const QString sample = generateInternalSku().section('-', 1);
    m_prefixPreview->setText(tr("New codes look like %1").arg(prefix + "-" + sample));
}

void CheckoutSettingsSection::save() {
    if (m_prefix->text().isEmpty()) {
        m_status->setText(tr("The code prefix can't be empty."));
        return;
    }
    QJsonArray methods;
    if (m_cash->isChecked()) methods.append("cash");
    if (m_card->isChecked()) methods.append("card");
    m_saving = true;
    m_saveButton->setEnabled(false);
    m_status->setText(tr("Saving…"));
    m_client->updateBusinessSettings({
        {"payment_methods", methods},
        {"default_payment_method", m_defaultMethod->currentData().toString()},
        {"default_low_stock_threshold", m_lowStock->value()},
        {"sku_prefix", m_prefix->text()},
    });
}

#include "SettingsPage.h"
#include "BusinessProfile.h"
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
#include <QPainter>
#include <QProcess>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimeZone>
#include <QVBoxLayout>

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
    addSection(tr("Appearance"), new AppearanceSettingsSection(client, this));
    addSection(tr("Language"), new LanguageSettingsSection(client, this));
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

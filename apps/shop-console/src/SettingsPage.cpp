#include "SettingsPage.h"
#include "BusinessProfile.h"
#include "SupabaseClient.h"

#include <QComboBox>
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
    addSection("Business", business);
    addSection("Software", softwareSection);

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
    auto *uploadLogoButton = new QPushButton("Upload logo…", this);
    m_removeLogoButton = new QPushButton("Remove", this);
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
    currencyRow->addWidget(new QLabel("Symbol", this));
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
    form->addRow("Logo", logoRow);
    form->addRow("Business name", m_name);
    form->addRow("Currency", currencyRow);
    form->addRow("Amounts look like", m_style);
    form->addRow("Time zone", m_timezone);
    form->addRow("", m_timePreview);

    m_saveButton = new QPushButton("Save", this);
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
        "Business", "Shared by every till and shop. Only owners and admins can change these.", this));
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
        m_status->setText("Saved. Other tills pick it up the next time they start.");
    });
    connect(m_client, &SupabaseClient::businessSettingsSaveFailed, this, [this](const QString &message) {
        if (!m_saving) return;
        m_saving = false;
        m_saveButton->setEnabled(true);
        m_status->setText("Couldn't save: " + message);
    });
    connect(m_client, &SupabaseClient::businessLogoUploaded, this, [this](const QString &storagePath) {
        m_oldLogoPath = BusinessProfile::instance()->settings().logoPath;
        m_saving = true;
        m_client->updateBusinessSettings({{"logo_path", storagePath}});
    });
    connect(m_client, &SupabaseClient::businessLogoUploadFailed, this, [this](const QString &message) {
        m_saving = false;
        m_saveButton->setEnabled(true);
        m_status->setText("Couldn't upload the logo: " + message);
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
                               ? "It's " + QDateTime::currentDateTimeUtc().toTimeZone(zone).toString("dddd HH:mm") + " there now."
                               : "Pick a time zone from the list.");
}

void BusinessSettingsSection::save() {
    const QString name = m_name->text().trimmed();
    const QString code = currencyCode(m_currency);
    const QString symbol = m_symbol->text().trimmed();
    const QString timezone = m_timezone->currentText().trimmed();
    if (name.isEmpty()) return m_status->setText("The business needs a name.");
    if (code.size() < 2 || code.size() > 4) return m_status->setText("Currency should be a code like GEL or USD.");
    if (symbol.isEmpty()) return m_status->setText("Give the currency a symbol (it can be the code itself).");
    if (!QTimeZone(timezone.toUtf8()).isValid()) return m_status->setText("Pick a time zone from the list.");

    m_saving = true;
    m_saveButton->setEnabled(false);
    m_status->setText("Saving…");
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
        QFileDialog::getOpenFileName(this, "Choose a logo", QString(), "Images (*.png *.jpg *.jpeg *.webp)");
    if (path.isEmpty()) return;
    m_saving = true;
    m_saveButton->setEnabled(false);
    m_status->setText("Uploading the logo…");
    m_client->uploadBusinessLogo(path);
}

void BusinessSettingsSection::removeLogo() {
    m_oldLogoPath = BusinessProfile::instance()->settings().logoPath;
    m_saving = true;
    m_saveButton->setEnabled(false);
    m_status->setText("Removing the logo…");
    m_client->updateBusinessSettings({{"logo_path", QJsonValue(QJsonValue::Null)}});
}

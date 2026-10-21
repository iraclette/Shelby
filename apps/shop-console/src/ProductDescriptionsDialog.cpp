#include "ProductDescriptionsDialog.h"
#include "Theme.h"
#include "Translations.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

ProductDescriptionsDialog::ProductDescriptionsDialog(SupabaseClient *client, QString productId,
                                                     const QString &productName, QWidget *parent)
    : QDialog(parent), m_client(client), m_productId(std::move(productId)),
      m_lastEdited(translations::startupLanguage()) {
    setWindowTitle(tr("Descriptions — %1").arg(productName));
    resize(640, 560);

    auto *hint = new QLabel(tr("Edit any language and save. Languages you didn't write yourself are translated "
                               "again from the one you edited; clear a language to have it translated."),
                            this);
    hint->setWordWrap(true);
    hint->setStyleSheet(theme::css(theme::palette().textMuted));

    auto *form = new QFormLayout;
    for (const QString &code : {QStringLiteral("ka"), QStringLiteral("en"), QStringLiteral("ru")}) {
        auto *editor = new QPlainTextEdit(this);
        editor->setEnabled(false);
        connect(editor, &QPlainTextEdit::textChanged, this, [this, code]() { m_lastEdited = code; });
        auto *badge = new QLabel(this);
        badge->setStyleSheet(theme::css(theme::palette().textMuted));

        auto *header = new QWidget(this);
        auto *headerLayout = new QVBoxLayout(header);
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->addWidget(new QLabel(translations::languageName(code), header));
        headerLayout->addWidget(badge);
        headerLayout->addStretch();
        form->addRow(header, editor);
        m_editors[code] = editor;
        m_badges[code] = badge;
    }

    m_statusLabel = new QLabel(tr("Loading…"), this);
    m_statusLabel->setWordWrap(true);
    m_saveButton = new QPushButton(tr("Save"), this);
    m_saveButton->setObjectName("primaryButton");
    m_saveButton->setEnabled(false);
    auto *closeButton = new QPushButton(tr("Close"), this);
    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_statusLabel, 1);
    buttonRow->addWidget(m_saveButton);
    buttonRow->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(hint);
    layout->addLayout(form, 1);
    layout->addLayout(buttonRow);

    connect(m_saveButton, &QPushButton::clicked, this, &ProductDescriptionsDialog::save);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    connect(m_client, &SupabaseClient::productDescriptionsFetched, this,
            [this](const QString &id, const QJsonObject &descriptions) {
        if (id != m_productId) return;
        showDescriptions(descriptions);
    });
    connect(m_client, &SupabaseClient::productDescriptionsFetchFailed, this, [this](const QString &message) {
        // Editors stay disabled: saving over descriptions we never loaded would lose them.
        m_statusLabel->setText(tr("Couldn't load descriptions: ") + message);
    });
    connect(m_client, &SupabaseClient::productDescriptionsSaved, this, [this](const QString &id) {
        if (id != m_productId) return;
        setBusy(true, tr("Saved. Translating the other languages…"));
        m_client->translateDescription(m_productId);
    });
    connect(m_client, &SupabaseClient::productDescriptionsSaveFailed, this, [this](const QString &message) {
        setBusy(false, tr("Couldn't save: ") + message);
    });
    connect(m_client, &SupabaseClient::descriptionTranslated, this, [this](const QString &id) {
        if (id != m_productId) return;
        setBusy(true, tr("Saved and translated."));
        m_client->fetchProductDescriptions(m_productId);
    });
    connect(m_client, &SupabaseClient::descriptionTranslateFailed, this,
            [this](const QString &id, const QString &message) {
        if (id != m_productId) return;
        setBusy(true, tr("Saved, but translating failed: ") + message);
        m_client->fetchProductDescriptions(m_productId);
    });

    m_client->fetchProductDescriptions(m_productId);
}

void ProductDescriptionsDialog::showDescriptions(const QJsonObject &descriptions) {
    m_descriptions = descriptions;
    const QString source = descriptions.value("source").toString();
    for (auto it = m_editors.constBegin(); it != m_editors.constEnd(); ++it) {
        const QJsonObject entry = descriptions.value(it.key()).toObject();
        {
            const QSignalBlocker blocker(it.value()); // not an edit
            it.value()->setPlainText(entry.value("text").toString());
        }
        QString badge;
        if (!entry.isEmpty()) {
            badge = entry.value("auto").toBool() ? tr("Translated automatically")
                    : it.key() == source       ? tr("Original")
                                               : tr("Written by staff");
        }
        m_badges[it.key()]->setText(badge);
    }
    // Keeps whatever "Saved…" message is showing.
    if (m_statusLabel->text() == tr("Loading…")) m_statusLabel->clear();
    setBusy(false, m_statusLabel->text());
}

void ProductDescriptionsDialog::save() {
    QMap<QString, QString> edits;
    for (auto it = m_editors.constBegin(); it != m_editors.constEnd(); ++it) {
        const QString text = it.value()->toPlainText().trimmed();
        const QString saved = m_descriptions.value(it.key()).toObject().value("text").toString().trimmed();
        if (text != saved) edits[it.key()] = text;
    }
    if (edits.isEmpty()) return;

    setBusy(true, tr("Saving…"));
    m_client->saveProductDescriptions(m_productId,
                                      descriptionsWithStaffEdits(m_descriptions, edits, m_lastEdited));
}

void ProductDescriptionsDialog::setBusy(bool busy, const QString &status) {
    for (QPlainTextEdit *editor : std::as_const(m_editors)) editor->setEnabled(!busy);
    m_saveButton->setEnabled(!busy);
    m_statusLabel->setText(status);
}

#include "SoftwarePage.h"
#include "UpdateChecker.h"

#include <QGroupBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QTime>
#include <QVBoxLayout>

SoftwarePage::SoftwarePage(UpdateChecker *updateChecker, QWidget *parent)
    : QWidget(parent), m_updateChecker(updateChecker) {
    auto *group = new QGroupBox("Shop Console", this);
    auto *groupLayout = new QVBoxLayout(group);

    m_versionLabel = new QLabel(QString("Version %1").arg(QStringLiteral(APP_VERSION)), group);
    m_versionLabel->setStyleSheet("font-weight: 600; font-size: 15px;");

    m_statusLabel = new QLabel("Checking for updates…", group);
    m_statusLabel->setOpenExternalLinks(true);
    m_statusLabel->setWordWrap(true);

    m_updateButton = new QPushButton("Update now", group);
    m_updateButton->setObjectName("primaryButton");
    m_updateButton->hide();

    // The app checks at launch and sign-in only; this re-checks without relaunching.
    m_checkButton = new QPushButton("Check for updates", group);

    groupLayout->addWidget(m_versionLabel);
    groupLayout->addWidget(m_statusLabel);
    auto *buttonsRow = new QHBoxLayout;
    buttonsRow->addWidget(m_updateButton);
    buttonsRow->addWidget(m_checkButton);
    buttonsRow->addStretch();
    groupLayout->addLayout(buttonsRow);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(group);
    layout->addStretch();

    connect(m_checkButton, &QPushButton::clicked, this, [this]() {
        m_checkButton->setEnabled(false);
        m_checkButton->setText("Checking…");
        m_statusLabel->setStyleSheet("color: #9aa0ac;");
        m_statusLabel->setText("Checking for updates…");
        m_updateChecker->checkForUpdate();
    });
    connect(m_updateButton, &QPushButton::clicked, this, [this]() {
        m_updateButton->setEnabled(false);
        m_updateButton->setText("Updating…");
        m_updateChecker->downloadAndInstall(m_pendingAssetUrl);
    });
    connect(m_updateChecker, &UpdateChecker::installFailed, this, [this](const QString &message) {
        m_updateButton->setEnabled(true);
        m_updateButton->setText("Update now");
        m_statusLabel->setStyleSheet("color: #dc2626;");
        m_statusLabel->setText("Update failed: " + message.toHtmlEscaped());
    });
    connect(m_updateChecker, &UpdateChecker::downloadProgress, this, [this](int percent) {
        m_updateButton->setText(QString("Updating… %1%").arg(percent));
    });
}

void SoftwarePage::reportUpdateCheckResult(bool ok, bool isNewer, const QString &version, const QString &releaseUrl,
                                            const QString &assetUrl, const QString &errorMessage) {
    m_pendingAssetUrl = assetUrl;
    m_checkButton->setEnabled(true);
    m_checkButton->setText("Check for updates");
    const QString checkedAt = QTime::currentTime().toString("HH:mm");
    m_versionLabel->setText(m_updateChecker->includesPrereleases()
                                ? QString("Version %1  ·  dev channel (pre-releases)").arg(QStringLiteral(APP_VERSION))
                                : QString("Version %1").arg(QStringLiteral(APP_VERSION)));

    if (!ok) {
        m_statusLabel->setStyleSheet("color: #dc2626;");
        m_statusLabel->setText("Update check failed: " + errorMessage.toHtmlEscaped());
        m_updateButton->hide();
        return;
    }

    if (isNewer) {
        m_statusLabel->setStyleSheet("color: #4f8cff;");
        m_statusLabel->setText(
            QString("<a href=\"%1\" style=\"color:#4f8cff;\">Update available — v%2</a>").arg(releaseUrl, version));
        if (assetUrl.isEmpty()) {
            m_updateButton->hide();
        } else {
            m_updateButton->setText(QString("Update now — v%1").arg(version));
            m_updateButton->setEnabled(true);
            m_updateButton->show();
        }
    } else {
        m_statusLabel->setStyleSheet("color: #737373;");
        m_statusLabel->setText(QString("Up to date (v%1) · checked at %2.").arg(version, checkedAt));
        m_updateButton->hide();
    }
}

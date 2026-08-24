#include "SoftwarePage.h"
#include "UpdateChecker.h"

#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
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

    groupLayout->addWidget(m_versionLabel);
    groupLayout->addWidget(m_statusLabel);
    groupLayout->addWidget(m_updateButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(group);
    layout->addStretch();

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
        m_statusLabel->setText(QString("Up to date (v%1).").arg(version));
        m_updateButton->hide();
    }
}

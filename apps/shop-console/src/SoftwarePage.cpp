#include "SoftwarePage.h"

#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

SoftwarePage::SoftwarePage(QWidget *parent) : QWidget(parent) {
    auto *group = new QGroupBox("Shop Console", this);
    auto *groupLayout = new QVBoxLayout(group);

    m_versionLabel = new QLabel(QString("Version %1").arg(QStringLiteral(APP_VERSION)), group);
    m_versionLabel->setStyleSheet("font-weight: 600; font-size: 15px;");

    m_statusLabel = new QLabel("Checking for updates…", group);
    m_statusLabel->setOpenExternalLinks(true);
    m_statusLabel->setWordWrap(true);

    groupLayout->addWidget(m_versionLabel);
    groupLayout->addWidget(m_statusLabel);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(group);
    layout->addStretch();
}

void SoftwarePage::reportUpdateCheckResult(bool ok, bool isNewer, const QString &version, const QString &releaseUrl,
                                            const QString &errorMessage) {
    if (!ok) {
        m_statusLabel->setStyleSheet("color: #dc2626;");
        m_statusLabel->setText("Update check failed: " + errorMessage.toHtmlEscaped());
        return;
    }

    if (isNewer) {
        m_statusLabel->setStyleSheet("color: #4f8cff;");
        m_statusLabel->setText(
            QString("<a href=\"%1\" style=\"color:#4f8cff;\">Update available — v%2</a>").arg(releaseUrl, version));
    } else {
        m_statusLabel->setStyleSheet("color: #737373;");
        m_statusLabel->setText(QString("Up to date (v%1).").arg(version));
    }
}

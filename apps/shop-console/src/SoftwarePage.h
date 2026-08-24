#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class UpdateChecker;

// Version + update-check diagnostics, plus the "Update now" action. Exists
// specifically because the update check used to fail completely invisibly
// (a rate-limit 403 with no visible symptom anywhere) — this is always
// driven by the same UpdateChecker::checkFinished result AdminWindow's
// toolbar button uses, so whatever happened is visible here even when
// there's nothing to announce (up to date) or the check failed outright.
class SoftwarePage : public QWidget {
    Q_OBJECT

public:
    explicit SoftwarePage(UpdateChecker *updateChecker, QWidget *parent = nullptr);

    void reportUpdateCheckResult(bool ok, bool isNewer, const QString &version, const QString &releaseUrl,
                                  const QString &assetUrl, const QString &errorMessage);

private:
    UpdateChecker *m_updateChecker;
    QString m_pendingAssetUrl;

    QLabel *m_versionLabel;
    QLabel *m_statusLabel;
    QPushButton *m_updateButton;
};

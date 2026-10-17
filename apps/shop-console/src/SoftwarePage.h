#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class UpdateChecker;

// Version info, update-check result, and "Update now".
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
    QPushButton *m_checkButton;
};

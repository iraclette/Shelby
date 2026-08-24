#pragma once

#include <QWidget>

class QLabel;

// Version + update-check diagnostics. Exists specifically because the
// update check used to fail completely invisibly (a rate-limit 403 with no
// visible symptom anywhere) — this is always driven by the same
// UpdateChecker::checkFinished result AdminWindow's toolbar banner uses,
// so whatever happened is visible here even when there's nothing to
// announce (up to date) or the check failed outright.
class SoftwarePage : public QWidget {
    Q_OBJECT

public:
    explicit SoftwarePage(QWidget *parent = nullptr);

    void reportUpdateCheckResult(bool ok, bool isNewer, const QString &version, const QString &releaseUrl,
                                  const QString &errorMessage);

private:
    QLabel *m_versionLabel;
    QLabel *m_statusLabel;
};

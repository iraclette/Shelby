#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QMap>
#include <QString>

#include "SupabaseClient.h"

class QLabel;
class QPlainTextEdit;
class QPushButton;

// Edit an existing product's description in every website language. Saving makes the
// edited languages staff-written; the translate-description edge function then fills in
// the rest. Same rules as the website's staff Descriptions page.
class ProductDescriptionsDialog : public QDialog {
    Q_OBJECT

public:
    ProductDescriptionsDialog(SupabaseClient *client, QString productId, const QString &productName,
                              QWidget *parent = nullptr);

private:
    void showDescriptions(const QJsonObject &descriptions);
    void save();
    void setBusy(bool busy, const QString &status);

    SupabaseClient *m_client;
    QString m_productId;
    QJsonObject m_descriptions;
    QString m_lastEdited;
    QMap<QString, QPlainTextEdit *> m_editors; // language code -> editor
    QMap<QString, QLabel *> m_badges;
    QPushButton *m_saveButton;
    QLabel *m_statusLabel;
};

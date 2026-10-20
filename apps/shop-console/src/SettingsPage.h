#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class SupabaseClient;

// Settings: a list of sections on the left (Business, Software…), the section on the right.
class SettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(SupabaseClient *client, QWidget *softwareSection, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);
};

// Settings → Business: name, logo, currency and time zone, shared by every till.
class BusinessSettingsSection : public QWidget {
    Q_OBJECT

public:
    explicit BusinessSettingsSection(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void statusMessage(const QString &message);

private:
    void loadFromProfile();
    void refreshLogo();
    void refreshPreviews();
    void save();
    void chooseLogo();
    void removeLogo();

    SupabaseClient *m_client;

    QLabel *m_logoPreview;
    QPushButton *m_removeLogoButton;
    QLineEdit *m_name;
    QComboBox *m_currency;
    QLineEdit *m_symbol;
    QComboBox *m_style;
    QComboBox *m_timezone;
    QLabel *m_timePreview;
    QPushButton *m_saveButton;
    QLabel *m_status;

    // A logo that was just replaced or removed: deleted from storage once the change is saved.
    QString m_oldLogoPath;
    bool m_saving = false;
};

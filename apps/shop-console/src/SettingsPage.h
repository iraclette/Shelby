#pragma once

#include <QWidget>

class QCheckBox;
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

// Settings → Appearance: theme, accent colour and interface size. Stored on this computer
// only, so each till can look its own way. Also shown on the till itself (PosWindow), since
// cashiers don't get the Settings page.
class AppearanceSettingsSection : public QWidget {
    Q_OBJECT

public:
    explicit AppearanceSettingsSection(SupabaseClient *client, QWidget *parent = nullptr);

private:
    void refresh();
    void setAccent(const QColor &accent); // invalid = the theme's own

    SupabaseClient *m_client;
    QList<QPair<QString, QPushButton *>> m_themeButtons;
    QList<QPair<QColor, QPushButton *>> m_accentButtons;
    QPushButton *m_defaultAccentButton;
    QPushButton *m_customAccentButton;
    QComboBox *m_scale;
    QLabel *m_scaleNote;
    QPushButton *m_restartButton;
};

// Settings → Modules: switch off console sections the business doesn't use, on every till.
class ModulesSettingsSection : public QWidget {
    Q_OBJECT

public:
    explicit ModulesSettingsSection(SupabaseClient *client, QWidget *parent = nullptr);

private:
    void loadFromProfile();
    void save();

    SupabaseClient *m_client;
    QList<QPair<QString, QCheckBox *>> m_switches; // module key -> its checkbox (checked = on)
    QPushButton *m_saveButton;
    QLabel *m_status;
    bool m_saving = false;
};

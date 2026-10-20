#pragma once

#include <QDateTime>
#include <QVector>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTimer;
class QVBoxLayout;
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

// Settings → Language: the console's language on this computer (applies after a restart).
class LanguageSettingsSection : public QWidget {
    Q_OBJECT

public:
    explicit LanguageSettingsSection(SupabaseClient *client, QWidget *parent = nullptr);

private:
    void refresh();

    SupabaseClient *m_client;
    QList<QPair<QString, QPushButton *>> m_languageButtons;
    QLabel *m_note;
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

// Settings → Checkout: the payment methods the till offers, and the defaults new products get
// (low stock alert, the prefix of codes made for products without a barcode). Every till.
class CheckoutSettingsSection : public QWidget {
    Q_OBJECT

public:
    explicit CheckoutSettingsSection(SupabaseClient *client, QWidget *parent = nullptr);

private:
    void loadFromProfile();
    void refreshDefaultMethods();
    void refreshPrefixPreview();
    void save();

    SupabaseClient *m_client;
    QCheckBox *m_cash;
    QCheckBox *m_card;
    QComboBox *m_defaultMethod;
    QSpinBox *m_lowStock;
    QLineEdit *m_prefix;
    QLabel *m_prefixPreview;
    QPushButton *m_saveButton;
    QLabel *m_status;
    bool m_saving = false;
};

// Settings → Notifications: desktop pop-ups for this account on this computer, and phones
// linked to the website's Telegram bot. Linking shows a QR code to scan with the phone, so the
// computer itself doesn't need Telegram.
class NotificationsSettingsSection : public QWidget {
    Q_OBJECT

public:
    explicit NotificationsSettingsSection(SupabaseClient *client, QWidget *parent = nullptr);

private:
    struct TelegramLink {
        QString id;
        QString chatName;
        QStringList events;
    };

    void startLinking();
    void stopLinking();
    void showQrLink();
    void loadLinks();
    void showLinks();
    void setLinkEvent(const QString &linkId, const QString &event, bool on);
    void disconnectLink(const QString &linkId, const QString &chatName);
    void showEvent(QShowEvent *event) override; // picks up phones linked from the website meanwhile

    SupabaseClient *m_client;
    QString m_botUsername;
    QPushButton *m_connectButton = nullptr;
    QWidget *m_qrPanel = nullptr;
    QLabel *m_qrCode = nullptr;
    QLabel *m_qrLink = nullptr;
    QString m_qrUrl; // the t.me link while the QR code is up
    QLabel *m_telegramStatus = nullptr;
    QVBoxLayout *m_linksLayout = nullptr;
    QTimer *m_linkPoll = nullptr; // while the QR code is up: has the phone linked yet?
    QDateTime m_linkStarted;
    QVector<TelegramLink> m_links;
};

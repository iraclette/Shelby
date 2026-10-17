#pragma once

#include <QWidget>

class PosWindow;
class QCheckBox;
class QComboBox;
class QLabel;
class QVBoxLayout;
class SupabaseClient;

// Dev-only admin page: the shop register, for any shop. Starts in practice
// mode so trying things out doesn't create real sales or move real stock.
class CheckoutPage : public QWidget {
    Q_OBJECT

public:
    explicit CheckoutPage(SupabaseClient *client, QWidget *parent = nullptr);

private:
    void openRegister(const QString &shopId);

    SupabaseClient *m_client;
    QComboBox *m_shopCombo;
    QCheckBox *m_recordCheck;
    QLabel *m_modeLabel;
    QVBoxLayout *m_registerLayout;
    PosWindow *m_register = nullptr;
};

#pragma once

#include <QDialog>
#include <QVector>

#include "SupabaseClient.h"

class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;

class EmployeeDialog : public QDialog {
    Q_OBJECT

public:
    EmployeeDialog(SupabaseClient *client, QVector<Shop> shops, QWidget *parent = nullptr);

private:
    void save();

    SupabaseClient *m_client;
    QVector<Shop> m_shops;

    QLineEdit *m_username;
    QLineEdit *m_password;
    QLineEdit *m_fullName;
    QComboBox *m_role;
    QComboBox *m_shop;
    QLabel *m_statusLabel;
    QPushButton *m_saveButton;
};

#pragma once

#include <QMainWindow>

class QTableWidget;
class SupabaseClient;

class AdminWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AdminWindow(SupabaseClient *client, QWidget *parent = nullptr);

signals:
    void signedOut();

private:
    SupabaseClient *m_client;
    QTableWidget *m_table;
};

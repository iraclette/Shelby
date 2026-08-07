#pragma once

#include <QMainWindow>

class PosWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit PosWindow(QWidget *parent = nullptr);

signals:
    void signedOut();
};

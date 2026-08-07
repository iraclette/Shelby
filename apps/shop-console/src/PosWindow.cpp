#include "PosWindow.h"

#include <QLabel>
#include <QPushButton>
#include <QToolBar>
#include <QWidget>

PosWindow::PosWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Shop Console — Checkout");
    resize(800, 600);

    auto *toolbar = addToolBar("main");
    toolbar->setMovable(false);
    auto *title = new QLabel("Checkout", this);
    title->setStyleSheet("font-weight: 600; padding: 0 8px;");
    toolbar->addWidget(title);
    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);
    auto *signOutButton = new QPushButton("Sign out", this);
    toolbar->addWidget(signOutButton);
    connect(signOutButton, &QPushButton::clicked, this, &PosWindow::signedOut);

    auto *placeholder = new QLabel("Checkout — coming in Phase 2 (offline-capable sales).", this);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("color: #737373;");
    setCentralWidget(placeholder);
}

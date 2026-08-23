#pragma once

// Dark theme with a blue accent, applied app-wide via qApp->setStyleSheet().
// Widgets opt into the accented "primary" look by setting
// setObjectName("primaryButton"); product/category buttons in the POS use
// "productTile" / "categoryChip" for their card/pill styling.
inline const char *kAppStyleSheet = R"(
QWidget {
    background-color: #14151a;
    color: #e8e9ec;
    font-size: 13px;
}

QMainWindow, QDialog {
    background-color: #14151a;
}

QLabel {
    background: transparent;
}

QLineEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    background-color: #1c1e26;
    border: 1px solid #2f323d;
    border-radius: 6px;
    padding: 6px 8px;
    selection-background-color: #2b3a5c;
}

QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid #4f8cff;
}

QComboBox::drop-down {
    border: none;
    width: 20px;
}

QComboBox QAbstractItemView {
    background-color: #1c1e26;
    color: #e8e9ec;
    selection-background-color: #2b3a5c;
    border: 1px solid #2f323d;
    outline: none;
}

QPushButton {
    background-color: #23252f;
    border: 1px solid #2f323d;
    border-radius: 6px;
    padding: 6px 14px;
}

QPushButton:hover {
    background-color: #2b2e3a;
}

QPushButton:pressed {
    background-color: #1c1e26;
}

QPushButton:disabled {
    color: #6b7280;
}

QPushButton#primaryButton {
    background-color: #4f8cff;
    border: 1px solid #4f8cff;
    color: white;
    font-weight: 600;
}

QPushButton#primaryButton:hover {
    background-color: #6a9dff;
}

QPushButton#primaryButton:pressed {
    background-color: #3670d6;
}

QPushButton#productTile {
    background-color: #1c1e26;
    border: 1px solid #2f323d;
    border-radius: 10px;
    padding: 10px;
    font-weight: 600;
}

QPushButton#productTile:hover {
    background-color: #23252f;
    border: 1px solid #4f8cff;
}

QPushButton#productTile:pressed {
    background-color: #2b3a5c;
}

QPushButton#categoryChip {
    background-color: #1c1e26;
    border: 1px solid #2f323d;
    border-radius: 14px;
    padding: 6px 14px;
    color: #9aa0ac;
}

QPushButton#categoryChip:checked {
    background-color: #4f8cff;
    border: 1px solid #4f8cff;
    color: white;
    font-weight: 600;
}

QPushButton#categoryChip:hover {
    border: 1px solid #4f8cff;
}

QGroupBox {
    border: 1px solid #2f323d;
    border-radius: 8px;
    margin-top: 14px;
    padding-top: 10px;
    font-weight: 600;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 4px;
    color: #9aa0ac;
}

QTableWidget {
    background-color: #1c1e26;
    alternate-background-color: #20222c;
    gridline-color: #2f323d;
    border: 1px solid #2f323d;
    border-radius: 6px;
}

QTableWidget::item {
    padding: 4px;
}

QTableWidget::item:selected {
    background-color: #2b3a5c;
}

QHeaderView::section {
    background-color: #23252f;
    color: #9aa0ac;
    padding: 6px;
    border: none;
    border-bottom: 1px solid #2f323d;
    font-weight: 600;
}

QToolBar {
    background-color: #1c1e26;
    border: none;
    border-bottom: 1px solid #2f323d;
    spacing: 6px;
    padding: 6px;
}

QStatusBar {
    background-color: #1c1e26;
    color: #9aa0ac;
}

QScrollArea {
    border: none;
}

QScrollBar:vertical {
    background: #14151a;
    width: 10px;
    margin: 0;
}

QScrollBar::handle:vertical {
    background: #2f323d;
    border-radius: 5px;
    min-height: 24px;
}

QScrollBar::handle:vertical:hover {
    background: #3a3e4c;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}

QScrollBar:horizontal {
    background: #14151a;
    height: 10px;
    margin: 0;
}

QScrollBar::handle:horizontal {
    background: #2f323d;
    border-radius: 5px;
    min-width: 24px;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}

QCheckBox::indicator {
    width: 14px;
    height: 14px;
    border: 1px solid #2f323d;
    border-radius: 3px;
    background-color: #1c1e26;
}

QCheckBox::indicator:checked {
    background-color: #4f8cff;
    border: 1px solid #4f8cff;
}

QMessageBox, QInputDialog {
    background-color: #1c1e26;
}

QWidget#loginBrandPanel {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
        stop:0 #0e0e10, stop:0.5 #231c12, stop:1 #14151a);
    border-right: 1px solid #2f323d;
}
)";

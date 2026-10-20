#include "Theme.h"

#include <QApplication>
#include <QSettings>
#include <utility>

namespace theme {
namespace {

QSettings localSettings() {
    return QSettings("Shelby", "ShopConsole");
}

// The stylesheet, with @role names where colours go (filled in by apply()).
const char *kStyleSheetTemplate = R"(
QWidget {
    background-color: @window;
    color: @text;
    font-size: 13px;
}

QMainWindow, QDialog {
    background-color: @window;
}

QLabel {
    background: transparent;
}

QLabel#pageTitle {
    font-size: 16px;
    font-weight: 700;
    padding: 0 8px;
    color: @textStrong;
}

QLineEdit, QPlainTextEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    background-color: @surface;
    border: 1px solid @border;
    border-radius: 6px;
    padding: 6px 8px;
    min-height: 20px;
    selection-background-color: @selection;
}

QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
    border: 1px solid @accent;
}

QComboBox::drop-down {
    border: none;
    width: 20px;
}

QComboBox QAbstractItemView {
    background-color: @surface;
    color: @text;
    selection-background-color: @selection;
    border: 1px solid @border;
    outline: none;
}

QPushButton {
    background-color: @control;
    border: 1px solid @border;
    border-radius: 6px;
    padding: 8px 16px;
}

QPushButton:hover {
    background-color: @controlHover;
}

QPushButton:pressed {
    background-color: @surface;
}

QPushButton:disabled {
    color: @textDisabled;
}

QPushButton#primaryButton {
    background-color: @accent;
    border: 1px solid @accent;
    color: @onAccent;
    font-weight: 600;
}

QPushButton#primaryButton:hover {
    background-color: @accentHover;
}

QPushButton#primaryButton:pressed {
    background-color: @accentPressed;
}

QPushButton#payButton {
    background-color: @accent;
    border: 1px solid @accent;
    border-radius: 8px;
    color: @onAccent;
    font-size: 17px;
    font-weight: 700;
}

QPushButton#payButton:hover {
    background-color: @accentHover;
}

QPushButton#payButton:pressed {
    background-color: @accentPressed;
}

QPushButton#payButton:disabled {
    background-color: @selection;
    border: 1px solid @selection;
    color: @textMuted;
}

QPushButton#dangerButton {
    color: @danger;
}

QPushButton#dangerButton:disabled {
    color: @textDisabled;
}

QPushButton#stepButton {
    padding: 8px 0;
    font-size: 16px;
    font-weight: 700;
}

QPushButton#compactButton {
    padding: 2px 6px;
}

QPushButton#productTile {
    background-color: @surface;
    border: 1px solid @border;
    border-radius: 10px;
    padding: 10px;
    font-weight: 600;
}

QPushButton#productTile:hover {
    background-color: @control;
    border: 1px solid @accent;
}

QPushButton#productTile:pressed {
    background-color: @selection;
}

QLabel#tileName {
    font-weight: 600;
    color: @text;
}

QLabel#tilePrice {
    font-weight: 700;
    color: @accent;
}

QLabel#tileStock {
    color: @textMuted;
    font-size: 12px;
}

QFrame#posPanel {
    background-color: @surface;
    border: 1px solid @border;
    border-radius: 10px;
}

QFrame#posPanel QPushButton {
    padding: 8px 10px;
}

QLineEdit#posSearch {
    background-color: @window;
    font-size: 14px;
    padding: 8px 10px;
}

QTableWidget#receiptTable {
    background-color: @window;
    alternate-background-color: @alternateRowDeep;
    border: none;
}

QLineEdit#cashReceived {
    font-size: 22px;
    font-weight: 700;
    padding: 8px 12px;
    min-height: 34px; 
}

QFrame#totalsPanel {
    background-color: @window;
    border: 1px solid @border;
    border-radius: 8px;
}

QLabel#totalsKey {
    color: @textMuted;
}

QLabel#totalsValue {
    font-weight: 600;
}

QLabel#totalKey {
    color: @textMuted;
    font-weight: 700;
    padding-bottom: 6px;
}

QLabel#totalValue {
    font-size: 30px;
    font-weight: 700;
    color: @textStrong;
}

QPushButton#categoryChip {
    background-color: @surface;
    border: 1px solid @border;
    border-radius: 14px;
    padding: 6px 14px;
    color: @textMuted;
}

QPushButton#categoryChip:checked {
    background-color: @accent;
    border: 1px solid @accent;
    color: @onAccent;
    font-weight: 600;
}

QPushButton#categoryChip:hover {
    border: 1px solid @accent;
}

QGroupBox {
    border: 1px solid @border;
    border-radius: 8px;
    margin-top: 16px;
    padding-top: 12px;
    font-weight: 600;
}

QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    padding: 0 4px;
    color: @textMuted;
}

QTableWidget {
    background-color: @surface;
    alternate-background-color: @alternateRow;
    gridline-color: @border;
    border: 1px solid @border;
    border-radius: 6px;
}

QTableWidget::item {
    padding: 6px;
}

QTableWidget::item:selected {
    background-color: @selection;
}

QTableView QLineEdit, QTableView QSpinBox, QTableView QDoubleSpinBox, QTableView QComboBox {
    border: 1px solid @accent;
    border-radius: 0;
    padding: 0 5px;
    min-height: 0;
    margin: 0;
}

QHeaderView::section {
    background-color: @control;
    color: @textMuted;
    padding: 8px;
    border: none;
    border-bottom: 1px solid @border;
    font-weight: 600;
}

QToolBar {
    background-color: @surface;
    border: none;
    border-bottom: 1px solid @border;
    spacing: 8px;
    padding: 8px;
}

QStatusBar {
    background-color: @surface;
    color: @textMuted;
}

QScrollArea {
    border: none;
}

QScrollBar:vertical {
    background: @window;
    width: 10px;
    margin: 0;
}

QScrollBar::handle:vertical {
    background: @border;
    border-radius: 5px;
    min-height: 24px;
}

QScrollBar::handle:vertical:hover {
    background: @borderStrong;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}

QScrollBar:horizontal {
    background: @window;
    height: 10px;
    margin: 0;
}

QScrollBar::handle:horizontal {
    background: @border;
    border-radius: 5px;
    min-width: 24px;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}

QCheckBox {
    background: transparent;
}

QCheckBox::indicator, QTableView::indicator, QListView::indicator {
    width: 14px;
    height: 14px;
    border: 1px solid @border;
    border-radius: 3px;
    background-color: @surface;
}

QCheckBox::indicator:checked, QTableView::indicator:checked, QListView::indicator:checked {
    background-color: @accent;
    border: 1px solid @accent;
}

QMessageBox, QInputDialog {
    background-color: @surface;
}

QWidget#loginBrandPanel {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
        stop:0 @loginGradientStart, stop:0.5 @loginGradientMid, stop:1 @window);
    border-right: 1px solid @border;
}

QLabel#greeting {
    font-size: 22px;
    font-weight: 700;
    color: @textStrong;
}

QFrame#card {
    background-color: @surface;
    border: 1px solid @border;
    border-radius: 10px;
}

QLabel#cardCaption {
    color: @textMuted;
    font-weight: 600;
}

QLabel#kpiValue {
    font-size: 24px;
    font-weight: 700;
    color: @textStrong;
}

QPushButton#attentionRow {
    background-color: @window;
    border: 1px solid @border;
    border-radius: 8px;
    padding: 0;
    text-align: left;
}

QPushButton#attentionRow:hover {
    border: 1px solid @accent;
}

QLabel#countPill {
    background-color: @border;
    color: @textMuted;
    border-radius: 9px;
    padding: 1px 8px;
    font-weight: 700;
}

QLabel#countPill[active="true"] {
    background-color: @accent;
    color: @onAccent;
}

QTableWidget#cardTable {
    background-color: @surface;
    alternate-background-color: @alternateRow;
    border: none;
}

QWidget#pageBar {
    background-color: @surface;
    border-bottom: 1px solid @border;
}

QHeaderView {
    background-color: @control;
}

QWidget#sidebarPanel {
    background-color: @surface;
    border-right: 1px solid @border;
}

QWidget#sidebarBrand, QWidget#sidebarFooter {
    background-color: transparent;
}

QWidget#sidebarFooter {
    border-top: 1px solid @border;
}

/* The business's logo in place of the letter mark (already cut round). */
QLabel#brandLogo {
    background: transparent;
}

QLabel#brandMark {
    background-color: @brand;
    color: @window;
    border-radius: 9px;
    font-size: 18px;
    font-weight: 800;
}

QLabel#brandName {
    font-size: 15px;
    font-weight: 700;
    color: @textStrong;
}

QLabel#brandSub {
    font-size: 11px;
    color: @textMuted;
}

QLabel#avatar {
    background-color: @selection;
    color: @avatarText;
    border-radius: 16px;
    font-weight: 700;
}

QLabel#userName {
    font-weight: 600;
}

QListWidget#adminSidebar {
    background-color: transparent;
    border: none;
    padding: 0 0 8px 0;
    outline: none;
}

QListWidget#adminSidebar::item {
    padding: 8px 16px;
    margin: 0 8px;
    border-radius: 6px;
    color: @textMuted;
}

QListWidget#adminSidebar::item:hover {
    background-color: @control;
    color: @text;
}

QListWidget#adminSidebar::item:selected {
    background-color: @accent;
    color: @onAccent;
    font-weight: 600;
}

/* Small grey helper text (replaces per-widget inline colours, so it follows the theme). */
QLabel#hint {
    color: @textMuted;
}

/* Login screen brand (LoginWindow). */
QLabel#loginTitle {
    font-size: 32px;
    font-weight: 600;
    color: @textStrong;
    background: transparent;
}

QLabel#loginSubtitle {
    font-size: 11px;
    letter-spacing: 2px;
    color: @brand;
    background: transparent;
    margin-top: 6px;
}

/* Settings: the list of sections on the left. */
QListWidget#settingsSections {
    background: transparent;
    border: none;
}

QListWidget#settingsSections::item {
    padding: 8px 12px;
    border-radius: 6px;
    color: @textMuted;
}

QListWidget#settingsSections::item:selected {
    background-color: @selection;
    color: @textStrong;
    font-weight: 600;
}
)";

Palette darkPalette() {
    return {
        .window = QColor("#14151a"), .surface = QColor("#1c1e26"), .control = QColor("#23252f"),
        .controlHover = QColor("#2b2e3a"), .alternateRow = QColor("#20222c"), .alternateRowDeep = QColor("#181a21"),
        .border = QColor("#2f323d"), .borderStrong = QColor("#3a3e4c"), .selection = QColor("#2b3a5c"),
        .accent = QColor("#4f8cff"), .accentHover = QColor("#6a9dff"), .accentPressed = QColor("#3670d6"),
        .onAccent = QColor("#ffffff"), .text = QColor("#e8e9ec"), .textStrong = QColor("#f2f3f5"),
        .textMuted = QColor("#9aa0ac"), .textDisabled = QColor("#6b7280"), .danger = QColor("#f87171"),
        .success = QColor("#22c55e"), .warning = QColor("#f59e0b"), .info = QColor("#a855f7"),
        .brand = QColor("#c8963e"), .avatarText = QColor("#cfe0ff"),
        .loginGradientStart = QColor("#0e0e10"), .loginGradientMid = QColor("#231c12"),
    };
}

Palette lightPalette() {
    return {
        .window = QColor("#f5f6f8"), .surface = QColor("#ffffff"), .control = QColor("#eef0f3"),
        .controlHover = QColor("#e3e6eb"), .alternateRow = QColor("#f8f9fb"), .alternateRowDeep = QColor("#f2f3f6"),
        .border = QColor("#d7dbe2"), .borderStrong = QColor("#b9bec8"), .selection = QColor("#d6e2ff"),
        .accent = QColor("#2563eb"), .accentHover = QColor("#1d4ed8"), .accentPressed = QColor("#1e40af"),
        .onAccent = QColor("#ffffff"), .text = QColor("#1f2329"), .textStrong = QColor("#0b0d10"),
        .textMuted = QColor("#5f6672"), .textDisabled = QColor("#a3a9b4"), .danger = QColor("#dc2626"),
        .success = QColor("#15803d"), .warning = QColor("#b45309"), .info = QColor("#7c3aed"),
        .brand = QColor("#b07a2a"), .avatarText = QColor("#1e3a8a"),
        .loginGradientStart = QColor("#ffffff"), .loginGradientMid = QColor("#f3e7cf"),
    };
}

// Near-black, oxblood red, tarnished gold and parchment text.
Palette oxbloodPalette() {
    return {
        .window = QColor("#0b0a0a"), .surface = QColor("#151313"), .control = QColor("#1f1b1b"),
        .controlHover = QColor("#2a2424"), .alternateRow = QColor("#121010"), .alternateRowDeep = QColor("#0e0d0d"),
        .border = QColor("#2a2424"), .borderStrong = QColor("#4a4040"), .selection = QColor("#3d1518"),
        .accent = QColor("#a3161c"), .accentHover = QColor("#c4282e"), .accentPressed = QColor("#7a0f14"),
        .onAccent = QColor("#f3ead6"), .text = QColor("#e6dcc6"), .textStrong = QColor("#f3ead6"),
        .textMuted = QColor("#a89e8a"), .textDisabled = QColor("#6b6258"), .danger = QColor("#e0484e"),
        .success = QColor("#8a9a62"), .warning = QColor("#cfae6d"), .info = QColor("#9c5370"),
        .brand = QColor("#b08d57"), .avatarText = QColor("#e6dcc6"),
        .loginGradientStart = QColor("#0b0a0a"), .loginGradientMid = QColor("#2a1214"),
    };
}

Palette basePalette(const QString &id) {
    if (id == "light") return lightPalette();
    if (id == "oxblood") return oxbloodPalette();
    return darkPalette();
}

QColor mix(const QColor &a, const QColor &b, double amountOfA) {
    return QColor::fromRgbF(a.redF() * amountOfA + b.redF() * (1 - amountOfA),
                            a.greenF() * amountOfA + b.greenF() * (1 - amountOfA),
                            a.blueF() * amountOfA + b.blueF() * (1 - amountOfA));
}

// A custom accent with its hover/pressed shades, selection tint and readable text on top.
void applyAccent(Palette &p, const QColor &accent) {
    const bool lightTheme = p.window.lightnessF() > 0.5;
    p.accent = accent;
    p.accentHover = lightTheme ? accent.darker(112) : accent.lighter(118);
    p.accentPressed = accent.darker(lightTheme ? 125 : 130);
    p.selection = mix(accent, p.surface, lightTheme ? 0.22 : 0.35);
    const double luminance = 0.2126 * accent.redF() + 0.7152 * accent.greenF() + 0.0722 * accent.blueF();
    p.onAccent = luminance > 0.6 ? QColor("#111111") : QColor("#ffffff");
}

Palette g_palette = darkPalette();
int g_startupScale = 100;

} // namespace

QStringList themeIds() {
    return {"dark", "light", "oxblood"};
}

QString themeName(const QString &id) {
    if (id == "light") return QObject::tr("Light");
    if (id == "oxblood") return QObject::tr("Oxblood");
    return QObject::tr("Dark");
}

QString currentThemeId() {
    QString id = localSettings().value("appearance/theme", "dark").toString();
    if (id == "hunter") id = "oxblood"; // its name in 2.0.0-rc3
    return themeIds().contains(id) ? id : "dark";
}

QColor customAccent() {
    return QColor(localSettings().value("appearance/accent").toString());
}

void setTheme(const QString &id, const QColor &accent) {
    QSettings s = localSettings();
    s.setValue("appearance/theme", id);
    s.setValue("appearance/accent", accent.isValid() ? accent.name() : QString());
    apply();
}

const Palette &palette() {
    return g_palette;
}

Palette themePalette(const QString &id) {
    return basePalette(id);
}

QString hex(const QColor &color) {
    return color.name();
}

QString css(const QColor &color) {
    return "color: " + color.name() + ";";
}

void apply() {
    Palette p = basePalette(currentThemeId());
    const QColor accent = customAccent();
    if (accent.isValid()) applyAccent(p, accent);
    g_palette = p;

    // Longest names first, so @accent doesn't eat the start of @accentHover.
    const QList<std::pair<QString, QColor>> roles{
        {"@loginGradientStart", p.loginGradientStart}, {"@loginGradientMid", p.loginGradientMid},
        {"@alternateRowDeep", p.alternateRowDeep}, {"@accentPressed", p.accentPressed},
        {"@textDisabled", p.textDisabled}, {"@controlHover", p.controlHover}, {"@alternateRow", p.alternateRow},
        {"@borderStrong", p.borderStrong}, {"@accentHover", p.accentHover}, {"@avatarText", p.avatarText},
        {"@textStrong", p.textStrong}, {"@textMuted", p.textMuted}, {"@selection", p.selection},
        {"@onAccent", p.onAccent}, {"@surface", p.surface}, {"@control", p.control}, {"@warning", p.warning},
        {"@success", p.success}, {"@window", p.window}, {"@border", p.border}, {"@accent", p.accent},
        {"@danger", p.danger}, {"@brand", p.brand}, {"@info", p.info}, {"@text", p.text},
    };
    QString sheet = QString::fromUtf8(kStyleSheetTemplate);
    for (const auto &[name, color] : roles) sheet.replace(name, color.name());
    qApp->setStyleSheet(sheet);
    emit notifier()->changed();
}

int interfaceScale() {
    return qBound(90, localSettings().value("appearance/scale", 100).toInt(), 150);
}

void setInterfaceScale(int percent) {
    localSettings().setValue("appearance/scale", qBound(90, percent, 150));
}

int startupInterfaceScale() {
    return g_startupScale;
}

void applyScaleBeforeStartup() {
    const int scale = interfaceScale();
    g_startupScale = scale;
    if (scale != 100) qputenv("QT_SCALE_FACTOR", QByteArray::number(scale / 100.0));
}

Notifier *notifier() {
    static Notifier *instance = new Notifier;
    return instance;
}

} // namespace theme

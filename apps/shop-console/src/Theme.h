#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <QStringList>

// The console's colours. Every widget is styled by one stylesheet built from the current
// theme's palette (Settings → Appearance), so switching theme restyles everything live.
// Code that needs a colour directly (charts, status text, small HTML snippets) reads it from
// theme::palette() at the time it draws, instead of hardcoding a hex value.
namespace theme {

struct Palette {
    QColor window;           // the app background
    QColor surface;          // inputs, cards, panels
    QColor control;          // buttons, table headers, hover rows
    QColor controlHover;
    QColor alternateRow;
    QColor alternateRowDeep; // the till's receipt
    QColor border;
    QColor borderStrong;     // scrollbar handle while hovered
    QColor selection;
    QColor accent;           // primary buttons, focus, the selected sidebar item
    QColor accentHover;
    QColor accentPressed;
    QColor onAccent;         // text on accent-coloured backgrounds
    QColor text;
    QColor textStrong;       // titles, totals
    QColor textMuted;        // hints, secondary text
    QColor textDisabled;
    QColor danger;           // errors, "out of stock"
    QColor success;          // saved, sold, hired
    QColor warning;          // low stock, pending
    QColor info;             // practice mode, the odd category
    QColor brand;            // the letter mark, login subtitle
    QColor avatarText;
    QColor loginGradientStart;
    QColor loginGradientMid;
};

// "dark", "light", "oxblood"
QStringList themeIds();
QString themeName(const QString &id);

// The theme and accent this computer uses (Settings → Appearance).
QString currentThemeId();
QColor customAccent(); // invalid = the theme's own accent
void setTheme(const QString &id, const QColor &accent);

const Palette &palette();
// A theme's own colours (without any custom accent), for previews.
Palette themePalette(const QString &id);
// "#rrggbb", and "color: #rrggbb;" for a quick setStyleSheet.
QString hex(const QColor &color);
QString css(const QColor &color);

// Builds the stylesheet from the current palette and applies it to the whole app.
void apply();

// Interface size in percent (90–150). Qt reads it before the app starts, so a change
// needs a restart; applyScaleBeforeStartup() must run before QApplication is created.
int interfaceScale();
void setInterfaceScale(int percent);
void applyScaleBeforeStartup();
int startupInterfaceScale(); // what this run started with; differs from interfaceScale() until a restart

// Emits changed() after every apply(), for anything that draws with palette colours.
class Notifier : public QObject {
    Q_OBJECT
signals:
    void changed();
};
Notifier *notifier();

} // namespace theme

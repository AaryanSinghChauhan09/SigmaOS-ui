#pragma once

// Omarchy UI for Qt Widgets and KDE Frameworks apps. One call after the application exists:
//
//   QApplication app(argc, argv);
//   Omarchy::Widgets::apply(app);        // or Omarchy::init(app)
//
// sets the palette, a QProxyStyle that draws stock widgets the Omarchy way and the font, keeps
// all three following the live theme (fading colors on a switch), and, with KF6, keeps KDE's
// color scheme (KColorScheme, Kirigami's org.kde.desktop theme) in step. The kit's widgets
// (Omarchy::Button, Omarchy::Toggle, ...) are in the headers this one includes.

#include <QApplication>

#include "Omarchy/Export.h"
#include "Omarchy/Theme.h"
#include "Omarchy/Paint.h"
#include "Omarchy/Control.h"
#include "Omarchy/Layout.h"
#include "Omarchy/Style.h"
#include "Omarchy/Button.h"
#include "Omarchy/Toggle.h"
#include "Omarchy/Inputs.h"
#include "Omarchy/Slider.h"
#include "Omarchy/Surfaces.h"
#include "Omarchy/Lists.h"
#include "Omarchy/Feedback.h"
#include "Omarchy/Navigation.h"
#include "Omarchy/Forms.h"
#include "Omarchy/Data.h"
#include "Omarchy/Editing.h"
#include "Omarchy/Composed.h"

namespace Omarchy {

namespace Widgets {

struct Options {
  // The application palette from the tokens.
  bool palette = true;
  // Omarchy::Style over Fusion for every stock widget.
  bool style = true;
  // The application font: fontconfig's monospace at the body size.
  bool font = true;
  // With KF6: a generated KDE color scheme applied the way KColorSchemeManager applies one, so
  // KColorScheme users and Kirigami (org.kde.desktop) follow too.
  bool kdeColorScheme = true;
  // Pick the org.kde.desktop Qt Quick Controls style for Kirigami/QML views when it is installed
  // and the app hasn't chosen one (QT_QUICK_CONTROLS_STYLE unset); it draws with this QStyle.
  bool quickControlsStyle = true;
};

// The one call. Safe to call again (it re-applies). Returns the theme it follows.
OMARCHY_EXPORT Theme *apply(QApplication &app, const Options &options = Options());
OMARCHY_EXPORT Theme *apply(QApplication *app, const Options &options = Options());

// The QPalette the tokens give (what apply() sets), for a widget tree of its own.
OMARCHY_EXPORT QPalette palette(const Tokens &tokens);

// The KDE color scheme file the tokens give (an INI in KColorScheme's format).
OMARCHY_EXPORT QByteArray kdeColorScheme(const Tokens &tokens);
// Where apply() writes it ($XDG_RUNTIME_DIR/omarchy-ui/Omarchy-<pid>.colors); empty without KF6.
OMARCHY_EXPORT QString kdeColorSchemePath();

}  // namespace Widgets

// The same one call.
inline Theme *init(QApplication &app, const Widgets::Options &options = Widgets::Options()) { return Widgets::apply(app, options); }

}  // namespace Omarchy

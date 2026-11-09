// A tiny Omarchy panel in Qt Widgets, following the live theme: the Network panel of
// examples/rune, from the kit's widgets, plus a stock QCheckBox drawn by Omarchy::Style.

#include <QApplication>
#include <QCheckBox>
#include <QTimer>
#include <cstring>

#include <Omarchy/Widgets.h>

using namespace Omarchy;

int main(int argc, char **argv) {
  const char *snap = nullptr;
  for (int i = 1; i + 1 < argc; ++i)
    if (std::strcmp(argv[i], "--snap") == 0) snap = argv[i + 1];
  if (snap) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("QT_SCALE_FACTOR", "2");
  }

  QApplication app(argc, argv);
  // The palette, Omarchy::Style, the font and KDE's color scheme from the live theme, kept
  // following it: colors fade, rounding and fonts switch, on every theme switch.
  Omarchy::Widgets::apply(app);

  const int width = 400;
  QWidget window;
  window.setWindowTitle(QStringLiteral("Network"));
  auto *page = new Column(&window);
  const int pad = shown().spacing("panelPadding");
  page->setContentsMargins(pad, pad, pad, pad);

  auto *pane = new Pane;
  pane->setBordered(true);
  setFill(pane);
  auto *heading = new Heading(QStringLiteral("Network"), QStringLiteral("Wi-Fi and saved networks"));
  heading->setLevel(QStringLiteral("heading"));
  setFill(heading);
  pane->addWidget(heading);

  auto *wifi = new Toggle(QStringLiteral("Wi-Fi"), QStringLiteral("On"));
  wifi->setChecked(true);
  setFill(wifi);
  pane->addWidget(wifi);

  auto *basecamp = new ListRow(QString::fromUcs4(U"\U000F0928", 1), QStringLiteral("Basecamp"));
  basecamp->setStatus(QStringLiteral("Connected"), QStringLiteral("success"));
  basecamp->setValue(QStringLiteral("5 GHz"));
  basecamp->setCurrent(true);
  setFill(basecamp);
  pane->addWidget(basecamp);

  // The app's state drives the views; the theme needs nothing from it.
  QObject::connect(wifi, &Toggle::toggled, [=](bool on) {
    wifi->setDescription(on ? QStringLiteral("On") : QStringLiteral("Off"));
    basecamp->setStatus(on ? QStringLiteral("Connected") : QStringLiteral("Saved"), on ? QStringLiteral("success") : QString());
    basecamp->setCurrent(on);
  });

  // A stock Qt widget sits in the same panel, drawn by Omarchy::Style.
  auto *autoJoin = new QCheckBox(QStringLiteral("Join automatically"));
  autoJoin->setChecked(true);
  pane->addWidget(autoJoin);

  auto *forget = new Button(QStringLiteral("Forget network"));
  forget->setBordered(true);
  forget->setFocusable(true);
  pane->addWidget(forget);

  page->addWidget(pane);
  window.resize(width, page->heightForWidth(width));
  window.setAutoFillBackground(true);
  window.show();

  if (snap) QTimer::singleShot(300, &app, [&] { app.exit(window.grab().save(QString::fromLocal8Bit(snap)) ? 0 : 1); });
  return app.exec();
}

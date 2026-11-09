// A tiny Omarchy panel in Electron, following the live theme (the Electron twin of examples/rune).
//
//   npm install && npm start                  a window; switch themes while it runs
//   npm start -- --snap out.png               render once, offscreen, to a PNG, and quit
//
// The whole adoption: followOmarchy() here, the preload in webPreferences, and
// `import "@omarchy/ui"` in the page.

import { app, BrowserWindow } from "electron";
import { writeFileSync } from "node:fs";
import { followOmarchy, omarchyPreload } from "@omarchy/ui/electron";

const WIDTH = 400;
const HEIGHT = 283;
const snapAt = process.argv.indexOf("--snap");
const snap = snapAt >= 0 ? process.argv[snapAt + 1] : null;

// The one call: spawn omarchy-theme --watch --state and push every change to every window.
followOmarchy();

app.whenReady().then(async () => {
  const win = new BrowserWindow({
    title: "Network",
    width: WIDTH,
    height: HEIGHT,
    show: !snap,
    useContentSize: true,
    autoHideMenuBar: true,
    webPreferences: {
      preload: omarchyPreload,
      contextIsolation: true,
      sandbox: true,
      nodeIntegration: false,
      // A snap renders offscreen: never a window on the desktop.
      offscreen: !!snap,
    },
  });
  // Chromium's headless platform has no screen to size a window against: set the size again.
  if (snap) win.setContentSize(WIDTH, HEIGHT);
  await win.loadFile(new URL("./index.html", import.meta.url).pathname);
  if (snap) {
    await win.webContents.executeJavaScript("window.ready");
    // An offscreen window paints on its own frame clock: take the first frame painted after the
    // page said it was ready.
    await new Promise((r) => setTimeout(r, 100));
    const image = await win.webContents.capturePage();
    writeFileSync(snap, image.toPNG());
    app.quit();
  }
});

app.on("window-all-closed", () => app.quit());

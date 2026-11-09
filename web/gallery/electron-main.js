// The gallery as an Electron app following the desktop: omarchy-web-gallery with no arguments
// (bin/review --live). It is also the smallest complete Electron adoption: one call in main, the
// preload, and `import "@omarchy/ui"` in the page.

import { app, BrowserWindow } from "electron";
import { followOmarchy, omarchyPreload } from "../electron/main.js";

followOmarchy();

app.whenReady().then(() => {
  const win = new BrowserWindow({
    title: "Omarchy UI · Web",
    width: 760,
    height: 900,
    webPreferences: { preload: omarchyPreload, contextIsolation: true, sandbox: true, nodeIntegration: false },
  });
  win.setMenuBarVisibility(false);
  win.loadFile(new URL("./index.html", import.meta.url).pathname);
});

app.on("window-all-closed", () => app.quit());

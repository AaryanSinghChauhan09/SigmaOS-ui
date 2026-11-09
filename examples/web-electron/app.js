// No bundler here, so the installed package by its path (npm links file:../../web into
// node_modules). With Vite, webpack or esbuild: import { omarchy } from "@omarchy/ui".
import { omarchy } from "./node_modules/@omarchy/ui/src/index.js";

const wifi = document.getElementById("wifi");
const row = document.getElementById("basecamp");

wifi.addEventListener("change", () => {
  wifi.description = wifi.checked ? "On" : "Off";
  row.current = wifi.checked;
  row.status = wifi.checked ? "Connected" : "Saved";
  row.statusTone = wifi.checked ? "success" : "";
});

// The changed hook: anything the kit can't reach (a canvas, a cache) re-reads the tokens here.
omarchy.onChange((tokens) => console.log(`theme changed (revision ${omarchy.revision}): ${tokens.palette.background}`));

// For --snap: resolves once the panel has laid out with its font.
window.ready = (async () => {
  await customElements.whenDefined("omarchy-pane");
  await document.fonts.ready;
  await new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)));
  return true;
})();

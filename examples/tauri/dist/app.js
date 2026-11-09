// With a bundler (Vite, esbuild, webpack): import { omarchy } from "@omarchy/ui". This example
// has no Node step, so build.rs copies the package's src/ next to the page.
import { omarchy } from "./omarchy-ui/index.js";

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

// For --snap: settles once the panel has laid out with its font.
window.omarchySnapReady = (async () => {
  await customElements.whenDefined("omarchy-pane");
  await document.fonts.ready;
  await new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)));
  return true;
})();

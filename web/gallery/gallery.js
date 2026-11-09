// The web gallery: every spec/gallery.md section with the kit's elements, in the live theme.
// ?section=<name> shows one (bin/omarchy-web-gallery --snap renders it headless).

import { omarchy, connectOmarchy } from "../src/index.js";
import { ORDER } from "./order.js";

const params = new URLSearchParams(location.search);
const only = params.get("section") || "";
const page = document.getElementById("page");

// Served by the dev helper (no Electron, no harness): follow its event stream.
if (!omarchy.state && location.protocol.startsWith("http")) connectOmarchy("/omarchy/events");

export function section(name, title, body) {
  const el = document.createElement("section");
  el.className = "section";
  el.dataset.section = name;
  el.innerHTML = `<omarchy-section-header text="${title}"></omarchy-section-header><div class="body">${body}</div>`;
  return el;
}

async function build() {
  const names = only ? ORDER.filter((n) => n === only) : ORDER;
  if (only && !names.length) throw new Error(`no section ${only}`);
  for (const name of names) {
    const mod = await import(`./sections/${name}.js`);
    const s = mod.default;
    const el = section(name, s.title, typeof s.html === "function" ? s.html(omarchy) : s.html);
    page.append(el);
    if (s.setup) await s.setup(el.querySelector(".body"), omarchy);
  }
}

const frame = () => new Promise((r) => requestAnimationFrame(() => r()));

build()
  .then(async () => {
    await customElements.whenDefined("omarchy-section-header");
    await document.fonts.ready;
    // Elements that load something (icons) count themselves in window.__omarchyPending.
    for (let i = 0; i < 200 && (window.__omarchyPending || 0) > 0; i++) await new Promise((r) => setTimeout(r, 10));
    await document.fonts.ready;
    await frame();
    await frame();
    window.__galleryReady = true;
  })
  .catch((e) => {
    window.__galleryError = String(e && e.stack ? e.stack : e);
    console.error(e);
  });

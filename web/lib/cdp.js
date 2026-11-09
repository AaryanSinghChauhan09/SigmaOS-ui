// A small DevTools-protocol client for headless Chromium over --remote-debugging-pipe: no port, no
// window, no npm dependency. The gallery's --snap and the tests drive pages with it.
//
//   const browser = await launch();
//   const page = await browser.newPage({ width: 760, height: 400, scale: 2 });
//   await page.goto("file:///…/index.html");
//   await page.eval("1 + 1");                 // 2
//   await page.screenshot("out.png");
//   await browser.close();

import { spawn } from "node:child_process";
import { existsSync, mkdirSync, mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { REPO } from "./core.js";

/** The Chromium to drive: $OMARCHY_WEB_CHROMIUM, else the system's. */
export function findChromium(env = process.env) {
  if (env.OMARCHY_WEB_CHROMIUM) return env.OMARCHY_WEB_CHROMIUM;
  for (const p of ["/usr/bin/chromium", "/usr/bin/chromium-browser", "/usr/bin/google-chrome-stable", "/usr/bin/google-chrome"]) {
    if (existsSync(p)) return p;
  }
  return null;
}

/** Scratch space for browser profiles: the checkout's build/ when there is one (a small /tmp
 *  fills up), else the system temp dir. */
export function scratchDir(prefix) {
  const base = process.env.OMARCHY_WEB_TMP || (existsSync(join(REPO, "build")) ? join(REPO, "build/web/tmp") : tmpdir());
  mkdirSync(base, { recursive: true });
  return mkdtempSync(join(base, prefix));
}

class Connection {
  constructor(write, read) {
    this.write = write;
    this.nextId = 1;
    this.pending = new Map();
    this.handlers = new Map();
    let buf = "";
    read.setEncoding("utf8");
    read.on("data", (chunk) => {
      buf += chunk;
      let i;
      while ((i = buf.indexOf("\0")) >= 0) {
        const msg = JSON.parse(buf.slice(0, i));
        buf = buf.slice(i + 1);
        this.dispatch(msg);
      }
    });
  }

  dispatch(msg) {
    if (msg.id && this.pending.has(msg.id)) {
      const { resolve, reject, method } = this.pending.get(msg.id);
      this.pending.delete(msg.id);
      if (msg.error) reject(new Error(`${method}: ${msg.error.message}`));
      else resolve(msg.result);
      return;
    }
    const key = `${msg.sessionId || ""}:${msg.method}`;
    for (const h of this.handlers.get(key) || []) h(msg.params);
  }

  send(method, params = {}, sessionId) {
    const id = this.nextId++;
    const msg = { id, method, params };
    if (sessionId) msg.sessionId = sessionId;
    return new Promise((resolve, reject) => {
      this.pending.set(id, { resolve, reject, method });
      this.write.write(JSON.stringify(msg) + "\0");
    });
  }

  on(method, sessionId, fn) {
    const key = `${sessionId || ""}:${method}`;
    if (!this.handlers.has(key)) this.handlers.set(key, []);
    this.handlers.get(key).push(fn);
  }

  closeAll(err) {
    for (const { reject } of this.pending.values()) reject(err);
    this.pending.clear();
  }
}

export class Page {
  constructor(conn, sessionId, targetId) {
    this.conn = conn;
    this.sessionId = sessionId;
    this.targetId = targetId;
    this.errors = [];
    this.logs = [];
  }

  send(method, params) {
    return this.conn.send(method, params, this.sessionId);
  }

  on(method, fn) {
    this.conn.on(method, this.sessionId, fn);
  }

  async goto(url, { waitFor = "load", timeout = 15000 } = {}) {
    const loaded = new Promise((resolve) => {
      const event = waitFor === "domcontentloaded" ? "Page.domContentEventFired" : "Page.loadEventFired";
      this.on(event, resolve);
    });
    const r = await this.send("Page.navigate", { url });
    if (r.errorText) throw new Error(`navigate ${url}: ${r.errorText}`);
    await withTimeout(loaded, timeout, `load ${url}`);
  }

  /** Evaluate an expression (awaiting a promise) and return its value. */
  async eval(expression, { timeout = 15000 } = {}) {
    const r = await withTimeout(
      this.send("Runtime.evaluate", { expression, awaitPromise: true, returnByValue: true, userGesture: true }),
      timeout,
      `eval ${expression.slice(0, 80)}`,
    );
    if (r.exceptionDetails) {
      const ex = r.exceptionDetails;
      throw new Error(`page: ${(ex.exception && ex.exception.description) || ex.text}`);
    }
    return r.result.value;
  }

  /** Wait until `expression` is truthy. */
  async waitFor(expression, { timeout = 10000, interval = 20 } = {}) {
    const end = Date.now() + timeout;
    for (;;) {
      const v = await this.eval(expression);
      if (v) return v;
      if (Date.now() > end) throw new Error(`timed out waiting for ${expression}`);
      await sleep(interval);
    }
  }

  async screenshot(path, { width, height } = {}) {
    const params = { format: "png", captureBeyondViewport: false };
    if (width && height) params.clip = { x: 0, y: 0, width, height, scale: 1 };
    const r = await this.send("Page.captureScreenshot", params);
    const data = Buffer.from(r.data, "base64");
    if (path) writeFileSync(path, data);
    return data;
  }

  // Input, as a user gives it: real key and mouse events through the browser.
  async key(key, { modifiers = 0, text } = {}) {
    const def = KEYS[key] || { key, code: key.length === 1 ? `Key${key.toUpperCase()}` : key, text: key.length === 1 ? key : "" };
    const t = text !== undefined ? text : def.text;
    const base = { key: def.key, code: def.code, windowsVirtualKeyCode: def.keyCode || 0, modifiers };
    await this.send("Input.dispatchKeyEvent", { type: t ? "keyDown" : "rawKeyDown", ...base, text: t || undefined, unmodifiedText: t || undefined });
    await this.send("Input.dispatchKeyEvent", { type: "keyUp", ...base });
  }

  async type(text) {
    for (const ch of text) await this.key(ch);
  }

  async mouse(type, x, y, { button = "left", clickCount = 1 } = {}) {
    await this.send("Input.dispatchMouseEvent", { type, x, y, button, clickCount, buttons: type === "mouseReleased" ? 0 : button === "left" ? 1 : 2 });
  }

  async move(x, y) {
    await this.send("Input.dispatchMouseEvent", { type: "mouseMoved", x, y, button: "none", buttons: 0 });
  }

  async click(x, y, opts) {
    await this.move(x, y);
    await this.mouse("mousePressed", x, y, opts);
    await this.mouse("mouseReleased", x, y, opts);
  }

  /** Click the center of the first element `selector` matches (piercing open shadow roots with `>>>`). */
  async clickOn(selector) {
    const box = await this.box(selector);
    await this.click(box.x + box.width / 2, box.y + box.height / 2);
  }

  async box(selector) {
    const r = await this.eval(`(() => { const e = __q(${JSON.stringify(selector)}); if (!e) return null; const b = e.getBoundingClientRect(); return { x: b.x, y: b.y, width: b.width, height: b.height }; })()`);
    if (!r) throw new Error(`no element ${selector}`);
    return r;
  }
}

// Shadow-piercing query for tests: "omarchy-menu >>> .row:nth-child(2)".
const QUERY_HELPER = `
window.__q = (sel) => {
  let scope = document;
  const parts = sel.split(">>>").map((s) => s.trim());
  let el = null;
  for (let i = 0; i < parts.length; i++) {
    el = scope.querySelector(parts[i]);
    if (!el) return null;
    if (i < parts.length - 1) scope = el.shadowRoot || el;
  }
  return el;
};`;

const KEYS = {
  Enter: { key: "Enter", code: "Enter", keyCode: 13, text: "\r" },
  Tab: { key: "Tab", code: "Tab", keyCode: 9 },
  Escape: { key: "Escape", code: "Escape", keyCode: 27 },
  Backspace: { key: "Backspace", code: "Backspace", keyCode: 8 },
  Delete: { key: "Delete", code: "Delete", keyCode: 46 },
  " ": { key: " ", code: "Space", keyCode: 32, text: " " },
  Space: { key: " ", code: "Space", keyCode: 32, text: " " },
  ArrowUp: { key: "ArrowUp", code: "ArrowUp", keyCode: 38 },
  ArrowDown: { key: "ArrowDown", code: "ArrowDown", keyCode: 40 },
  ArrowLeft: { key: "ArrowLeft", code: "ArrowLeft", keyCode: 37 },
  ArrowRight: { key: "ArrowRight", code: "ArrowRight", keyCode: 39 },
  Home: { key: "Home", code: "Home", keyCode: 36 },
  End: { key: "End", code: "End", keyCode: 35 },
  PageUp: { key: "PageUp", code: "PageUp", keyCode: 33 },
  PageDown: { key: "PageDown", code: "PageDown", keyCode: 34 },
};
export const SHIFT = 8;
export const CTRL = 2;
export const ALT = 1;

function sleep(ms) {
  return new Promise((r) => setTimeout(r, ms));
}

function withTimeout(promise, ms, what) {
  let t;
  return Promise.race([
    promise.finally(() => clearTimeout(t)),
    new Promise((_, reject) => {
      t = setTimeout(() => reject(new Error(`timed out: ${what}`)), ms);
    }),
  ]);
}

/**
 * Start headless Chromium. Never shows a window: --headless, and the pipe instead of a port.
 */
export async function launch({ chromium = findChromium(), args = [], env = process.env } = {}) {
  if (!chromium) throw new Error("no Chromium: install chromium or set OMARCHY_WEB_CHROMIUM");
  const profile = scratchDir("chromium-");
  const child = spawn(
    chromium,
    [
      "--headless=new",
      "--remote-debugging-pipe",
      `--user-data-dir=${profile}`,
      "--no-first-run",
      "--no-default-browser-check",
      "--disable-extensions",
      "--disable-background-networking",
      "--disable-sync",
      "--disable-component-update",
      "--mute-audio",
      "--allow-file-access-from-files",
      "--force-color-profile=srgb",
      "--hide-scrollbars",
      "--disable-lcd-text",
      "--ozone-platform=headless",
      ...args,
      "about:blank",
    ],
    { env, stdio: ["ignore", "ignore", "pipe", "pipe", "pipe"] },
  );
  let stderr = "";
  child.stderr.on("data", (d) => {
    stderr += d;
    if (stderr.length > 20000) stderr = stderr.slice(-10000);
  });
  const conn = new Connection(child.stdio[3], child.stdio[4]);
  const exited = new Promise((resolve) => child.on("exit", resolve));
  exited.then(() => conn.closeAll(new Error(`chromium exited: ${stderr.slice(-800)}`)));

  const browser = {
    child,
    conn,
    async newPage({ width = 800, height = 600, scale = 1 } = {}) {
      const { targetId } = await conn.send("Target.createTarget", { url: "about:blank" });
      const { sessionId } = await conn.send("Target.attachToTarget", { targetId, flatten: true });
      const page = new Page(conn, sessionId, targetId);
      page.on("Runtime.exceptionThrown", (p) => page.errors.push(p.exceptionDetails.exception?.description || p.exceptionDetails.text));
      page.on("Runtime.consoleAPICalled", (p) => page.logs.push(`${p.type}: ${p.args.map((a) => a.value ?? a.description).join(" ")}`));
      await page.send("Page.enable");
      await page.send("Runtime.enable");
      await page.send("Emulation.setDeviceMetricsOverride", { width, height, deviceScaleFactor: scale, mobile: false });
      await page.send("Page.addScriptToEvaluateOnNewDocument", { source: QUERY_HELPER });
      return page;
    },
    async close() {
      try {
        await withTimeout(conn.send("Browser.close"), 3000, "Browser.close");
      } catch {
        child.kill("SIGKILL");
      }
      await withTimeout(exited, 5000, "chromium exit").catch(() => child.kill("SIGKILL"));
      rmSync(profile, { recursive: true, force: true });
    },
  };
  return browser;
}

/**
 * Give a page the state of the core and icon lookups, as the Electron preload would: before any
 * script runs, `window.omarchyBridge` holds { initial, subscribe(cb), icons(size, names) }.
 * Returns { push(state) } to deliver a change.
 */
export async function bridgePage(page, initial, lookup) {
  await page.send("Runtime.addBinding", { name: "__omarchyIconRequest" });
  page.on("Runtime.bindingCalled", async (p) => {
    if (p.name !== "__omarchyIconRequest") return;
    const { id, size, names } = JSON.parse(p.payload);
    let found = {};
    try {
      found = await lookup(size, names);
    } catch {
      found = {};
    }
    await page.send("Runtime.evaluate", { expression: `window.__omarchyIconReply(${id}, ${JSON.stringify(found)})` }).catch(() => {});
  });
  const source = `(() => {
    const waiting = new Map();
    let next = 1;
    let deliver = null;
    window.__omarchyIconReply = (id, found) => { const r = waiting.get(id); waiting.delete(id); if (r) r(found); };
    window.__omarchyDeliver = (state) => { if (deliver) deliver(state); };
    window.omarchyBridge = {
      initial: ${JSON.stringify(initial)},
      subscribe(cb) { deliver = cb; },
      icons(size, names) {
        const id = next++;
        return new Promise((resolve) => { waiting.set(id, resolve); __omarchyIconRequest(JSON.stringify({ id, size, names })); });
      },
    };
  })();`;
  await page.send("Page.addScriptToEvaluateOnNewDocument", { source });
  return {
    push: (state) => page.eval(`window.__omarchyDeliver(${JSON.stringify(state)})`),
  };
}

export { sleep };

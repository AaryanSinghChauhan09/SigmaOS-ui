#!/usr/bin/env node
// The Omarchy theme for a plain browser page (or Tauri, or any Chromium shell without a preload):
// a tiny dev server that follows `omarchy-theme --watch --state` and serves it as Server-Sent
// Events, beside static files.
//
//   node web/dev/serve.js [--root DIR] [--port 7357] [--host 127.0.0.1] [--open PATH]
//
//   GET /omarchy/events              text/event-stream: the state now, then one event per change
//   GET /omarchy/state               the state now, as JSON
//   GET /omarchy/icons?size=N&names=a,b   { a: "/omarchy/icon/…", b: null } by the core's lookup
//   GET /omarchy/icon/<n>            an icon file the lookup returned (nothing else is readable)
//   GET /…                           files under --root (default: the current directory)
//
// In the page: <meta name="omarchy-events" content="/omarchy/events"> and import "@omarchy/ui",
// or call connectOmarchy("/omarchy/events"). It binds to 127.0.0.1 only: it is a development aid.
//
// From your own Node server: import { omarchyHandler } from "@omarchy/ui/dev-server" and call it
// first in your request handler (it returns false for URLs it doesn't own).

import { spawn } from "node:child_process";
import { createReadStream, statSync } from "node:fs";
import { createServer } from "node:http";
import { extname, join, normalize, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";
import { lookupIcons, readState, watchState } from "../lib/core.js";

const TYPES = {
  ".html": "text/html; charset=utf-8", ".js": "text/javascript", ".mjs": "text/javascript", ".css": "text/css",
  ".json": "application/json", ".svg": "image/svg+xml", ".png": "image/png", ".xpm": "image/x-xpixmap",
  ".woff2": "font/woff2", ".ttf": "font/ttf", ".ico": "image/x-icon",
};

/**
 * A request handler for the /omarchy/* routes, following the theme from its first call.
 * `(req, res) => boolean`: true when it answered.
 */
export function omarchyHandler({ env = process.env, bin } = {}) {
  let state = readState({ env, bin });
  const clients = new Set();
  const icons = []; // paths handed out, by index
  const iconIndex = new Map();
  const watch = watchState(
    (next, isChange) => {
      if (!isChange && JSON.stringify(next) === JSON.stringify(state)) return;
      state = next;
      for (const res of clients) res.write(`data: ${JSON.stringify(state)}\n\n`);
    },
    { env, bin },
  );

  const handler = (req, res) => {
    const url = new URL(req.url, "http://localhost");
    if (!url.pathname.startsWith("/omarchy/")) return false;
    if (url.pathname === "/omarchy/events") {
      res.writeHead(200, { "content-type": "text/event-stream", "cache-control": "no-store", connection: "keep-alive" });
      res.write(`data: ${JSON.stringify(state)}\n\n`);
      clients.add(res);
      req.on("close", () => clients.delete(res));
      return true;
    }
    if (url.pathname === "/omarchy/state") {
      res.writeHead(200, { "content-type": "application/json", "cache-control": "no-store" });
      res.end(JSON.stringify(state));
      return true;
    }
    if (url.pathname === "/omarchy/icons") {
      const size = Number(url.searchParams.get("size")) || 48;
      const names = String(url.searchParams.get("names") || "").split(",").filter(Boolean).slice(0, 256);
      const found = lookupIcons(size, names, { env, bin });
      const out = {};
      for (const [n, p] of Object.entries(found)) {
        if (!p) {
          out[n] = null;
          continue;
        }
        if (!iconIndex.has(p)) {
          iconIndex.set(p, icons.length);
          icons.push(p);
        }
        out[n] = `/omarchy/icon/${iconIndex.get(p)}${extname(p)}`;
      }
      res.writeHead(200, { "content-type": "application/json", "cache-control": "no-store" });
      res.end(JSON.stringify(out));
      return true;
    }
    const m = /^\/omarchy\/icon\/(\d+)/.exec(url.pathname);
    if (m && icons[Number(m[1])]) {
      const p = icons[Number(m[1])];
      res.writeHead(200, { "content-type": TYPES[extname(p).toLowerCase()] || "application/octet-stream" });
      createReadStream(p).pipe(res);
      return true;
    }
    res.writeHead(404);
    res.end();
    return true;
  };
  handler.stop = () => watch.stop();
  return handler;
}

function serveFile(root, req, res) {
  const url = new URL(req.url, "http://localhost");
  let path = normalize(join(root, decodeURIComponent(url.pathname)));
  if (path !== root && !path.startsWith(root + sep)) {
    res.writeHead(403);
    res.end();
    return;
  }
  try {
    if (statSync(path).isDirectory()) path = join(path, "index.html");
    statSync(path);
  } catch {
    res.writeHead(404);
    res.end("not found");
    return;
  }
  res.writeHead(200, { "content-type": TYPES[extname(path).toLowerCase()] || "application/octet-stream", "cache-control": "no-store" });
  createReadStream(path).pipe(res);
}

/** Start the dev server. Resolves to { server, port, url, stop }. */
export function serve({ root = process.cwd(), port = 7357, host = "127.0.0.1", env = process.env } = {}) {
  const dir = resolve(root);
  const omarchy = omarchyHandler({ env });
  const server = createServer((req, res) => {
    if (!omarchy(req, res)) serveFile(dir, req, res);
  });
  return new Promise((ok, fail) => {
    server.once("error", fail);
    server.listen(port, host, () => {
      const p = server.address().port;
      ok({
        server,
        port: p,
        url: `http://${host}:${p}`,
        stop() {
          omarchy.stop();
          server.closeAllConnections();
          server.close();
        },
      });
    });
  });
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const a = process.argv.slice(2);
  const opt = (n, d) => (a.includes(n) ? a[a.indexOf(n) + 1] : d);
  const here = fileURLToPath(new URL("../..", import.meta.url));
  const open = opt("--open", null);
  const s = await serve({
    root: opt("--root", open ? here : process.cwd()),
    port: Number(opt("--port", "7357")),
    host: opt("--host", "127.0.0.1"),
  });
  console.log(`omarchy: serving ${s.url} (events at ${s.url}/omarchy/events)`);
  if (open) spawn("xdg-open", [s.url + open], { stdio: "ignore", detached: true }).unref();
  const stop = () => {
    s.stop();
    process.exit(0);
  };
  process.on("SIGINT", stop);
  process.on("SIGTERM", stop);
}

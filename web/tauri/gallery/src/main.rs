//! The web gallery (web/gallery, spec/gallery.md) in a real Tauri v2 app on WebKitGTK, adopting
//! the theme the way any Tauri app does: `.plugin(tauri_plugin_omarchy::init())`.
//!
//!   omarchy-tauri-gallery                         a window following the desktop
//!   omarchy-tauri-gallery --list-sections         the sections, one per line, in order
//!   omarchy-tauri-gallery --snap out.png --section buttons --size 760x400 --scale 2
//!                                                 one section to a PNG, on a private broadway
//!                                                 display (no window on the desktop)
//!   omarchy-tauri-gallery --drive PAGE [--size WxH] [--scale N]
//!                                                 PAGE (a path under web/) headless, driven over
//!                                                 stdio by web/lib/tauri.js: one JSON request per
//!                                                 line in, one answer per line out
//!
//! Pages come from web/ itself through the gallery's own `gallery:` protocol, so an edit to the
//! kit shows up without a rebuild. `$OMARCHY_WEB_DIR` overrides where web/ is.

use std::borrow::Cow;
use std::io::{BufRead, Write};
use std::path::{Path, PathBuf};
use std::time::Duration;

use tauri::http::{Response, StatusCode, header};
use std::sync::Arc;
use std::sync::atomic::{AtomicU32, Ordering};

use tauri::{WebviewUrl, WebviewWindowBuilder};
use tauri_plugin_omarchy::{OmarchyExt, snap};

const ORDER_JS: &str = include_str!("../../../gallery/order.js");

fn sections() -> Vec<String> {
    let body = ORDER_JS.split('[').nth(1).unwrap_or("").split(']').next().unwrap_or("");
    body.split(',').map(|s| s.trim().trim_matches('"').to_string()).filter(|s| !s.is_empty()).collect()
}

fn web_dir() -> PathBuf {
    if let Some(d) = std::env::var_os("OMARCHY_WEB_DIR") {
        return PathBuf::from(d);
    }
    // The checkout this binary was built in: web/tauri/target/release/… → web/
    if let Ok(exe) = std::env::current_exe()
        && let Some(web) = exe.ancestors().find(|p| p.join("gallery/order.js").is_file() && p.join("src/index.js").is_file())
    {
        return web.to_path_buf();
    }
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..").canonicalize().unwrap_or_default()
}

fn main() {
    let args: Vec<String> = std::env::args().skip(1).collect();
    let has = |name: &str| args.iter().any(|a| a == name);
    let value = |name: &str| args.iter().position(|a| a == name).and_then(|i| args.get(i + 1)).cloned();
    if has("--help") || has("-h") {
        println!(
            "usage: omarchy-tauri-gallery [--list-sections | --snap OUT [--section NAME] [--size WxH] [--scale N] | --drive PAGE [--size WxH] [--scale N]]"
        );
        return;
    }
    if has("--list-sections") {
        println!("{}", sections().join("\n"));
        return;
    }
    let section = value("--section").unwrap_or_default();
    if !section.is_empty() && !sections().contains(&section) {
        eprintln!("omarchy-tauri-gallery: no section {section} (--list-sections)");
        std::process::exit(1);
    }
    let drive = value("--drive");
    let size = value("--size").and_then(|s| {
        let (w, h) = s.split_once('x')?;
        Some((w.parse::<f64>().ok()?, h.parse::<f64>().ok()?))
    });

    // Headless first, before GTK or GLib start.
    let snap = snap::from_args().map(|s| {
        s.ready("window.__galleryReady || window.__galleryError")
            .failure("window.__galleryError")
            // Size to the content, as Qt's window does (at least 200 tall).
            .content_height(r#"Math.max(200, Math.ceil(document.getElementById("page").getBoundingClientRect().height))"#)
    });
    let display = if snap.is_none() && drive.is_some() {
        let scale = value("--scale").and_then(|s| s.parse::<f64>().ok()).unwrap_or(2.0).round() as i32;
        match snap::display::start() {
            Ok(d) => {
                snap::scale::set(scale);
                Some(d)
            }
            Err(e) => {
                eprintln!("omarchy-tauri-gallery: {e}");
                std::process::exit(1);
            }
        }
    } else {
        None
    };
    let driving = display.is_some();
    let headless = snap.is_some() || driving;

    let page = match &drive {
        Some(p) => p.clone(),
        None if section.is_empty() => "gallery/index.html".into(),
        None => format!("gallery/index.html?section={section}"),
    };
    let url: tauri::Url = format!("gallery://localhost/{}", page.trim_start_matches('/')).parse().expect("url");
    let root = web_dir();
    // A snap starts at its width (and any height: it resizes once the page is up).
    let (w, h) = match (size, headless) {
        (Some((w, h)), true) => (w, if h > 0.0 { h } else { 600.0 }),
        (None, true) => (400.0, 200.0),
        _ => (760.0, 900.0),
    };

    let mut builder = tauri::Builder::default()
        .plugin(tauri_plugin_omarchy::init())
        .register_asynchronous_uri_scheme_protocol("gallery", move |_ctx, request, responder| {
            responder.respond(serve_file(&root, request.uri().path()));
        });
    if let Some(snap) = snap {
        builder = builder.plugin(snap.plugin());
    }
    let app = builder
        .setup(move |app| {
            let win = WebviewWindowBuilder::new(app, "main", WebviewUrl::CustomProtocol(url))
                .title("Omarchy UI · Tauri")
                .inner_size(w, h)
                .decorations(!headless)
                .build()?;
            if driving {
                snap::headless_gtk();
                let handle = app.handle().clone();
                // The Rust side's changed hook, for the tests ({"host":true}).
                let changes = Arc::new(AtomicU32::new(0));
                let counter = changes.clone();
                app.omarchy().on_change(move |_| {
                    counter.fetch_add(1, Ordering::SeqCst);
                });
                std::thread::spawn(move || {
                    drive_page(win.as_ref(), (w, h), &changes);
                    handle.exit(0);
                });
            }
            Ok(())
        })
        .build(tauri::generate_context!())
        .expect("omarchy-tauri-gallery: tauri");
    let mut display = display;
    app.run(move |_, event| {
        if let tauri::RunEvent::Exit = event {
            // broadwayd and its directory (Tauri ends the process right after this).
            display.take();
        }
    });
}

// ---- the gallery: protocol --------------------------------------------------------------------

fn serve_file(root: &Path, path: &str) -> Response<Cow<'static, [u8]>> {
    let respond = |status: StatusCode, mime: &str, body: Vec<u8>| {
        Response::builder()
            .status(status)
            .header(header::CONTENT_TYPE, mime)
            .header(header::CACHE_CONTROL, "no-store")
            .body(Cow::Owned(body))
            .unwrap()
    };
    let missing = || respond(StatusCode::NOT_FOUND, "text/plain", vec![]);
    let rel = percent_decode(path.trim_start_matches('/'));
    let (Ok(root), Ok(file)) = (root.canonicalize(), root.join(&rel).canonicalize()) else { return missing() };
    // Only files under web/ (no ../, no symlink out).
    if !file.starts_with(&root) || !file.is_file() {
        return missing();
    }
    let mime = match file.extension().and_then(|e| e.to_str()).unwrap_or("") {
        "html" => "text/html; charset=utf-8",
        "js" | "mjs" => "text/javascript; charset=utf-8",
        "css" => "text/css; charset=utf-8",
        "json" => "application/json",
        "png" => "image/png",
        "svg" => "image/svg+xml",
        _ => "application/octet-stream",
    };
    match std::fs::read(&file) {
        Ok(b) => respond(StatusCode::OK, mime, b),
        Err(_) => missing(),
    }
}

fn percent_decode(s: &str) -> String {
    let b = s.as_bytes();
    let mut out = Vec::with_capacity(b.len());
    let mut i = 0;
    while i < b.len() {
        if b[i] == b'%'
            && i + 2 < b.len()
            && let Ok(v) = u8::from_str_radix(std::str::from_utf8(&b[i + 1..i + 3]).unwrap_or(""), 16)
        {
            out.push(v);
            i += 3;
            continue;
        }
        out.push(b[i]);
        i += 1;
    }
    String::from_utf8_lossy(&out).into_owned()
}

// ---- --drive: the page over stdio, for web/test ----------------------------------------------

/// One request per stdin line, one answer per stdout line:
///   {"id":1,"eval":"expr"}            → {"id":1,"ok":<JSON value>} or {"id":1,"err":"…"}
///   {"id":2,"snap":"/abs/out.png"}    → {"id":2,"ok":true}
///   {"id":3,"size":[w,h]}             → {"id":3,"ok":true}
///   {"id":4,"reload":true}            → {"id":4,"ok":true} once the page has loaded again
///   {"id":5,"host":true}              → {"id":5,"ok":{"revision":1,"changes":1,"background":"#…"}}:
///                                       the Rust side (app.omarchy() and its on_change)
/// Starts with {"ready":true} once the page has loaded. Ends at EOF.
fn drive_page<R: tauri::Runtime>(webview: &tauri::Webview<R>, (w, h): (f64, f64), changes: &AtomicU32) {
    let print = |v: serde_json::Value| {
        let mut out = std::io::stdout().lock();
        let _ = writeln!(out, "{v}");
        let _ = out.flush();
    };
    let loaded = "document.readyState === 'complete'";
    let ready = snap::wait_for(webview, loaded, Duration::from_secs(20)).and_then(|_| snap::resize(webview, w, h));
    if let Err(e) = ready {
        print(serde_json::json!({ "ready": false, "err": e }));
        return;
    }
    print(serde_json::json!({ "ready": true }));
    for line in std::io::stdin().lock().lines() {
        let Ok(line) = line else { break };
        let Ok(req) = serde_json::from_str::<serde_json::Value>(&line) else { continue };
        let id = req.get("id").cloned().unwrap_or_default();
        let result = if let Some(expr) = req.get("eval").and_then(|v| v.as_str()) {
            snap::eval(webview, expr).and_then(|t| serde_json::from_str::<serde_json::Value>(&t).map_err(|e| e.to_string()))
        } else if let Some(out) = req.get("snap").and_then(|v| v.as_str()) {
            snap::snapshot(webview, Path::new(out)).map(|_| serde_json::Value::Bool(true))
        } else if let Some([w, h]) = req.get("size").and_then(|v| v.as_array()).map(|a| a.as_slice())
            && let (Some(w), Some(h)) = (w.as_f64(), h.as_f64())
        {
            snap::resize(webview, w, h).map(|_| serde_json::Value::Bool(true))
        } else if req.get("host").is_some() {
            let o = webview.omarchy();
            Ok(serde_json::json!({
                "revision": o.revision(),
                "changes": changes.load(Ordering::SeqCst),
                "background": o.tokens().palette.background.hex(),
            }))
        } else if req.get("reload").is_some() {
            let _ = snap::eval(webview, "(window.__driveOld = true)");
            webview
                .reload()
                .map_err(|e| e.to_string())
                .and_then(|_| snap::wait_for(webview, &format!("!window.__driveOld && {loaded}"), Duration::from_secs(20)))
                .map(|_| serde_json::Value::Bool(true))
        } else {
            Err("unknown request".into())
        };
        print(match result {
            Ok(v) => serde_json::json!({ "id": id, "ok": v }),
            Err(e) => serde_json::json!({ "id": id, "err": e }),
        });
    }
}

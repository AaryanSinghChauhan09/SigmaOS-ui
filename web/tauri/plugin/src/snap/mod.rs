//! `--snap out.png`: render an app's first window to a PNG on a private display, never on the
//! desktop (feature `snap`, Linux). For stills of what you built, as every binding has
//! (AGENTS.md), and for the kit's own gallery and tests.
//!
//! ```ignore
//! fn main() {
//!     // First thing: with --snap in the arguments, this starts the private display.
//!     let snap = tauri_plugin_omarchy::snap::from_args();
//!     let mut app = tauri::Builder::default().plugin(tauri_plugin_omarchy::init());
//!     if let Some(snap) = snap {
//!         app = app.plugin(snap.plugin());
//!     }
//!     app.run(tauri::generate_context!()).expect("tauri");
//! }
//! ```
//!
//! `app --snap out.png [--size WxH] [--scale N]` then waits for the first window's page (loaded,
//! fonts in, and `window.omarchySnapReady` settled when the page defines it), sets the window to
//! `WxH` points (default: its size in tauri.conf.json), and writes the page at device scale `N` (default 2).
//!
//! How: a GTK 3 broadway display of its own ([`display`]), WebKit's software path, and
//! `webkit_web_view_get_snapshot`, which has the web process paint the page into a bitmap at the
//! view's device scale. Broadway is always 1×; [`scale`] makes WebKit's views N× when the binary
//! exports it (one line in build.rs: `cargo:rustc-link-arg-bins=-Wl,--export-dynamic-symbol=gtk_widget_get_scale_factor`).
//! Without that line the snap is 1×.

pub mod display;
pub mod scale;

use std::path::{Path, PathBuf};
use std::sync::mpsc;
use std::time::{Duration, Instant};

use tauri::plugin::TauriPlugin;
use tauri::{LogicalSize, Manager, Runtime, Webview};

pub use display::Display;

/// A snap requested on the command line, with its display running.
pub struct Snap {
    pub out: PathBuf,
    /// Points; a height of 0 sizes the window to [`Snap::content_height`]'s answer.
    pub size: Option<(f64, f64)>,
    pub scale: i32,
    ready: String,
    failure: Option<String>,
    content_height: Option<String>,
    display: Option<Display>,
}

/// `--snap OUT [--size WxH] [--scale N]` from the process's arguments: starts the private display
/// and returns the snap, or `None` (nothing started) without `--snap`. Call it first in `main`,
/// before Tauri, GTK or GLib start. Exits with a message when the display can't start.
pub fn from_args() -> Option<Snap> {
    let args: Vec<String> = std::env::args().collect();
    let value = |name: &str| args.iter().position(|a| a == name).and_then(|i| args.get(i + 1)).cloned();
    let out = value("--snap")?;
    let size = value("--size").and_then(|s| {
        let (w, h) = s.split_once('x')?;
        Some((w.parse().ok()?, h.parse().ok()?))
    });
    let scale = value("--scale").and_then(|s| s.parse::<f64>().ok()).unwrap_or(2.0).round() as i32;
    match start(std::path::absolute(&out).unwrap_or_else(|_| PathBuf::from(&out)), size, scale) {
        Ok(s) => Some(s),
        Err(e) => {
            eprintln!("{e}");
            std::process::exit(1);
        }
    }
}

/// The same with the values given: starts the display now.
pub fn start(out: PathBuf, size: Option<(f64, f64)>, scale: i32) -> Result<Snap, String> {
    let display = display::start()?;
    scale::set(scale);
    Ok(Snap {
        out,
        size,
        scale,
        ready: "document.readyState === 'complete' && document.fonts.status === 'loaded'".into(),
        failure: None,
        content_height: None,
        display: Some(display),
    })
}

impl Snap {
    /// Wait for `expr` (truthy, or a promise) in the page before the snap, instead of load + fonts.
    pub fn ready(mut self, expr: impl Into<String>) -> Self {
        self.ready = expr.into();
        self
    }

    /// Fail the snap with `expr`'s text when it's non-empty once the page is ready.
    pub fn failure(mut self, expr: impl Into<String>) -> Self {
        self.failure = Some(expr.into());
        self
    }

    /// The height in points to use when `--size` gives 0: `expr` evaluated in the ready page.
    pub fn content_height(mut self, expr: impl Into<String>) -> Self {
        self.content_height = Some(expr.into());
        self
    }

    /// The plugin that takes the snap of the first webview and exits (0, or 1 with a message).
    pub fn plugin<R: Runtime>(mut self) -> TauriPlugin<R> {
        let display = std::sync::Mutex::new(self.display.take());
        let mut taken = false;
        tauri::plugin::Builder::<R>::new("omarchy-snap")
            .setup(|_app, _api| {
                headless_gtk();
                Ok(())
            })
            .on_webview_ready(move |webview| {
                if std::mem::replace(&mut taken, true) {
                    return;
                }
                let snap = Snap {
                    out: self.out.clone(),
                    size: self.size,
                    scale: self.scale,
                    ready: self.ready.clone(),
                    failure: self.failure.clone(),
                    content_height: self.content_height.clone(),
                    display: None,
                };
                std::thread::spawn(move || {
                    let app = webview.app_handle().clone();
                    match snap.take(&webview) {
                        Ok(()) => app.exit(0),
                        Err(e) => {
                            eprintln!("snap: {e}");
                            app.exit(1);
                        }
                    }
                });
            })
            .on_event(move |_app, event| {
                if let tauri::RunEvent::Exit = event {
                    // broadwayd and its directory (Tauri ends the process right after this).
                    display.lock().unwrap().take();
                }
            })
            .build()
    }

    fn take<R: Runtime>(&self, webview: &Webview<R>) -> Result<(), String> {
        wait_for(webview, &self.ready, Duration::from_secs(20))?;
        if let Some(expr) = &self.failure {
            let text: String = serde_json::from_str(&eval(webview, &format!("String(({expr}) || '')"))?).unwrap_or_default();
            if !text.is_empty() {
                return Err(text);
            }
        }
        eval(webview, "window.omarchySnapReady")?;
        let (w, h) = match self.size {
            Some(s) => s,
            None => {
                // The window's size in tauri.conf.json, else what the page has now.
                let label = webview.window().label().to_string();
                match webview.config().app.windows.iter().find(|w| w.label == label) {
                    Some(w) => (w.width, w.height),
                    None => {
                        let v = eval(webview, "[innerWidth, innerHeight]")?;
                        let v: Vec<f64> = serde_json::from_str(&v).map_err(|e| e.to_string())?;
                        (v[0], v[1])
                    }
                }
            }
        };
        let h = match (&self.content_height, h > 0.0) {
            (_, true) => h,
            (Some(expr), false) => eval(webview, expr)?.parse::<f64>().map_err(|e| e.to_string())?,
            (None, false) => eval(webview, "Math.ceil(document.documentElement.scrollHeight)")?.parse::<f64>().map_err(|e| e.to_string())?,
        };
        resize(webview, w, h)?;
        eval(webview, FRAMES)?;
        snapshot(webview, &self.out)
    }
}

/// What a headless GTK lacks, set once GTK runs: broadway has no screen resolution (-1), and
/// WebKit scales pages by it over 96, so give it a 96 dpi screen as a display would; and no
/// desktop font settings.
pub fn headless_gtk() {
    use gtk::prelude::GtkSettingsExt;
    if let Some(s) = gtk::Settings::default() {
        s.set_gtk_xft_dpi(96 * 1024);
        // Text as Omarchy sets it up (slight hinting, grayscale antialiasing), not GTK's
        // fallback of full hinting, which rounds every glyph advance to a whole device pixel.
        s.set_gtk_xft_hinting(1);
        s.set_gtk_xft_hintstyle(Some("hintslight"));
        s.set_gtk_xft_antialias(1);
        s.set_gtk_xft_rgba(Some("none"));
    }
}

/// Two animation frames.
pub const FRAMES: &str = "new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(() => r(true))))";

/// `expr` (awaited) in the page, as JSON text; Err with the page's exception. Call it off the main
/// thread: it waits for WebKit, which answers on the main thread.
pub fn eval<R: Runtime>(webview: &Webview<R>, expr: &str) -> Result<String, String> {
    let (tx, rx) = mpsc::channel::<Result<String, String>>();
    // The promise is kept reachable from the page until it settles: WebKit drops the completion
    // handler of one nothing else holds.
    let body = format!(
        "const p = (async () => {{ \
           try {{ const v = await ({expr}); return JSON.stringify({{ ok: v === undefined ? null : v }}); }} \
           catch (e) {{ return JSON.stringify({{ err: String(e && e.stack ? e.stack : e) }}); }} \
         }})(); \
         const keep = (window.__omarchySnapPending ||= new Set()); keep.add(p); p.finally(() => keep.delete(p)); \
         return p;"
    );
    webview
        .with_webview(move |pv| {
            use javascriptcore::ValueExt;
            use webkit2gtk::WebViewExt;
            pv.inner().call_async_javascript_function(&body, None, None, None, None::<&gio::Cancellable>, move |r| {
                let _ = tx.send(match r {
                    Ok(v) => Ok(v.to_str().to_string()),
                    Err(e) => Err(e.to_string()),
                });
            });
        })
        .map_err(|e| e.to_string())?;
    let text = rx.recv_timeout(Duration::from_secs(30)).map_err(|_| "no answer from the page".to_string())??;
    let v: serde_json::Value = serde_json::from_str(&text).map_err(|e| format!("{e}: {text}"))?;
    if let Some(err) = v.get("err") {
        return Err(err.as_str().unwrap_or("error").to_string());
    }
    Ok(v.get("ok").cloned().unwrap_or(serde_json::Value::Null).to_string())
}

/// Poll `expr` until it's truthy.
pub fn wait_for<R: Runtime>(webview: &Webview<R>, expr: &str, timeout: Duration) -> Result<(), String> {
    let start = Instant::now();
    loop {
        match eval(webview, expr) {
            Ok(v) if !matches!(v.as_str(), "false" | "null" | "0" | "\"\"") => return Ok(()),
            _ if start.elapsed() > timeout => return Err(format!("timed out waiting for {expr}")),
            _ => std::thread::sleep(Duration::from_millis(20)),
        }
    }
}

/// Set the window to `w`×`h` points and wait until the page sees it. (The first size a window
/// gets on broadway has room for decorations it doesn't draw, so a snap always sets it.)
pub fn resize<R: Runtime>(webview: &Webview<R>, w: f64, h: f64) -> Result<(), String> {
    webview.window().set_size(LogicalSize::new(w, h)).map_err(|e| e.to_string())?;
    wait_for(webview, &format!("innerWidth === {w} && innerHeight === {h}"), Duration::from_secs(5))
        .map_err(|e| format!("{e} ({})", eval(webview, "[innerWidth, innerHeight]").unwrap_or_default()))
}

/// The visible page at the view's device scale, to a PNG.
pub fn snapshot<R: Runtime>(webview: &Webview<R>, out: &Path) -> Result<(), String> {
    let (tx, rx) = mpsc::channel::<Result<(), String>>();
    let out = out.to_path_buf();
    webview
        .with_webview(move |pv| {
            use webkit2gtk::{SnapshotOptions, SnapshotRegion, WebViewExt};
            pv.inner().snapshot(SnapshotRegion::Visible, SnapshotOptions::NONE, None::<&gio::Cancellable>, move |r| {
                let result = r.map_err(|e| e.to_string()).and_then(|surface| {
                    let image = cairo::ImageSurface::try_from(surface).map_err(|_| "not an image surface".to_string())?;
                    let mut f = std::fs::File::create(&out).map_err(|e| format!("{}: {e}", out.display()))?;
                    image.write_to_png(&mut f).map_err(|e| e.to_string())
                });
                let _ = tx.send(result);
            });
        })
        .map_err(|e| e.to_string())?;
    rx.recv_timeout(Duration::from_secs(30)).map_err(|_| "no snapshot".to_string())?
}

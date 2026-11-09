//! Omarchy's look and live theme following for Tauri v2 apps, with one call:
//!
//! ```ignore
//! tauri::Builder::default()
//!     .plugin(tauri_plugin_omarchy::init())
//!     .run(tauri::generate_context!())
//!     .expect("tauri");
//! ```
//!
//! Every webview then gets the same `omarchy` global the Electron preload gives
//! (`web/electron/preload.cjs`): `omarchy.tokens`, `omarchy.revision`, `omarchy.onChange(cb)`,
//! `omarchy.string(key, …)`, `omarchy.colors`, `omarchy.icon(name, size)` and
//! `omarchy.icons(size, names)`. `import "@omarchy/ui"` in the page finds it and applies it as CSS
//! custom properties, with no other code (docs/using-tauri.md).
//!
//! - **Before first paint**: an initialization script installs the state before the page's own
//!   scripts run. It reads the state of the moment from the plugin's `omarchy:` protocol (a reload
//!   after a switch gets the new theme), with the state as of the webview's creation baked in as a
//!   fallback.
//! - **Changes**: [`omarchy_theme::watch`] (the core, in process: no CLI to spawn) delivers each
//!   real change once; the plugin evaluates it into every webview, where `createOmarchy` (the
//!   same source as `web/src/api.js`) applies it and runs the changed hook.
//! - **Icons**: looked up by the core in the active icon theme and served by the `omarchy:`
//!   protocol by name. Only names resolve; no path from the page ever reaches the filesystem.
//! - **Windows** get the theme's background (no white flash before the page paints) and its
//!   light or dark mode, and follow both.
//!
//! The protocol needs no capability: `.plugin(init())` is the whole Rust side. Rust code can follow
//! too, through [`OmarchyExt`]: `app.omarchy().tokens()`, `.revision()`, `.on_change(f)`.

use std::borrow::Cow;
use std::collections::HashMap;
use std::sync::{Arc, Mutex};

use omarchy_theme::Tokens;
use omarchy_theme::icons::{IconLookup, search_dirs};
use tauri::http::{Request, Response, StatusCode, header};
use tauri::plugin::{Builder, Plugin, TauriPlugin};
use tauri::{AppHandle, Manager, RunEvent, Runtime, Webview, Window};

#[cfg(all(feature = "snap", target_os = "linux"))]
pub mod snap;

/// `web/src/api.js`: the renderer API, in the one source both Electron and Tauri carry.
const API_JS: &str = include_str!("../../../src/api.js");

/// The scheme of the plugin's protocol: `omarchy://localhost/state`, `/icons`, `/icon/…`.
pub const SCHEME: &str = "omarchy";

/// The protocol's base URL as a page sees it (WebKitGTK and WKWebView use the scheme as is;
/// WebView2 and Android map custom schemes onto `http://<scheme>.localhost`).
#[cfg(any(windows, target_os = "android"))]
const BASE: &str = "http://omarchy.localhost/";
#[cfg(not(any(windows, target_os = "android")))]
const BASE: &str = "omarchy://localhost/";

type Listener = Box<dyn Fn(&Tokens) + Send + Sync>;

struct Current {
    tokens: Tokens,
    revision: u64,
    /// `tokens.to_state_json(revision)`: what the page's `omarchy.state` is.
    state: String,
}

struct Shared {
    current: Mutex<Current>,
    icons: Mutex<Option<Arc<IconLookup>>>,
    listeners: Mutex<Vec<Listener>>,
    watch: Mutex<Option<omarchy_theme::Watch>>,
}

impl Shared {
    fn lookup(&self, theme: &str) -> Arc<IconLookup> {
        let mut slot = self.icons.lock().unwrap();
        match &*slot {
            Some(l) if l.theme() == theme => l.clone(),
            _ => {
                let l = Arc::new(IconLookup::new(theme, search_dirs()));
                *slot = Some(l.clone());
                l
            }
        }
    }

    fn state(&self) -> String {
        self.current.lock().unwrap().state.clone()
    }
}

/// The theme as Rust code in the app sees it: `app.omarchy()`.
#[derive(Clone)]
pub struct Omarchy(Arc<Shared>);

impl Omarchy {
    /// The tokens now (the canonical tokens of `spec/tokens.md`, typed).
    pub fn tokens(&self) -> Tokens {
        self.0.current.lock().unwrap().tokens.clone()
    }

    /// 0 for the tokens the app started with, plus one per change since.
    pub fn revision(&self) -> u64 {
        self.0.current.lock().unwrap().revision
    }

    /// The state the webviews get, as JSON: `{ theme, iconTheme, language, tokens, colors, strings }`.
    pub fn state_json(&self) -> String {
        self.0.state()
    }

    /// The process's changed hook: `f(tokens)` once per real change, after the webviews were sent
    /// the new state, on the main thread. Never for the initial load.
    pub fn on_change(&self, f: impl Fn(&Tokens) + Send + Sync + 'static) {
        self.0.listeners.lock().unwrap().push(Box::new(f));
    }

    /// Icon `name` at `size` device pixels in the active icon theme, by the core's lookup.
    pub fn icon_path(&self, name: &str, size: u32) -> Option<std::path::PathBuf> {
        if !icon_name_ok(name) {
            return None;
        }
        let theme = self.0.current.lock().unwrap().tokens.icon_theme.clone();
        self.0.lookup(&theme).find(name, size.max(1))
    }
}

/// `app.omarchy()` on anything that reaches the app (`AppHandle`, `Window`, `Webview`).
pub trait OmarchyExt<R: Runtime> {
    fn omarchy(&self) -> Omarchy;
}

impl<R: Runtime, T: Manager<R>> OmarchyExt<R> for T {
    fn omarchy(&self) -> Omarchy {
        self.state::<Omarchy>().inner().clone()
    }
}

/// The plugin. `tauri::Builder::default().plugin(tauri_plugin_omarchy::init())`.
pub fn init<R: Runtime>() -> OmarchyPlugin<R> {
    let tokens = omarchy_theme::load();
    let state = tokens.to_state_json(0);
    let shared = Arc::new(Shared {
        current: Mutex::new(Current { tokens, revision: 0, state }),
        icons: Mutex::new(None),
        listeners: Mutex::new(Vec::new()),
        watch: Mutex::new(None),
    });

    let for_protocol = shared.clone();
    let for_setup = shared.clone();
    let inner = Builder::<R>::new("omarchy")
        .register_asynchronous_uri_scheme_protocol(SCHEME, move |_ctx, request, responder| {
            let shared = for_protocol.clone();
            // Icon lookups read directories: off the main thread.
            std::thread::spawn(move || responder.respond(serve(&shared, &request)));
        })
        .setup(move |app, _api| {
            app.manage(Omarchy(for_setup.clone()));
            follow(app.clone(), for_setup);
            Ok(())
        })
        .build();
    OmarchyPlugin { inner, shared }
}

/// Start the core's watch: each change updates the state, every webview and every window.
fn follow<R: Runtime>(app: AppHandle<R>, shared: Arc<Shared>) {
    let initial = shared.current.lock().unwrap().tokens.clone();
    let on = shared.clone();
    let watch = omarchy_theme::watch(initial, move |tokens| {
        let state = {
            let mut cur = on.current.lock().unwrap();
            cur.revision += 1;
            cur.state = tokens.to_state_json(cur.revision);
            cur.tokens = tokens.clone();
            cur.state.clone()
        };
        // Webviews, windows and the changed hook on the main thread (spec/following.md: the hook
        // runs on the UI thread, so a handler can touch windows).
        let (app, on, tokens) = (app.clone(), on.clone(), tokens.clone());
        let main = app.clone();
        let _ = main.run_on_main_thread(move || {
            let script = push_script(&state);
            for win in app.webview_windows().values() {
                let _ = win.eval(&script);
                dress_window(&win.as_ref().window(), &tokens);
                dress_webview(win.as_ref(), &tokens);
            }
            for f in on.listeners.lock().unwrap().iter() {
                f(&tokens);
            }
        });
    });
    *shared.watch.lock().unwrap() = Some(watch);
}

fn push_script(state: &str) -> String {
    format!("window.__omarchyTauri && window.__omarchyTauri.push({state});")
}

fn background(tokens: &Tokens) -> tauri::window::Color {
    let c = tokens.palette.background;
    tauri::window::Color(c.r, c.g, c.b, 255)
}

fn dress_window<R: Runtime>(window: &Window<R>, tokens: &Tokens) {
    let _ = window.set_background_color(Some(background(tokens)));
    let _ = window.set_theme(Some(if tokens.is_dark() { tauri::Theme::Dark } else { tauri::Theme::Light }));
}

fn dress_webview<R: Runtime>(webview: &Webview<R>, tokens: &Tokens) {
    let _ = webview.set_background_color(Some(background(tokens)));
}

/// What the page runs before its own scripts: `createOmarchy` (web/src/api.js), fed the state of
/// the moment, with icons from the protocol, as `window.omarchy`.
fn init_script(state: &str) -> String {
    let api = API_JS.replacen("export function createOmarchy", "function createOmarchy", 1);
    format!(
        r#"
if (!window.omarchy) {{
  const base = {base};
  let initial = {state};
  try {{
    // The state now, not as of this webview's creation: a reload after a switch gets the new one.
    const x = new XMLHttpRequest();
    x.open("GET", base + "state", false);
    x.send();
    if (x.status === 200) initial = JSON.parse(x.responseText);
  }} catch (e) {{}}
{api}
  const api = createOmarchy();
  api._setIcons(async (size, names) => {{
    const u = new URL("icons", base);
    u.searchParams.set("size", String(size));
    u.searchParams.set("names", Array.from(names || [], String).join(","));
    const r = await fetch(u.href);
    return r.ok ? r.json() : {{}};
  }});
  api._push(initial);
  Object.defineProperty(window, "__omarchyTauri", {{ value: Object.freeze({{ push: (s) => api._push(s) }}) }});
  Object.defineProperty(window, "omarchy", {{ value: api, configurable: true }});
}}
"#,
        base = serde_json::to_string(BASE).unwrap(),
    )
}

/// An icon name the protocol answers: a theme icon name, never a path.
fn icon_name_ok(name: &str) -> bool {
    !name.is_empty() && name.len() <= 255 && !name.contains(['/', '\\', '\0']) && !name.starts_with('.')
}

fn query(uri: &tauri::http::Uri) -> HashMap<String, String> {
    let mut out = HashMap::new();
    for pair in uri.query().unwrap_or("").split('&').filter(|p| !p.is_empty()) {
        let (k, v) = pair.split_once('=').unwrap_or((pair, ""));
        out.insert(decode(k), decode(v));
    }
    out
}

fn decode(s: &str) -> String {
    let b = s.as_bytes();
    let mut out = Vec::with_capacity(b.len());
    let mut i = 0;
    while i < b.len() {
        match b[i] {
            b'+' => out.push(b' '),
            b'%' if i + 2 < b.len() => match u8::from_str_radix(std::str::from_utf8(&b[i + 1..i + 3]).unwrap_or(""), 16) {
                Ok(v) => {
                    out.push(v);
                    i += 2;
                }
                Err(_) => out.push(b'%'),
            },
            c => out.push(c),
        }
        i += 1;
    }
    String::from_utf8_lossy(&out).into_owned()
}

fn encode(s: &str) -> String {
    let mut out = String::new();
    for b in s.bytes() {
        if b.is_ascii_alphanumeric() || b"-_.~".contains(&b) {
            out.push(b as char);
        } else {
            out.push_str(&format!("%{b:02X}"));
        }
    }
    out
}

fn respond(status: StatusCode, mime: &str, body: Vec<u8>) -> Response<Cow<'static, [u8]>> {
    Response::builder()
        .status(status)
        .header(header::CONTENT_TYPE, mime)
        // The page's origin (tauri://localhost, a dev server) is never this scheme's.
        .header(header::ACCESS_CONTROL_ALLOW_ORIGIN, "*")
        .header(header::CACHE_CONTROL, "no-store")
        .body(Cow::Owned(body))
        .unwrap()
}

/// `GET /state`: the state now. `GET /icons?size=48&names=a,b`: `{ a: url | null, … }`.
/// `GET /icon/<size>/<name>`: that icon's file, as the core's lookup finds it.
fn serve(shared: &Shared, request: &Request<Vec<u8>>) -> Response<Cow<'static, [u8]>> {
    let uri = request.uri();
    let path = uri.path().trim_start_matches('/');
    let not_found = || respond(StatusCode::NOT_FOUND, "text/plain", b"not found".to_vec());
    if path == "state" {
        return respond(StatusCode::OK, "application/json", shared.state().into_bytes());
    }
    let theme = shared.current.lock().unwrap().tokens.icon_theme.clone();
    if path == "icons" {
        let q = query(uri);
        let size: u32 = q.get("size").and_then(|s| s.parse::<f64>().ok()).map(|s| s.round().max(1.0) as u32).unwrap_or(48);
        let lookup = shared.lookup(&theme);
        let mut out = serde_json::Map::new();
        for name in q.get("names").map(String::as_str).unwrap_or("").split(',').filter(|n| !n.is_empty()).take(256) {
            let found = icon_name_ok(name) && lookup.find(name, size).is_some_and(|p| mime(&p).is_some());
            let url = found.then(|| format!("{BASE}icon/{size}/{}?theme={}", encode(name), encode(&theme)));
            out.insert(name.to_string(), url.map_or(serde_json::Value::Null, serde_json::Value::from));
        }
        return respond(StatusCode::OK, "application/json", serde_json::Value::Object(out).to_string().into_bytes());
    }
    if let Some(rest) = path.strip_prefix("icon/") {
        let Some((size, name)) = rest.split_once('/') else { return not_found() };
        let (Ok(size), name) = (size.parse::<u32>(), decode(name)) else { return not_found() };
        if !icon_name_ok(&name) {
            return not_found();
        }
        let Some(file) = shared.lookup(&theme).find(&name, size.max(1)) else { return not_found() };
        let Some(kind) = mime(&file) else { return not_found() };
        return match std::fs::read(&file) {
            Ok(bytes) => respond(StatusCode::OK, kind, bytes),
            Err(_) => not_found(),
        };
    }
    not_found()
}

fn mime(path: &std::path::Path) -> Option<&'static str> {
    match path.extension()?.to_str()?.to_ascii_lowercase().as_str() {
        "svg" => Some("image/svg+xml"),
        "png" => Some("image/png"),
        "xpm" => Some("image/x-xpixmap"),
        _ => None,
    }
}

/// The plugin: Tauri's own (the protocol, the setup) plus a fresh initialization script for every
/// webview (a builder's script is fixed when the app starts; the theme isn't).
pub struct OmarchyPlugin<R: Runtime> {
    inner: TauriPlugin<R>,
    shared: Arc<Shared>,
}

impl<R: Runtime> Plugin<R> for OmarchyPlugin<R> {
    fn name(&self) -> &'static str {
        self.inner.name()
    }

    fn initialize(&mut self, app: &AppHandle<R>, config: serde_json::Value) -> Result<(), Box<dyn std::error::Error>> {
        self.inner.initialize(app, config)
    }

    fn initialization_script(&self) -> Option<String> {
        Some(init_script(&self.shared.state()))
    }

    fn window_created(&mut self, window: Window<R>) {
        dress_window(&window, &self.shared.current.lock().unwrap().tokens);
        self.inner.window_created(window)
    }

    fn webview_created(&mut self, webview: Webview<R>) {
        dress_webview(&webview, &self.shared.current.lock().unwrap().tokens);
        self.inner.webview_created(webview)
    }

    fn on_navigation(&mut self, webview: &Webview<R>, url: &tauri::Url) -> bool {
        self.inner.on_navigation(webview, url)
    }

    fn on_page_load(&mut self, webview: &Webview<R>, payload: &tauri::webview::PageLoadPayload<'_>) {
        self.inner.on_page_load(webview, payload)
    }

    fn on_event(&mut self, app: &AppHandle<R>, event: &RunEvent) {
        if let RunEvent::Exit = event {
            self.shared.watch.lock().unwrap().take();
        }
        self.inner.on_event(app, event)
    }

    fn extend_api(&mut self, invoke: tauri::ipc::Invoke<R>) -> bool {
        self.inner.extend_api(invoke)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn names_never_paths() {
        assert!(icon_name_ok("firefox"));
        assert!(icon_name_ok("org.gnome.Nautilus"));
        assert!(!icon_name_ok("/etc/passwd"));
        assert!(!icon_name_ok("../x"));
        assert!(!icon_name_ok(".hidden"));
        assert!(!icon_name_ok("a/b"));
        assert!(!icon_name_ok(""));
    }

    #[test]
    fn decodes_queries() {
        assert_eq!(decode("a%2Cb+c"), "a,b c");
        assert_eq!(decode("%2Fetc"), "/etc");
        assert_eq!(decode("100%"), "100%");
        assert_eq!(encode("a b/c"), "a%20b%2Fc");
    }

    #[test]
    fn the_script_carries_the_api() {
        let s = init_script(r#"{"tokens":{}}"#);
        assert!(s.contains("function createOmarchy()"));
        assert!(!s.contains("export function"));
        assert!(s.contains(BASE));
    }
}

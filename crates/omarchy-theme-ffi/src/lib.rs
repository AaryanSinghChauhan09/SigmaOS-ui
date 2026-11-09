//! `libomarchy-theme`: the Omarchy UI resolver (`omarchy-theme`) as a C library, so a binding in
//! any language (GTK through GObject, Qt Widgets, Python, Node) reads the same tokens and follows
//! the same watch instead of re-implementing them. The API is `include/omarchy-theme.h`, which
//! documents every function; docs/using-c.md shows it in use.
//!
//! Every string returned is UTF-8, NUL-terminated, owned by the caller and freed with
//! `omarchy_theme_free`. Every function is safe to call from any thread.

use std::ffi::{CStr, CString, c_char, c_int, c_void};
use std::io::{ErrorKind, Read, Write};
use std::os::fd::AsRawFd;
use std::os::unix::net::UnixStream;
use std::ptr;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Mutex};

use omarchy_theme::icons::{self, IconLookup};
use omarchy_theme::{Tokens, strings};

/// The header's `OMARCHY_THEME_ABI`: bumped with the soname when a signature changes.
pub const ABI: u32 = 0;

fn out(s: String) -> *mut c_char {
    CString::new(s).map(CString::into_raw).unwrap_or(ptr::null_mut())
}

/// # Safety
/// `s` is NULL or a NUL-terminated string.
unsafe fn arg<'a>(s: *const c_char) -> Option<&'a str> {
    if s.is_null() { None } else { unsafe { CStr::from_ptr(s) }.to_str().ok() }
}

/// Frees a string this library returned. NULL is ignored.
///
/// # Safety
/// `s` is NULL or a string from this library, not yet freed.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn omarchy_theme_free(s: *mut c_char) {
    if !s.is_null() {
        drop(unsafe { CString::from_raw(s) });
    }
}

/// The library's ABI version (`OMARCHY_THEME_ABI` in the header it was built with).
#[unsafe(no_mangle)]
pub extern "C" fn omarchy_theme_abi_version() -> u32 {
    ABI
}

/// The tokens now, as the canonical JSON of spec/tokens.md, on one line.
#[unsafe(no_mangle)]
pub extern "C" fn omarchy_theme_tokens_json() -> *mut c_char {
    out(omarchy_theme::load().to_canonical_json_compact())
}

/// Theme name, icon theme, language, tokens and strings in one JSON object (`--state`).
#[unsafe(no_mangle)]
pub extern "C" fn omarchy_theme_state_json() -> *mut c_char {
    out(omarchy_theme::load().to_state_json(0))
}

/// The theme's name (`tokyo-night`), or the pinned directory's name.
#[unsafe(no_mangle)]
pub extern "C" fn omarchy_theme_name() -> *mut c_char {
    out(omarchy_theme::source::load_sources().theme_name)
}

/// The freedesktop icon theme the Omarchy theme names (`Yaru-blue` when none).
#[unsafe(no_mangle)]
pub extern "C" fn omarchy_theme_icon_theme() -> *mut c_char {
    out(icons::theme_name())
}

/// One lookup per icon theme, rebuilt when the theme names another.
static ICONS: Mutex<Option<IconLookup>> = Mutex::new(None);

/// The file icon `name` resolves to at `size` device pixels in the icon theme, or NULL.
///
/// # Safety
/// `name` is NULL or a NUL-terminated string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn omarchy_theme_icon_lookup(name: *const c_char, size: c_int) -> *mut c_char {
    let Some(name) = (unsafe { arg(name) }) else { return ptr::null_mut() };
    let theme = icons::theme_name();
    let mut cache = ICONS.lock().unwrap_or_else(|e| e.into_inner());
    if cache.as_ref().is_none_or(|l| l.theme() != theme) {
        *cache = Some(IconLookup::new(theme, icons::search_dirs()));
    }
    match cache.as_ref().and_then(|l| l.find(name, size.max(1) as u32)) {
        Some(p) => out(p.display().to_string()),
        None => ptr::null_mut(),
    }
}

/// The kit's strings in the display language: `{"language":"de","strings":{"common.cancel":...}}`.
#[unsafe(no_mangle)]
pub extern "C" fn omarchy_theme_strings_json() -> *mut c_char {
    out(strings::table_json(&strings::language()))
}

/// One of the kit's strings in the display language, or NULL when the catalog has no `key`.
///
/// # Safety
/// `key` is NULL or a NUL-terminated string.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn omarchy_theme_string(key: *const c_char) -> *mut c_char {
    let Some(key) = (unsafe { arg(key) }) else { return ptr::null_mut() };
    match strings::lookup(&strings::language(), key) {
        Some(s) => out(s.to_string()),
        None => ptr::null_mut(),
    }
}

/// `void (*)(const char *tokens_json, uint64_t revision, void *user_data)`.
pub type Changed = extern "C" fn(*const c_char, u64, *mut c_void);
/// `void (*)(void *user_data)`.
pub type Destroy = extern "C" fn(*mut c_void);

struct Callback {
    f: Changed,
    data: *mut c_void,
    destroy: Option<Destroy>,
}

// The caller hands over `data` for the callback's thread; that's the contract in the header.
unsafe impl Send for Callback {}

struct Shared {
    /// The tokens the watch knows: the initial load, then each change.
    current: Mutex<String>,
    /// A change not yet taken.
    pending: Mutex<Option<String>>,
    revision: AtomicU64,
    /// The callback variant's callback; `None` once freed.
    callback: Mutex<Option<Callback>>,
    /// The fd variant's write end: one byte per change makes the read end readable.
    writer: Option<UnixStream>,
}

/// A running watch (opaque in C).
pub struct OmarchyThemeWatch {
    shared: Arc<Shared>,
    reader: Option<UnixStream>,
    _watch: omarchy_theme::Watch,
}

fn start(callback: Option<Callback>) -> *mut OmarchyThemeWatch {
    let (reader, writer) = if callback.is_none() {
        match UnixStream::pair() {
            Ok((r, w)) if r.set_nonblocking(true).is_ok() && w.set_nonblocking(true).is_ok() => (Some(r), Some(w)),
            _ => return ptr::null_mut(),
        }
    } else {
        (None, None)
    };
    let initial: Tokens = omarchy_theme::load();
    let shared = Arc::new(Shared {
        current: Mutex::new(initial.to_canonical_json_compact()),
        pending: Mutex::new(None),
        revision: AtomicU64::new(0),
        callback: Mutex::new(callback),
        writer,
    });
    let deliver = shared.clone();
    let watch = omarchy_theme::watch(initial, move |fresh| {
        let json = fresh.to_canonical_json_compact();
        let revision = deliver.revision.fetch_add(1, Ordering::SeqCst) + 1;
        *deliver.current.lock().unwrap_or_else(|e| e.into_inner()) = json.clone();
        if let Some(w) = &deliver.writer {
            *deliver.pending.lock().unwrap_or_else(|e| e.into_inner()) = Some(json);
            // A full buffer is already readable; nothing to add.
            let _ = (&*w).write(&[1]);
        } else if let Some(cb) = deliver.callback.lock().unwrap_or_else(|e| e.into_inner()).as_ref()
            && let Ok(c) = CString::new(json)
        {
            (cb.f)(c.as_ptr(), revision, cb.data);
        }
    });
    Box::into_raw(Box::new(OmarchyThemeWatch { shared, reader, _watch: watch }))
}

/// Start following the desktop for a main loop: watch `omarchy_theme_watch_fd` for readability
/// and call `omarchy_theme_watch_take`. NULL when the fd can't be made.
#[unsafe(no_mangle)]
pub extern "C" fn omarchy_theme_watch_new() -> *mut OmarchyThemeWatch {
    start(None)
}

/// Start following the desktop with a callback, called on a background thread once per change
/// with the new tokens JSON (valid during the call) and the revision. `destroy`, when not NULL,
/// gets `user_data` once the watch is freed.
///
/// # Safety
/// `user_data` must be usable from the callback's thread until `destroy` (or the free) runs.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn omarchy_theme_watch_callback(
    callback: Option<Changed>,
    user_data: *mut c_void,
    destroy: Option<Destroy>,
) -> *mut OmarchyThemeWatch {
    let Some(f) = callback else { return ptr::null_mut() };
    start(Some(Callback { f, data: user_data, destroy }))
}

/// The fd that turns readable when a change waits in `omarchy_theme_watch_take`; -1 for a
/// callback watch. Owned by the watch: don't close it.
///
/// # Safety
/// `w` is NULL or a live watch.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn omarchy_theme_watch_fd(w: *const OmarchyThemeWatch) -> c_int {
    let Some(w) = (unsafe { w.as_ref() }) else { return -1 };
    w.reader.as_ref().map_or(-1, |r| r.as_raw_fd())
}

/// The latest change's tokens JSON, or NULL when nothing changed since the last take. Clears
/// the fd's readability. Several changes between two takes return only the latest (the
/// revision still counts each).
///
/// # Safety
/// `w` is NULL or a live watch.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn omarchy_theme_watch_take(w: *mut OmarchyThemeWatch) -> *mut c_char {
    let Some(w) = (unsafe { w.as_ref() }) else { return ptr::null_mut() };
    if let Some(mut r) = w.reader.as_ref() {
        let mut buf = [0u8; 64];
        loop {
            match r.read(&mut buf) {
                Ok(n) if n > 0 => continue,
                Err(e) if e.kind() == ErrorKind::Interrupted => continue,
                _ => break,
            }
        }
    }
    match w.shared.pending.lock().unwrap_or_else(|e| e.into_inner()).take() {
        Some(json) => out(json),
        None => ptr::null_mut(),
    }
}

/// The tokens JSON the watch has now: the initial load until the first change, then the latest.
/// Read the starting tokens from here, so none can slip between reading them and watching.
///
/// # Safety
/// `w` is NULL or a live watch.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn omarchy_theme_watch_current_json(w: *const OmarchyThemeWatch) -> *mut c_char {
    let Some(w) = (unsafe { w.as_ref() }) else { return ptr::null_mut() };
    out(w.shared.current.lock().unwrap_or_else(|e| e.into_inner()).clone())
}

/// 0 for the initial load, plus one per change delivered so far.
///
/// # Safety
/// `w` is NULL or a live watch.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn omarchy_theme_watch_revision(w: *const OmarchyThemeWatch) -> u64 {
    unsafe { w.as_ref() }.map_or(0, |w| w.shared.revision.load(Ordering::SeqCst))
}

/// Stop the watch and free it. Waits for a callback already running, so don't call it from
/// inside the callback. NULL is ignored.
///
/// # Safety
/// `w` is NULL or a live watch, freed once.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn omarchy_theme_watch_free(w: *mut OmarchyThemeWatch) {
    if w.is_null() {
        return;
    }
    let w = unsafe { Box::from_raw(w) };
    let cb = w.shared.callback.lock().unwrap_or_else(|e| e.into_inner()).take();
    drop(w);
    if let Some(Callback { data, destroy: Some(destroy), .. }) = cb {
        destroy(data);
    }
}

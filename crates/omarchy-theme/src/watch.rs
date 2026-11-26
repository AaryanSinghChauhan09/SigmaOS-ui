//! Following the theme live, without polling.
//!
//! - A theme switch swaps `~/.local/state/omarchy/current/theme` as a whole directory and then
//!   rewrites `current/theme.name`, so the watch is on `current/`, not on files inside the theme.
//!   The theme directory itself is watched too, re-armed after every change (a swap leaves the
//!   watch on the old directory), so an edit inside it, such as a new `icons.theme`, is seen.
//! - `~/.config/omarchy/shell.toml` (`omarchy display text size`) and fontconfig's `fonts.conf`
//!   (`omarchy font set`) are watched through their parent directories, since editors and the
//!   CLIs replace them by rename.
//! - Hyprland announces `configreloaded` on `.socket2.sock` once new rounding and gaps are in
//!   effect, which also covers `omarchy toggle animations`.
//!
//! Events are coalesced over 150 ms, then the tokens are re-resolved and delivered only if
//! they changed.

use std::io::{BufRead, BufReader};
use std::os::unix::net::UnixStream;
use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, mpsc};
use std::time::Duration;

use notify::{RecursiveMode, Watcher as _};

use crate::Tokens;
use crate::source;

/// Keeps the watch running; dropping it stops delivering changes. Once the drop returns, no
/// new delivery starts (one already running finishes on its own thread).
pub struct Watch {
    _watcher: Arc<Mutex<Option<notify::RecommendedWatcher>>>,
    stop: Arc<AtomicBool>,
    wake: mpsc::Sender<()>,
}

impl Drop for Watch {
    fn drop(&mut self) {
        self.stop.store(true, Ordering::SeqCst);
        // Wake the delivery thread so it sees `stop` and ends, letting the rest go with it.
        let _ = self.wake.send(());
    }
}

/// Call `on_change` on a background thread with fresh tokens whenever the theme, the user's
/// shell.toml, the font or Hyprland's config changes. With `OMARCHY_UI_THEME_DIR` set, watches
/// that directory instead and ignores the desktop.
pub fn watch(initial: Tokens, mut on_change: impl FnMut(Tokens) + Send + 'static) -> Watch {
    let (tx, rx) = mpsc::channel::<()>();

    let mut dirs: Vec<PathBuf> = Vec::new();
    if source::is_pinned() {
        dirs.push(source::theme_dir());
    } else {
        dirs.push(source::current_dir());
        if let Some(p) = source::user_shell_path().parent() {
            dirs.push(p.to_path_buf());
        }
        if let Some(h) = std::env::var_os("HOME") {
            dirs.push(PathBuf::from(h).join(".config/fontconfig"));
        }
    }

    let fs_tx = tx.clone();
    let watcher = notify::recommended_watcher(move |res: notify::Result<notify::Event>| {
        if let Ok(ev) = res
            && !ev.kind.is_access()
        {
            let _ = fs_tx.send(());
        }
    })
    .ok()
    .map(|mut w| {
        for d in &dirs {
            let _ = w.watch(d, RecursiveMode::NonRecursive);
        }
        if !source::is_pinned() {
            let _ = w.watch(&source::theme_dir(), RecursiveMode::NonRecursive);
        }
        w
    });
    let watcher = Arc::new(Mutex::new(watcher));
    let rearm = watcher.clone();
    let stop = Arc::new(AtomicBool::new(false));
    let stopped = stop.clone();

    if !source::is_pinned() {
        let hypr_tx = tx.clone();
        std::thread::Builder::new()
            .name("omarchy-theme-hypr".into())
            .spawn(move || hypr_events(hypr_tx))
            .ok();
    }

    std::thread::Builder::new()
        .name("omarchy-theme-watch".into())
        .spawn(move || {
            let mut last = initial;
            while rx.recv().is_ok() {
                // Coalesce the burst a theme switch makes (dir swap, theme.name, Hyprland reload).
                while rx.recv_timeout(Duration::from_millis(150)).is_ok() {}
                if stopped.load(Ordering::SeqCst) {
                    return;
                }
                if !source::is_pinned()
                    && let Some(w) = rearm.lock().unwrap().as_mut()
                {
                    let theme = source::theme_dir();
                    let _ = w.unwatch(&theme);
                    let _ = w.watch(&theme, RecursiveMode::NonRecursive);
                }
                let next = crate::load();
                if next != last && !stopped.load(Ordering::SeqCst) {
                    last = next.clone();
                    on_change(next);
                }
            }
        })
        .ok();

    Watch {
        _watcher: watcher,
        stop,
        wake: tx,
    }
}

fn hypr_events(tx: mpsc::Sender<()>) {
    loop {
        if let Some(dir) = source::hypr_dir()
            && let Ok(stream) = UnixStream::connect(dir.join(".socket2.sock"))
        {
            for line in BufReader::new(stream).lines() {
                let Ok(line) = line else { break };
                if line.starts_with("configreloaded>>") && tx.send(()).is_err() {
                    return;
                }
            }
        }
        // Hyprland restarted or isn't up yet; try again shortly.
        std::thread::sleep(Duration::from_secs(2));
    }
}

//! The live-switch test (spec/following.md) for the Rust watch: a real `omarchy theme set`,
//! simulated by tools/live/theme-set under a HOME of its own, reaches `watch` once, with the new
//! tokens; an event that changes nothing reaches it not at all.
//!
//! One test in its own binary, because it points HOME and XDG_RUNTIME_DIR at temp directories
//! for the whole process.

use std::path::{Path, PathBuf};
use std::process::Command;
use std::sync::mpsc;
use std::time::Duration;

fn root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")
}

fn theme_set(home: &Path, name: &str) {
    let ok = Command::new(root().join("tools/live/theme-set")).arg(home).arg(name).status().expect("tools/live/theme-set");
    assert!(ok.success(), "theme-set {name}");
}

/// A desktop of its own: fresh HOME, no Hyprland, nothing pinned (what tools/live/run sets up).
fn isolate() -> PathBuf {
    let tmp = std::env::temp_dir().join(format!("omarchy-theme-live-{}", std::process::id()));
    let _ = std::fs::remove_dir_all(&tmp);
    std::fs::create_dir_all(tmp.join("home")).unwrap();
    std::fs::create_dir_all(tmp.join("run")).unwrap();
    let cache = std::env::var_os("XDG_CACHE_HOME")
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from(std::env::var_os("HOME").unwrap_or_default()).join(".cache"));
    // SAFETY: this binary's only test sets these before it starts any thread.
    unsafe {
        std::env::set_var("HOME", tmp.join("home"));
        std::env::set_var("XDG_RUNTIME_DIR", tmp.join("run"));
        std::env::set_var("XDG_CACHE_HOME", cache);
        std::env::set_var("OMARCHY_UI_ROUNDING", "0");
        std::env::remove_var("OMARCHY_UI_THEME_DIR");
        std::env::remove_var("HYPRLAND_INSTANCE_SIGNATURE");
    }
    tmp
}

#[test]
fn a_theme_switch_reaches_the_watch_once() {
    let tmp = isolate();
    let home = tmp.join("home");
    theme_set(&home, "tokyo-night");

    let initial = omarchy_theme::load();
    assert_eq!(initial.palette.background.hex(), "#1a1b26ff");
    assert_eq!(initial.theme, "tokyo-night");

    let (tx, rx) = mpsc::channel();
    let _watch = omarchy_theme::watch(initial, move |fresh| {
        let _ = tx.send(fresh);
    });
    assert!(rx.recv_timeout(Duration::from_millis(400)).is_err(), "the initial load is not a change");

    theme_set(&home, "catppuccin-latte");
    let fresh = rx.recv_timeout(Duration::from_secs(5)).expect("the switch is delivered");
    assert_eq!(fresh.palette.background.hex(), "#eff1f5ff");
    assert_eq!(fresh.theme, "catppuccin-latte");
    assert!(!fresh.is_dark());
    assert!(rx.recv_timeout(Duration::from_millis(600)).is_err(), "one switch is one change");

    // Something in current/ that alters no token (omarchy-theme-set rewrites theme.generation
    // and the background link on every run) is coalesced, re-resolved, and dropped.
    std::fs::write(home.join(".local/state/omarchy/current/theme.generation"), "again\n").unwrap();
    assert!(rx.recv_timeout(Duration::from_millis(800)).is_err(), "no change, no delivery");

    // An edit inside the swapped-in directory is followed: the watch moved to the new one.
    std::fs::write(home.join(".local/state/omarchy/current/theme/icons.theme"), "Yaru-red\n").unwrap();
    let fresh = rx.recv_timeout(Duration::from_secs(5)).expect("the icon theme edit is delivered");
    assert_eq!(fresh.icon_theme, "Yaru-red");

    let _ = std::fs::remove_dir_all(&tmp);
}

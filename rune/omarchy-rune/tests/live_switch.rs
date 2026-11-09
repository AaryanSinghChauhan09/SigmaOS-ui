//! The live-switch test (spec/following.md) for Rune: an app started the one-call way
//! ([`omarchy_rune::app`]) gets a simulated `omarchy theme set` as new tokens, its changed hook
//! fires once, and its own setup and poll still run beside the kit's.
//!
//! One test in its own binary, because it points HOME and XDG_RUNTIME_DIR at temp directories
//! for the whole process.

use std::path::{Path, PathBuf};
use std::process::Command;
use std::sync::Arc;
use std::sync::mpsc;
use std::time::Duration;

use omarchy_rune::omarchy_theme::Tokens;
use omarchy_rune::rune::{AppSpec, Cx, El, Env, Runtime, Tx, Waker, el};
use omarchy_rune::{Theme, Themed};

struct App {
    theme: Theme,
    /// Backgrounds the changed hook saw, and the revision with each.
    changes: Vec<(String, u64)>,
    own_setups: u32,
    own_polls: u32,
}

impl Themed for App {
    fn theme(&self) -> &Theme {
        &self.theme
    }
    fn theme_mut(&mut self) -> &mut Theme {
        &mut self.theme
    }
    fn theme_changed(&mut self, tokens: &Tokens) {
        // In effect already: the theme the views read has the tokens the hook was handed.
        assert_eq!(self.theme.tokens(), tokens);
        self.changes.push((tokens.palette.background.hex(), self.theme.revision()));
    }
}

fn init() -> App {
    App { theme: Theme::load(), changes: Vec::new(), own_setups: 0, own_polls: 0 }
}

fn view(s: &App, _: &mut Cx<App>) -> El {
    el().key("page").w_full().h_full().fill(s.theme.bg()).radius(s.theme.radius())
}

fn own_setup(s: &mut App, _: Waker) {
    s.own_setups += 1;
}

fn own_poll(s: &mut App, _: f64, _: &mut Tx) -> bool {
    s.own_polls += 1;
    false
}

fn root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")
}

fn theme_set(home: &Path, name: &str) {
    let ok = Command::new(root().join("tools/live/theme-set")).arg(home).arg(name).status().expect("tools/live/theme-set");
    assert!(ok.success(), "theme-set {name}");
}

/// A desktop of its own: fresh HOME, no Hyprland, nothing pinned (what tools/live/run sets up).
fn isolate() -> PathBuf {
    let tmp = std::env::temp_dir().join(format!("omarchy-rune-live-{}", std::process::id()));
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
fn a_theme_switch_reaches_a_rune_app_once() {
    let tmp = isolate();
    let home = tmp.join("home");
    theme_set(&home, "tokyo-night");

    let spec = omarchy_rune::app(AppSpec {
        setup: Some(own_setup),
        poll: Some(own_poll),
        ..AppSpec::new("live", (240.0, 120.0), init, view)
    });
    let again = omarchy_rune::app(spec);
    let mut rt = Runtime::new(init(), again.view, (240.0, 120.0), Env { system_dark: true, scale: 1.0 });
    rt.poll = again.poll;
    let (tx, woken) = mpsc::channel();
    let wake: Waker = Arc::new(move || {
        let _ = tx.send(());
    });
    (again.setup.expect("the kit's setup"))(&mut rt.state, wake);
    assert_eq!(rt.state.own_setups, 1, "the app's own setup runs once, wrapped once");
    assert_eq!(rt.state.theme.tokens().palette.background.hex(), "#1a1b26ff");

    let mut t = 0.0;
    let mut frame = |rt: &mut Runtime<App>| {
        t += 1.0 / 60.0;
        rt.frame(t);
    };
    frame(&mut rt);
    assert!(rt.state.own_polls >= 1, "the app's own poll runs with the kit's");
    assert!(rt.state.changes.is_empty(), "the initial load is not a change");
    assert_eq!(rt.state.theme.revision(), 0);

    let tokyo = omarchy_rune::color(rt.state.theme.tokens().palette.background);
    // A new rounding with the switch (the watch thread is idle between deliveries).
    unsafe { std::env::set_var("OMARCHY_UI_ROUNDING", "8") };
    theme_set(&home, "catppuccin-latte");
    woken.recv_timeout(Duration::from_secs(5)).expect("the watch wakes the app");
    frame(&mut rt);
    let latte = omarchy_rune::color(rt.state.theme.tokens().palette.background);
    let page = |rt: &Runtime<App>| rt.elements().into_iter().find(|e| e.key.as_deref() == Some("page")).expect("the page");
    // The transition: the rounding is there at once, the color is on its way.
    frame(&mut rt);
    let mid = page(&rt);
    assert_eq!(mid.radius, 8.0, "rounding switches instantly");
    assert!(mid.fill != tokyo && mid.fill != latte, "the background fades: {:?}", mid.fill);
    assert_eq!(rt.state.theme.tokens().palette.background.hex(), "#eff1f5ff");
    assert_eq!(rt.state.theme.tokens().theme, "catppuccin-latte");
    assert_eq!(rt.state.changes, vec![("#eff1f5ff".to_string(), 1)], "the changed hook, once, after the change");

    // Nothing more arrives for the same switch.
    assert!(woken.recv_timeout(Duration::from_millis(600)).is_err(), "one switch, one wake");
    for _ in 0..3 {
        frame(&mut rt);
    }
    assert_eq!(rt.state.changes.len(), 1);
    assert_eq!(rt.state.theme.revision(), 1);
    for _ in 0..60 {
        frame(&mut rt);
    }
    assert_eq!(page(&rt).fill, latte, "and lands exactly");

    // Reduced motion: the next switch lands at once.
    unsafe { std::env::set_var("OMARCHY_UI_REDUCE_MOTION", "1") };
    theme_set(&home, "tokyo-night");
    woken.recv_timeout(Duration::from_secs(5)).expect("the switch back wakes the app");
    frame(&mut rt);
    frame(&mut rt);
    assert!(rt.state.theme.tokens().reduce_motion);
    assert_eq!(page(&rt).fill, tokyo, "no fade under reduced motion");
    assert_eq!(rt.state.changes.len(), 2);
    assert_eq!(rt.state.theme.revision(), 2);

    let _ = std::fs::remove_dir_all(&tmp);
}

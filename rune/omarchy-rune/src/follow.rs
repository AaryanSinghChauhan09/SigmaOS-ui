//! The one-call setup: [`run`] / [`app`] wire following the theme into a Rune `AppSpec`
//! (`spec/following.md`), keeping the app's own `setup`, `poll` and `view`.
//!
//! `AppSpec` takes plain `fn` pointers, which can't capture the app's own hooks, so [`app`]
//! keeps them in a small registry keyed by the state type and installs generic wrappers that
//! look them up. One spec per state type per process (the last [`app`] call for a type wins),
//! which is what an app is.

use std::any::{Any, TypeId};
use std::sync::Mutex;

use omarchy_theme::Tokens;
use rune::{AppSpec, Cx, El, Spring, Tx, Waker};

use crate::Theme;

/// The spring a theme switch's colors ease on: Rune's default state-change spring
/// (`tok::motion::SETTLE`, 0.3 s, critically damped), which every other binding matches with
/// `motion.theme` (280 ms) on `motion.ease` (`spec/following.md`). Text leads it by half, as
/// Rune does for every color change.
pub const THEME_SPRING: Spring = Spring::new(0.3, 0.0);

/// App state that holds the [`Theme`], so [`run`] can follow the desktop for it.
///
/// ```ignore
/// impl Themed for App {
///     fn theme(&self) -> &Theme { &self.theme }
///     fn theme_mut(&mut self) -> &mut Theme { &mut self.theme }
///     fn theme_changed(&mut self, tokens: &Tokens) { self.chart.recolor(tokens) }
/// }
/// ```
pub trait Themed: 'static {
    fn theme(&self) -> &Theme;
    fn theme_mut(&mut self) -> &mut Theme;

    /// The changed hook: called once per change that alters the tokens, on the UI thread at the
    /// start of the frame that shows it, after the new tokens are in effect (`self.theme()`
    /// already has them, and [`Theme::revision`] counts this change). Never for the tokens the
    /// app started with. For what the kit can't reach: caches, custom drawing, a child process
    /// to re-theme. Views re-render on their own; they don't need this.
    fn theme_changed(&mut self, _tokens: &Tokens) {}
}

type View<S> = fn(&S, &mut Cx<S>) -> El;
type Setup<S> = fn(&mut S, Waker);
type Poll<S> = fn(&mut S, f64, &mut Tx) -> bool;

/// The app's own hooks, from the spec handed to [`app`].
struct Hooks<S: 'static> {
    view: View<S>,
    setup: Option<Setup<S>>,
    poll: Option<Poll<S>>,
}

static HOOKS: Mutex<Vec<(TypeId, &'static (dyn Any + Send + Sync))>> = Mutex::new(Vec::new());

fn hooks<S: 'static>() -> &'static Hooks<S> {
    let all = HOOKS.lock().unwrap_or_else(|e| e.into_inner());
    let found = all
        .iter()
        .rev()
        .find(|(t, _)| *t == TypeId::of::<S>())
        .map(|(_, h)| *h);
    found
        .and_then(|h| h.downcast_ref::<Hooks<S>>())
        .expect("omarchy_rune::app registers the hooks before Rune calls them")
}

fn view<S: Themed>(s: &S, cx: &mut Cx<S>) -> El {
    s.theme().apply(cx);
    (hooks::<S>().view)(s, cx)
}

fn setup<S: Themed>(s: &mut S, wake: Waker) {
    s.theme_mut().follow(wake.clone());
    if let Some(own) = hooks::<S>().setup {
        own(s, wake);
    }
}

fn poll<S: Themed>(s: &mut S, t: f64, tx: &mut Tx) -> bool {
    let changed = s.theme_mut().poll_tx(tx);
    if changed {
        let tokens = s.theme().tokens().clone();
        s.theme_changed(&tokens);
    }
    let own = hooks::<S>().poll.is_some_and(|p| p(s, t, tx));
    changed || own
}

/// `spec` with following the theme wired in, for `rune::run` or a headless `Runtime`:
///
/// - `setup` starts [`Theme::follow`], then runs the spec's own `setup`;
/// - `poll` takes a change with [`Theme::poll_tx`] (the theme-switch transition), calls
///   [`Themed::theme_changed`], then runs the spec's own `poll`;
/// - `view` calls [`Theme::apply`], then the spec's own view (which no longer needs to);
/// - `families` become the theme's font, unless the spec set fonts or families of its own.
///
/// Calling it on a spec it already returned changes nothing.
pub fn app<S: Themed>(mut spec: AppSpec<S>) -> AppSpec<S> {
    if std::ptr::fn_addr_eq(spec.view, view::<S> as View<S>) {
        return spec;
    }
    let own: &'static Hooks<S> = Box::leak(Box::new(Hooks {
        view: spec.view,
        setup: spec.setup,
        poll: spec.poll,
    }));
    {
        let mut all = HOOKS.lock().unwrap_or_else(|e| e.into_inner());
        all.retain(|(t, _)| *t != TypeId::of::<S>());
        all.push((TypeId::of::<S>(), own));
    }
    if spec.fonts.is_empty() && spec.families == ("Inter", "Inter") {
        spec.families = Theme::load().families();
    }
    spec.view = view::<S>;
    spec.setup = Some(setup::<S>);
    spec.poll = Some(poll::<S>);
    spec
}

/// `rune::run(omarchy_rune::app(spec))`: the app's window (or Rune's headless `--snap`),
/// following the Omarchy theme.
pub fn run<S: Themed>(spec: AppSpec<S>) {
    rune::run(app(spec));
}

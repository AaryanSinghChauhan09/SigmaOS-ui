//! The live Omarchy theme as GPUI globals, and following it (`spec/following.md`).
//!
//! Two globals:
//!
//! - [`Theme`]: the tokens in effect (the target of any fade), the revision, the watch. Set once
//!   per change, so `cx.observe_global::<Theme>()` fires once per change too.
//! - [`Paint`]: what is on screen: the same tokens with colors part-way through a theme fade.
//!   Components draw from it; it moves every frame of a fade and nobody needs to observe it.

use std::cell::RefCell;
use std::rc::Rc;
use std::sync::Arc;
use std::time::{Duration, Instant};

use futures::StreamExt;
use futures::channel::mpsc;
use gpui::{App, Global, Hsla, Pixels, SharedString, Subscription, Task, px};
use omarchy_theme::{Rgba, Tokens};

use crate::fade::{cubic_bezier, mix_tokens};

/// A GPUI color from a token color (straight alpha, sRGB).
pub fn color(c: Rgba) -> Hsla {
    gpui::Rgba { r: c.r as f32 / 255.0, g: c.g as f32 / 255.0, b: c.b as f32 / 255.0, a: c.a as f32 / 255.0 }.into()
}

/// The theme in effect. One per app, installed by [`crate::init`].
pub struct Theme {
    tokens: Arc<Tokens>,
    revision: u64,
    _watch: Option<omarchy_theme::Watch>,
    _follow: Option<Task<()>>,
}

impl Global for Theme {}

impl Theme {
    /// The installed theme. Panics before [`crate::init`].
    pub fn get(cx: &App) -> &Theme {
        cx.global::<Theme>()
    }

    /// The tokens in effect: after a change, the new ones (a fade's target, never a frame of it).
    pub fn tokens(&self) -> &Arc<Tokens> {
        &self.tokens
    }

    /// 0 for the tokens the app started with, plus one per change: a cache key.
    pub fn revision(&self) -> u64 {
        self.revision
    }

    /// The kit's string `key` in the display language (`omarchy_theme::strings`), else `fallback`.
    pub fn string(&self, key: &str, fallback: &str) -> SharedString {
        self.tokens.string(key, fallback).into()
    }

    /// The file for the freedesktop icon `name` at `size` px in the theme's icon theme.
    pub fn icon(&self, name: &str, size: u32) -> Option<std::path::PathBuf> {
        icon_lookup(&self.tokens.icon_theme).find(name, size)
    }
}

thread_local! {
    static ICONS: RefCell<Option<Rc<omarchy_theme::icons::IconLookup>>> = const { RefCell::new(None) };
}

fn icon_lookup(theme: &str) -> Rc<omarchy_theme::icons::IconLookup> {
    ICONS.with(|cell| {
        let mut c = cell.borrow_mut();
        if c.as_ref().is_none_or(|l| l.theme() != theme) {
            *c = Some(Rc::new(omarchy_theme::icons::IconLookup::new(theme, omarchy_theme::icons::search_dirs())));
        }
        c.clone().unwrap()
    })
}

/// What is on screen: the tokens with colors where the theme fade has them now. Equal to
/// [`Theme::tokens`] except during a fade. Components, and an app's own drawing that should fade
/// with them, read colors from here.
#[derive(Clone)]
pub struct Paint {
    shown: Arc<Tokens>,
    fade: Option<Fade>,
}

#[derive(Clone)]
struct Fade {
    from: Arc<Tokens>,
    start: Instant,
    duration: Duration,
    _driver: Rc<Task<()>>,
}

impl Global for Paint {}

impl Paint {
    /// What is on screen now.
    pub fn of(cx: &App) -> Paint {
        cx.global::<Paint>().clone()
    }
    /// True while a theme fade runs.
    pub fn fading(cx: &App) -> bool {
        cx.try_global::<Paint>().is_some_and(|p| p.fade.is_some())
    }
    pub fn tokens(&self) -> &Tokens {
        &self.shown
    }

    pub fn fg(&self) -> Hsla {
        color(self.shown.palette.foreground)
    }
    pub fn bg(&self) -> Hsla {
        color(self.shown.palette.background)
    }
    pub fn accent(&self) -> Hsla {
        color(self.shown.palette.accent)
    }
    pub fn urgent(&self) -> Hsla {
        color(self.shown.palette.urgent)
    }
    pub fn success(&self) -> Hsla {
        color(self.shown.palette.success)
    }
    pub fn warning(&self) -> Hsla {
        color(self.shown.palette.warning)
    }
    pub fn muted(&self) -> Hsla {
        color(self.shown.palette.muted)
    }
    /// Secondary text (`roles.textDim`).
    pub fn dim(&self) -> Hsla {
        color(self.shown.roles.text_dim)
    }
    /// Disabled and placeholder text (`roles.textFaint`).
    pub fn faint(&self) -> Hsla {
        color(self.shown.roles.text_faint)
    }
    /// Meter, progress and scrollbar tracks; separators (`roles.track`).
    pub fn track(&self) -> Hsla {
        color(self.shown.roles.track)
    }
    pub fn scrim(&self) -> Hsla {
        color(self.shown.roles.scrim)
    }
    /// Foreground darkened as Qt's `darker(factor)`.
    pub fn fg_darker(&self, factor: f64) -> Hsla {
        color(self.shown.palette.foreground.darker(factor))
    }
    /// Hyprland's rounding.
    pub fn radius(&self) -> Pixels {
        px(self.shown.radius as f32)
    }
    /// The rounding for small parts: capped at 4.
    pub fn radius_small(&self) -> Pixels {
        px(self.shown.radius_small() as f32)
    }
    /// `Style.space(px)`: a default-scale size on the theme's spacing scale.
    pub fn space(&self, v: f32) -> f32 {
        self.shown.spacing.space(v as f64) as f32
    }
    /// The font family to ask GPUI for: what fontconfig's `monospace` resolves to.
    pub fn family(&self) -> SharedString {
        family_of(&self.shown).into()
    }
    /// The line box for text at `size` px, as Qt lays it out (the font's ascent + descent).
    pub fn line(&self, size: i64) -> Pixels {
        px(self.shown.font.line_px(size) as f32)
    }
}

/// The family GPUI should load: the resolved `monospace` (GPUI's text systems don't resolve
/// fontconfig aliases), or an explicit family as given.
pub(crate) fn family_of(t: &Tokens) -> String {
    if !t.font.resolved_family.is_empty() && t.font.family == "monospace" {
        t.font.resolved_family.clone()
    } else {
        t.font.family.clone()
    }
}

type Hook = Rc<RefCell<Box<dyn FnMut(&Tokens, &mut App)>>>;

#[derive(Default)]
struct Hooks {
    list: Rc<RefCell<Vec<(u64, Hook)>>>,
    next: u64,
}

impl Global for Hooks {}

/// The changed hook: `f` runs once per change that alters the tokens, on the app thread, after
/// the new tokens are in effect ([`Theme::tokens`] has them and [`Theme::revision`] counts the
/// change), with those tokens. Never for the tokens the app started with. For what the kit can't
/// reach: caches, custom drawing, a child process to re-theme. Dropping the subscription
/// unregisters it; `.detach()` keeps it for the app's life.
///
/// `cx.observe_global::<Theme>(..)` fires at the same moments.
pub fn on_change(cx: &mut App, f: impl FnMut(&Tokens, &mut App) + 'static) -> Subscription {
    let hooks = cx.default_global::<Hooks>();
    let id = hooks.next;
    hooks.next += 1;
    let list = hooks.list.clone();
    list.borrow_mut().push((id, Rc::new(RefCell::new(Box::new(f)))));
    Subscription::new(move || list.borrow_mut().retain(|(i, _)| *i != id))
}

/// Install `tokens` without following anything: for tests and apps that pin a theme. [`crate::init`]
/// is this plus the watch. A second call replaces the theme as a change would.
pub fn install(cx: &mut App, tokens: Tokens) {
    if cx.has_global::<Theme>() {
        set_tokens(cx, tokens);
        return;
    }
    let tokens = Arc::new(tokens);
    crate::font::setup(cx, &tokens);
    cx.set_global(Paint { shown: tokens.clone(), fade: None });
    cx.set_global(Theme { tokens: tokens.clone(), revision: 0, _watch: None, _follow: None });
    #[cfg(feature = "gpui-component")]
    crate::component::apply(cx, &tokens);
}

/// Follow the desktop: the watch delivers on its own thread; each change crosses to the app
/// thread over a channel and lands through [`set_tokens`].
pub(crate) fn follow(cx: &mut App) {
    let initial = (**Theme::get(cx).tokens()).clone();
    let (tx, mut rx) = mpsc::unbounded::<Tokens>();
    let watch = omarchy_theme::watch(initial, move |fresh| {
        let _ = tx.unbounded_send(fresh);
    });
    let task = cx.spawn(async move |cx| {
        while let Some(fresh) = rx.next().await {
            cx.update(|cx| set_tokens(cx, fresh));
        }
    });
    let theme = cx.global_mut::<Theme>();
    theme._watch = Some(watch);
    theme._follow = Some(task);
}

/// Deliver a change: new tokens take effect at once (layout, sizes, the font and the icon theme
/// switch now), colors fade from what is on screen unless motion is reduced, windows refresh, and
/// the changed hook runs. Tokens equal to the current ones change nothing. The watch calls this;
/// an app can too, to pin or preview a theme.
pub fn set_tokens(cx: &mut App, fresh: Tokens) {
    let Some(theme) = cx.try_global::<Theme>() else {
        install(cx, fresh);
        return;
    };
    if **theme.tokens() == fresh {
        return;
    }
    let tokens = Arc::new(fresh);
    let revision = theme.revision + 1;
    let family_changed = family_of(theme.tokens()) != family_of(&tokens);
    {
        let t = cx.global_mut::<Theme>();
        t.tokens = tokens.clone();
        t.revision = revision;
    }
    if family_changed {
        crate::font::setup(cx, &tokens);
    }
    start_fade(cx, tokens.clone());
    #[cfg(feature = "gpui-component")]
    crate::component::apply_current(cx);
    cx.refresh_windows();

    let hooks: Vec<Hook> = match cx.try_global::<Hooks>() {
        Some(h) => h.list.borrow().iter().map(|(_, f)| f.clone()).collect(),
        None => Vec::new(),
    };
    for hook in hooks {
        (hook.borrow_mut())(&tokens, cx);
    }
}

/// How often a fade repaints: a frame at 120 Hz.
const FRAME: Duration = Duration::from_micros(8333);

fn start_fade(cx: &mut App, to: Arc<Tokens>) {
    let from = cx.global::<Paint>().shown.clone();
    let ms = to.motion.theme;
    if to.reduce_motion || ms == 0 {
        cx.set_global(Paint { shown: to, fade: None });
        return;
    }
    let duration = Duration::from_millis(ms as u64);
    let driver = cx.spawn(async move |cx| {
        loop {
            cx.background_executor().timer(FRAME).await;
            let done = cx.update(step);
            cx.refresh();
            if done {
                break;
            }
        }
    });
    let start = cx.background_executor().now();
    // The first frame of the fade is `from` exactly; it moves from the next one.
    cx.set_global(Paint { shown: from.clone(), fade: Some(Fade { from, start, duration, _driver: Rc::new(driver) }) });
}

/// One frame of the fade; true when it has landed.
fn step(cx: &mut App) -> bool {
    let target = cx.global::<Theme>().tokens.clone();
    let now = cx.background_executor().now();
    let paint = cx.global::<Paint>();
    let Some(fade) = &paint.fade else { return true };
    let x = now.saturating_duration_since(fade.start).as_secs_f64() / fade.duration.as_secs_f64();
    if x >= 1.0 {
        // Land exactly on the tokens, with no interpolation drift.
        cx.set_global(Paint { shown: target, fade: None });
        #[cfg(feature = "gpui-component")]
        crate::component::apply_current(cx);
        return true;
    }
    let p = cubic_bezier(target.motion.ease, x);
    let shown = Arc::new(mix_tokens(&fade.from, &target, p));
    let fade = fade.clone();
    cx.set_global(Paint { shown, fade: Some(fade) });
    #[cfg(feature = "gpui-component")]
    crate::component::apply_current(cx);
    false
}

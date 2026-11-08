//! Omarchy's look for Rune.
//!
//! Two layers:
//!
//! - [`Theme`]: the live Omarchy theme (colors, Hyprland rounding, the `monospace` font, spacing,
//!   interactive states), following theme switches without a restart, plus a rune-kit [`Kit`]
//!   built from it, so every rune-kit component already lands close to Omarchy.
//! - [`ui`]: Omarchy's own primitives (Button, Toggle, TextField, Card, MenuRow, ...), drawn
//!   the way the Omarchy shell draws them, so a Rune app and a Qt app look the same.
//!
//! ```ignore
//! struct App { theme: omarchy_rune::Theme, wifi: bool }
//!
//! fn view(s: &App, cx: &mut Cx<App>) -> El {
//!     let t = &s.theme;
//!     t.apply(cx);
//!     omarchy_rune::ui::Card::new().title("Network")
//!         .child(Toggle::new("Wi-Fi").checked(s.wifi).on_toggle(|s: &mut App| s.wifi ^= true).build(t))
//!         .build(t)
//! }
//!
//! rune::run(AppSpec {
//!     setup: Some(|s, wake| s.theme.follow(wake)),
//!     poll: Some(|s, _, _| s.theme.poll()),
//!     ..AppSpec::new("App", (480.0, 360.0), || App { theme: Theme::load(), wifi: true }, view)
//! });
//! ```

pub mod ui;

use std::collections::HashMap;
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::{Arc, Mutex, OnceLock};

use omarchy_theme::icons::{IconLookup, search_dirs};
use omarchy_theme::{BorderSpec, Rgba, Tokens};
use rune::{Color, Cx, El, Family, Image, ImageCache, TextStyle, Waker};
use rune_kit::{Kit, PaletteSeed};

pub use omarchy_theme;

/// A Rune color from a token color. Rune blends in sRGB by default, as Qt does, so translucent
/// tokens land on the same pixels.
pub fn color(c: Rgba) -> Color {
    rune::color::hex(&c.hex()).unwrap_or(Color::TRANSPARENT)
}

/// The live Omarchy theme. Keep one in the app state, call [`Theme::follow`] from
/// `AppSpec::setup` and [`Theme::poll`] from `AppSpec::poll`, and every view that reads it
/// re-renders (with Rune's color springs) when the theme, rounding or font changes.
pub struct Theme {
    tokens: Tokens,
    kit: Kit,
    family: Family,
    inbox: Arc<Mutex<Option<Tokens>>>,
    watch: Option<omarchy_theme::Watch>,
    icons: Arc<Icons>,
    /// The key map typed for the app's state, from [`Theme::apply`] (see [`Theme::key_map`]).
    aliases: Arc<Mutex<Option<Aliases>>>,
}

/// Attaches an `on_key` that re-sends or swallows the keys a [`KeyMap`] names.
type Aliases = fn(El, KeyMap) -> El;

/// Keys a primitive takes itself, by name (`"j"`, `"Right"`, see [`key_name`]): re-sent as
/// another key (`Some`), or swallowed (`None`), as Qt's components take them.
pub type KeyMap = &'static [(&'static str, Option<rune::Key>)];

/// A key's name in a [`KeyMap`]: the typed text, or the arrow, Home or End key's name.
pub fn key_name(k: &rune::Key) -> Option<&str> {
    use rune::Key;
    Some(match k {
        Key::Text(t) => t.as_str(),
        Key::Left => "Left",
        Key::Right => "Right",
        Key::Up => "Up",
        Key::Down => "Down",
        Key::Home => "Home",
        Key::End => "End",
        _ => return None,
    })
}

fn aliases<S: 'static>(e: El, map: KeyMap) -> El {
    e.on_key(move |_: &mut S, k: &rune::KeyEvent, tx: &mut rune::Tx| {
        let Some(name) = key_name(&k.key) else { return false };
        let Some((_, to)) = map.iter().find(|(from, _)| *from == name) else { return false };
        if let Some(to) = to {
            tx.send_key(rune::KeyEvent { key: to.clone(), mods: k.mods });
        }
        true
    })
}

impl Theme {
    /// The theme as it is now (or the `OMARCHY_UI_THEME_DIR` fixture).
    pub fn load() -> Theme {
        Theme::from_tokens(omarchy_theme::load())
    }

    pub fn from_tokens(tokens: Tokens) -> Theme {
        let family = family_for(&tokens);
        let kit = build_kit(&tokens, family);
        Theme { tokens, kit, family, inbox: Arc::default(), watch: None, icons: Arc::default(), aliases: Arc::default() }
    }

    /// Start following the desktop: theme switches, `omarchy font set`, `omarchy display text
    /// size`, and Hyprland reloads (rounding, gaps, the animations toggle).
    pub fn follow(&mut self, wake: Waker) {
        let inbox = self.inbox.clone();
        *self.icons.wake.lock().unwrap() = Some(wake.clone());
        self.watch = Some(omarchy_theme::watch(self.tokens.clone(), move |fresh| {
            *inbox.lock().unwrap() = Some(fresh);
            wake();
        }));
    }

    /// Take a change delivered since the last frame; true when there was one.
    pub fn poll(&mut self) -> bool {
        let Some(fresh) = self.inbox.lock().unwrap().take() else { return false };
        // The icon store carries over: it re-resolves by itself when the icon theme changed.
        *self = Theme {
            inbox: self.inbox.clone(),
            watch: self.watch.take(),
            icons: self.icons.clone(),
            aliases: self.aliases.clone(),
            ..Theme::from_tokens(fresh)
        };
        true
    }

    pub fn tokens(&self) -> &Tokens {
        &self.tokens
    }

    /// rune-kit's tokens, mapped from Omarchy's: for rune-kit components (Select, Menu, Table,
    /// Markdown, ...) next to the [`ui`] primitives.
    pub fn kit(&self) -> &Kit {
        &self.kit
    }

    /// Window-wide settings a view should apply once per frame: Omarchy's focus ring (focus
    /// mirrors hover by default) and Hyprland's animations toggle as reduced motion.
    pub fn apply<S: 'static>(&self, cx: &mut Cx<S>) {
        let focus = &self.tokens.controls.focus.border;
        cx.focus_ring(focus.max_width().max(1.0) as f32, 0.0, color(focus.color));
        if self.tokens.reduce_motion {
            cx.reduced_motion(true);
        }
        self.icons.scale.store(cx.env().scale.to_bits(), Ordering::Relaxed);
        *self.aliases.lock().unwrap_or_else(|e| e.into_inner()) = Some(aliases::<S>);
    }

    /// `e` taking the keys `map` names while it or something in it has focus: each re-sent as
    /// its key, or swallowed. `[("j", Some(Key::Down)), ("k", Some(Key::Up))]` walks a focus
    /// group the vim way; `[("Right", None)]` on a group's last item keeps it from wrapping. An
    /// element has one key handler, so this is for elements without their own. Needs a
    /// [`Theme::apply`] first (to learn the state's type); a no-op before one. A re-sent key
    /// lands on the next frame.
    pub fn key_map(&self, e: El, map: KeyMap) -> El {
        match *self.aliases.lock().unwrap_or_else(|e| e.into_inner()) {
            Some(attach) => attach(e, map),
            None => e,
        }
    }

    /// The icon named `name` (`firefox`, `org.gnome.Nautilus`) in the theme's icon theme
    /// (`omarchy_theme::icons`), decoded to fit `size` points square at the window's scale (as of
    /// the last [`Theme::apply`]; SVGs rasterize at that size). `None` when no icon theme has it.
    /// The image decodes off the view thread: until [`Image::is_ready`], draw a fallback; the
    /// waker from [`Theme::follow`] re-renders once it is. [`Avatar::icon`](ui::Avatar::icon)
    /// does all of this.
    pub fn icon(&self, name: &str, size: f32) -> Option<Image> {
        let scale = f32::from_bits(self.icons.scale.load(Ordering::Relaxed));
        self.icons.image(&self.tokens.icon_theme, name, size, scale)
    }

    /// `AppSpec::families`, so rune-kit's sans and mono slots both use the Omarchy font.
    /// Call before `rune::run`; the names live for the process.
    pub fn families(&self) -> (&'static str, &'static str) {
        let name: &'static str = Box::leak(self.tokens.font.resolved_family.clone().into_boxed_str());
        (name, name)
    }

    // ---- shorthands the primitives use, public so apps can match them in their own views

    pub fn fg(&self) -> Color {
        color(self.tokens.palette.foreground)
    }
    pub fn bg(&self) -> Color {
        color(self.tokens.palette.background)
    }
    pub fn accent(&self) -> Color {
        color(self.tokens.palette.accent)
    }
    pub fn urgent(&self) -> Color {
        color(self.tokens.palette.urgent)
    }
    /// Secondary text (`roles.textDim`).
    pub fn dim(&self) -> Color {
        color(self.tokens.roles.text_dim)
    }
    /// Disabled and placeholder text (`roles.textFaint`).
    pub fn faint(&self) -> Color {
        color(self.tokens.roles.text_faint)
    }
    /// Meter, progress and scrollbar tracks (`roles.track`).
    pub fn track(&self) -> Color {
        color(self.tokens.roles.track)
    }
    /// Behind modals (`roles.scrim`).
    pub fn scrim(&self) -> Color {
        color(self.tokens.roles.scrim)
    }
    pub fn success(&self) -> Color {
        color(self.tokens.palette.success)
    }
    pub fn warning(&self) -> Color {
        color(self.tokens.palette.warning)
    }
    pub fn muted(&self) -> Color {
        color(self.tokens.palette.muted)
    }
    /// Foreground darkened as Qt's `darker(factor)`: 1.4 for labels and headers, 1.5 for
    /// descriptions, 1.6 for placeholders.
    pub fn fg_darker(&self, factor: f64) -> Color {
        color(self.tokens.palette.foreground.darker(factor))
    }
    /// Hyprland's rounding, for panels, cards and controls.
    pub fn radius(&self) -> f32 {
        self.tokens.radius as f32
    }
    /// The rounding for small elements: capped at 4.
    pub fn radius_small(&self) -> f32 {
        self.tokens.radius_small() as f32
    }
    /// `Style.space(px)`: a default-scale pixel value on the theme's spacing scale.
    pub fn space(&self, px: f32) -> f32 {
        self.tokens.spacing.space(px as f64) as f32
    }
    /// Text in the Omarchy font at `size` px.
    /// The line box matches Qt's (the font's own ascent + descent), so controls built from
    /// it are the same height in both toolkits.
    pub fn text_style(&self, size: i64) -> TextStyle {
        TextStyle::new(size as f32, 400.0).family(self.family).lh(self.tokens.font.line_px(size) as f32)
    }
    pub fn text_style_bold(&self, size: i64) -> TextStyle {
        TextStyle::new(size as f32, 700.0).family(self.family).lh(self.tokens.font.line_px(size) as f32)
    }
}

/// Icons looked up by name, decoded and kept alive while the icon theme stays the same.
struct Icons {
    /// The app's waker, once [`Theme::follow`] has one: a decoded icon re-renders through it.
    wake: Arc<Mutex<Option<Waker>>>,
    /// Device pixels per point, from the last [`Theme::apply`].
    scale: AtomicU32,
    cache: OnceLock<ImageCache>,
    state: Mutex<IconState>,
}

#[derive(Default)]
struct IconState {
    lookup: Option<IconLookup>,
    held: HashMap<(String, u32), Option<Image>>,
}

impl Default for Icons {
    fn default() -> Self {
        Icons { wake: Arc::default(), scale: AtomicU32::new(2.0f32.to_bits()), cache: OnceLock::new(), state: Mutex::default() }
    }
}

impl Icons {
    fn image(&self, theme: &str, name: &str, size: f32, scale: f32) -> Option<Image> {
        let mut st = self.state.lock().unwrap();
        if st.lookup.as_ref().is_none_or(|l| l.theme() != theme) {
            st.lookup = Some(IconLookup::new(theme, search_dirs()));
            st.held.clear();
        }
        let px = (size * scale).ceil().max(1.0) as u32;
        let key = (name.to_string(), px);
        if let Some(hit) = st.held.get(&key) {
            return hit.clone();
        }
        let path = st.lookup.as_ref().and_then(|l| l.find(name, px));
        let image = path.map(|p| self.cache().file(p, size, scale));
        st.held.insert(key, image.clone());
        image
    }

    fn cache(&self) -> &ImageCache {
        self.cache.get_or_init(|| {
            let wake = self.wake.clone();
            ImageCache::new(Arc::new(move || {
                let w = wake.lock().unwrap().clone();
                if let Some(w) = w {
                    w();
                }
            }))
        })
    }
}

/// Paint a border spec on an element: per-side widths, and a gradient's first and last stops as
/// Rune's vertical border gradient (Hyprland gradients are angled; Rune's border gradient runs
/// top to bottom, which is close for the common 45–90deg active borders).
pub fn border(e: El, spec: &BorderSpec) -> El {
    if spec.is_none() {
        return e;
    }
    let w = spec.widths.map(|w| w as f32);
    match &spec.gradient {
        Some(g) if spec.is_uniform() && g.colors.len() > 1 => {
            e.border_gradient(w[0], color(g.colors[0]), color(*g.colors.last().unwrap()))
        }
        _ => e.border_sides(w, color(spec.color)),
    }
}

fn family_for(t: &Tokens) -> Family {
    if t.font.family == "monospace" {
        // fontconfig's `monospace`, which `omarchy font set` points at the chosen font.
        Family::SystemMono
    } else {
        Family::Named(Box::leak(t.font.family.clone().into_boxed_str()))
    }
}

fn build_kit(t: &Tokens, family: Family) -> Kit {
    let p = &t.palette;
    let mut k = Kit::from_palette(PaletteSeed { bg: color(p.background), fg: color(p.foreground), accent: color(p.accent) });
    let c = &t.controls;

    // Corners follow Hyprland; small parts cap at 4 like the shell's.
    let r = t.radius as f32;
    let small = r.min(4.0);
    k.radius.xs = r.min(2.0);
    k.radius.sm = small;
    k.radius.md = r;
    k.radius.lg = r;
    k.radius.xl = r;

    // Interactive states: foreground glass, not accent fills.
    k.c.hover = color(c.hover.fill);
    k.c.pressed = color(c.pressed.fill);
    k.c.border_base = color(c.normal.border.color);
    k.c.border_weak = color(c.hover.border.color);
    k.c.border_hover = color(c.hover.border.color);
    k.c.border_focus = color(c.focus.border.color);
    k.c.border_active = color(c.selected.text);
    k.c.danger.icon = color(p.urgent);
    k.c.danger.fg = color(p.urgent);
    k.c.danger.border = color(p.urgent);
    // Secondary and faint text are the theme's own stops (roles), not the kit's derived ones.
    k.c.text_muted = color(t.roles.text_dim);
    k.c.text_weak = color(t.roles.text_dim);
    k.c.text_faint = color(t.roles.text_faint);
    k.c.icon_muted = color(t.roles.text_dim);
    k.c.icon_faint = color(t.roles.text_faint);
    k.c.text_accent = color(p.accent);
    k.c.icon_accent = color(p.accent);
    k.c.scrim = color(t.surfaces.menu.scrim);
    k.c.surface_float = color(t.surfaces.popups.background);
    k.c.surface_raised = color(t.surfaces.popups.background);

    // One family everywhere, on Omarchy's type scale.
    let f = &t.font;
    let style = |size: i64, weight: f32| TextStyle::new(size as f32, weight).family(family).lh(f.line_px(size) as f32);
    k.ty.ui = style(f.body, 400.0);
    k.ty.ui_medium = style(f.body, 700.0);
    k.ty.ui_compact = style(f.body, 400.0);
    k.ty.small = style(f.caption, 400.0);
    k.ty.small_medium = style(f.caption, 700.0);
    k.ty.body = style(f.subtitle, 400.0);
    k.ty.body_medium = style(f.subtitle, 700.0);
    k.ty.text_small = style(f.body, 400.0);
    k.ty.text_small_medium = style(f.body, 700.0);
    k.ty.text_xsmall = style(f.body_small, 400.0);
    k.ty.text_xsmall_medium = style(f.body_small, 700.0);
    k.ty.mono = style(f.body, 400.0);

    // Omarchy draws flat: no drop shadows on controls or overlays.
    k.elevation.raised.clear();
    k.elevation.floating.clear();
    k.elevation.overlay.clear();
    k.elevation.button_neutral.clear();
    k.elevation.button_contrast.clear();
    k.elevation.elements.clear();
    k.elevation.md.clear();
    k
}

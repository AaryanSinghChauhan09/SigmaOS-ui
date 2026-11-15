# Using Omarchy UI from GPUI

> Paths such as `examples/...` are relative to a checkout.

`omarchy-gpui` (`gpui/omarchy-gpui`) gives a [GPUI](https://gpui.rs) app Omarchy's look with one call: the live theme as a GPUI global, followed without a restart, colors that fade on a switch, the `monospace` font, and the kit's components (`Button`, `Toggle`, `ListRow`, `Menu`, `Table`, …) drawn the way the Qt kit draws them ([`spec/components.md`](../spec/components.md)). With the `gpui-component` feature, [gpui-component](https://github.com/longbridge/gpui-component)'s widgets follow the theme too.

## Adopt it

```toml
[dependencies]
omarchy-gpui = { path = "../omarchy-ui/gpui/omarchy-gpui" }            # or
omarchy-gpui = { git = "https://github.com/omacom/omarchy-ui", rev = "<commit>" }
```

That one line is all. Use GPUI through the re-exports, `omarchy_gpui::gpui` and `omarchy_gpui::gpui_platform`: they are exactly the GPUI the kit was built against (see [Pins](#pins)). A second `gpui` dependency at another version is a second, incompatible crate (``expected `gpui::App`, found `gpui::App` ``).

`omarchy-gpui` lives in its own Cargo workspace (`gpui/Cargo.toml`), which Cargo resolves for a path or git dependency alike. If your crate sits inside another workspace's directory without being a member, give it a `[workspace]` of its own, as [`examples/gpui`](../examples/gpui) does.

## The one call

[`examples/gpui/src/main.rs`](../examples/gpui/src/main.rs), a "Network" panel:

```rust
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Button, Heading, HeadingLevel, ListRow, Pane, Toggle, Tone};
use omarchy_gpui::{Paint, gpui_platform};

struct Network { wifi: bool }

impl Render for Network {
    fn render(&mut self, _: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        let t = Paint::of(cx);                      // what's on screen, colors mid-fade included
        let pad = t.tokens().spacing.panel_padding as f32;
        let panel = Pane::new()
            .bordered(true)
            .child(Heading::new("Network").subtitle("Wi-Fi and saved networks").level(HeadingLevel::Heading))
            .child(Toggle::new("Wi-Fi").checked(self.wifi).on_toggle(cx.listener(|this, on: &bool, _, cx| {
                this.wifi = *on;
                cx.notify();
            })))
            .child(ListRow::new("Basecamp").glyph("\u{F0928}").status("Connected", Tone::Success).current(self.wifi))
            .child(div().flex().child(Button::new("Forget network").bordered(true)));
        div().size_full().bg(t.bg()).p(px(pad)).child(panel)
    }
}

fn main() {
    gpui_platform::application().run(|cx| {
        omarchy_gpui::init(cx); // the theme, following the desktop, the font, the color fade
        cx.open_window(WindowOptions::default(), |_, cx| cx.new(|_| Network { wifi: true })).unwrap();
    });
}
```

`omarchy_gpui::init(cx)`:

- installs the theme as the global [`Theme`] (the tokens in effect, the revision) and [`Paint`] (what's on screen);
- follows the desktop through `omarchy_theme::watch` (theme switches, `omarchy font set`, `omarchy display text size`, Hyprland reloads, the language and the icon theme), delivered over a channel onto GPUI's app thread, then refreshes every window;
- fades colors from the old theme to the new over `motion.theme` (280 ms) on `motion.ease`, per [`spec/following.md`](../spec/following.md): rounding, spacing, sizes and the font switch at once; no fade on first paint or under reduced motion; a second change mid-fade starts from the colors on screen; the fade lands exactly on the tokens;
- makes the font available: fontconfig's `monospace`, asked for by the family it resolves to (`tokens.font.resolved_family`, because GPUI's Linux text system doesn't resolve fontconfig aliases), its file added to GPUI's text system if GPUI didn't load it;
- with the `gpui-component` feature, maps the tokens onto gpui-component's theme, on every change and every fade frame.

Calling it twice does nothing. `omarchy_gpui::install(cx, tokens)` installs tokens without following (tests, a pinned theme), and `omarchy_gpui::set_tokens(cx, tokens)` delivers a change by hand (a preview).

GPUI has no app-wide default text style, so text your own views draw takes GPUI's default font unless you set it: put `.font_family(Paint::of(cx).family())` and `.text_color(t.fg())` on your root element, as the gallery does, or use `omarchy_gpui::ui::label` / `paragraph`. The kit's components set their font themselves.

## The pieces

- **`Theme`** (`Theme::get(cx)`): `tokens()` (the full `omarchy_theme::Tokens`, the target of any fade), `revision()`, `string(key, english)` (the kit's strings in the display language), `icon(name, px)` (the freedesktop icon file in the theme's icon theme).
- **`Paint`** (`Paint::of(cx)`): the tokens as on screen, with helpers the components use and your views can too: `fg()`, `bg()`, `accent()`, `urgent()`, `dim()` (`roles.textDim`), `faint()`, `track()`, `scrim()`, `radius()`, `radius_small()`, `space(px)`, `family()`, `line(size)`. `Paint::fading(cx)` is true during a fade. `omarchy_gpui::color(rgba)` turns a token color into GPUI's `Hsla`.
- **`omarchy_gpui::ui::*`**: the components, each a builder that is a GPUI element (`RenderOnce`), so it goes straight into `.child(..)`. Handlers take GPUI's `Fn(&Event, &mut Window, &mut App)`, which `cx.listener(..)` makes from a view method. Same names, metrics, states, keys and strings as Qt and Rune. `.cursor(true)` forces the hover-cursor look, as Qt's `hasCursor` does.
  - State the kit can't own lives in entities you keep: `TextInput::new(cx, "text")` for `TextField` / `PasswordField`, `TextAreaInput` for `TextArea`, `SearchState::new(cx, "")` for `SearchList`.
  - Focusable components keep their focus handle under their element id, by default derived from their label. Give two with the same label in one view distinct `.id(..)`s.
  - Tab and Shift+Tab walk the focusable components; focus looks like hover; Return/Space activate; lists, menus, tabs and pickers take the arrows and the vim keys as Qt's do.
- **Tokens**: a color, radius, font, size or duration a token covers is never a literal. [`AGENTS.md`](../AGENTS.md) has the rules that make a view read as Omarchy.

## The changed hook

For what the kit can't reach (a cache, custom drawing, a child process to re-theme):

```rust
omarchy_gpui::on_change(cx, |tokens, cx| {
    // Once per real change, on the app thread, after the new tokens are in effect
    // (Theme::get(cx).tokens() is `tokens`), never for the initial load.
    rebuild_chart_colors(tokens, cx);
})
.detach();                                         // or keep the Subscription to unregister
```

`cx.observe_global::<omarchy_gpui::Theme>(..)` fires at the same moments (once per change; the fade's frames move `Paint`, not `Theme`). `Theme::get(cx).revision()` is 0 for the theme the app started with, plus one per change: a cache key. Views re-render on their own; they don't need the hook.

## With gpui-component

```toml
omarchy-gpui = { path = "...", features = ["gpui-component"] }
gpui-component = "=0.8.1"     # the release built on the same GPUI (see Pins)
```

```rust
gpui_platform::application().run(|cx| {
    gpui_component::init(cx);  // first: it installs its own theme
    omarchy_gpui::init(cx);    // then Omarchy's tokens go on top, and follow
    // ...
});
```

The mapping (`omarchy_gpui::component`) keeps gpui-component's widgets in Omarchy's rules: buttons, inputs and secondary fills are the foreground glass (`controls.normal` / `hover` / `pressed`, composited over the background since gpui-component expects opaque fills); `primary` is the selected state (its fill and ink), not an accent fill; `danger` is urgent; borders, rings and selection come from `[controls]`; popovers from the popups surface, lists and tables select with the menu surface's colors; `radius` and `radius_lg` are Hyprland's rounding; no shadows; the font and mono font are the Omarchy family at `body`. Light and dark follow the theme's `mode`. If you call `gpui_component::init` after `omarchy_gpui::init`, call `omarchy_gpui::component::apply_current(cx)` once after it.

Prefer the kit's own components where both have one: they match the Qt kit to the pixel; gpui-component's are only re-colored.

## Stills

With the `snap` feature, `omarchy_gpui::snap` renders a view to a PNG at 2× with **no window on screen**: GPUI's headless app context (the test platform's windows, never shown), Linux's real text system, and the wgpu headless renderer reading the frame back. It needs a GPU adapter (any Vulkan or GL driver; Mesa's llvmpipe works) but no display server or compositor.

```rust
fn main() {
    // `app --snap out.png [--size WxH]` renders and exits; otherwise run as usual.
    if omarchy_gpui::snap::from_args((400.0, 283.0), |_, _| Network { wifi: true }) {
        return;
    }
    // ...
}
```

`snap` turns on GPUI's `test-support`, so keep it behind a feature of your app (`examples/gpui` has `snap = ["omarchy-gpui/snap"]`). The same overrides as every binding pick the theme ([`docs/using-rust.md`](using-rust.md#preview-other-themes-roundings-and-icon-themes)):

```sh
OMARCHY_UI_THEME_DIR=../omarchy-ui/fixtures/themes/catppuccin-latte OMARCHY_UI_ROUNDING=8 \
  cargo run --release --features snap -- --snap latte-r8.png
```

The gallery (`bin/build gpui`, then `gpui/target/release/omarchy-gpui-gallery`) shows every component live; `--section NAME`, `--list-sections`, and `--snap OUT --section NAME --size WxH --scale 2` for stills. `gpui/gallery/src/sections/*.rs` is example code for each component.

## Pins

| Crate | Pin | Why |
|---|---|---|
| GPUI | `gpui-pre =0.3.10` (crates.io), with `gpui-pre-platform` and `gpui-pre-wgpu =0.3.10` | `gpui-pre` is a weekly crates.io snapshot of Zed's `crates/gpui`. 0.3.10 is zed@08cbb88, whose `crates/gpui` is the same source as zed@a5c4ca8. It has the headless renderer stills need. |
| gpui-component | `=0.8.1` (optional feature) | The newest release built on `gpui-pre =0.3.10`. 0.9.0 needs `gpui-pre =0.3.11`. gpui-component has used `gpui-pre`, not Zed's git, since 0.6, so an app using it can't also use GPUI from Zed's git. |
| Rust | the toolchain in `rust-toolchain.toml` (1.99.1) or newer | |

Updating means moving `gpui-pre`, `gpui-pre-platform`, `gpui-pre-wgpu` and `gpui-component` together in `gpui/Cargo.toml`'s `[workspace.dependencies]`: each gpui-component release pins one exact `gpui-pre`. An app using GPUI from Zed's git directly can't share types with this crate; use the `gpui` it re-exports.

Both crates are `0.1.0`, unpublished and pre-1.0: pin a git dependency to a `rev` and commit your `Cargo.lock`.

## Limits

- **No hover transitions or press give.** GPUI has no animated style transitions, so the 120 ms color change on hover and the 0.97 press scale (Qt, Rune) aren't there: states swap at once. Tabs' underline and IconRail's bar don't slide; Meter's fill doesn't animate its width. The theme-switch fade is there.
- **Glyph ink.** GPUI's Linux text system reports a glyph's advance as its bounds, so the kit reads Nerd Font icons' ink from the font file (`ui/ink.rs`) or, for menus and toolbars, rasterizes them once, to box and center them as Qt does.
- **Stills are 2× only.** GPUI's test windows have a fixed scale factor of 2; `--scale` other than 2 is ignored with a warning.
- **App text.** GPUI has no app-wide default font; set the family on your root (above).
- **Test platform text.** Behavior tests run on GPUI's test platform, which can't read text back off the screen. SearchList's no-matches line is checked in English and German through a debug selector; the dialogs', MultiSelect's and calendar's default labels read the same catalog (`Theme::string`) but no test checks them.

## Where things are

- `gpui/omarchy-gpui/src/theme.rs`: the globals, following, the fade. `component.rs`: the gpui-component mapping. `ui/`: the components. `snap.rs`: stills.
- `gpui/omarchy-gpui/tests/`: behavior tests (`cargo test --manifest-path gpui/Cargo.toml -p omarchy-gpui --features gpui-component`), the same keys and clicks `qt/tests` drives.
- `gpui/gallery/src/bin/omarchy-gpui-live-test.rs`: the live-switch test (`tools/live/run gpui/target/release/omarchy-gpui-live-test`, and with `OMARCHY_UI_REDUCE_MOTION=1`).
- `tools/bindings.d/gpui.json`: the registry entry `bin/build`, `bin/test` and `bin/review` read.

## Troubleshooting

| Symptom | Cause and fix |
|---|---|
| ``expected `gpui::...`, found `gpui::...` `` | A second GPUI. Drop your own `gpui` dependency and use `omarchy_gpui::gpui`, or match the pins above. |
| Text in your views is in the wrong font | Set `.font_family(Paint::of(cx).family())` on your root element. |
| gpui-component widgets keep their own colors | `omarchy_gpui::init` ran before `gpui_component::init`, or the `gpui-component` feature is off. |
| Two components share focus or state | They have the same label (the default id). Give each a distinct `.id(..)`. |
| The app doesn't follow theme switches | `install` instead of `init`, or `OMARCHY_UI_THEME_DIR` is set (a pinned theme follows edits to that directory, not the desktop). |
| `--snap` fails with "No usable headless GPU adapter" | No Vulkan or GL driver. Install Mesa (`vulkan-swrast` gives llvmpipe). |
| Glyphs show as boxes | The kit's glyphs are Nerd Font icons from the theme's `monospace` font. |

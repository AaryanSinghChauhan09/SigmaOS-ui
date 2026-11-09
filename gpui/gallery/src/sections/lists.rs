//! The `lists` section (spec/gallery.md).

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Avatar, AvatarSize, Badge, CursorKey, CursorModel, Hero, KeyValue, ListRow, ScrollArea, ToggleSwitch, Tone};

use crate::Gallery;

const WIDTH: f32 = 420.0;

/// A ToggleSwitch's track and knob alone, not a control of its own (the row owns the click), as
/// Qt's `ToggleSwitch { interactive: false }` draws it.
// TODO: `ToggleSwitch::interactive(false)` once the kit has it.

/// The cursor the rows follow, and the scroll it reveals rows in; the keys work in the live
/// gallery once the list has focus.
pub struct State {
    pub cursor: CursorModel,
    pub scroll: ScrollHandle,
    pub focus: FocusHandle,
}

impl State {
    pub fn new(cx: &mut App) -> State {
        let mut cursor = CursorModel::new([("rows", 9)]);
        cursor.set("rows", 1);
        State { cursor, scroll: ScrollHandle::new(), focus: cx.focus_handle().tab_stop(true) }
    }
}

fn row(g: &Gallery, i: usize, title: &'static str, cx: &mut Context<Gallery>) -> ListRow {
    ListRow::new(title)
        .id(SharedString::from(format!("lists-row-{i}")))
        .cursor(g.more.lists.cursor.is_at("rows", i))
        .on_pointer_move(cx.listener(move |g: &mut Gallery, p: &Point<Pixels>, _, cx| {
            if g.more.lists.cursor.pointer_moved(p.x.into(), p.y.into(), "rows", i) {
                cx.notify();
            }
        }))
}

pub fn build(g: &mut Gallery, _: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let sp = t.tokens().spacing.clone();
    let rows = vec![
        row(g, 0, "Coffee Shop", cx).glyph("\u{F0928}").subtitle("WPA2").value("5 GHz").trailing_glyph("\u{F033E}", Tone::Neutral),
        row(g, 1, "Basecamp Guest", cx).glyph("\u{F0925}").subtitle("Open"),
        row(g, 2, "Basecamp", cx).glyph("\u{F0928}").status("Connected", Tone::Success).current(true),
        row(g, 3, "Neighbor 5G", cx).glyph("\u{F091F}").subtitle("Out of range").disabled(true),
        row(g, 4, "Messages", cx)
            .leading(Avatar::new("Messages").size(AvatarSize::Small))
            .subtitle("From Basecamp")
            .trailing(Badge::new("3").tone(Tone::Accent).filled(true)),
        row(g, 5, "Headphones", cx).glyph("\u{F00AF}").subtitle("Battery 80%").trailing(ToggleSwitch::new("headphones", true).interactive(false)),
        row(g, 6, "Office", cx)
            .glyph("\u{F0922}")
            .subtitle("Saved")
            .trailing_glyph("\u{F033E}", Tone::Neutral)
            .action("\u{F0159}", true)
            .action_cursor(true),
        // Past the cap, so the bar shows.
        row(g, 7, "Library", cx).glyph("\u{F0922}").subtitle("WPA2").trailing_glyph("\u{F033E}", Tone::Neutral).action("\u{F0159}", true),
        row(g, 8, "Airport Free WiFi", cx).glyph("\u{F091F}").subtitle("Open"),
    ];
    let scroll = ScrollArea::new("lists-scroll").track_scroll(&g.more.lists.scroll).width(WIDTH).max_height(340.0).gap(sp.xxs as f32).children(rows);
    let scroll = div()
        .id("lists-keys")
        .track_focus(&g.more.lists.focus)
        .w(px(WIDTH))
        .on_key_down(cx.listener(|g: &mut Gallery, e: &KeyDownEvent, _, cx| {
            let l = &mut g.more.lists;
            match l.cursor.apply_key(&e.keystroke) {
                CursorKey::Moved => ScrollArea::ensure_visible(&l.scroll, l.cursor.index()),
                CursorKey::Ignored => return,
                _ => {}
            }
            cx.stop_propagation();
            cx.notify();
        }))
        .child(scroll);
    // Trailing glyphs that stay: drill-in chevrons and a check.
    let glyphs = div().flex().flex_col().w(px(WIDTH)).gap(px(sp.xxs as f32)).children([
        ListRow::new("Appearance").glyph("\u{F03D8}").subtitle("Theme, font, rounding").trailing_glyph("\u{F0142}", Tone::Neutral),
        ListRow::new("Display").glyph("\u{F0379}").subtitle("2560 × 1440").trailing_glyph("\u{F0142}", Tone::Neutral),
        ListRow::new("Headphones").id("lists-output").glyph("\u{F02CB}").subtitle("Output").current(true).trailing_glyph("\u{F012C}", Tone::Accent),
    ]);
    vec![
        scroll.into_any_element(),
        glyphs.into_any_element(),
        Hero::new("\u{F0928}", "Wi-Fi")
            .status("Connected to Basecamp")
            .width(WIDTH)
            .trailing(ToggleSwitch::new("lists-hero-wifi", true))
            .into_any_element(),
        Hero::new("\u{F00B2}", "Bluetooth").status("No adapter found").error(true).glyph_opacity(0.5).width(WIDTH).into_any_element(),
        KeyValue::new()
            .columns(2)
            .pair("Ping", "18 ms")
            .pair_toned("Packet loss", "4%", Tone::Urgent)
            .pair("Receiving", "1.2 MB/s")
            .pair("Sending", "240 KB/s")
            .pair("IP address", "192.168.1.24")
            .pair("Gateway", "192.168.1.1")
            .into_any_element(),
    ]
}

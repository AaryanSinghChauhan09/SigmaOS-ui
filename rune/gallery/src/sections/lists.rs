//! The `lists` section (spec/gallery.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::{
    Avatar, AvatarSize, Badge, CursorKey, CursorModel, Hero, KeyValue, ListRow, ScrollArea,
    ToggleSwitch, Tone,
};
use rune::{El, col};

use crate::State;

/// The cursor the rows follow; the keys work in the live gallery.
pub struct Lists {
    pub cursor: CursorModel,
}

impl Lists {
    pub fn init() -> Lists {
        let mut cursor = CursorModel::new([("rows", 9)]);
        cursor.set("rows", 1);
        Lists { cursor }
    }
}

const WIDTH: f32 = 420.0;

fn row(s: &State, i: usize, title: &str) -> ListRow {
    ListRow::new(title)
        .key(format!("lists-row-{i}"))
        .cursor(s.lists.cursor.is_at("rows", i))
        .on_pointer_move(move |s: &mut State, x, y| {
            s.lists.cursor.pointer_moved(x, y, "rows", i);
        })
}

pub fn build(s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    let sp = &t.tokens().spacing;
    let rows = vec![
        row(s, 0, "Coffee Shop")
            .glyph("\u{F0928}")
            .subtitle("WPA2")
            .value("5 GHz")
            .trailing_glyph("\u{F033E}", Tone::Neutral)
            .build(t),
        row(s, 1, "Basecamp Guest")
            .glyph("\u{F0925}")
            .subtitle("Open")
            .build(t),
        row(s, 2, "Basecamp")
            .glyph("\u{F0928}")
            .status("Connected", Tone::Success)
            .current(true)
            .build(t),
        row(s, 3, "Neighbor 5G")
            .glyph("\u{F091F}")
            .subtitle("Out of range")
            .disabled(true)
            .build(t),
        row(s, 4, "Messages")
            .leading(Avatar::new("Messages").size(AvatarSize::Small).build(t))
            .subtitle("From Basecamp")
            .trailing(Badge::new("3").tone(Tone::Accent).filled(true).build(t))
            .build(t),
        row(s, 5, "Headphones")
            .glyph("\u{F00AF}")
            .subtitle("Battery 80%")
            .trailing(ToggleSwitch::new(true).interactive(false).build(t))
            .build(t),
        row(s, 6, "Office")
            .glyph("\u{F0922}")
            .subtitle("Saved")
            .trailing_glyph("\u{F033E}", Tone::Neutral)
            .action("\u{F0159}", true)
            .action_cursor(true)
            .build(t),
        // Past the cap, so the bar shows.
        row(s, 7, "Library")
            .glyph("\u{F0922}")
            .subtitle("WPA2")
            .trailing_glyph("\u{F033E}", Tone::Neutral)
            .action("\u{F0159}", true)
            .build(t),
        row(s, 8, "Airport Free WiFi")
            .glyph("\u{F091F}")
            .subtitle("Open")
            .build(t),
    ];
    let scroll = ScrollArea::new("lists-scroll")
        .width(WIDTH)
        .max_height(340.0)
        .gap(sp.xxs as f32)
        .children(rows)
        .build(t)
        .focusable(true)
        .on_key(|s: &mut State, k, tx| match s.lists.cursor.apply_key(k) {
            CursorKey::Moved => {
                ScrollArea::ensure_visible(tx, format!("lists-row-{}", s.lists.cursor.index()));
                true
            }
            CursorKey::Ignored => false,
            _ => true,
        });
    // Trailing glyphs that stay: drill-in chevrons and a check.
    let glyphs = col().w(WIDTH).gap(sp.xxs as f32).children([
        ListRow::new("Appearance")
            .glyph("\u{F03D8}")
            .subtitle("Theme, font, rounding")
            .trailing_glyph("\u{F0142}", Tone::Neutral)
            .build(t),
        ListRow::new("Display")
            .glyph("\u{F0379}")
            .subtitle("2560 × 1440")
            .trailing_glyph("\u{F0142}", Tone::Neutral)
            .build(t),
        ListRow::new("Headphones")
            .glyph("\u{F02CB}")
            .subtitle("Output")
            .current(true)
            .trailing_glyph("\u{F012C}", Tone::Accent)
            .build(t),
    ]);
    vec![
        scroll,
        glyphs,
        Hero::new("\u{F0928}", "Wi-Fi")
            .status("Connected to Basecamp")
            .trailing(ToggleSwitch::new(true).build(t))
            .build(t)
            .w(WIDTH),
        Hero::new("\u{F00B2}", "Bluetooth")
            .status("No adapter found")
            .error(true)
            .glyph_opacity(0.5)
            .build(t)
            .w(WIDTH),
        KeyValue::new()
            .columns(2)
            .pair("Ping", "18 ms")
            .pair_toned("Packet loss", "4%", Tone::Urgent)
            .pair("Receiving", "1.2 MB/s")
            .pair("Sending", "240 KB/s")
            .pair("IP address", "192.168.1.24")
            .pair("Gateway", "192.168.1.1")
            .build(t),
    ]
}

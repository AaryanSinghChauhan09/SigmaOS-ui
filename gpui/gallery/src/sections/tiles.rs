//! The `tiles` section (spec/gallery.md).

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Avatar, AvatarSize, Badge, CursorModel, StatusDot, Tile, TileGrid, Tone};

use crate::Gallery;

/// The sample picture, the same file as the Qt gallery's.
const PHOTO: &str = concat!(env!("CARGO_MANIFEST_DIR"), "/../../qt/gallery/assets/avatar.png");
/// A picture that isn't there, for the fallback.
const MISSING: &str = concat!(env!("CARGO_MANIFEST_DIR"), "/assets/missing.png");

/// The grid's cursor.
pub struct State {
    pub cursor: CursorModel,
}

impl State {
    pub fn new(_cx: &mut App) -> State {
        let mut cursor = CursorModel::list(8).columns(4);
        cursor.set("", 2);
        State { cursor }
    }
}

/// The icon-name specimens: name, label, size, and the glyph for a name nothing has.
const ICONS: [(&str, &str, AvatarSize, &str); 7] = [
    ("org.gnome.Nautilus", "Files", AvatarSize::Medium, ""),
    ("utilities-terminal", "Terminal", AvatarSize::Medium, ""),
    ("firefox", "Firefox", AvatarSize::Medium, ""),
    ("chromium", "Chromium", AvatarSize::Small, ""),
    ("steam", "Steam", AvatarSize::Large, ""),
    ("btop", "Btop", AvatarSize::Small, ""),
    ("no-such-app", "Nothing", AvatarSize::Medium, "\u{F120}"),
];

pub fn build(g: &mut Gallery, _: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let sp = t.tokens().spacing.clone();
    let bg = t.tokens().palette.background;
    let line = |gap: i64| div().flex().flex_row().items_center().gap(px(gap as f32));

    let badges = line(sp.control_gap)
        .child(Badge::new("New"))
        .child(Badge::new("3").tone(Tone::Accent).filled(true))
        .child(Badge::new("12").tone(Tone::Urgent).filled(true))
        .child(Badge::new("Online").tone(Tone::Success))
        .child(Badge::new("Low").tone(Tone::Warning))
        .child(Badge::new("Offline").tone(Tone::Urgent));

    let dots = line(sp.xxl)
        .children([Tone::Success, Tone::Warning, Tone::Urgent, Tone::Muted, Tone::Accent].map(StatusDot::new))
        .child(Avatar::new("Ryan").round(true).status(Tone::Success).ground(bg))
        .child(Avatar::new("Photo").source(PHOTO).status(Tone::Warning).ground(bg))
        .child(Avatar::new("Server").glyph("\u{F048B}").status(Tone::Urgent).ground(bg));

    let avatars = line(sp.xxl)
        .child(Avatar::new("ada").size(AvatarSize::Small))
        .child(Avatar::new("Basecamp"))
        .child(Avatar::new("Omarchy").size(AvatarSize::Large).round(true))
        .child(Avatar::new("Robot").glyph("\u{F06A9}"))
        .child(Avatar::new("Photo").source(PHOTO).size(AvatarSize::Small))
        .child(Avatar::new("Photo").source(PHOTO).round(true))
        .child(Avatar::new("Photo").source(PHOTO).size(AvatarSize::Large))
        .child(Avatar::new("Missing").source(MISSING));

    // By icon name, from the icon theme (the fixture icon theme under bin/review).
    let icons = line(sp.xxl).children(ICONS.map(|(name, label, size, glyph)| Avatar::new(label).icon(name).size(size).glyph(glyph)));

    let grid = TileGrid::new([
        Tile::new("Terminal").glyph("\u{F120}"),
        Tile::new("Files").glyph("\u{F07B}"),
        Tile::new("Browser").glyph("\u{F0AC}"),
        Tile::new("Music").glyph("\u{F001}"),
        Tile::new("Settings").glyph("\u{F013}"),
        Tile::new("Calendar").glyph("\u{F073}"),
        Tile::new("Screen Recorder").glyph("\u{F03D}"),
        Tile::new("Photos").avatar(Avatar::new("Photos").source(PHOTO)),
    ])
    .id("tiles-grid")
    .columns(4)
    .cursor(g.more.tiles.cursor.cursor_in(""))
    .on_pointer_move(cx.listener(|g: &mut Gallery, (i, p): &(usize, Point<Pixels>), _, cx| {
        if g.more.tiles.cursor.pointer_moved(p.x.into(), p.y.into(), "", *i) {
            cx.notify();
        }
    }));

    vec![badges.into_any_element(), dots.into_any_element(), avatars.into_any_element(), icons.into_any_element(), grid.into_any_element()]
}

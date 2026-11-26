//! The `tiles` section (spec/gallery.md).

use std::sync::Arc;

use omarchy_rune::Theme;
use omarchy_rune::ui::{Avatar, AvatarSize, Badge, CursorModel, StatusDot, Tile, TileGrid, Tone};
use rune::{El, Image, ImageCache, row};

use crate::State;

/// The sample picture (the same file as the Qt gallery's), a missing one for the fallback, and
/// the grid's cursor.
pub struct Tiles {
    _cache: ImageCache,
    pub photo: Image,
    pub missing: Image,
    pub cursor: CursorModel,
}

impl Tiles {
    pub fn init() -> Tiles {
        let cache = ImageCache::new(Arc::new(|| {}));
        let photo = cache.load(&include_bytes!("../../assets/avatar.png")[..]);
        let missing = cache.file(
            concat!(env!("CARGO_MANIFEST_DIR"), "/assets/missing.png"),
            48.0,
            2.0,
        );
        // Decoded before the first frame, so stills always show them.
        let _ = photo.wait();
        let _ = missing.wait();
        let mut cursor = CursorModel::list(8).columns(4);
        cursor.set("", 2);
        Tiles {
            _cache: cache,
            photo,
            missing,
            cursor,
        }
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

/// Decode the icon specimens before the first frame, at the 2× stills render at (a still is a
/// single frame, so an icon still decoding would be missing from it).
pub fn preload(t: &Theme) {
    for (name, _, size, _) in ICONS {
        if let Some(icon) = t.icon(name, size.px(t)) {
            let _ = icon.wait();
        }
    }
}

pub fn build(s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    let sp = &t.tokens().spacing;
    let bg = t.tokens().palette.background;
    let photo = Some(&s.tiles.photo);

    let badges = row()
        .items_center()
        .gap(sp.control_gap as f32)
        .child(Badge::new("New").build(t))
        .child(Badge::new("3").tone(Tone::Accent).filled(true).build(t))
        .child(Badge::new("12").tone(Tone::Urgent).filled(true).build(t))
        .child(Badge::new("Online").tone(Tone::Success).build(t))
        .child(Badge::new("Low").tone(Tone::Warning).build(t))
        .child(Badge::new("Offline").tone(Tone::Urgent).build(t));

    let dots = row()
        .items_center()
        .gap(sp.xxl as f32)
        .children(
            [
                Tone::Success,
                Tone::Warning,
                Tone::Urgent,
                Tone::Muted,
                Tone::Accent,
            ]
            .map(|tone| StatusDot::new(tone).build(t)),
        )
        .child(
            Avatar::new("Ryan")
                .round(true)
                .status(Tone::Success)
                .ground(bg)
                .build(t),
        )
        .child(
            Avatar::new("Photo")
                .image(photo)
                .status(Tone::Warning)
                .ground(bg)
                .build(t),
        )
        .child(
            Avatar::new("Server")
                .glyph("\u{F048B}")
                .status(Tone::Urgent)
                .ground(bg)
                .build(t),
        );

    let avatars = row()
        .items_center()
        .gap(sp.xxl as f32)
        .child(Avatar::new("ada").size(AvatarSize::Small).build(t))
        .child(Avatar::new("Basecamp").build(t))
        .child(
            Avatar::new("Omarchy")
                .size(AvatarSize::Large)
                .round(true)
                .build(t),
        )
        .child(Avatar::new("Robot").glyph("\u{F06A9}").build(t))
        .child(
            Avatar::new("Photo")
                .image(photo)
                .size(AvatarSize::Small)
                .build(t),
        )
        .child(Avatar::new("Photo").image(photo).round(true).build(t))
        .child(
            Avatar::new("Photo")
                .image(photo)
                .size(AvatarSize::Large)
                .build(t),
        )
        .child(
            Avatar::new("Missing")
                .image(Some(&s.tiles.missing))
                .build(t),
        );

    // By icon name, from the icon theme (the fixture icon theme under bin/review).
    let icons = row().items_center().gap(sp.xxl as f32).children(ICONS.map(
        |(name, label, size, glyph)| {
            Avatar::new(label)
                .icon(name)
                .size(size)
                .glyph(glyph)
                .build(t)
        },
    ));

    let grid = TileGrid::new([
        Tile::new("Terminal").glyph("\u{F120}"),
        Tile::new("Files").glyph("\u{F07B}"),
        Tile::new("Browser").glyph("\u{F0AC}"),
        Tile::new("Music").glyph("\u{F001}"),
        Tile::new("Settings").glyph("\u{F013}"),
        Tile::new("Calendar").glyph("\u{F073}"),
        Tile::new("Screen Recorder").glyph("\u{F03D}"),
        Tile::new("Photos").avatar(Avatar::new("Photos").image(photo)),
    ])
    .columns(4)
    .key("tile")
    .cursor(s.tiles.cursor.cursor_in(""))
    .on_pointer_move(|s: &mut State, i, x, y| {
        s.tiles.cursor.pointer_moved(x, y, "", i);
    })
    .build(t);

    vec![badges, dots, avatars, icons, grid]
}

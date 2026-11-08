//! The `layout` section (spec/gallery.md): a SplitView with a Sidebar, and a Calendar.

use omarchy_rune::Theme;
use omarchy_rune::ui::{Calendar, Date, SplitPane, Sidebar, SidebarItem, SidebarSection, SplitView};
use rune::{El, el, row, text};

use crate::State;

/// The section's own state: the sidebar's width and choice, the calendar's month and day.
pub struct Layout {
    pub sidebar: f32,
    pub place: String,
    pub month: (i32, u32),
    pub day: Date,
}

pub fn init() -> Layout {
    Layout { sidebar: 200.0, place: "recent".into(), month: (2026, 11), day: Date::new(2026, 11, 14) }
}

pub fn build(s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    let tk = t.tokens();
    let l = &s.layout;
    let sidebar = Sidebar::new([
        SidebarSection::new(
            "Library",
            [
                SidebarItem::new("all", "All files", "\u{f024b}"),
                SidebarItem::new("recent", "Recent", "\u{f02da}"),
                SidebarItem::new("starred", "Starred", "\u{f04ce}"),
                SidebarItem::new("trash", "Trash", "\u{f0a7a}"),
            ],
        ),
        SidebarSection::new(
            "Places",
            [SidebarItem::new("documents", "Documents", "\u{f0219}"), SidebarItem::new("downloads", "Downloads", "\u{f01da}")],
        ),
    ])
    .selected(Some(&l.place))
    .key("places")
    .on_select(|s: &mut State, id| s.layout.place = id.to_string())
    .build(t);
    let content = row().center().child(text("No file selected").font(t.text_style(tk.font.body)).color(t.faint()));
    let split = SplitView::new()
        .pane(SplitPane::new(sidebar).size(l.sidebar).min(120.0))
        .pane(SplitPane::new(content))
        .on_resize(|s: &mut State, _, w| s.layout.sidebar = w)
        .key("split")
        .build(t);
    // Framed by a track-colored line, so its edges show.
    let frame = el().pin(Some(0.0), Some(0.0), Some(0.0), Some(0.0)).radius(t.radius()).border(tk.spacing.hairline as f32, t.track());
    let specimen = el().size(720.0, 220.0).child(split).child(frame);

    let calendar = Calendar::new(l.month.0, l.month.1)
        .today(Date::new(2026, 11, 8))
        .selected(Some(l.day))
        .key("cal")
        .on_pick(|s: &mut State, d| s.layout.day = d)
        .on_month(|s: &mut State, y, m| s.layout.month = (y, m))
        .build(t);
    vec![specimen, calendar]
}

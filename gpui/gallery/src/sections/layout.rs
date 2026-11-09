//! The `layout` section (spec/gallery.md): a SplitView with a Sidebar, and a Calendar.

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Calendar, Date, Sidebar, SidebarItem, SidebarSection, SplitPane, SplitView, label};

use crate::Gallery;

/// The sidebar's width and choice, the calendar's month and day.
pub struct State {
    pub sidebar: f32,
    pub place: SharedString,
    pub month: (i32, u32),
    pub day: Date,
}

impl State {
    pub fn new(_cx: &mut App) -> State {
        State { sidebar: 200.0, place: "recent".into(), month: (2026, 11), day: Date::new(2026, 11, 14) }
    }
}

pub fn build(g: &mut Gallery, _: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let tk = t.tokens();
    let l = &g.more.layout;
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
        SidebarSection::new("Places", [SidebarItem::new("documents", "Documents", "\u{f0219}"), SidebarItem::new("downloads", "Downloads", "\u{f01da}")]),
    ])
    .id("places")
    .selected(Some(l.place.clone()))
    .on_select(cx.listener(|g: &mut Gallery, id: &SharedString, _, cx| {
        g.more.layout.place = id.clone();
        cx.notify();
    }));
    let content = div().size_full().flex().items_center().justify_center().child(label(&t, "No file selected", tk.font.body, false, t.faint()));
    let split = SplitView::new("split")
        .pane(SplitPane::new(sidebar).size(l.sidebar).min(120.0))
        .pane(SplitPane::new(content))
        .on_resize(cx.listener(|g: &mut Gallery, (_, w): &(usize, f32), _, cx| {
            g.more.layout.sidebar = *w;
            cx.notify();
        }));
    // Framed by a track-colored line, so its edges show.
    let frame = div().absolute().inset_0().rounded(t.radius()).border(px(tk.spacing.hairline as f32)).border_color(t.track());
    let specimen = div().relative().flex_shrink_0().w(px(720.0)).h(px(220.0)).child(split).child(frame);

    let calendar = Calendar::new(l.month.0, l.month.1)
        .id("cal")
        .today(Date::new(2026, 11, 8))
        .selected(Some(l.day))
        .on_pick(cx.listener(|g: &mut Gallery, d: &Date, _, cx| {
            g.more.layout.day = *d;
            cx.notify();
        }))
        .on_month(cx.listener(|g: &mut Gallery, m: &(i32, u32), _, cx| {
            g.more.layout.month = *m;
            cx.notify();
        }));
    vec![specimen.into_any_element(), calendar.into_any_element()]
}

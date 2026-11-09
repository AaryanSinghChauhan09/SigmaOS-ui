//! The `composed` section (spec/components/composed.md): real UIs built only from kit components
//! and tokens, the test that the kit is complete. No element of its own but layout.

use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{
    Button, Card, ColorWell, Dropdown, Hero, IconButton, KeyValue, ListRow, NumberField, Pane, PasswordField, SEARCH_SECTION, ScrollArea,
    SearchItem, SearchList, SearchState, SettingRow, Side, Slider, Tabs, TextInput, ToggleSwitch, Tone, section_header, separator,
};
use omarchy_gpui::{Paint, color};

use crate::Gallery;

pub struct State {
    pub wifi: bool,
    pub password: Entity<TextInput>,
    pub tab: usize,
    pub radius: f32,
    pub border: SharedString,
    pub shadow: bool,
    pub padding: i64,
    pub search: Entity<SearchState>,
    pub empty: Entity<SearchState>,
    /// The last result activated, by title.
    pub picked: Option<SharedString>,
    /// The last result asked to be deleted, by title.
    pub deleted: Option<SharedString>,
    /// Times the overlay asked to close.
    pub closes: usize,
}

impl State {
    pub fn new(cx: &mut App) -> State {
        let search = SearchState::new(cx, "term");
        // Qt's specimen opens with the cursor on the second result.
        search.update(cx, |s, _| {
            s.cursor.set_count(SEARCH_SECTION, 4);
            s.cursor.set(SEARCH_SECTION, 1);
        });
        State {
            wifi: true,
            password: TextInput::new(cx, ""),
            tab: 0,
            radius: 0.4,
            border: "solid".into(),
            shadow: true,
            padding: 24,
            search,
            empty: SearchState::new(cx, "zzz"),
            picked: None,
            deleted: None,
            closes: 0,
        }
    }
}

fn search_items() -> Vec<SearchItem> {
    vec![
        SearchItem::new("Terminal").glyph("\u{F120}").subtitle("App").detail("Open a new terminal window."),
        SearchItem::new("Terminal font").glyph("\u{F031}").subtitle("Style").detail("Pick the monospace font every terminal uses."),
        SearchItem::new("Terminal theme").glyph("\u{F1FC}").subtitle("Style").detail("Match the terminal colors to the desktop theme."),
        SearchItem::new("Intermediate CA").glyph("\u{F0A3}").subtitle("Certificate").detail("Trust a certificate authority for this machine."),
    ]
}

pub fn filtered(query: &str) -> Vec<SearchItem> {
    search_items().into_iter().filter(|i| SearchList::matches(&i.title, query)).collect()
}

pub fn build(g: &mut Gallery, _: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let sp = t.tokens().spacing.clone();
    let c = &g.more.composed;
    let column_gap = px(sp.huge as f32 * 2.0);
    let row = || div().flex().flex_row().items_start().gap(column_gap);

    // A Wi-Fi panel, as the shell's network panel.
    let networks = ScrollArea::new("composed-networks")
        .max_height(280.0)
        .gap(sp.xxs as f32)
        .child(ListRow::new("Basecamp").glyph("\u{F0928}").status("Connected", Tone::Success).current(true))
        .child(ListRow::new("Basecamp Guest").glyph("\u{F0925}").subtitle("Open").action("\u{F0159}", true).cursor(true))
        .child(
            ListRow::new("Coffee Shop").glyph("\u{F0922}").subtitle("WPA2").trailing_glyph("\u{F033E}", Tone::Neutral).expansion(
                div()
                    .flex()
                    .flex_row()
                    .w_full()
                    .items_center()
                    .gap(px(sp.md as f32))
                    .child(div().flex_1().min_w_0().child(PasswordField::new(&c.password).placeholder("Password")))
                    .child(Button::new("Connect").id("composed-connect").bordered(true)),
            ),
        )
        .child(
            ListRow::new("Library").glyph("\u{F091F}").subtitle("Saved").trailing_glyph("\u{F033E}", Tone::Neutral).action("\u{F0159}", true),
        );
    let wifi = Card::new()
        .width(360.0)
        .child(
            Hero::new("\u{F0928}", "Wi-Fi").status("Connected to Basecamp").trailing(ToggleSwitch::new("composed-wifi", c.wifi).on_toggle(
                cx.listener(|g: &mut Gallery, v: &bool, _, cx| {
                    g.more.composed.wifi = *v;
                    cx.notify();
                }),
            )),
        )
        .child(section_header(&t, "Networks"))
        .child(networks)
        .child(separator(&t))
        .child(KeyValue::new().pair("IP", "192.168.1.24").pair("Gateway", "192.168.1.1").pair("DNS", "1.1.1.1"));

    // An app inspector, as omareel's side panel: a pane on the window, ruled off from the canvas.
    let tabs = Tabs::new(["Shape", "Cursor", "Zoom"], c.tab).id("composed-tabs").on_choose(cx.listener(|g: &mut Gallery, i: &usize, _, cx| {
        g.more.composed.tab = *i;
        cx.notify();
    }));
    let frame = div()
        .flex()
        .flex_col()
        .w_full()
        .child(SettingRow::new("Radius").control(Slider::new("composed-radius", c.radius).width(110.0).on_change(cx.listener(
            |g: &mut Gallery, v: &f32, _, cx| {
                g.more.composed.radius = *v;
                cx.notify();
            },
        ))))
        .child(SettingRow::new("Border").control(
            Dropdown::new("composed-border", [("solid", "Solid"), ("none", "None")], c.border.clone()).width(110.0).on_choose(cx.listener(
                |g: &mut Gallery, v: &SharedString, _, cx| {
                    g.more.composed.border = v.clone();
                    cx.notify();
                },
            )),
        ))
        .child(SettingRow::new("Shadow").cursor(true).control(ToggleSwitch::new("composed-shadow", c.shadow).on_toggle(cx.listener(
            |g: &mut Gallery, v: &bool, _, cx| {
                g.more.composed.shadow = *v;
                cx.notify();
            },
        ))));
    let background = div()
        .flex()
        .flex_col()
        .w_full()
        .child(SettingRow::new("Fill").control(ColorWell::new(color(t.tokens().palette.accent))))
        .child(SettingRow::new("Padding").control(NumberField::new("", c.padding).id("composed-padding").range(0, 96).on_change(
            cx.listener(|g: &mut Gallery, v: &i64, _, cx| {
                g.more.composed.padding = *v;
                cx.notify();
            }),
        )));
    let inspector = Pane::new()
        .width(300.0)
        .title("Recording 1")
        .subtitle("1920 × 1080 · 0:42")
        .divider(Side::Left)
        .action(IconButton::new("\u{F0E2}"))
        .action(IconButton::new("\u{F01E}"))
        .action(Button::new("Export").bordered(true))
        .child(tabs)
        .child(section_header(&t, "Frame"))
        .child(frame)
        .child(section_header(&t, "Background"))
        .child(background);

    // A type-ahead overlay with "term" typed: four results, the second under the cursor, and the
    // detail pane. Then one where nothing matches "zzz".
    let query = c.search.read(cx).query(cx);
    let search = SearchList::new("composed-search", &c.search)
        .items(filtered(&query))
        .detail(true)
        .size(360.0, 300.0)
        .on_activate(cx.listener(|g: &mut Gallery, i: &usize, _, cx| {
            let q = g.more.composed.search.read(cx).query(cx);
            g.more.composed.picked = filtered(&q).get(*i).map(|i| i.title.clone());
            cx.notify();
        }))
        .on_delete(cx.listener(|g: &mut Gallery, i: &usize, _, cx| {
            let q = g.more.composed.search.read(cx).query(cx);
            g.more.composed.deleted = filtered(&q).get(*i).map(|i| i.title.clone());
            cx.notify();
        }))
        .on_close(cx.listener(|g: &mut Gallery, _: &(), _, cx| {
            g.more.composed.closes += 1;
            cx.notify();
        }));
    let empty_query = c.empty.read(cx).query(cx);
    let empty = SearchList::new("composed-empty", &c.empty).items(filtered(&empty_query)).size(300.0, 200.0);

    vec![row().child(wifi).child(inspector).into_any_element(), row().child(search).child(empty).into_any_element()]
}

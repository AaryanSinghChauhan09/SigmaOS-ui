//! The `composed` section (spec/components/composed.md): real UIs built only from kit components
//! and tokens, the test that the kit is complete. No element of its own but layout.

use omarchy_rune::Theme;
use omarchy_rune::ui::{
    Button, Card, ColorWell, CursorModel, Dropdown, Hero, IconButton, KeyValue, ListRow, NumberField, PasswordField, ScrollArea,
    SearchItem, SearchList, SettingRow, Slider, Tabs, Toolbar, ToggleSwitch, Tone, section_header, separator,
};
use rune::{El, TextEdit, col, row};

use crate::State;

pub struct Composed {
    pub wifi: bool,
    pub password: TextEdit,
    pub tab: usize,
    pub radius: f32,
    pub border: String,
    pub border_open: bool,
    pub shadow: bool,
    pub padding: i64,
    pub query: TextEdit,
    pub cursor: CursorModel,
    pub empty_query: TextEdit,
    pub empty_cursor: CursorModel,
    /// The last result activated, by title.
    pub picked: Option<String>,
}

pub fn init() -> Composed {
    let mut cursor = SearchList::cursor();
    cursor.set_count(omarchy_rune::ui::SEARCH_SECTION, 4);
    cursor.set(omarchy_rune::ui::SEARCH_SECTION, 1);
    Composed {
        wifi: true,
        password: TextEdit::new(""),
        tab: 0,
        radius: 0.4,
        border: "solid".into(),
        border_open: false,
        shadow: true,
        padding: 24,
        query: TextEdit::new("term"),
        cursor,
        empty_query: TextEdit::new("zzz"),
        empty_cursor: SearchList::cursor(),
        picked: None,
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

fn filtered(query: &str) -> Vec<SearchItem> {
    search_items().into_iter().filter(|i| SearchList::matches(&i.title, query)).collect()
}

pub fn build(s: &State, t: &Theme, layer: &mut Vec<El>) -> Vec<El> {
    let c = &s.composed;
    let sp = &t.tokens().spacing;
    let column_gap = sp.huge as f32 * 2.0;

    // A Wi-Fi panel, as the shell's network panel.
    let networks = ScrollArea::new("composed-networks")
        .max_height(280.0)
        .gap(sp.xxs as f32)
        .child(ListRow::new("Basecamp").glyph("\u{F0928}").status("Connected", Tone::Success).current(true).build(t))
        .child(ListRow::new("Basecamp Guest").glyph("\u{F0925}").subtitle("Open").action("\u{F0159}", true).cursor(true).build(t))
        .child(
            ListRow::new("Coffee Shop")
                .glyph("\u{F0922}")
                .subtitle("WPA2")
                .value("\u{F033E}")
                .expansion(
                    row()
                        .w_full()
                        .items_center()
                        .gap(sp.md as f32)
                        .child(
                            PasswordField::new(&c.password)
                                .placeholder("Password")
                                .on_edit(|s: &mut State| &mut s.composed.password)
                                .build(t)
                                .grow(1.0)
                                .min_w(0.0),
                        )
                        .child(Button::new("Connect").bordered(true).build(t)),
                )
                .build(t),
        )
        .child(ListRow::new("Library").glyph("\u{F091F}").subtitle("Saved").build(t))
        .build(t);
    let wifi = Card::new()
        .width(360.0)
        .child(
            Hero::new("\u{F0928}", "Wi-Fi")
                .status("Connected to Basecamp")
                .trailing(ToggleSwitch::new(c.wifi).on_toggle(|s: &mut State| s.composed.wifi ^= true).build(t))
                .build(t),
        )
        .child(section_header(t, "Networks"))
        .child(networks)
        .child(separator(t))
        .child(KeyValue::new().pair("IP", "192.168.1.24").pair("Gateway", "192.168.1.1").pair("DNS", "1.1.1.1").build(t))
        .build(t);

    // An app inspector, as omareel's side panel.
    let head = col()
        .w_full()
        .child(
            Toolbar::new()
                .divided(false)
                .leading([IconButton::new("\u{F0E2}").build(t), IconButton::new("\u{F01E}").build(t)])
                .trailing([Button::new("Export").bordered(true).build(t)])
                .build(t),
        )
        .child(Tabs::new(["Shape", "Cursor", "Zoom"], c.tab).on_choose(|s: &mut State, i| s.composed.tab = i).build(t));
    let frame = col()
        .w_full()
        .child(SettingRow::new("Radius").control(Slider::new(c.radius).width(110.0).on_change(|s: &mut State, v| s.composed.radius = v).build(t)).build(t))
        .child(
            SettingRow::new("Border")
                .control(
                    Dropdown::new("composed-border", [("solid", "Solid"), ("none", "None")], c.border.clone())
                        .width(110.0)
                        .open(c.border_open)
                        .on_toggle(|s: &mut State| s.composed.border_open ^= true)
                        .on_choose(|s: &mut State, v: &str| {
                            s.composed.border = v.into();
                            s.composed.border_open = false;
                        })
                        .build(t, layer),
                )
                .build(t),
        )
        .child(
            SettingRow::new("Shadow")
                .cursor(true)
                .control(ToggleSwitch::new(c.shadow).on_toggle(|s: &mut State| s.composed.shadow ^= true).build(t))
                .build(t),
        );
    let background = col()
        .w_full()
        .child(SettingRow::new("Fill").control(ColorWell::new(t.accent()).build(t)).build(t))
        .child(
            SettingRow::new("Padding")
                .control(NumberField::new("", c.padding).range(0, 96).on_change(|s: &mut State, v| s.composed.padding = v).build(t))
                .build(t),
        );
    let inspector = Card::new()
        .width(300.0)
        .child(head)
        .child(section_header(t, "Frame"))
        .child(frame)
        .child(section_header(t, "Background"))
        .child(background)
        .build(t);

    // A type-ahead overlay with "term" typed: four results, the second under the cursor, and the
    // detail pane. Then one where nothing matches "zzz".
    let search = SearchList::new("composed-search", &c.query, &c.cursor)
        .items(filtered(&c.query.text))
        .detail(true)
        .size(360.0, 300.0)
        .on_state(|s: &mut State| (&mut s.composed.query, &mut s.composed.cursor))
        .on_activate(|s: &mut State, i| s.composed.picked = filtered(&s.composed.query.text).get(i).map(|i| i.title.clone()))
        .build(t);
    let empty = SearchList::new("composed-empty", &c.empty_query, &c.empty_cursor)
        .items(filtered(&c.empty_query.text))
        .size(300.0, 200.0)
        .on_state(|s: &mut State| (&mut s.composed.empty_query, &mut s.composed.empty_cursor))
        .build(t);

    vec![
        row().items_start().gap(column_gap).child(wifi).child(inspector),
        row().items_start().gap(column_gap).child(search).child(empty),
    ]
}

//! The gallery's state: everything a section's specimens show that the keys and the pointer can
//! change. The initial values are the specimens spec/terminal.md (and spec/gallery.md) list.

use omarchy_ratatui::ui::{AvatarImage, CursorModel, Date, Hsva, MultiSelectState};

pub struct State {
    /// The interactive window's focused control in the current section (None in snapshots).
    pub focus: Option<&'static str>,
    pub picture: Option<AvatarImage>,

    // buttons
    pub align: usize,
    // toggles
    pub switches: [bool; 4],
    pub wifi: bool,
    pub bluetooth: bool,
    // inputs
    pub search: String,
    pub name: String,
    pub gaps: i64,
    pub theme_choice: usize,
    pub dropdown_open: bool,
    pub dropdown_row: usize,
    // slider
    pub level: f32,
    pub stepped: f32,
    // surfaces
    pub network_on: bool,
    pub night_light: bool,
    pub confirm_selected: usize,
    // lists
    pub rows: CursorModel,
    pub current_row: usize,
    pub rows_offset: u16,
    pub tile_cursor: Option<usize>,
    // menus
    pub menu_cursor: usize,
    pub context_cursor: usize,
    pub sidebar_shown: bool,
    pub word_wrap: bool,
    pub sort_by_name: bool,
    pub ran: Option<&'static str>,
    // navigation
    pub tab: usize,
    pub tab_cursor: Option<usize>,
    pub panel: usize,
    // forms
    pub checks: [bool; 5],
    pub position: String,
    pub password: String,
    pub revealed: bool,
    pub night_toggle: bool,
    // pickers
    pub modules: Vec<String>,
    pub notify: Vec<String>,
    pub notify_state: MultiSelectState,
    pub swatch: usize,
    pub swatch_cursor: Option<usize>,
    pub discard_selected: usize,
    // data
    pub table_selected: usize,
    pub table_cursor: usize,
    pub tree_expanded: Vec<String>,
    pub tree_selected: String,
    pub tree_cursor: String,
    // layout
    pub sidebar_selected: String,
    pub sidebar_width: u16,
    pub month: (i32, u32),
    pub day_selected: Date,
    pub day_cursor: Option<Date>,
    // timeline
    pub playhead: f64,
    pub clip: Option<(usize, usize)>,
    pub scrub: f64,
    pub drag_cursor: usize,
    // color
    pub color: Hsva,
    // composed
    pub query: String,
    pub search_cursor: CursorModel,
    pub activated: Option<usize>,
    pub wifi_rows: CursorModel,
    pub inspector_tab: usize,
}

/// The sample picture (the same 96×96 file as the Qt and Rune galleries').
const PICTURE: &[u8] = include_bytes!("../../../../qt/gallery/assets/avatar.png");

impl State {
    pub fn init() -> State {
        let mut rows = CursorModel::list(9);
        rows.set("", 1);
        let mut search_cursor = CursorModel::list(4);
        search_cursor.set("", 1);
        let mut wifi_rows = CursorModel::list(4);
        wifi_rows.set("", 1);
        let picture = AvatarImage::decode(PICTURE);
        State {
            focus: None,
            picture,
            align: 1,
            switches: [false, true, false, true],
            wifi: true,
            bluetooth: false,
            search: String::new(),
            name: "Omarchy".into(),
            gaps: 10,
            theme_choice: 0,
            dropdown_open: false,
            dropdown_row: 0,
            level: 0.35,
            stepped: 0.75,
            network_on: true,
            night_light: true,
            confirm_selected: 1,
            rows,
            current_row: 2,
            rows_offset: 0,
            tile_cursor: Some(2),
            menu_cursor: 1,
            context_cursor: 0,
            sidebar_shown: true,
            word_wrap: false,
            sort_by_name: true,
            ran: None,
            tab: 1,
            tab_cursor: Some(2),
            panel: 1,
            checks: [false, true, false, false, true],
            position: "bottom".into(),
            password: "hunter22".into(),
            revealed: false,
            night_toggle: true,
            modules: ["clock", "battery", "network", "volume"].map(String::from).to_vec(),
            notify: ["mail", "updates"].map(String::from).to_vec(),
            notify_state: MultiSelectState { open: true, search: String::new(), row: None },
            swatch: 2,
            swatch_cursor: None,
            discard_selected: 1,
            table_selected: 1,
            table_cursor: 3,
            tree_expanded: ["root", "qt", "spec"].map(String::from).to_vec(),
            tree_selected: "qt/table".into(),
            tree_cursor: "readme".into(),
            sidebar_selected: "recent".into(),
            sidebar_width: 25,
            month: (2026, 11),
            day_selected: Date::new(2026, 11, 14),
            day_cursor: None,
            playhead: 12.0,
            clip: Some((0, 1)),
            scrub: 42.0,
            drag_cursor: 1,
            color: Hsva::from_hex("#7aa2f7", 0.0).unwrap(),
            query: "term".into(),
            search_cursor,
            activated: None,
            wifi_rows,
            inspector_tab: 0,
        }
    }
}

pub const THEMES: [&str; 5] = ["Tokyo Night", "Catppuccin", "Gruvbox", "Nord", "Rose Pine"];
pub const MODULES: [(&str, &str); 5] = [("clock", "Clock"), ("battery", "Battery"), ("network", "Network"), ("volume", "Volume"), ("bluetooth", "Bluetooth")];
pub const NOTIFY: [(&str, &str); 5] = [("calendar", "Calendar"), ("mail", "Mail"), ("messages", "Messages"), ("updates", "Updates"), ("battery", "Battery")];

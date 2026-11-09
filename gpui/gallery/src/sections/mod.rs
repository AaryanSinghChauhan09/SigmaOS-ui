//! The gallery's sections (spec/gallery.md), one module each.

use omarchy_gpui::gpui::App;

pub mod buttons;
pub mod color;
pub mod composed;
pub mod data;
pub mod forms;
pub mod inputs;
pub mod layout;
pub mod lists;
pub mod menus;
pub mod messages;
pub mod navigation;
pub mod pickers;
pub mod progress;
pub mod slider;
pub mod surfaces;
pub mod tiles;
pub mod timeline;
pub mod toggles;
pub mod tokens;

/// The state the sections keep, one field per section that has any.
pub struct More {
    pub lists: lists::State,
    pub tiles: tiles::State,
    pub progress: progress::State,
    pub messages: messages::State,
    pub menus: menus::State,
    pub navigation: navigation::State,
    pub forms: forms::State,
    pub pickers: pickers::State,
    pub data: data::State,
    pub layout: layout::State,
    pub timeline: timeline::State,
    pub color: color::State,
    pub composed: composed::State,
}

impl More {
    pub fn new(cx: &mut App) -> More {
        More {
            lists: lists::State::new(cx),
            tiles: tiles::State::new(cx),
            progress: progress::State::new(cx),
            messages: messages::State::new(cx),
            menus: menus::State::new(cx),
            navigation: navigation::State::new(cx),
            forms: forms::State::new(cx),
            pickers: pickers::State::new(cx),
            data: data::State::new(cx),
            layout: layout::State::new(cx),
            timeline: timeline::State::new(cx),
            color: color::State::new(cx),
            composed: composed::State::new(cx),
        }
    }
}

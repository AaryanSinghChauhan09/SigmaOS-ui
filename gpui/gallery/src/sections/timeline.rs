//! The `timeline` section (spec/gallery.md).

use omarchy_gpui::gpui::{prelude::*, *};

use crate::Gallery;

/// What this section keeps between frames.
pub struct State {}

impl State {
    pub fn new(_cx: &mut App) -> State {
        State {}
    }
}

pub fn build(_g: &mut Gallery, _window: &mut Window, _cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    Vec::new()
}

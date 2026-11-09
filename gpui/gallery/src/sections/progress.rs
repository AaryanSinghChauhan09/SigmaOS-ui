//! The `progress` section (spec/gallery.md).

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Meter, ProgressBar, Spinner};

use crate::Gallery;

/// What this section keeps between frames.
pub struct State {}

impl State {
    pub fn new(_cx: &mut App) -> State {
        State {}
    }
}

pub fn build(_g: &mut Gallery, _window: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let tk = t.tokens();
    let gap = px(tk.spacing.huge as f32);
    let marked = |v: f32| Meter::new(v).markers([0.5, 0.8]).width(220.0);
    let row = || div().flex().flex_row().gap(gap);
    vec![
        row().child(marked(0.2)).child(marked(0.65)).child(marked(0.92).alarming(true)).into_any_element(),
        Meter::new(0.65).label("Battery").value_text("65%").width(360.0).into_any_element(),
        row().child(ProgressBar::new(0.4).width(340.0)).child(ProgressBar::indeterminate().width(340.0)).into_any_element(),
        row()
            .items_center()
            .child(Spinner::new())
            .child(Spinner::new().size(tk.font.icon_large))
            .child(Spinner::new().pulse(true))
            .child(Spinner::new().pulse(true).size(tk.font.icon_large))
            .into_any_element(),
    ]
}

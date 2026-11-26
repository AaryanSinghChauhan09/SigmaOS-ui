//! The `progress` section (spec/gallery.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::{Meter, ProgressBar, Spinner};
use rune::{El, row};

use crate::State;

pub fn build(_s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    let tk = t.tokens();
    let gap = tk.spacing.huge as f32;
    let marked = |v: f32| Meter::new(v).markers([0.5, 0.8]).width(220.0);
    vec![
        row()
            .gap(gap)
            .child(marked(0.2).build(t))
            .child(marked(0.65).build(t))
            .child(marked(0.92).alarming(true).build(t)),
        Meter::new(0.65)
            .label("Battery")
            .value_text("65%")
            .width(360.0)
            .build(t),
        row()
            .gap(gap)
            .child(ProgressBar::new(0.4).width(340.0).build(t))
            .child(ProgressBar::indeterminate().width(340.0).build(t)),
        row()
            .items_center()
            .gap(gap)
            .child(Spinner::new().build(t))
            .child(Spinner::new().size(tk.font.icon_large).build(t))
            .child(Spinner::new().pulse(true).build(t))
            .child(Spinner::new().pulse(true).size(tk.font.icon_large).build(t)),
    ]
}

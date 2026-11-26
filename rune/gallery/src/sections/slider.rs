//! The `slider` section (spec/gallery.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::Slider;
use rune::El;

use crate::State;

pub fn build(s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    vec![
        Slider::new(s.level)
            .width(360.0)
            .on_change(|s: &mut State, v| s.level = v)
            .build(t),
        Slider::new(s.stepped)
            .ticks(5)
            .width(360.0)
            .on_change(|s: &mut State, v| s.stepped = (v * 4.0).round() / 4.0)
            .build(t),
    ]
}

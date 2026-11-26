//! The `color` section (spec/gallery.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::{ColorPicker, Hsva};
use rune::{El, TextEdit};

use crate::State;

pub struct Picker {
    pub value: Hsva,
    pub hex: TextEdit,
}

/// A fixed color, not the theme's accent, so the picker's content is the same in every theme.
pub fn init() -> Picker {
    let value = Hsva::from_hex("#7aa2f7", 0.0).expect("a hex color");
    Picker {
        value,
        hex: TextEdit::new(value.hex()),
    }
}

pub fn build(s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    vec![
        ColorPicker::new(s.color.value)
            .size(300.0, 260.0)
            .hex(&s.color.hex)
            .on_change(
                |s: &mut State| &mut s.color.hex,
                |s: &mut State, v| s.color.value = v,
            )
            .build(t),
    ]
}

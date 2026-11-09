//! The Omarchy font in GPUI: fontconfig's `monospace` (`omarchy font set`), by the family it
//! resolves to. GPUI's Linux text system loads the system's fonts but doesn't resolve fontconfig
//! aliases, so the kit asks for `tokens.font.resolved_family`. When that family isn't among the
//! fonts GPUI loaded (a font installed after the app started), its file is added.

use std::borrow::Cow;

use gpui::App;
use omarchy_theme::Tokens;

use crate::theme::family_of;

pub(crate) fn setup(cx: &mut App, tokens: &Tokens) {
    let family = family_of(tokens);
    let text = cx.text_system().clone();
    if text.all_font_names().iter().any(|n| *n == family) {
        return;
    }
    if let Some(bytes) = tokens.font.file.as_ref().and_then(|f| std::fs::read(f).ok()) {
        let _ = text.add_fonts(vec![Cow::Owned(bytes)]);
    }
}

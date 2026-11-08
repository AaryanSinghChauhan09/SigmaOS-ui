//! The active Omarchy theme as framework-neutral design tokens.
//!
//! ```no_run
//! let tokens = omarchy_theme::load();
//! println!("{} on {}", tokens.palette.foreground, tokens.palette.background);
//! let _watch = omarchy_theme::watch(tokens, |fresh| println!("theme changed: {}", fresh.palette.accent));
//! ```
//!
//! Resolution follows `spec/tokens.md`, which follows the Omarchy shell. Any UI framework in Rust
//! (Rune, GPUI, iced, egui) can build its theme from [`Tokens`].

pub mod color;
pub mod parse;
pub mod resolve;
pub mod source;
pub mod tokens;
pub mod watch;

pub use color::Rgba;
pub use resolve::{HyprValues, Sources, resolve};
pub use tokens::*;
pub use watch::{Watch, watch};

/// Resolve the theme as it is right now (or the `OMARCHY_UI_THEME_DIR` fixture).
pub fn load() -> Tokens {
    resolve(&source::load_sources())
}

#[cfg(test)]
mod tests;

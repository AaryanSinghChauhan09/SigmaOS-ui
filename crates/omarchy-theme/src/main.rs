//! `omarchy-theme`: print the resolved tokens.
//!
//!   omarchy-theme                 canonical JSON for the live theme
//!   omarchy-theme --watch         print again on every change
//!   OMARCHY_UI_THEME_DIR=fixtures/themes/nord omarchy-theme

fn main() {
    let args: Vec<String> = std::env::args().skip(1).collect();
    if args.iter().any(|a| a == "-h" || a == "--help") {
        println!("usage: omarchy-theme [--watch]\n\nPrints the resolved Omarchy UI tokens as canonical JSON (spec/tokens.md).");
        return;
    }
    let tokens = omarchy_theme::load();
    println!("{}", tokens.to_canonical_json());
    if args.iter().any(|a| a == "--watch") {
        let _w = omarchy_theme::watch(tokens, |t| println!("{}", t.to_canonical_json()));
        loop {
            std::thread::park();
        }
    }
}

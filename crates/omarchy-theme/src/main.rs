//! `omarchy-theme`: print the resolved tokens.
//!
//!   omarchy-theme                 canonical JSON for the live theme
//!   omarchy-theme --watch         print again on every change
//!   omarchy-theme --icon firefox [96]   the file an icon name resolves to in the icon theme
//!   OMARCHY_UI_THEME_DIR=fixtures/themes/nord omarchy-theme

fn main() {
    let args: Vec<String> = std::env::args().skip(1).collect();
    if args.iter().any(|a| a == "-h" || a == "--help") {
        println!(
            "usage: omarchy-theme [--watch | --icon NAME [SIZE]]\n\nPrints the resolved Omarchy UI tokens as canonical JSON (spec/tokens.md),\nor the file an icon name resolves to in the theme's icon theme (empty if none; SIZE in pixels, default 48)."
        );
        return;
    }
    if let Some(i) = args.iter().position(|a| a == "--icon") {
        let name = args.get(i + 1).map(String::as_str).unwrap_or("");
        let size = args.get(i + 2).and_then(|s| s.parse().ok()).unwrap_or(48);
        let found = omarchy_theme::icons::IconLookup::current().find(name, size);
        println!("{}", found.map(|p| p.display().to_string()).unwrap_or_default());
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

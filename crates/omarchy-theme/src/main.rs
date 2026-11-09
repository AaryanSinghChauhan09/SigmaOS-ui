//! `omarchy-theme`: the resolver as a command, for scripts and for toolkits that spawn a process
//! (Electron, editors, anything without a binding). docs/using-c.md has the stream format.

use std::io::Write;

const USAGE: &str = "\
usage: omarchy-theme [OPTIONS]

Prints the live Omarchy theme resolved as Omarchy UI tokens (spec/tokens.md).

  (none)                 the canonical token JSON, indented
  --compact              the canonical token JSON on one line
  --state                one line: {schema, revision, theme, iconTheme, language, tokens, colors, strings}
  --watch                print once, then one line per change (spec/following.md); with --state,
                         state lines, else compact token lines. Runs until stdout closes.
  --strings              the kit's strings in the display language: {language, strings}
  --icon-theme           the icon theme the Omarchy theme names
  --icon NAME [SIZE]     the file an icon name resolves to (empty if none; SIZE px, default 48)
  --icons SIZE NAME...   several at once, as one JSON object {name: path or null}
  --version              the version

OMARCHY_UI_THEME_DIR, OMARCHY_UI_ROUNDING and the other OMARCHY_UI_* overrides apply.";

fn main() {
    let args: Vec<String> = std::env::args().skip(1).collect();
    let has = |flag: &str| args.iter().any(|a| a == flag);
    if has("-h") || has("--help") {
        println!("{USAGE}");
        return;
    }
    if has("--version") {
        println!("omarchy-theme {}", env!("CARGO_PKG_VERSION"));
        return;
    }
    if let Some(i) = args.iter().position(|a| a == "--icon") {
        let name = args.get(i + 1).map(String::as_str).unwrap_or("");
        let size = args.get(i + 2).and_then(|s| s.parse().ok()).unwrap_or(48);
        let found = omarchy_theme::icons::IconLookup::current().find(name, size);
        println!("{}", found.map(|p| p.display().to_string()).unwrap_or_default());
        return;
    }
    if let Some(i) = args.iter().position(|a| a == "--icons") {
        let size = args.get(i + 1).and_then(|s| s.parse().ok()).unwrap_or(48);
        let icons = omarchy_theme::icons::IconLookup::current();
        let found: serde_json::Map<String, serde_json::Value> = args[(i + 2).min(args.len())..]
            .iter()
            .map(|n| (n.clone(), icons.find(n, size).map(|p| p.display().to_string()).into()))
            .collect();
        println!("{}", serde_json::Value::Object(found));
        return;
    }
    if has("--icon-theme") {
        println!("{}", omarchy_theme::icons::theme_name());
        return;
    }
    if has("--strings") {
        println!("{}", omarchy_theme::strings::table_json(&omarchy_theme::strings::language()));
        return;
    }

    let state = has("--state");
    let line = move |t: &omarchy_theme::Tokens, revision: u64| {
        if state { t.to_state_json(revision) } else { t.to_canonical_json_compact() }
    };
    let tokens = omarchy_theme::load();
    if !has("--watch") {
        if state || has("--compact") {
            println!("{}", line(&tokens, 0));
        } else {
            println!("{}", tokens.to_canonical_json());
        }
        return;
    }

    // One line per change, flushed as it's written (Node's readline, a shell `while read`). A
    // reader that goes away ends the process.
    emit(&line(&tokens, 0));
    let mut revision = 0;
    let _watch = omarchy_theme::watch(tokens, move |fresh| {
        revision += 1;
        emit(&line(&fresh, revision));
    });
    loop {
        std::thread::park();
    }
}

fn emit(line: &str) {
    let mut out = std::io::stdout().lock();
    if writeln!(out, "{line}").and_then(|_| out.flush()).is_err() {
        std::process::exit(0);
    }
}

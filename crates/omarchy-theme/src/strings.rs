//! The strings the kit's components show (Cancel, Search..., No matches for “%1”), in the
//! language picked under Setup > Language (`omarchy language`, `~/.config/omarchy/language`).
//!
//! The catalog is the one the Qt binding reads: `qt/qml/Omarchy/Theme/Strings.js`, which
//! `bin/harvest-qt` extracts from Omarchy's `default/i18n` for every key a kit component asks
//! for. Lookups fall back from the exact language (`pt-BR`) to its base (`pt`) to English, as
//! the Qt binding's `I18n.t` does; a plural entry gives its `other` form. Placeholders stay as
//! the catalog writes them (`%1`).

use std::collections::{BTreeMap, HashMap};
use std::sync::OnceLock;

use serde_json::Value;

const STRINGS_JS: &str = include_str!("../../../qt/qml/Omarchy/Theme/Strings.js");

type Catalog = HashMap<String, HashMap<String, String>>;

fn catalog() -> &'static Catalog {
    static CATALOG: OnceLock<Catalog> = OnceLock::new();
    CATALOG.get_or_init(|| {
        let json = STRINGS_JS.lines().find_map(|l| l.strip_prefix("var catalog = ")).unwrap_or("{}");
        let raw: HashMap<String, HashMap<String, Value>> = serde_json::from_str(json).unwrap_or_default();
        raw.into_iter()
            .map(|(lang, strings)| {
                let strings = strings
                    .into_iter()
                    .filter_map(|(k, v)| {
                        let s = match v {
                            Value::String(s) => s,
                            Value::Object(forms) => forms.get("other")?.as_str()?.to_string(),
                            _ => return None,
                        };
                        Some((k, s))
                    })
                    .collect();
                (lang, strings)
            })
            .collect()
    })
}

/// A language code as the Qt binding's `I18n.languageCode` takes it: `xx`, `xxx` or `xx-YY`,
/// anything else (or nothing) is English.
pub fn language_code(raw: &str) -> String {
    let code = raw.trim();
    let (base, region) = code.split_once('-').unwrap_or((code, ""));
    let base_ok = (2..=3).contains(&base.len()) && base.bytes().all(|b| b.is_ascii_lowercase());
    let region_ok = region.is_empty() || (region.len() == 2 && region.bytes().all(|b| b.is_ascii_alphabetic()));
    let dashed = code.contains('-');
    if base_ok && region_ok && (!dashed || !region.is_empty()) { code.to_string() } else { "en".to_string() }
}

/// The display language now: `~/.config/omarchy/language`, as a code (`en` when unset).
pub fn language() -> String {
    language_code(&std::fs::read_to_string(crate::source::language_path()).unwrap_or_default())
}

/// `key` in `language`, else its base language, else English; `None` when the catalog has no
/// such key.
pub fn lookup(language: &str, key: &str) -> Option<&'static str> {
    let c = catalog();
    let base = language.split('-').next().unwrap_or(language);
    [language, base, "en"].into_iter().find_map(|l| c.get(l)?.get(key)).map(String::as_str).filter(|s| !s.is_empty())
}

/// `key` in `language`, or `fallback` (the English the caller would show without a catalog).
pub fn t(language: &str, key: &str, fallback: &str) -> String {
    lookup(language, key).unwrap_or(fallback).to_string()
}

/// Every key the catalog has, in `language`: the exact language over its base over English, as
/// [`lookup`] falls back. Empty strings are left out (the caller's fallback shows instead).
pub fn table(language: &str) -> BTreeMap<String, String> {
    let c = catalog();
    let base = language.split('-').next().unwrap_or(language);
    let mut out = BTreeMap::new();
    for l in ["en", base, language] {
        for (k, v) in c.get(l).into_iter().flatten() {
            if !v.is_empty() {
                out.insert(k.clone(), v.clone());
            }
        }
    }
    out
}

/// [`table`] for the display language as one JSON object: `{"language":"de","strings":{...}}`
/// (`omarchy-theme --strings`, `omarchy_theme_strings_json`).
pub fn table_json(language: &str) -> String {
    serde_json::json!({ "language": language, "strings": table(language) }).to_string()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_catalog_has_the_kits_keys_in_every_language() {
        assert_eq!(lookup("en", "common.noMatchesFor"), Some("No matches for “%1”"));
        assert_eq!(lookup("en", "common.search"), Some("Search..."));
        // Every language the harvest carries has the no-matches line, with the query in it.
        for (lang, strings) in catalog() {
            let s = strings.get("common.noMatchesFor").unwrap_or_else(|| panic!("{lang} lacks common.noMatchesFor"));
            assert!(s.contains("%1"), "{lang}: {s}");
        }
    }

    #[test]
    fn lookups_fall_back_as_qt_does() {
        assert_eq!(lookup("de", "common.cancel"), Some("Abbrechen"));
        assert_eq!(lookup("de-AT", "common.cancel"), lookup("de", "common.cancel"), "region falls back to its base");
        assert_eq!(lookup("xx", "common.cancel"), Some("Cancel"), "an unknown language is English");
        assert_eq!(lookup("en", "no.such.key"), None);
        assert_eq!(t("en", "no.such.key", "Fallback"), "Fallback");
        assert_eq!(lookup("en", "multiSelect.selectedCount"), Some("%1 selected"), "a plural gives its other form");
    }

    #[test]
    fn the_table_falls_back_key_by_key() {
        let de = table("de-AT");
        assert_eq!(de.get("common.cancel").map(String::as_str), lookup("de", "common.cancel"));
        assert_eq!(de.len(), table("en").len(), "every English key is there, translated or not");
        assert!(table_json("de").starts_with("{\"language\":\"de\""));
    }

    #[test]
    fn language_codes_are_read_as_qt_reads_them() {
        assert_eq!(language_code("de\n"), "de");
        assert_eq!(language_code("pt-BR"), "pt-BR");
        assert_eq!(language_code("fil"), "fil");
        assert_eq!(language_code(""), "en");
        assert_eq!(language_code("German"), "en");
        assert_eq!(language_code("de-"), "en");
    }
}

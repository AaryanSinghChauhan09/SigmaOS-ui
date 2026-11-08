//! Freedesktop icon-theme lookup, as `spec/components/lists.md` (Avatar) describes it.
//!
//! The active Omarchy theme names its icon theme in `current/theme/icons.theme` (one line, such
//! as `Yaru-magenta`), which `omarchy-theme-set-gnome` hands to GNOME's `icon-theme` setting,
//! falling back to `Yaru-blue` when the theme has none. A lookup walks that theme, its
//! `index.theme` `Inherits` chain depth-first, then `hicolor`; in each it prefers an SVG (from a
//! `Scalable` directory first), else the PNG from the smallest directory at or above the
//! requested size, else the largest. Nothing in any theme: a loose file in a search root
//! (`/usr/share/pixmaps`).
//!
//! ```no_run
//! let icons = omarchy_theme::icons::IconLookup::current();
//! if let Some(path) = icons.find("firefox", 96) {
//!     println!("{}", path.display());
//! }
//! ```
//!
//! `OMARCHY_UI_ICON_THEME` pins the theme name and `OMARCHY_UI_ICON_DIRS` (colon-separated)
//! replaces the search roots, so reviews and tests don't depend on what is installed.

use std::collections::{HashMap, HashSet};
use std::path::{Path, PathBuf};
use std::sync::Mutex;

use crate::source;

/// The icon theme Omarchy applies when the theme names none (`omarchy-theme-set-gnome`).
pub const DEFAULT_THEME: &str = "Yaru-blue";

/// Extensions tried, in order of preference.
const EXTENSIONS: [&str; 2] = ["svg", "png"];

/// The icon theme to use: `OMARCHY_UI_ICON_THEME`, else the active theme's `icons.theme` (the
/// `OMARCHY_UI_THEME_DIR` fixture's when pinned), else [`DEFAULT_THEME`].
pub fn theme_name() -> String {
    if let Ok(v) = std::env::var("OMARCHY_UI_ICON_THEME")
        && !v.trim().is_empty()
    {
        return v.trim().to_string();
    }
    read_theme_name(&source::theme_dir().join("icons.theme")).unwrap_or_else(|| DEFAULT_THEME.to_string())
}

/// The first non-empty line of an `icons.theme` file, trimmed.
pub fn read_theme_name(path: &Path) -> Option<String> {
    let text = std::fs::read_to_string(path).ok()?;
    text.lines().map(str::trim).find(|l| !l.is_empty()).map(str::to_string)
}

/// The search roots, in order: `OMARCHY_UI_ICON_DIRS` when set, else `$XDG_DATA_HOME/icons`,
/// each `$XDG_DATA_DIRS/*/icons`, `~/.icons` and `/usr/share/pixmaps`. Each root holds theme
/// directories and, as a last resort, loose icon files.
pub fn search_dirs() -> Vec<PathBuf> {
    if let Ok(v) = std::env::var("OMARCHY_UI_ICON_DIRS")
        && !v.trim().is_empty()
    {
        return v.split(':').filter(|d| !d.is_empty()).map(PathBuf::from).collect();
    }
    let home = std::env::var_os("HOME").map(PathBuf::from).unwrap_or_else(|| PathBuf::from("/"));
    let data_home = std::env::var_os("XDG_DATA_HOME")
        .filter(|d| !d.is_empty())
        .map(PathBuf::from)
        .unwrap_or_else(|| home.join(".local/share"));
    let data_dirs = std::env::var("XDG_DATA_DIRS").ok().filter(|d| !d.is_empty()).unwrap_or_else(|| "/usr/local/share:/usr/share".into());
    let mut dirs = vec![data_home.join("icons")];
    dirs.extend(data_dirs.split(':').filter(|d| !d.is_empty()).map(|d| Path::new(d).join("icons")));
    dirs.push(home.join(".icons"));
    dirs.push(PathBuf::from("/usr/share/pixmaps"));
    dirs
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Kind {
    Fixed,
    Scalable,
    Threshold,
}

/// One `Directories` entry of an `index.theme`.
#[derive(Clone, Debug)]
struct Subdir {
    path: String,
    /// `Size × Scale`: the pixels its icons have.
    pixels: u32,
    kind: Kind,
}

/// A parsed `index.theme`: its parents and its directories, in the file's order.
#[derive(Clone, Debug, Default)]
struct Index {
    inherits: Vec<String>,
    dirs: Vec<Subdir>,
}

fn parse_index(text: &str) -> Index {
    let mut groups: HashMap<String, HashMap<String, String>> = HashMap::new();
    let mut group = String::new();
    for line in text.lines().map(str::trim) {
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        if let Some(name) = line.strip_prefix('[').and_then(|l| l.strip_suffix(']')) {
            group = name.to_string();
            continue;
        }
        if let Some((k, v)) = line.split_once('=') {
            groups.entry(group.clone()).or_default().insert(k.trim().to_string(), v.trim().to_string());
        }
    }
    let list = |v: Option<&String>| -> Vec<String> {
        v.map(|v| v.split(',').map(str::trim).filter(|s| !s.is_empty()).map(str::to_string).collect()).unwrap_or_default()
    };
    let head = groups.get("Icon Theme");
    let mut names = list(head.and_then(|h| h.get("Directories")));
    names.extend(list(head.and_then(|h| h.get("ScaledDirectories"))));
    let mut seen = HashSet::new();
    let dirs = names
        .into_iter()
        .filter(|n| seen.insert(n.clone()))
        .filter_map(|name| {
            let g = groups.get(&name)?;
            let num = |k: &str| g.get(k).and_then(|v| v.parse::<u32>().ok());
            let size = num("Size")?;
            let kind = match g.get("Type").map(String::as_str) {
                Some("Fixed") => Kind::Fixed,
                Some("Scalable") => Kind::Scalable,
                _ => Kind::Threshold,
            };
            Some(Subdir { path: name, pixels: size * num("Scale").unwrap_or(1).max(1), kind })
        })
        .collect();
    Index { inherits: list(head.and_then(|h| h.get("Inherits"))), dirs }
}

/// An icon theme ready to search: its inheritance chain resolved once, and every answer cached.
/// Build one per theme (rebuild it when [`theme_name`] changes, as a theme switch does).
pub struct IconLookup {
    theme: String,
    roots: Vec<PathBuf>,
    chain: Vec<(String, Index)>,
    found: Mutex<HashMap<(String, u32), Option<PathBuf>>>,
}

impl IconLookup {
    /// `theme` searched under `roots` (see [`search_dirs`]).
    pub fn new(theme: impl Into<String>, roots: Vec<PathBuf>) -> IconLookup {
        let theme = theme.into();
        let mut chain = Vec::new();
        let mut seen = HashSet::new();
        Self::visit(&theme, &roots, &mut seen, &mut chain);
        Self::visit("hicolor", &roots, &mut seen, &mut chain);
        IconLookup { theme, roots, chain, found: Mutex::new(HashMap::new()) }
    }

    /// The active Omarchy theme's icon theme ([`theme_name`]) under the usual roots
    /// ([`search_dirs`]).
    pub fn current() -> IconLookup {
        IconLookup::new(theme_name(), search_dirs())
    }

    /// A theme, then its parents depth-first. Themes that aren't installed are skipped.
    fn visit(name: &str, roots: &[PathBuf], seen: &mut HashSet<String>, chain: &mut Vec<(String, Index)>) {
        if !seen.insert(name.to_string()) {
            return;
        }
        let Some(text) = roots.iter().find_map(|r| std::fs::read_to_string(r.join(name).join("index.theme")).ok()) else {
            return;
        };
        let index = parse_index(&text);
        let parents = index.inherits.clone();
        chain.push((name.to_string(), index));
        for p in &parents {
            Self::visit(p, roots, seen, chain);
        }
    }

    /// The theme asked for.
    pub fn theme(&self) -> &str {
        &self.theme
    }

    /// The themes searched, in order: the theme, its `Inherits` chain, `hicolor`.
    pub fn themes(&self) -> Vec<&str> {
        self.chain.iter().map(|(n, _)| n.as_str()).collect()
    }

    /// The file for icon `name` at `size` device pixels, or `None` when no theme and no search
    /// root has it. An absolute path to an existing file is returned as is.
    pub fn find(&self, name: &str, size: u32) -> Option<PathBuf> {
        if name.is_empty() {
            return None;
        }
        let key = (name.to_string(), size);
        if let Some(hit) = self.found.lock().unwrap().get(&key) {
            return hit.clone();
        }
        let hit = self.search(name, size);
        self.found.lock().unwrap().insert(key, hit.clone());
        hit
    }

    fn search(&self, name: &str, size: u32) -> Option<PathBuf> {
        let path = Path::new(name);
        if path.is_absolute() {
            return path.is_file().then(|| path.to_path_buf());
        }
        if name.contains('/') {
            return None;
        }
        for (theme, index) in &self.chain {
            if let Some(p) = self.in_theme(theme, index, name, size) {
                return Some(p);
            }
        }
        // Loose files in a root: /usr/share/pixmaps.
        self.roots.iter().find_map(|r| EXTENSIONS.iter().map(|e| r.join(format!("{name}.{e}"))).find(|p| p.is_file()))
    }

    fn in_theme(&self, theme: &str, index: &Index, name: &str, size: u32) -> Option<PathBuf> {
        // Every directory's file, from the first root that has one.
        let mut svgs: Vec<(&Subdir, PathBuf)> = Vec::new();
        let mut pngs: Vec<(&Subdir, PathBuf)> = Vec::new();
        for dir in &index.dirs {
            for ext in EXTENSIONS {
                let file = format!("{name}.{ext}");
                if let Some(p) = self.roots.iter().map(|r| r.join(theme).join(&dir.path).join(&file)).find(|p| p.is_file()) {
                    if ext == "svg" { svgs.push((dir, p)) } else { pngs.push((dir, p)) }
                }
            }
        }
        if let Some((_, p)) = svgs.iter().find(|(d, _)| d.kind == Kind::Scalable) {
            return Some(p.clone());
        }
        nearest(&svgs, size).or_else(|| nearest(&pngs, size))
    }
}

/// The smallest directory at or above `size`, else the largest; the first in the theme's order
/// on a tie.
fn nearest(found: &[(&Subdir, PathBuf)], size: u32) -> Option<PathBuf> {
    let above = found.iter().filter(|(d, _)| d.pixels >= size).min_by_key(|(d, _)| d.pixels);
    let pick = above.or_else(|| found.iter().rev().max_by_key(|(d, _)| d.pixels))?;
    Some(pick.1.clone())
}

#[cfg(test)]
mod tests {
    use super::*;

    // fixtures/icons: Fixture-Accent (Inherits Fixture-Base, which names a missing theme and
    // hicolor) under system/, with one more icon of it under local/; loose files in pixmaps/.
    // The icons carry real app names so the live galleries show real icons for the same
    // specimens: utilities-terminal is PNGs at 16, 48 and 96 in Fixture-Accent (and an SVG in
    // its parent), org.gnome.Nautilus an SVG beside a 32px PNG, firefox only in Fixture-Base,
    // chromium only in hicolor, btop in local/ and in the parent, steam only in pixmaps.
    fn fixture() -> (PathBuf, IconLookup) {
        let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../fixtures/icons").canonicalize().unwrap();
        let roots = ["local", "system", "pixmaps"].map(|d| root.join(d)).to_vec();
        (root, IconLookup::new("Fixture-Accent", roots))
    }

    fn rel(root: &Path, p: Option<PathBuf>) -> Option<String> {
        p.map(|p| p.strip_prefix(root).unwrap().to_string_lossy().into_owned())
    }

    #[test]
    fn follows_inherits_then_hicolor() {
        let (_, icons) = fixture();
        // Fixture-Base names a theme that isn't installed (skipped) and hicolor (not repeated).
        assert_eq!(icons.themes(), ["Fixture-Accent", "Fixture-Base", "hicolor"]);
        let (root, _) = fixture();
        assert_eq!(rel(&root, icons.find("firefox", 48)).as_deref(), Some("system/Fixture-Base/scalable/apps/firefox.svg"));
        assert_eq!(rel(&root, icons.find("chromium", 48)).as_deref(), Some("system/hicolor/scalable/apps/chromium.svg"));
    }

    #[test]
    fn hicolor_without_the_theme() {
        let (root, _) = fixture();
        let icons = IconLookup::new("Not-Installed", ["system", "pixmaps"].map(|d| root.join(d)).to_vec());
        assert_eq!(icons.themes(), ["hicolor"]);
        assert_eq!(rel(&root, icons.find("chromium", 16)).as_deref(), Some("system/hicolor/scalable/apps/chromium.svg"));
    }

    #[test]
    fn prefers_scalable_svg() {
        let (root, icons) = fixture();
        // A 32px PNG sits beside it, nearer the request.
        assert_eq!(rel(&root, icons.find("org.gnome.Nautilus", 32)).as_deref(), Some("system/Fixture-Accent/scalable/apps/org.gnome.Nautilus.svg"));
    }

    #[test]
    fn picks_the_nearest_size_at_or_above() {
        let (root, icons) = fixture();
        let at = |size| rel(&root, icons.find("utilities-terminal", size));
        assert_eq!(at(8).as_deref(), Some("system/Fixture-Accent/16x16/apps/utilities-terminal.png"));
        assert_eq!(at(16).as_deref(), Some("system/Fixture-Accent/16x16/apps/utilities-terminal.png"));
        assert_eq!(at(17).as_deref(), Some("system/Fixture-Accent/48x48/apps/utilities-terminal.png"));
        assert_eq!(at(56).as_deref(), Some("system/Fixture-Accent/96x96/apps/utilities-terminal.png"));
        // Nothing big enough: the largest. The parent's SVG never wins over the theme's own PNGs.
        assert_eq!(at(200).as_deref(), Some("system/Fixture-Accent/96x96/apps/utilities-terminal.png"));
    }

    #[test]
    fn searches_a_theme_across_roots_in_order() {
        let (root, icons) = fixture();
        // Fixture-Accent's index.theme is under system/, but local/ adds an icon to it, which
        // wins over the parent theme's copy.
        assert_eq!(rel(&root, icons.find("btop", 48)).as_deref(), Some("local/Fixture-Accent/48x48/apps/btop.png"));
    }

    #[test]
    fn falls_back_to_pixmaps() {
        let (root, icons) = fixture();
        assert_eq!(rel(&root, icons.find("steam", 48)).as_deref(), Some("pixmaps/steam.png"));
    }

    #[test]
    fn missing_icons_are_none() {
        let (root, icons) = fixture();
        assert_eq!(icons.find("no-such-app", 48), None);
        assert_eq!(icons.find("", 48), None);
        assert_eq!(icons.find("../pixmaps/steam", 48), None);
        // Asked again: answered from the cache, the same.
        assert_eq!(icons.find("no-such-app", 48), None);
        let abs = root.join("pixmaps/steam.png");
        assert_eq!(icons.find(abs.to_str().unwrap(), 48), Some(abs));
    }

    #[test]
    fn reads_the_theme_name_line() {
        let dir = std::env::temp_dir().join(format!("omarchy-icons-{}", std::process::id()));
        std::fs::create_dir_all(&dir).unwrap();
        let f = dir.join("icons.theme");
        std::fs::write(&f, "\n  Yaru-magenta  \n").unwrap();
        assert_eq!(read_theme_name(&f).as_deref(), Some("Yaru-magenta"));
        std::fs::write(&f, "\n").unwrap();
        assert_eq!(read_theme_name(&f), None);
        assert_eq!(read_theme_name(&dir.join("absent")), None);
        std::fs::remove_dir_all(&dir).unwrap();
    }
}

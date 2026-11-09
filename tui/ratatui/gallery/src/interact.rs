//! What the keys, clicks and the pointer do in the interactive gallery: each section's focusable
//! controls (Tab walks them), the keys the focused one takes, and what a click on a keyed part
//! does. The behavior tests (main.rs) drive these the way the GUI galleries' tests drive theirs.

use omarchy_ratatui::crossterm::event::{KeyCode, KeyEvent, KeyModifiers};
use omarchy_ratatui::ratatui::layout::Rect;
use omarchy_ratatui::ui::*;

use crate::sections::{CONTEXT_ITEMS, MENU_ITEMS, RAIL, TABLE_ROWS, search_items, sidebar_sections, tree_nodes};
use crate::state::{NOTIFY, State, THEMES};

/// The focusable controls of a section, in Tab order.
pub fn focusables(section: &str) -> &'static [&'static str] {
    match section {
        "buttons" => &["plain", "bordered", "icon", "power", "align"],
        "toggles" => &["switch-0", "switch-1", "wifi"],
        "inputs" => &["search", "name", "gaps", "theme"],
        "slider" => &["level", "stepped"],
        "surfaces" => &["network", "confirm", "add-layer", "night-light"],
        "lists" => &["rows"],
        "tiles" => &["tile"],
        "messages" => &["scan"],
        "menus" => &["menu", "context"],
        "navigation" => &["tabs", "rail"],
        "forms" => &["night", "check-0", "check-1", "check-4", "radio", "password"],
        "pickers" => &["notify", "swatch", "discard"],
        "data" => &["table", "tree"],
        "layout" => &["sidebar", "handle", "cal"],
        "timeline" => &["timeline", "scrubber", "drag"],
        "color" => &["picker"],
        "composed" => &["searchlist", "networks"],
        _ => &[],
    }
}

fn activate(k: &KeyEvent) -> bool {
    matches!(k.code, KeyCode::Enter | KeyCode::Char(' '))
}

fn edit(text: &mut String, k: &KeyEvent) -> bool {
    match k.code {
        KeyCode::Char(c) if !k.modifiers.intersects(KeyModifiers::CONTROL | KeyModifiers::ALT) => text.push(c),
        KeyCode::Backspace => {
            text.pop();
        }
        _ => return false,
    }
    true
}

fn flip(b: &mut bool) -> bool {
    *b = !*b;
    true
}

fn toggle(values: &mut Vec<String>, v: &str) {
    if let Some(i) = values.iter().position(|x| x == v) {
        values.remove(i);
    } else {
        values.push(v.to_string());
    }
}

fn run_menu(s: &mut State, i: usize) {
    match i {
        4 => s.sidebar_shown = !s.sidebar_shown,
        5 => s.word_wrap = !s.word_wrap,
        _ => s.ran = Some(MENU_ITEMS[i]),
    }
}

fn run_context(s: &mut State, i: usize) {
    match i {
        3 => s.sort_by_name = true,
        4 => s.sort_by_name = false,
        _ => s.ran = Some(CONTEXT_ITEMS[i]),
    }
}

fn reveal_row(s: &mut State) {
    s.rows_offset = ScrollArea::ensure_visible(s.rows_offset, s.rows.index() as u16, 1, 7);
}

/// The focused control takes a key; true when it did.
pub fn key(s: &mut State, k: &KeyEvent) -> bool {
    let Some(f) = s.focus else { return false };
    match f {
        "plain" | "bordered" | "icon" | "power" if activate(k) => {
            s.ran = Some(f);
            true
        }
        "align" => ButtonGroup::handle_key(&mut s.align, 3, k),
        "switch-0" | "switch-1" if activate(k) => flip(&mut s.switches[(f == "switch-1") as usize]),
        "wifi" if activate(k) => flip(&mut s.wifi),
        "network" if activate(k) => flip(&mut s.network_on),
        "night-light" if activate(k) => flip(&mut s.night_light),
        "night" if activate(k) => flip(&mut s.night_toggle),
        "search" => edit(&mut s.search, k),
        "name" => edit(&mut s.name, k),
        "gaps" => NumberField::handle_key(&mut s.gaps, 0, 40, k),
        "theme" => {
            if !s.dropdown_open {
                if activate(k) || k.code == KeyCode::Down {
                    s.dropdown_open = true;
                    s.dropdown_row = s.theme_choice;
                    return true;
                }
                return false;
            }
            match k.code {
                KeyCode::Down | KeyCode::Char('j') => s.dropdown_row = (s.dropdown_row + 1).min(THEMES.len() - 1),
                KeyCode::Up | KeyCode::Char('k') => s.dropdown_row = s.dropdown_row.saturating_sub(1),
                KeyCode::Enter | KeyCode::Char(' ') => {
                    s.theme_choice = s.dropdown_row;
                    s.dropdown_open = false;
                }
                KeyCode::Esc => s.dropdown_open = false,
                _ => return false,
            }
            true
        }
        "level" => Slider::handle_key(&mut s.level, 0, k),
        "stepped" => Slider::handle_key(&mut s.stepped, 5, k),
        "confirm" => Dialog::handle_key(&mut s.confirm_selected, k),
        "add-layer" if activate(k) => {
            s.ran = Some("add-layer");
            true
        }
        "rows" => match s.rows.apply_key(k) {
            CursorKey::Moved => {
                reveal_row(s);
                true
            }
            CursorKey::Activate => {
                if s.rows.index() != 3 {
                    s.current_row = s.rows.index();
                }
                true
            }
            CursorKey::Delete => {
                s.ran = Some("forget");
                true
            }
            _ => false,
        },
        "tile" => {
            let i = s.tile_cursor.unwrap_or(0);
            s.tile_cursor = Some(match k.code {
                KeyCode::Right | KeyCode::Char('l') if i % 4 < 3 => (i + 1).min(7),
                KeyCode::Left | KeyCode::Char('h') if !i.is_multiple_of(4) => i - 1,
                KeyCode::Down | KeyCode::Char('j') => (i + 4).min(7),
                KeyCode::Up | KeyCode::Char('k') => i.saturating_sub(4),
                _ => return false,
            });
            true
        }
        "scan" if activate(k) => {
            s.ran = Some("scan");
            true
        }
        "menu" => {
            let enabled = [true, true, true, false, true, true, true];
            if activate(k) {
                run_menu(s, s.menu_cursor);
                return true;
            }
            Menu::handle_key(&mut s.menu_cursor, &enabled, k)
        }
        "context" => {
            if activate(k) {
                run_context(s, s.context_cursor);
                return true;
            }
            Menu::handle_key(&mut s.context_cursor, &[true; 6], k)
        }
        "tabs" => {
            let r = Tabs::handle_key(&mut s.tab, 4, k);
            s.tab_cursor = Some(s.tab);
            r
        }
        "rail" => IconRail::handle_key(&mut s.panel, RAIL.len(), k),
        "check-0" | "check-1" | "check-4" if activate(k) => {
            let i = f[6..].parse::<usize>().unwrap();
            flip(&mut s.checks[i])
        }
        "radio" => RadioGroup::handle_key(&mut s.position, &["top", "bottom", "hidden"], k),
        "password" => match k.code {
            KeyCode::F(2) => flip(&mut s.revealed),
            _ => edit(&mut s.password, k),
        },
        "notify" => {
            let opts: Vec<(&str, &str)> = NOTIFY.to_vec();
            let (handled, toggled) = MultiSelect::handle_key(&mut s.notify_state, &opts, k);
            if let Some(v) = toggled {
                toggle(&mut s.notify, &v);
            }
            handled
        }
        "swatch" => {
            let i = s.swatch_cursor.unwrap_or(s.swatch);
            match k.code {
                KeyCode::Left | KeyCode::Char('h') => s.swatch_cursor = Some(i.saturating_sub(1)),
                KeyCode::Right | KeyCode::Char('l') => s.swatch_cursor = Some((i + 1).min(7)),
                KeyCode::Enter | KeyCode::Char(' ') => s.swatch = i,
                _ => return false,
            }
            true
        }
        "discard" => Dialog::handle_key(&mut s.discard_selected, k),
        "table" => {
            if activate(k) {
                s.table_selected = s.table_cursor;
                return true;
            }
            Table::handle_key(&mut s.table_cursor, TABLE_ROWS.len(), k)
        }
        "tree" => tree_key(s, k),
        "sidebar" => {
            let ids: Vec<String> = sidebar_sections().into_iter().flat_map(|s| s.items.into_iter().map(|i| i.id)).collect();
            let i = ids.iter().position(|x| *x == s.sidebar_selected).unwrap_or(0);
            let j = match k.code {
                KeyCode::Down | KeyCode::Char('j') => (i + 1).min(ids.len() - 1),
                KeyCode::Up | KeyCode::Char('k') => i.saturating_sub(1),
                _ => return false,
            };
            s.sidebar_selected = ids[j].clone();
            true
        }
        "handle" => {
            match k.code {
                KeyCode::Left | KeyCode::Char('h') => s.sidebar_width = s.sidebar_width.saturating_sub(1).max(15),
                KeyCode::Right | KeyCode::Char('l') => s.sidebar_width = (s.sidebar_width + 1).min(40),
                _ => return false,
            }
            true
        }
        "cal" => {
            let mut c = s.day_cursor.unwrap_or(s.day_selected);
            if activate(k) {
                s.day_selected = c;
                return true;
            }
            let (mut y, mut m) = s.month;
            let r = Calendar::handle_key(&mut c, &mut y, &mut m, k);
            if r {
                s.day_cursor = Some(c);
                s.month = (y, m);
            }
            r
        }
        "timeline" => Timeline::handle_key(&mut s.playhead, 30.0, 90, k),
        "scrubber" => Scrubber::handle_key(&mut s.scrub, 130.0, k),
        "drag" => {
            match k.code {
                KeyCode::Down | KeyCode::Char('j') => s.drag_cursor = (s.drag_cursor + 1).min(3),
                KeyCode::Up | KeyCode::Char('k') => s.drag_cursor = s.drag_cursor.saturating_sub(1),
                _ => return false,
            }
            true
        }
        "picker" => ColorPicker::handle_key(&mut s.color, k),
        "searchlist" => {
            let r = SearchList::handle_key(&mut s.query, &mut s.search_cursor, k);
            match r {
                SearchKey::Query => {
                    let n = search_items().iter().filter(|i| SearchList::matches(&i.title, &s.query)).count();
                    s.search_cursor.set_count("", n);
                    true
                }
                SearchKey::Activate(i) => {
                    s.activated = Some(i);
                    true
                }
                SearchKey::Ignored => false,
                _ => true,
            }
        }
        "networks" => matches!(s.wifi_rows.apply_key(k), CursorKey::Moved | CursorKey::Activate),
        _ => false,
    }
}

fn tree_key(s: &mut State, k: &KeyEvent) -> bool {
    let nodes = tree_nodes();
    let rows = Tree::visible(&nodes, &s.tree_expanded);
    let i = rows.iter().position(|(_, n)| n.id == s.tree_cursor).unwrap_or(0);
    let node = rows[i].1;
    let open = s.tree_expanded.contains(&node.id);
    match k.code {
        KeyCode::Down | KeyCode::Char('j') => s.tree_cursor = rows[(i + 1).min(rows.len() - 1)].1.id.clone(),
        KeyCode::Up | KeyCode::Char('k') => s.tree_cursor = rows[i.saturating_sub(1)].1.id.clone(),
        KeyCode::Right | KeyCode::Char('l') if !node.children.is_empty() && !open => s.tree_expanded.push(node.id.clone()),
        KeyCode::Left | KeyCode::Char('h') if open => s.tree_expanded.retain(|x| *x != node.id),
        KeyCode::Enter | KeyCode::Char(' ') => s.tree_selected = node.id.clone(),
        _ => return false,
    }
    true
}

/// The focusable a clicked key belongs to, if any.
pub fn focus_of(section: &str, key: &str) -> Option<&'static str> {
    let base = match key {
        k if k.starts_with("align-") => "align",
        k if k.starts_with("row-") => "rows",
        k if k.starts_with("tile-") => "tile",
        k if k.starts_with("menu-") => "menu",
        k if k.starts_with("context-") => "context",
        k if k.starts_with("tab-") => "tabs",
        k if k.starts_with("rail-") => "rail",
        k if k.starts_with("radio-") => "radio",
        k if k.starts_with("notify") => "notify",
        k if k.starts_with("swatch-") && section == "pickers" => "swatch",
        k if k.starts_with("discard-") => "discard",
        k if k.starts_with("confirm-") => "confirm",
        k if k.starts_with("table-") => "table",
        k if k.starts_with("tree-") => "tree",
        k if k.starts_with("side-") => "sidebar",
        k if k.starts_with("split-handle") => "handle",
        k if k.starts_with("cal-") => "cal",
        k if k.starts_with("clip-") || k == "timeline-ruler" => "timeline",
        k if k.starts_with("drag-") => "drag",
        k if k.starts_with("search-") => "searchlist",
        k if k.starts_with("wifi-") => "networks",
        k if k.starts_with("gaps") => "gaps",
        k if k.starts_with("swatch-") || k == "picker-square" => "picker",
        k => k,
    };
    focusables(section).iter().copied().find(|f| *f == base)
}

/// A click on a keyed part, at a cell; `rect` is where that part was drawn. True when it did
/// something.
pub fn click(s: &mut State, t: &omarchy_ratatui::Theme, section: &str, key: &str, col: u16, rect: Rect) -> bool {
    let n = |prefix: &str| key.strip_prefix(prefix).and_then(|r| r.parse::<usize>().ok());
    match key {
        "plain" | "bordered" | "icon" | "power" | "add-layer" | "empty-action" => s.ran = Some(if key == "empty-action" { "scan" } else { "button" }),
        "wifi" if section == "toggles" => s.wifi = !s.wifi,
        "bluetooth" => s.bluetooth = !s.bluetooth,
        "network" => s.network_on = !s.network_on,
        "night-light" => s.night_light = !s.night_light,
        "night" => s.night_toggle = !s.night_toggle,
        "gaps-dec" => s.gaps = (s.gaps - 1).max(0),
        "gaps-inc" => s.gaps = (s.gaps + 1).min(40),
        "theme" => {
            s.dropdown_open = !s.dropdown_open;
            s.dropdown_row = s.theme_choice;
        }
        "level" => s.level = Slider::value_at(rect, col),
        "stepped" => s.stepped = (Slider::value_at(rect, col) * 4.0).round() / 4.0,
        "confirm-cancel" => s.confirm_selected = 0,
        "confirm-accept" => s.confirm_selected = 1,
        "discard-cancel" => s.discard_selected = 0,
        "discard-accept" => s.discard_selected = 1,
        "password" => {}
        "notify" => s.notify_state.open = !s.notify_state.open,
        "modules" => {}
        "timeline-ruler" => s.playhead = Timeline::time_at(col - rect.x, 30.0, rect.width),
        "scrubber" => s.scrub = (col.saturating_sub(rect.x) as f64 / rect.width.saturating_sub(1).max(1) as f64 * 130.0).clamp(0.0, 130.0),
        "cal-prev" | "cal-next" => {
            let d = Date::new(s.month.0, s.month.1, 1).add_months(if key == "cal-prev" { -1 } else { 1 });
            s.month = (d.year, d.month);
        }
        "picker-square" => {}
        k if k.starts_with("theme-list-") => {
            s.theme_choice = n("theme-list-").unwrap_or(0);
            s.dropdown_open = false;
        }
        k if k.starts_with("align-") => s.align = n("align-").unwrap_or(0),
        k if k.starts_with("switch-") => {
            let i = n("switch-").unwrap_or(0);
            s.switches[i] = !s.switches[i];
        }
        k if k.starts_with("row-") && k.ends_with("-action") => s.ran = Some("forget"),
        k if k.starts_with("row-") => {
            let i = n("row-").unwrap_or(0);
            s.rows.set("", i);
            s.current_row = i;
        }
        k if k.starts_with("tile-") => s.tile_cursor = n("tile-"),
        k if k.starts_with("menu-") => {
            let i = ["menu-new", "menu-open", "menu-save", "menu-duplicate", "menu-sidebar", "menu-wrap", "menu-export"].iter().position(|x| *x == k);
            if let Some(i) = i {
                s.menu_cursor = i;
                run_menu(s, i);
            }
        }
        k if k.starts_with("context-") => {
            let i = ["context-cut", "context-copy", "context-paste", "context-name", "context-date", "context-delete"].iter().position(|x| *x == k);
            if let Some(i) = i {
                s.context_cursor = i;
                run_context(s, i);
            }
        }
        k if k.starts_with("tab-") => {
            s.tab = n("tab-").unwrap_or(0);
            s.tab_cursor = Some(s.tab);
        }
        k if k.starts_with("rail-") => s.panel = n("rail-").unwrap_or(0),
        k if k.starts_with("check-") => {
            let i = n("check-").unwrap_or(0);
            if i != 3 {
                s.checks[i] = !s.checks[i];
            }
        }
        k if k.starts_with("radio-") => s.position = k["radio-".len()..].to_string(),
        k if k.starts_with("notify-") => toggle(&mut s.notify, &k["notify-".len()..]),
        k if k.starts_with("modules-") => toggle(&mut s.modules, &k["modules-".len()..]),
        k if k.starts_with("swatch-") && section == "pickers" => {
            s.swatch = n("swatch-").unwrap_or(0);
            s.swatch_cursor = Some(s.swatch);
        }
        k if k.starts_with("swatch-") => {
            // The color picker's palette: pick the swatch, keep the alpha.
            let names = ["red", "orange", "yellow", "green", "cyan", "blue", "magenta", "foreground"];
            let p = &t.shown().palette;
            let colors: Vec<_> = names.iter().filter_map(|n| if *n == "foreground" { Some(p.foreground) } else { p.named.get(*n).copied() }).collect();
            if let Some(c) = n("swatch-").and_then(|i| colors.get(i))
                && let Some(h) = Hsva::from_hex(&c.hex()[..7], s.color.h)
            {
                s.color = Hsva { a: s.color.a, ..h };
            }
        }
        k if k.starts_with("table-col-") => {}
        k if k.starts_with("table-") => {
            let r = n("table-").unwrap_or(0);
            s.table_cursor = r;
            s.table_selected = r;
        }
        k if k.starts_with("tree-") && k.ends_with("-toggle") => {
            let id = &k["tree-".len()..k.len() - "-toggle".len()];
            if s.tree_expanded.iter().any(|x| x == id) {
                s.tree_expanded.retain(|x| x != id);
            } else {
                s.tree_expanded.push(id.to_string());
            }
        }
        k if k.starts_with("tree-") => {
            s.tree_selected = k["tree-".len()..].to_string();
            s.tree_cursor = s.tree_selected.clone();
        }
        k if k.starts_with("side-") => s.sidebar_selected = k["side-".len()..].to_string(),
        k if k.starts_with("cal-") => {
            let iso = &k["cal-".len()..];
            let p: Vec<&str> = iso.split('-').collect();
            if let [y, m, d] = p[..] {
                s.day_selected = Date::new(y.parse().unwrap_or(2026), m.parse().unwrap_or(1), d.parse().unwrap_or(1));
                s.month = (s.day_selected.year, s.day_selected.month);
            }
        }
        k if k.starts_with("clip-") => {
            let p: Vec<usize> = k["clip-".len()..].split('-').filter_map(|x| x.parse().ok()).collect();
            if let [a, b] = p[..] {
                s.clip = Some((a, b));
            }
        }
        k if k.starts_with("drag-") => s.drag_cursor = n("drag-").unwrap_or(0),
        k if k.starts_with("search-row-") => s.activated = n("search-row-"),
        k if k.starts_with("wifi-") => s.wifi_rows.set("", n("wifi-").unwrap_or(0)),
        _ => return false,
    }
    true
}

/// The pointer over a keyed part: list cursors follow it (through CursorModel's gate).
pub fn hover(s: &mut State, key: &str, col: u16, row: u16) -> bool {
    let n = |prefix: &str| key.strip_prefix(prefix).and_then(|r| r.parse::<usize>().ok());
    if let Some(i) = n("row-").filter(|i| *i != 3) {
        return s.rows.pointer_moved(col, row, "", i);
    }
    if let Some(i) = n("wifi-") {
        return s.wifi_rows.pointer_moved(col, row, "", i);
    }
    if let Some(i) = n("search-row-") {
        return s.search_cursor.pointer_moved(col, row, "", i);
    }
    if let Some(i) = n("tile-") {
        let changed = s.tile_cursor != Some(i);
        s.tile_cursor = Some(i);
        return changed;
    }
    false
}

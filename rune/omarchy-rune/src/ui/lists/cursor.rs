//! CursorModel: the keyboard cursor every shell panel re-implements (`focusSection`,
//! `selectedIndex`, `cursorActive`, `moveCursor`, j/k), as one plain struct.

use rune::{Key, KeyEvent};

/// One named run of rows (or tiles) the cursor walks: "header", "connected", "known", ...
#[derive(Clone, Debug, PartialEq)]
pub struct CursorSection {
    pub name: String,
    pub count: usize,
}

/// What a key did to a [`CursorModel`], for the app to act on.
#[derive(Clone, Debug, PartialEq)]
pub enum CursorKey {
    /// The cursor moved (or woke up): reveal the row it's on (`Tx::scroll_into_view`).
    Moved,
    /// Return, Enter or Space on the row under the cursor.
    Activate,
    /// Escape.
    Close,
    /// Delete or x: forget or remove the row under the cursor.
    Delete,
    /// A printable key no binding took: type-ahead filter text.
    Text(String),
    /// Not a cursor key; let it through.
    Ignored,
}

/// The keyboard cursor over a list or grid split into sections.
///
/// - `j`/`k` and Down/Up move a row, crossing into the next or previous section at the ends;
///   `h`/`l` and Left/Right move a column in a grid (`columns > 1`), else do nothing.
/// - Page Up/Down move `page` rows within the section; Home/End go to the very first or last row.
/// - The first key after the cursor was hidden (`active == false`) only shows it, as the shell's
///   panels do, so the user sees where they are before moving.
/// - The pointer moves the cursor through [`CursorModel::pointer_moved`], which ignores hover
///   from rows scrolling under a still pointer (the shell's PointerMoveGate).
///
/// ```ignore
/// row.cursor(s.cursor.is_at("wifi", i))
///    .on_hover(move |s: &mut App, x, y| { s.cursor.pointer_moved(x, y, "wifi", i); })
/// list.on_key(|s: &mut App, k, tx| match s.cursor.apply_key(k) {
///     CursorKey::Moved => { tx.scroll_into_view(s.row_key()); true }
///     CursorKey::Activate => { s.connect(); true }
///     CursorKey::Ignored => false,
///     _ => true,
/// })
/// ```
#[derive(Clone, Debug)]
pub struct CursorModel {
    sections: Vec<CursorSection>,
    section: usize,
    index: usize,
    /// Whether the cursor shows. Hidden until the first key or pointer movement.
    pub active: bool,
    /// Tiles per line, for a grid; 1 for a list.
    pub columns: usize,
    /// Rows Page Up/Down move.
    pub page: usize,
    gate: Option<(f32, f32)>,
    initial_sample: bool,
}

/// How far the pointer must travel, in points, to count as moving (PointerMoveGate's threshold).
const GATE: f32 = 1.0;

impl CursorModel {
    /// A cursor over `sections` (name, row count), at the first row, hidden.
    pub fn new<N: Into<String>>(sections: impl IntoIterator<Item = (N, usize)>) -> CursorModel {
        let sections = sections.into_iter().map(|(n, c)| CursorSection { name: n.into(), count: c }).collect();
        let mut m = CursorModel { sections, section: 0, index: 0, active: false, columns: 1, page: 5, gate: None, initial_sample: false };
        m.settle();
        m
    }

    /// A cursor over one unnamed list of `count` rows.
    pub fn list(count: usize) -> CursorModel {
        CursorModel::new([("", count)])
    }

    pub fn columns(mut self, n: usize) -> Self {
        self.columns = n.max(1);
        self
    }

    /// The section the cursor is in.
    pub fn section(&self) -> &str {
        self.sections.get(self.section).map_or("", |s| s.name.as_str())
    }

    /// The row within the section.
    pub fn index(&self) -> usize {
        self.index
    }

    /// Whether the cursor shows on row `index` of `section`: the row's `.cursor(..)` flag.
    pub fn is_at(&self, section: &str, index: usize) -> bool {
        self.active && self.section() == section && self.index == index
    }

    /// The cursor's row when it shows in `section`.
    pub fn cursor_in(&self, section: &str) -> Option<usize> {
        (self.active && self.section() == section).then_some(self.index)
    }

    /// New row counts (the list changed). The cursor stays on its section and clamps its row.
    pub fn set_count(&mut self, section: &str, count: usize) {
        if let Some(s) = self.sections.iter_mut().find(|s| s.name == section) {
            s.count = count;
        }
        self.settle();
    }

    /// Put the cursor on a row and show it.
    pub fn set(&mut self, section: &str, index: usize) {
        if let Some(i) = self.sections.iter().position(|s| s.name == section) {
            self.section = i;
            self.index = index;
            self.active = true;
            self.settle();
        }
    }

    /// Hide the cursor and forget the pointer (a panel closing or reopening).
    pub fn reset(&mut self) {
        self.active = false;
        self.gate = None;
        self.initial_sample = false;
    }

    /// Move `dx` columns and `dy` rows. Rows cross into the neighbouring non-empty section at
    /// the ends and stop at the first and last row; columns stay within the row line.
    pub fn move_by(&mut self, dx: i32, dy: i32) -> bool {
        if !self.wake() {
            return true;
        }
        let before = (self.section, self.index);
        if dx != 0 && self.columns > 1 {
            let col = self.index % self.columns;
            let count = self.count();
            let to = col as i64 + dx as i64;
            if to >= 0 && (to as usize) < self.columns {
                let next = self.index as i64 + dx as i64;
                if next >= 0 && (next as usize) < count {
                    self.index = next as usize;
                }
            }
        }
        if dy != 0 {
            let step = self.columns as i64;
            for _ in 0..dy.unsigned_abs() {
                let next = self.index as i64 + step * dy.signum() as i64;
                if next >= 0 && (next as usize) < self.count() {
                    self.index = next as usize;
                } else if dy > 0 {
                    if let Some(s) = self.neighbour(1) {
                        self.section = s;
                        self.index = (self.index % self.columns).min(self.count().saturating_sub(1));
                    } else if self.columns > 1 && self.index / self.columns < (self.count().saturating_sub(1)) / self.columns {
                        // A short last line: land on its last tile.
                        self.index = self.count() - 1;
                    }
                } else if let Some(s) = self.neighbour(-1) {
                    self.section = s;
                    let count = self.count();
                    let last_line = (count.saturating_sub(1) / self.columns) * self.columns;
                    self.index = (last_line + self.index % self.columns).min(count.saturating_sub(1));
                }
            }
        }
        (self.section, self.index) != before
    }

    /// Jump to the first row of the next (`delta > 0`) or previous non-empty section.
    pub fn move_section(&mut self, delta: i32) -> bool {
        if !self.wake() {
            return true;
        }
        match self.neighbour(delta.signum()) {
            Some(s) => {
                self.section = s;
                self.index = 0;
                true
            }
            None => false,
        }
    }

    /// Page Up (`-1`) or Page Down (`1`): `page` rows within the section, stopping at its ends.
    pub fn page_by(&mut self, delta: i32) -> bool {
        if !self.wake() {
            return true;
        }
        let before = self.index;
        let last = self.count().saturating_sub(1) as i64;
        let step = (self.page.max(1) * self.columns) as i64;
        self.index = (self.index as i64 + step * delta.signum() as i64).clamp(0, last) as usize;
        self.index != before
    }

    /// The very first row of the first non-empty section.
    pub fn home(&mut self) -> bool {
        self.active = true;
        let before = (self.section, self.index);
        if let Some(s) = self.sections.iter().position(|s| s.count > 0) {
            self.section = s;
            self.index = 0;
        }
        (self.section, self.index) != before
    }

    /// The very last row of the last non-empty section.
    pub fn end(&mut self) -> bool {
        self.active = true;
        let before = (self.section, self.index);
        if let Some(s) = self.sections.iter().rposition(|s| s.count > 0) {
            self.section = s;
            self.index = self.sections[s].count - 1;
        }
        (self.section, self.index) != before
    }

    /// Apply a key the shell's way (PanelKeyCatcher's bindings plus paging and type-ahead).
    pub fn apply_key(&mut self, key: &KeyEvent) -> CursorKey {
        let m = key.mods;
        let plain = !m.ctrl && !m.alt && !m.logo && !m.cmd;
        let moved = |_: bool| CursorKey::Moved;
        match &key.key {
            Key::Down => moved(self.move_by(0, 1)),
            Key::Up => moved(self.move_by(0, -1)),
            Key::Right => moved(self.move_by(1, 0)),
            Key::Left => moved(self.move_by(-1, 0)),
            Key::PageDown => moved(self.page_by(1)),
            Key::PageUp => moved(self.page_by(-1)),
            Key::Home => moved(self.home()),
            Key::End => moved(self.end()),
            Key::Enter => CursorKey::Activate,
            Key::Escape => CursorKey::Close,
            Key::Delete => CursorKey::Delete,
            Key::Tab => moved(self.move_section(if m.shift { -1 } else { 1 })),
            Key::Text(t) if plain => match t.as_str() {
                "j" => moved(self.move_by(0, 1)),
                "k" => moved(self.move_by(0, -1)),
                "l" => moved(self.move_by(1, 0)),
                "h" => moved(self.move_by(-1, 0)),
                " " => CursorKey::Activate,
                "x" | "X" => CursorKey::Delete,
                _ => match printable(key) {
                    Some(t) => CursorKey::Text(t.to_string()),
                    None => CursorKey::Ignored,
                },
            },
            _ => CursorKey::Ignored,
        }
    }

    /// The pointer moved over row `index` of `section`, at window point `(x, y)`. Moves the
    /// cursor there only when the pointer really moved since the last sample (more than a
    /// point), so rows scrolling or re-laying out under a still pointer don't steal the
    /// keyboard's cursor. Returns whether it moved the cursor.
    pub fn pointer_moved(&mut self, x: f32, y: f32, section: &str, index: usize) -> bool {
        let moved = match self.gate {
            Some((lx, ly)) => (x - lx).abs() > GATE || (y - ly).abs() > GATE,
            None => self.initial_sample,
        };
        if self.gate.is_none() || moved {
            self.gate = Some((x, y));
        }
        self.initial_sample = false;
        if moved {
            self.set(section, index);
        }
        moved
    }

    /// After a keyboard move or a list change: the next pointer sample only primes the gate.
    pub fn reset_pointer(&mut self) {
        self.gate = None;
        self.initial_sample = false;
    }

    /// The next pointer sample counts as a move even if it's the first (a view opened by the
    /// pointer keeps the row under it).
    pub fn allow_initial_sample(&mut self) {
        self.gate = None;
        self.initial_sample = true;
    }

    fn count(&self) -> usize {
        self.sections.get(self.section).map_or(0, |s| s.count)
    }

    /// Show a hidden cursor; false when it was hidden (the key only woke it).
    fn wake(&mut self) -> bool {
        self.reset_pointer();
        if self.active {
            return true;
        }
        self.active = true;
        self.settle();
        false
    }

    fn neighbour(&self, dir: i32) -> Option<usize> {
        let mut i = self.section as i64 + dir as i64;
        while i >= 0 && (i as usize) < self.sections.len() {
            if self.sections[i as usize].count > 0 {
                return Some(i as usize);
            }
            i += dir as i64;
        }
        None
    }

    /// Keep the cursor on a real row: off an empty section, within the count.
    fn settle(&mut self) {
        if self.count() == 0 {
            if let Some(s) = self.neighbour(1).or_else(|| self.neighbour(-1)) {
                self.section = s;
                self.index = 0;
            }
        }
        self.index = self.index.min(self.count().saturating_sub(1));
    }
}

/// The text a key types, for a type-ahead filter: one printable character typed without Ctrl,
/// Alt or Super. Space and control characters are not filter text.
pub fn printable(key: &KeyEvent) -> Option<&str> {
    let m = key.mods;
    if m.ctrl || m.alt || m.logo || m.cmd {
        return None;
    }
    match &key.key {
        Key::Text(t) if t.chars().count() == 1 && t.chars().all(|c| !c.is_control() && !c.is_whitespace()) => Some(t),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use rune::Mods;

    fn key(k: Key) -> KeyEvent {
        KeyEvent { key: k, mods: Mods::default() }
    }
    fn text(t: &str) -> KeyEvent {
        key(Key::Text(t.into()))
    }

    fn panel() -> CursorModel {
        CursorModel::new([("header", 1), ("connected", 2), ("known", 0), ("discovered", 3)])
    }

    #[test]
    fn first_key_only_shows_the_cursor() {
        let mut c = panel();
        assert!(!c.is_at("header", 0));
        assert_eq!(c.apply_key(&text("j")), CursorKey::Moved);
        assert!(c.is_at("header", 0));
        c.apply_key(&text("j"));
        assert!(c.is_at("connected", 0));
    }

    #[test]
    fn rows_cross_sections_and_skip_empty_ones() {
        let mut c = panel();
        c.set("connected", 1);
        c.apply_key(&key(Key::Down));
        assert!(c.is_at("discovered", 0), "skips the empty known section");
        c.apply_key(&text("k"));
        assert!(c.is_at("connected", 1), "back to the previous section's last row");
        c.apply_key(&key(Key::End));
        assert!(c.is_at("discovered", 2));
        c.apply_key(&key(Key::Down));
        assert!(c.is_at("discovered", 2), "stops at the last row");
        c.apply_key(&key(Key::Home));
        assert!(c.is_at("header", 0));
        c.apply_key(&key(Key::Up));
        assert!(c.is_at("header", 0), "stops at the first row");
    }

    #[test]
    fn pages_stay_in_their_section() {
        let mut c = CursorModel::new([("a", 12), ("b", 4)]);
        c.set("a", 0);
        c.apply_key(&key(Key::PageDown));
        assert_eq!((c.section(), c.index()), ("a", 5));
        c.apply_key(&key(Key::PageDown));
        c.apply_key(&key(Key::PageDown));
        assert_eq!((c.section(), c.index()), ("a", 11));
        c.apply_key(&key(Key::PageUp));
        assert_eq!(c.index(), 6);
    }

    #[test]
    fn grids_move_by_columns() {
        let mut c = CursorModel::list(8).columns(4);
        c.set("", 1);
        c.apply_key(&text("l"));
        assert_eq!(c.index(), 2);
        c.apply_key(&key(Key::Down));
        assert_eq!(c.index(), 6);
        c.apply_key(&key(Key::Right));
        c.apply_key(&key(Key::Right));
        assert_eq!(c.index(), 7, "stays on its line");
        c.apply_key(&text("k"));
        assert_eq!(c.index(), 3);
        c.apply_key(&text("h"));
        assert_eq!(c.index(), 2);
        // A list ignores columns moves.
        let mut l = CursorModel::list(3);
        l.set("", 1);
        l.apply_key(&key(Key::Right));
        assert_eq!(l.index(), 1);
    }

    #[test]
    fn short_last_line() {
        let mut c = CursorModel::list(6).columns(4);
        c.set("", 3);
        c.apply_key(&key(Key::Down));
        assert_eq!(c.index(), 5, "lands on the short line's last tile");
    }

    #[test]
    fn actions_and_type_ahead() {
        let mut c = CursorModel::list(3);
        assert_eq!(c.apply_key(&key(Key::Enter)), CursorKey::Activate);
        assert_eq!(c.apply_key(&text(" ")), CursorKey::Activate);
        assert_eq!(c.apply_key(&key(Key::Escape)), CursorKey::Close);
        assert_eq!(c.apply_key(&text("x")), CursorKey::Delete);
        assert_eq!(c.apply_key(&text("q")), CursorKey::Text("q".into()));
        assert_eq!(c.apply_key(&text("Ж")), CursorKey::Text("Ж".into()));
        let ctrl = KeyEvent { key: Key::Text("q".into()), mods: Mods { ctrl: true, ..Mods::default() } };
        assert_eq!(c.apply_key(&ctrl), CursorKey::Ignored);
        assert_eq!(printable(&ctrl), None);
        assert_eq!(printable(&key(Key::Tab)), None);
    }

    #[test]
    fn the_pointer_moves_the_cursor_only_when_it_moves() {
        let mut c = CursorModel::list(5);
        // First sample under a still pointer only primes the gate.
        assert!(!c.pointer_moved(10.0, 40.0, "", 2));
        assert!(!c.active);
        assert!(c.pointer_moved(10.0, 44.0, "", 2));
        assert!(c.is_at("", 2));
        // The keyboard moves on; a row scrolling under the still pointer re-reports it.
        c.apply_key(&key(Key::Down));
        assert!(c.is_at("", 3));
        assert!(!c.pointer_moved(10.0, 44.0, "", 1));
        assert!(c.is_at("", 3));
        // Sub-threshold jitter is ignored, a real move is not.
        assert!(!c.pointer_moved(10.5, 44.5, "", 1));
        assert!(c.pointer_moved(10.0, 52.0, "", 1));
        assert!(c.is_at("", 1));
        // A view opened by the pointer keeps the row under it.
        c.reset();
        c.allow_initial_sample();
        assert!(c.pointer_moved(10.0, 52.0, "", 4));
        assert!(c.is_at("", 4));
    }

    #[test]
    fn counts_change_under_the_cursor() {
        let mut c = panel();
        c.set("discovered", 2);
        c.set_count("discovered", 1);
        assert!(c.is_at("discovered", 0));
        c.set_count("discovered", 0);
        assert!(c.is_at("header", 0) || c.section() != "discovered");
    }
}

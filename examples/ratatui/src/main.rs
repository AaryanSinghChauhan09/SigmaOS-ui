//! A Wi-Fi panel in the terminal: a Card with a Hero and a list of networks under a keyboard
//! cursor, following the Omarchy theme live. j/k or the arrows move, Return connects, the mouse
//! hovers and clicks, q quits. `--snap out.ans` writes it as an ANSI snapshot instead.

use omarchy_ratatui::crossterm::event::{Event, KeyEventKind, MouseEventKind};
use omarchy_ratatui::layout::col;
use omarchy_ratatui::ratatui::Frame;
use omarchy_ratatui::ui::*;
use omarchy_ratatui::{App, Draw, Flow, Theme, Themed, Tokens};

const NETWORKS: [(&str, &str, bool); 4] = [("Basecamp", "WPA2", true), ("Basecamp Guest", "Open", false), ("Coffee Shop", "WPA2", true), ("Library", "Saved", true)];

struct Wifi {
    theme: Theme,
    cursor: CursorModel,
    connected: usize,
    /// Theme changes seen: the changed hook counts them.
    changes: u64,
}

impl Themed for Wifi {
    fn theme(&self) -> &Theme {
        &self.theme
    }
    fn theme_mut(&mut self) -> &mut Theme {
        &mut self.theme
    }
    fn theme_changed(&mut self, _: &Tokens) {
        self.changes += 1;
    }
}

impl Wifi {
    fn view(&self) -> impl Draw + 'static {
        let mut list = ScrollArea::new(42).max_rows(8);
        for (i, (name, security, locked)) in NETWORKS.iter().enumerate() {
            let mut row = ListRow::new(*name).glyph("\u{f0928}").width(42).cursor(self.cursor.is_at("", i)).current(i == self.connected).key(format!("net-{i}"));
            row = if i == self.connected { row.status("Connected", Tone::Success) } else { row.subtitle(*security) };
            if *locked {
                row = row.trailing_glyph("\u{f033e}", Tone::Neutral);
            }
            list = list.child(row);
        }
        Card::new()
            .width(48)
            .child(Hero::new("\u{f0928}", "Wi-Fi").status(format!("Connected to {}", NETWORKS[self.connected].0)).trailing(ToggleSwitch::new(true)).width(42))
            .child(section_header("Networks"))
            .child(list)
            .child(col(0).child(Separator(42)).child(KeyValue::new().pair("Theme changes", self.changes.to_string()).pair("Revision", self.theme.revision().to_string())))
    }
}

impl App for Wifi {
    fn draw(&mut self, frame: &mut Frame) {
        let view = self.view();
        frame.render_widget(view.with(&self.theme), frame.area());
    }
    fn event(&mut self, ev: &Event) -> Flow {
        if omarchy_ratatui::is_quit(ev) {
            return Flow::Quit;
        }
        match ev {
            Event::Key(k) if k.kind != KeyEventKind::Release => {
                if self.cursor.apply_key(k) == CursorKey::Activate {
                    self.connected = self.cursor.index();
                }
            }
            Event::Mouse(m) => {
                let hit = self.theme.hit_at(m.column, m.row).and_then(|k| k.strip_prefix("net-").and_then(|i| i.parse::<usize>().ok()));
                match (m.kind, hit) {
                    (MouseEventKind::Moved, Some(i)) => {
                        self.cursor.pointer_moved(m.column, m.row, "", i);
                    }
                    (MouseEventKind::Down(_), Some(i)) => self.connected = i,
                    _ => {}
                }
            }
            _ => {}
        }
        Flow::Continue
    }
}

fn main() -> std::io::Result<()> {
    let mut cursor = CursorModel::list(NETWORKS.len());
    cursor.set("", 1);
    let mut app = Wifi { theme: Theme::load(), cursor, connected: 0, changes: 0 };
    let view = app.view();
    if omarchy_ratatui::snap::from_args(&mut app.theme, &view)? {
        return Ok(());
    }
    omarchy_ratatui::run(app)?;
    Ok(())
}

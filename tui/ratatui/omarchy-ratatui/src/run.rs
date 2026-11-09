//! The one call: [`run`] owns the terminal and the loop, follows the theme, and hands the app
//! its events and frames (`spec/following.md`, Adoption).

use std::io;
use std::time::Duration;

use crossterm::event::{self, DisableMouseCapture, EnableMouseCapture, Event, KeyCode, KeyEventKind, KeyModifiers};
use crossterm::execute;
use omarchy_theme::Tokens;
use ratatui::backend::Backend;
use ratatui::{Frame, Terminal};

use crate::Theme;

/// An app that keeps a [`Theme`]: two accessors, and the changed hook.
pub trait Themed {
    fn theme(&self) -> &Theme;
    fn theme_mut(&mut self) -> &mut Theme;
    /// The changed hook: runs once per change that alters the tokens, on the app's loop (the UI
    /// thread), after the new tokens are in effect ([`Theme::tokens`] has them, the revision
    /// counts the change), with those tokens; never for the tokens the app started with. For what
    /// the kit can't reach: caches, a child process to re-theme.
    fn theme_changed(&mut self, _tokens: &Tokens) {}
}

/// Whether the loop goes on.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Flow {
    Continue,
    Quit,
}

/// A terminal app run by [`run`].
pub trait App: Themed {
    /// Draw the whole frame. Read colors from the theme (`self.theme()`), never cache them.
    fn draw(&mut self, frame: &mut Frame);
    /// A key, mouse or resize event.
    fn event(&mut self, ev: &Event) -> Flow;
    /// Called every turn of the loop, after events.
    fn tick(&mut self) -> Flow {
        Flow::Continue
    }
    /// True while something of the app's animates (a spinner): the loop redraws at 30 fps.
    fn animating(&self) -> bool {
        false
    }
}

/// Ctrl+C, or `q` without modifiers.
pub fn is_quit(ev: &Event) -> bool {
    match ev {
        Event::Key(k) if k.kind != KeyEventKind::Release => {
            (k.code == KeyCode::Char('c') && k.modifiers.contains(KeyModifiers::CONTROL))
                || (k.code == KeyCode::Char('q') && k.modifiers.is_empty())
        }
        _ => false,
    }
}

/// Run `app` in the terminal until it returns [`Flow::Quit`]: the alternate screen, raw mode
/// and mouse reporting on, the theme followed live (theme switches, `omarchy font set`, text size,
/// Hyprland reloads), colors faded on a switch in truecolor (none under reduced motion or below
/// 24-bit), the terminal restored on the way out. Returns the app.
pub fn run<A: App>(mut app: A) -> io::Result<A> {
    let mut terminal = ratatui::init();
    let _ = execute!(io::stdout(), EnableMouseCapture);
    app.theme_mut().follow();
    let result = run_on(&mut terminal, &mut app, &mut |timeout| {
        if event::poll(timeout)? { event::read().map(Some) } else { Ok(None) }
    });
    let _ = execute!(io::stdout(), DisableMouseCapture);
    ratatui::restore();
    result.map(|_| app)
}

/// The loop [`run`] runs, on any backend and event source (`next(timeout)` waits up to `timeout`
/// for an event). Tests drive it with a `TestBackend`. The theme must already follow
/// ([`Theme::follow`]) for changes to arrive.
pub fn run_on<B: Backend, A: App>(
    terminal: &mut Terminal<B>,
    app: &mut A,
    next: &mut dyn FnMut(Duration) -> io::Result<Option<Event>>,
) -> io::Result<()> {
    let mut dirty = true;
    let mut was_fading = false;
    loop {
        while let Some(tokens) = app.theme_mut().take_change() {
            app.theme_changed(&tokens);
            dirty = true;
        }
        let fading = app.theme_mut().tick();
        if dirty || fading || was_fading || app.animating() {
            terminal
                .draw(|f| {
                    app.theme().clear_hits();
                    app.draw(f);
                    app.theme().finish(f.buffer_mut());
                })
                .map_err(|e| io::Error::other(e.to_string()))?;
            dirty = false;
        }
        was_fading = fading;
        let timeout = if fading {
            Duration::from_millis(8)
        } else if app.animating() {
            Duration::from_millis(33)
        } else {
            Duration::from_millis(50)
        };
        if let Some(ev) = next(timeout)? {
            dirty = true;
            if app.event(&ev) == Flow::Quit {
                return Ok(());
            }
        }
        if app.tick() == Flow::Quit {
            return Ok(());
        }
    }
}

//! Shared by the behavior tests: a fixture theme installed into a `TestAppContext`, and a host
//! view that renders whatever a test builds and keeps a log of what its handlers saw.
#![allow(dead_code)]

use std::path::Path;

use gpui::{AnyElement, Context, IntoElement, ParentElement, Render, Styled, TestAppContext, VisualTestContext, Window, div};
use omarchy_gpui::omarchy_theme::{self, Sources, Tokens};

/// The tokens of `fixtures/themes/<name>`, resolved with the `monospace` family as is.
pub fn fixture(name: &str) -> Tokens {
    let dir = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../fixtures/themes").join(name);
    let read = |f: &str| std::fs::read_to_string(dir.join(f)).unwrap_or_default();
    let mut t = omarchy_theme::resolve(&Sources {
        colors: omarchy_theme::parse::parse_colors(&read("colors.toml")),
        shell: omarchy_theme::parse::parse_shell(&read("shell.toml")),
        font_family: "monospace".into(),
        resolved_font_family: "monospace".into(),
        ..Default::default()
    });
    t.reduce_motion = false;
    t
}

/// What a host renders: built fresh every frame from the host's state.
pub type Build<S> = Box<dyn Fn(&mut Host<S>, &mut Window, &mut Context<Host<S>>) -> AnyElement>;

/// A view holding test state `S` and a log handlers write to.
pub struct Host<S: 'static> {
    pub state: S,
    pub log: Vec<String>,
    build: Option<Build<S>>,
}

impl<S: 'static> Render for Host<S> {
    fn render(&mut self, window: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        let build = self.build.take().unwrap();
        let body = build(self, window, cx);
        self.build = Some(build);
        div().size_full().child(body)
    }
}

/// A window showing `build` over `state`, in tokyo-night.
pub fn host<S: 'static>(
    cx: &mut TestAppContext,
    state: S,
    build: impl Fn(&mut Host<S>, &mut Window, &mut Context<Host<S>>) -> AnyElement + 'static,
) -> (gpui::Entity<Host<S>>, &mut VisualTestContext) {
    cx.update(|cx| omarchy_gpui::install(cx, fixture("tokyo-night")));
    cx.add_window_view(move |_, _| Host { state, log: Vec::new(), build: Some(Box::new(build)) })
}

/// The host's log, then cleared.
pub fn take_log<S: 'static>(view: &gpui::Entity<Host<S>>, cx: &mut VisualTestContext) -> Vec<String> {
    view.update(cx, |h, _| std::mem::take(&mut h.log))
}

/// Focus the window's first tab stop (Tab from nothing focused).
pub fn focus_first(cx: &mut VisualTestContext) {
    cx.update(|window, cx| window.focus_next(cx));
    cx.run_until_parked();
}

//! SplitView: panes side by side (or stacked) with a draggable handle between each pair: a
//! `md`-wide strip holding a hairline in the track color, lit with the hover glass under the
//! pointer.

use rune::{Cursor, El, Phase, col, el, row};

use crate::{Theme, color};

/// One pane: its content and either a size (the pane the handle resizes) or a share of what the
/// sized panes leave.
pub struct SplitPane {
    content: El,
    size: Option<f32>,
    min: f32,
    max: f32,
    visible: bool,
}

impl SplitPane {
    pub fn new(content: El) -> SplitPane {
        SplitPane { content, size: None, min: 0.0, max: f32::INFINITY, visible: true }
    }
    /// A fixed size along the split (width when horizontal); the handle beside it changes it.
    pub fn size(mut self, s: f32) -> Self {
        self.size = Some(s);
        self
    }
    pub fn min(mut self, s: f32) -> Self {
        self.min = s;
        self
    }
    pub fn max(mut self, s: f32) -> Self {
        self.max = s;
        self
    }
    /// Collapse the pane (and its handle) when false.
    pub fn visible(mut self, on: bool) -> Self {
        self.visible = on;
        self
    }
}

type Resize = std::rc::Rc<dyn Fn(El, usize, f32, f32, f32, f32) -> El>;

pub struct SplitView {
    key: Option<String>,
    panes: Vec<SplitPane>,
    vertical: bool,
    cursor: bool,
    resize: Option<Resize>,
}

impl SplitView {
    pub fn new() -> SplitView {
        SplitView { panes: Vec::new(), vertical: false, cursor: false, resize: None, key: None }
    }
    pub fn pane(mut self, p: SplitPane) -> Self {
        self.panes.push(p);
        self
    }
    /// Stack the panes top to bottom instead of side by side.
    pub fn vertical(mut self, on: bool) -> Self {
        self.vertical = on;
        self
    }
    /// Force the handles' hot look.
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    /// Called while a handle is dragged, with the sized pane's index and its new size.
    pub fn on_resize<S: 'static>(mut self, f: impl Fn(&mut S, usize, f32) + Clone + 'static) -> Self {
        let vertical = self.vertical;
        self.resize = Some(std::rc::Rc::new(move |e: El, index: usize, size: f32, sign: f32, min: f32, max: f32| {
            let f = f.clone();
            e.on_drag(move |s: &mut S, d, _tx| {
                if d.phase == Phase::Down {
                    d.data = size;
                    return;
                }
                let delta = if vertical { d.dy } else { d.dx };
                f(s, index, (d.data + sign * delta).clamp(min, max.max(min)));
            })
        }));
        self
    }

    /// Key the parts for tests and focus: `<prefix>-handle-<n>` for the handles, from 0.
    pub fn key(mut self, prefix: impl Into<String>) -> Self {
        self.key = Some(prefix.into());
        self
    }
    pub fn build(self, t: &Theme) -> El {
        let tk = t.tokens();
        let thick = tk.spacing.md as f32;
        let hair = tk.spacing.hairline as f32;
        let hover = color(tk.controls.hover.fill);
        let across = !self.vertical;

        let handle = |n: usize, sized: Option<(usize, f32, f32, f32, f32)>| {
            let at = ((thick - hair) / 2.0).floor();
            let line = if across {
                el().pin(Some(at), Some(0.0), None, Some(0.0)).w(hair).fill(t.track())
            } else {
                el().pin(Some(0.0), Some(at), Some(0.0), None).h(hair).fill(t.track())
            };
            let mut h = el()
                .radius(t.radius_small())
                .fill(if self.cursor { hover } else { color(omarchy_theme::Rgba::TRANSPARENT) })
                .hover(hover)
                .cursor(if across { Cursor::ResizeColumn } else { Cursor::ResizeRow })
                .role("separator")
                .child(line);
            h = if across { h.w(thick).h_full() } else { h.h(thick).w_full() };
            if let (Some((index, size, sign, min, max)), Some(resize)) = (sized, &self.resize) {
                h = resize(h, index, size, sign, min, max);
            }
            if let Some(k) = &self.key {
                h = h.key(format!("{k}-handle-{n}"));
            }
            h
        };

        let visible: Vec<(usize, SplitPane)> = self.panes.into_iter().enumerate().filter(|(_, p)| p.visible).collect();
        // Which pane each handle resizes: the sized one beside it, the left (top) one first.
        let sized_beside = |i: usize| -> Option<(usize, f32, f32, f32, f32)> {
            let (ai, a) = &visible[i];
            let (bi, b) = &visible[i + 1];
            if let Some(s) = a.size {
                Some((*ai, s, 1.0, a.min, a.max))
            } else {
                b.size.map(|s| (*bi, s, -1.0, b.min, b.max))
            }
        };
        let handles: Vec<_> = (0..visible.len().saturating_sub(1)).map(sized_beside).collect();

        let mut out = if across { row() } else { col() }.w_full().h_full();
        for (k, (_, p)) in visible.into_iter().enumerate() {
            let mut pane = el().child(p.content.w_full().h_full());
            pane = match (p.size, across) {
                (Some(s), true) => pane.w(s.clamp(p.min, p.max.max(p.min))).h_full(),
                (Some(s), false) => pane.h(s.clamp(p.min, p.max.max(p.min))).w_full(),
                (None, true) => pane.grow(1.0).min_w(0.0).h_full(),
                (None, false) => pane.grow(1.0).min_h(0.0).w_full(),
            };
            if k > 0 {
                out = out.child(handle(k - 1, handles[k - 1]));
            }
            out = out.child(pane.clip_fade(0.0, 0.0, 0.0, 0.0));
        }
        out
    }
}

impl Default for SplitView {
    fn default() -> Self {
        SplitView::new()
    }
}

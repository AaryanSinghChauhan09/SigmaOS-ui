//! The `data` section (spec/gallery.md): a Table and a Tree side by side.

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Column, Table, Tree, TreeNode};

use crate::Gallery;

/// What the table and the tree show as chosen.
pub struct State {
    pub row: usize,
    pub sort_ascending: bool,
    pub expanded: Vec<SharedString>,
    pub node: SharedString,
}

impl State {
    pub fn new(_cx: &mut App) -> State {
        State { row: 1, sort_ascending: true, expanded: vec!["root".into(), "qt".into(), "spec".into()], node: "qt/table".into() }
    }
}

const ROWS: [[&str; 4]; 6] = [
    ["assets", "Folder", "—", "2026-11-02"],
    ["Cargo.toml", "TOML", "1.2 KB", "2026-11-07"],
    ["README.md", "Markdown", "4.8 KB", "2026-11-05"],
    ["screenshot-of-the-new-settings-panel.png", "PNG image", "1.4 MB", "2026-11-08"],
    ["src", "Folder", "—", "2026-11-08"],
    ["theme.toml", "TOML", "812 B", "2026-10-30"],
];

pub fn build(g: &mut Gallery, _: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let d = &g.more.data;
    let table = Table::new([Column::new("Name"), Column::new("Kind").width(90.0), Column::new("Size").width(70.0).right(), Column::new("Modified").width(100.0)])
        .id("files")
        .rows(ROWS)
        .sort(0, d.sort_ascending)
        .selected(Some(d.row))
        .cursor(Some(3))
        .width(480.0)
        .on_select(cx.listener(|g: &mut Gallery, r: &usize, _, cx| {
            g.more.data.row = *r;
            cx.notify();
        }))
        .on_sort(cx.listener(|g: &mut Gallery, _: &usize, _, cx| {
            g.more.data.sort_ascending ^= true;
            cx.notify();
        }));

    let file = |id: &'static str, label: &'static str| TreeNode::new(id, label).glyph("\u{f0219}");
    let open = |id: &str| d.expanded.iter().any(|e| e == id);
    let folder = |id: &'static str, label: &'static str| TreeNode::new(id, label).glyph(if open(id) { "\u{f0770}" } else { "\u{f024b}" });
    let nodes = [folder("root", "omarchy-ui").children([
        folder("qt", "qt").children([file("qt/button", "Button.qml"), file("qt/table", "Table.qml")]),
        folder("rune", "rune").children([file("rune/lib", "lib.rs")]),
        folder("spec", "spec").children([file("spec/tokens", "tokens.md")]),
        file("readme", "README.md"),
    ])];
    let tree = Tree::new(nodes)
        .id("tree")
        .expanded(d.expanded.clone())
        .selected(Some(d.node.clone()))
        .cursor(Some("readme"))
        .width(220.0)
        .on_toggle(cx.listener(|g: &mut Gallery, id: &SharedString, _, cx| {
            let e = &mut g.more.data.expanded;
            match e.iter().position(|x| x == id) {
                Some(i) => {
                    e.remove(i);
                }
                None => e.push(id.clone()),
            }
            cx.notify();
        }))
        .on_select(cx.listener(|g: &mut Gallery, id: &SharedString, _, cx| {
            g.more.data.node = id.clone();
            cx.notify();
        }));

    vec![div().flex().flex_row().items_start().gap(px(t.tokens().spacing.huge as f32)).child(table).child(tree).into_any_element()]
}

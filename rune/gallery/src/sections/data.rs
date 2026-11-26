//! The `data` section (spec/gallery.md): a Table and a Tree side by side.

use omarchy_rune::Theme;
use omarchy_rune::ui::{Column, Table, Tree, TreeNode};
use rune::{El, row};

use crate::State;

/// The section's own state: what the table and tree show as chosen.
pub struct Data {
    pub row: usize,
    pub sort_ascending: bool,
    pub expanded: Vec<String>,
    pub node: String,
}

pub fn init() -> Data {
    Data {
        row: 1,
        sort_ascending: true,
        expanded: vec!["root".into(), "qt".into(), "spec".into()],
        node: "qt/table".into(),
    }
}

const ROWS: [[&str; 4]; 6] = [
    ["assets", "Folder", "—", "2026-11-02"],
    ["Cargo.toml", "TOML", "1.2 KB", "2026-11-07"],
    ["README.md", "Markdown", "4.8 KB", "2026-11-05"],
    [
        "screenshot-of-the-new-settings-panel.png",
        "PNG image",
        "1.4 MB",
        "2026-11-08",
    ],
    ["src", "Folder", "—", "2026-11-08"],
    ["theme.toml", "TOML", "812 B", "2026-10-30"],
];

pub fn build(s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    let sp = &t.tokens().spacing;
    let d = &s.data;
    let table = Table::new([
        Column::new("Name"),
        Column::new("Kind").width(90.0),
        Column::new("Size").width(70.0).right(),
        Column::new("Modified").width(100.0),
    ])
    .rows(ROWS)
    .sort(0, d.sort_ascending)
    .selected(Some(d.row))
    .cursor(Some(3))
    .width(480.0)
    .key("files")
    .on_select(|s: &mut State, r| s.data.row = r)
    .on_sort(|s: &mut State, _| s.data.sort_ascending ^= true)
    .build(t);

    let file = |id: &str, label: &str| TreeNode::new(id, label).glyph("\u{f0219}");
    let folder = |id: &str, label: &str, open: bool| {
        TreeNode::new(id, label).glyph(if open { "\u{f0770}" } else { "\u{f024b}" })
    };
    let open = |id: &str| d.expanded.iter().any(|e| e == id);
    let nodes = [folder("root", "omarchy-ui", open("root")).children([
        folder("qt", "qt", open("qt")).children([
            file("qt/button", "Button.qml"),
            file("qt/table", "Table.qml"),
        ]),
        folder("rune", "rune", open("rune")).children([file("rune/lib", "lib.rs")]),
        folder("spec", "spec", open("spec")).children([file("spec/tokens", "tokens.md")]),
        file("readme", "README.md"),
    ])];
    let tree = Tree::new(nodes)
        .expanded(&d.expanded)
        .selected(Some(&d.node))
        .cursor(Some("readme"))
        .width(220.0)
        .key("tree")
        .on_toggle(|s: &mut State, id| {
            let e = &mut s.data.expanded;
            match e.iter().position(|x| x == id) {
                Some(i) => {
                    e.remove(i);
                }
                None => e.push(id.to_string()),
            }
        })
        .on_select(|s: &mut State, id| s.data.node = id.to_string())
        .build(t);

    vec![
        row()
            .items_start()
            .gap(sp.huge as f32)
            .child(table)
            .child(tree),
    ]
}

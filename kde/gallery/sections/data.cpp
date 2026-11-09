#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  using G = QString;
  auto g = Gallery::glyph;

  // A Table and a Tree side by side, top-aligned, huge apart.
  Row *row = Gallery::row(body, parent, Gallery::sp("huge"), Qt::AlignTop);

  auto *table = new Table;
  table->setColumns({{G("Name")}, {G("Kind"), 90}, {G("Size"), 70, true}, {G("Modified"), 100}});
  const G dash = G::fromUtf8("—");
  table->setRows({{G("assets"), G("Folder"), dash, G("2026-11-02")},
                  {G("Cargo.toml"), G("TOML"), G("1.2 KB"), G("2026-11-07")},
                  {G("README.md"), G("Markdown"), G("4.8 KB"), G("2026-11-05")},
                  {G("screenshot-of-the-new-settings-panel.png"), G("PNG image"), G("1.4 MB"), G("2026-11-08")},
                  {G("src"), G("Folder"), dash, G("2026-11-08")},
                  {G("theme.toml"), G("TOML"), G("812 B"), G("2026-10-30")}});
  table->setSortColumn(0);
  table->setSortAscending(true);
  table->setSelectedRow(1);
  table->setCursorRow(3);
  table->setHasCursor(true);
  table->setFixedWidth(480);
  row->addWidget(table, Qt::AlignTop);

  auto *tree = new Tree;
  const G file = g(0xF0219), open = g(0xF0770), folder = g(0xF024B);
  tree->setNodes({TreeNode(G("root"), G("omarchy-ui"), open,
                           {TreeNode(G("qt"), G("qt"), open,
                                     {TreeNode(G("qt/button"), G("Button.qml"), file), TreeNode(G("qt/table"), G("Table.qml"), file)}),
                            TreeNode(G("rune"), G("rune"), folder, {TreeNode(G("rune/lib"), G("lib.rs"), file)}),
                            TreeNode(G("spec"), G("spec"), open, {TreeNode(G("spec/tokens"), G("tokens.md"), file)}),
                            TreeNode(G("readme"), G("README.md"), file)})});
  tree->setExpanded({G("root"), G("qt"), G("spec")});
  tree->setSelectedId(G("qt/table"));
  tree->setCursorId(G("readme"));
  tree->setHasCursor(true);
  tree->setFixedWidth(220);
  row->addWidget(tree, Qt::AlignTop);
}

Gallery::Register reg("data", "Data", build);

}  // namespace

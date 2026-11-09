// SearchList, as qt/tests/tst_searchlist.qml drives Qt's: typing filters (j, k, x and Space are
// text), the arrows and paging keys move the cursor, Return activates, Delete asks to delete,
// Escape clears then asks to close. The caller filters, as SearchList asks.

const ALL = [
  { title: "Terminal", subtitle: "App", glyph: "", detail: "Open a new terminal window." },
  { title: "Terminal font", subtitle: "Style", glyph: "", detail: "Pick the monospace font every terminal uses." },
  { title: "Terminal theme", subtitle: "Style", glyph: "", detail: "Match the terminal colors to the desktop theme." },
  { title: "Intermediate CA", subtitle: "Certificate", glyph: "", detail: "Trust a certificate authority for this machine." },
];

async function make(t, query = "") {
  await t.mount(`<omarchy-search-list id="s" style="width: 360px; height: 300px" show-detail></omarchy-search-list>`);
  await t.eval(`(() => {
    const s = document.getElementById("s"), all = ${JSON.stringify(ALL)};
    const filter = () => (s.items = all.filter((i) => s.matches(i.title, s.query)));
    s.addEventListener("query", filter);
    s.query = ${JSON.stringify(query)};
    filter();
    s.focus();
  })()`);
  await t.frame();
  await t.clearEvents();
  t.assert.equal(await t.eval(`document.getElementById("s").shadowRoot.activeElement?.tagName`), "INPUT", "the filter line holds focus");
}
const q = (t) => t.prop("#s", "query");
const idx = (t) => t.prop("#s", "cursorIndex");
const titles = (t) => t.eval(`document.getElementById("s").items.map((i) => i.title)`);
const fired = async (t, type) => (await t.events()).filter((e) => e[0] === "omarchy-search-list" && e[1] === type);

export const tests = {
  async search_typing_filters(t) {
    await make(t);
    t.assert.equal((await titles(t)).length, 4);
    await t.type("font");
    t.assert.equal(await q(t), "font");
    t.assert.deepEqual(await titles(t), ["Terminal font"]);
    await t.key("Backspace");
    t.assert.equal(await q(t), "fon");
  },

  async search_vim_keys_and_space_are_text(t) {
    await make(t);
    await t.type("jk");
    t.assert.equal(await q(t), "jk", "j and k type, they don't move");
    t.assert.equal(await idx(t), -1, "nothing matches jk");
    await t.key("Backspace");
    await t.key("Backspace");
    await t.type("term");
    await t.key(" ");
    t.assert.equal(await q(t), "term ");
    t.assert.equal((await titles(t)).length, 0);
    await t.key("Backspace");
    await t.type("x");
    t.assert.equal(await q(t), "termx", "x is text here, not delete");
    t.assert.equal((await fired(t, "delete")).length, 0);
  },

  async search_arrows_move_the_cursor(t) {
    await make(t, "term");
    t.assert.equal(await idx(t), 0, "the cursor shows on the first match from the start");
    for (const [k, want] of [["ArrowDown", 1], ["ArrowDown", 2], ["ArrowDown", 3], ["ArrowDown", 3], ["ArrowUp", 2], ["Home", 0], ["End", 3], ["PageUp", 0], ["PageDown", 3]]) {
      await t.key(k);
      t.assert.equal(await idx(t), want, `after ${k}`);
    }
    t.assert.equal(await t.eval(`document.getElementById("s").currentItem.title`), "Intermediate CA");
    t.assert.equal(await t.eval(`__q("#s >>> .detail .t").textContent`), "Intermediate CA", "the detail pane follows the cursor");
  },

  async search_enter_activates_the_cursor_row(t) {
    await make(t, "term");
    await t.key("ArrowDown");
    await t.key("ArrowDown");
    await t.key("Enter");
    const a = await fired(t, "activate");
    t.assert.equal(a.length, 1);
    t.assert.equal(a[0][2].index, 2);
    t.assert.equal(a[0][2].item.title, "Terminal theme");
  },

  async search_new_query_starts_at_first_match(t) {
    await make(t, "term");
    await t.key("ArrowDown");
    await t.key("ArrowDown");
    t.assert.equal(await idx(t), 2);
    await t.type("i");
    t.assert.equal(await q(t), "termi");
    t.assert.equal(await idx(t), 0);
  },

  async search_delete_asks_to_delete_the_cursor_row(t) {
    await make(t, "term");
    await t.key("ArrowDown");
    await t.key("Delete");
    const d = await fired(t, "delete");
    t.assert.equal(d.length, 1);
    t.assert.equal(d[0][2].index, 1);
  },

  async search_escape_clears_then_closes(t) {
    await make(t, "termix");
    t.assert.equal((await titles(t)).length, 0);
    await t.key("Escape");
    t.assert.equal(await q(t), "");
    t.assert.equal((await titles(t)).length, 4, "an empty query lists everything");
    t.assert.equal((await fired(t, "close")).length, 0);
    await t.key("Escape");
    t.assert.equal((await fired(t, "close")).length, 1);
  },

  async search_nothing_matches_shows_the_empty_state(t) {
    await make(t, "zzz");
    t.assert.equal(await idx(t), -1);
    await t.key("Enter");
    t.assert.equal((await fired(t, "activate")).length, 0, "Return with no rows does nothing");
    const expected = (await t.eval(`omarchy.string("common.noMatchesFor", "", "zzz")`));
    t.assert.ok(expected.includes("zzz"));
    t.assert.equal(await t.eval(`__q("#s >>> omarchy-empty-state >>> .title").textContent`), expected);
  },

  async search_click_activates_the_row(t) {
    await make(t, "term");
    await t.click(`#s >>> omarchy-list-row[data-index="1"]`);
    const a = await fired(t, "activate");
    t.assert.equal(a.length, 1);
    t.assert.equal(a[0][2].index, 1);
    t.assert.equal(await idx(t), 1);
  },
};

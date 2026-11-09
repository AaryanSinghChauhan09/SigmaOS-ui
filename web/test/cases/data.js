// Table, Tree, Calendar, Sidebar and SplitView, as qt/tests/tst_data.qml drives Qt's: a press
// selects a row or folds a branch, a header press asks to sort, keys move the cursor and choose.

const TABLE = `<omarchy-table id="t" style="width: 480px"
  columns='[{"title":"Name"},{"title":"Kind","width":90},{"title":"Size","width":70,"align":"right"}]'
  rows='[["assets","Folder","—"],["Cargo.toml","TOML","1.2 KB"],["README.md","Markdown","4.8 KB"],["src","Folder","—"],["target","Folder","—"]]'
  sort-column="0"></omarchy-table>`;

// The app owns sort, expanded and selection, as the Qt tests' components do.
const TABLE_APP = `(() => { const t = __q("#t"); t.addEventListener("sort", (e) => {
  const c = e.detail.column; if (c === t.sortColumn) t.sortAscending = !t.sortAscending; else { t.sortColumn = c; t.sortAscending = true; } }); })()`;

const TREE = `<omarchy-tree id="tr" style="width: 220px" expanded='["root","qt","spec"]' nodes='[{"id":"root","label":"omarchy-ui","children":[
  {"id":"qt","label":"qt","children":[{"id":"qt/button","label":"Button.qml"},{"id":"qt/table","label":"Table.qml"}]},
  {"id":"rune","label":"rune","children":[{"id":"rune/lib","label":"lib.rs"}]},
  {"id":"spec","label":"spec","children":[{"id":"spec/tokens","label":"tokens.md"}]}]}]'></omarchy-tree>`;

const TREE_APP = `(() => { const t = __q("#tr");
  t.addEventListener("toggle", (e) => { const id = e.detail.id; const x = t.expanded.slice(); const i = x.indexOf(id); if (i >= 0) x.splice(i, 1); else x.push(id); t.expanded = x; });
  t.addEventListener("activate", (e) => { t.selectedId = e.detail.id; }); })()`;

const rowWith = (host, text) =>
  `[...__q("${host}").shadowRoot.querySelectorAll(".row")].find((r) => r.textContent.trim() === ${JSON.stringify(text)})`;

async function clickText(t, host, text, dx = null) {
  const b = await t.eval(`(() => { const r = ${rowWith(host, text)}; if (!r) return null; const l = r.querySelector(".label, .cell") || r; const b = l.getBoundingClientRect(); return { x: b.x, y: b.y, w: b.width, h: b.height }; })()`);
  if (!b) throw new Error(`no row ${text}`);
  await t.page.click(dx === null ? b.x + 4 : b.x + dx, b.y + b.h / 2);
}

export const tests = {
  // tst_data: test_table_selects_on_click_and_sorts_on_header
  async table_selects_on_click_and_sorts_on_header(t) {
    await t.mount(TABLE);
    await t.eval(TABLE_APP);
    await t.click(`#t >>> .row[data-i="3"] .cell`);
    t.assert.equal(await t.prop("#t", "selectedRow"), 3);
    await t.click(`#t >>> .hd[data-i="0"] .title`);
    t.assert.equal(await t.prop("#t", "sortAscending"), false, "the sorted column's header flips the order");
    await t.click(`#t >>> .hd[data-i="1"] .title`);
    t.assert.equal(await t.prop("#t", "sortColumn"), 1);
    t.assert.equal(await t.prop("#t", "sortAscending"), true);
    t.assert.equal(await t.eval(`__q("#t >>> .hd[data-i='1'] .sort").textContent`), "\u{F0143}", "the chevron moves to the sorted column");
  },

  // tst_data: test_table_keys_move_the_cursor_and_choose
  async table_keys_move_the_cursor_and_choose(t) {
    await t.mount(TABLE);
    await t.focus("#t");
    await t.key("ArrowDown");
    t.assert.equal(await t.prop("#t", "cursorRow"), 0, "the first key lands on the first row");
    await t.key("ArrowDown");
    await t.key("Enter");
    t.assert.equal(await t.prop("#t", "selectedRow"), 1);
    await t.key("End");
    await t.key("ArrowDown");
    t.assert.equal(await t.prop("#t", "cursorRow"), 4, "stops at the last row");
    await t.key(" ");
    t.assert.equal(await t.prop("#t", "selectedRow"), 4);
    await t.key("Home");
    await t.key("ArrowUp");
    t.assert.equal(await t.prop("#t", "cursorRow"), 0, "stops at the first row");
    await t.key("Enter");
    t.assert.equal(await t.prop("#t", "selectedRow"), 0);
    const acts = (await t.events()).filter((e) => e[0] === "omarchy-table" && e[1] === "activate");
    t.assert.equal(acts.length, 3);
    t.assert.ok(await t.eval(`__q("#t >>> .row[data-i='0']").classList.contains("cur")`), "focus shows the cursor");
  },

  // tst_data: test_tree_folds_and_selects_on_click
  async tree_folds_and_selects_on_click(t) {
    await t.mount(TREE);
    await t.eval(TREE_APP);
    await t.click(`#tr >>> .row[data-id="qt"] .chev`);
    t.assert.ok(!(await t.prop("#tr", "expanded")).includes("qt"), "the chevron folds qt");
    t.assert.equal(await t.eval(`!!${rowWith("#tr", "Button.qml")}`), false);
    await clickText(t, "#tr", "tokens.md");
    t.assert.equal(await t.prop("#tr", "selectedId"), "spec/tokens");
    const b = await t.box(`#tr >>> .row[data-id="rune"] .label`);
    await t.page.click(b.x + 4, b.y + b.height / 2, { clickCount: 1 });
    await t.page.mouse("mousePressed", b.x + 4, b.y + b.height / 2, { clickCount: 2 });
    await t.page.mouse("mouseReleased", b.x + 4, b.y + b.height / 2, { clickCount: 2 });
    t.assert.ok((await t.prop("#tr", "expanded")).includes("rune"), "a double click unfolds a branch");
  },

  // tst_data: test_tree_keys_fold_unfold_and_choose
  async tree_keys_fold_unfold_and_choose(t) {
    await t.mount(TREE);
    await t.eval(TREE_APP);
    await t.focus("#tr");
    await t.key("ArrowDown");
    t.assert.equal(await t.prop("#tr", "cursorId"), "root");
    await t.key("ArrowDown");
    t.assert.equal(await t.prop("#tr", "cursorId"), "qt");
    await t.key("ArrowLeft");
    t.assert.ok(!(await t.prop("#tr", "expanded")).includes("qt"), "Left folds an open branch");
    await t.key("ArrowLeft");
    t.assert.ok(!(await t.prop("#tr", "expanded")).includes("qt"), "and does nothing to a folded one");
    await t.key("ArrowRight");
    t.assert.ok((await t.prop("#tr", "expanded")).includes("qt"), "Right unfolds it");
    await t.key("ArrowDown");
    t.assert.equal(await t.prop("#tr", "cursorId"), "qt/button");
    await t.key("ArrowRight");
    t.assert.equal((await t.prop("#tr", "expanded")).length, 3, "a leaf doesn't fold");
    await t.key("Enter");
    t.assert.equal(await t.prop("#tr", "selectedId"), "qt/button");
  },

  async calendar_keys_move_pick_and_follow_months(t) {
    await t.mount(`<omarchy-calendar id="c" year="2026" month="11" today="2026-11-08" selected="2026-11-14"></omarchy-calendar>`);
    await t.focus("#c");
    await t.key("ArrowRight");
    t.assert.equal(await t.prop("#c", "cursorDate"), "2026-11-15", "the cursor starts from the selected day");
    await t.key("ArrowDown");
    await t.key("ArrowDown");
    await t.key("ArrowDown");
    t.assert.equal(await t.prop("#c", "cursorDate"), "2026-12-06", "and follows into the next month");
    t.assert.equal(await t.prop("#c", "month"), 12);
    await t.key("Enter");
    t.assert.equal(await t.prop("#c", "selected"), "2026-12-06");
    await t.key("ArrowUp");
    t.assert.equal(await t.prop("#c", "month"), 11);
    await t.key("PageDown");
    await t.key("PageDown");
    t.assert.deepEqual([await t.prop("#c", "year"), await t.prop("#c", "month")], [2027, 1], "PageDown steps the month across the year");
    await t.key("PageUp");
    t.assert.equal(await t.prop("#c", "month"), 12);
    await t.key(" ");
    const picks = (await t.events()).filter((e) => e[1] === "pick").map((e) => e[2].date);
    t.assert.deepEqual(picks, ["2026-12-06", "2026-11-29"]);
  },

  async calendar_clicks_pick_and_step(t) {
    await t.mount(`<omarchy-calendar id="c" year="2026" month="11" today="2026-11-08"></omarchy-calendar>`);
    await t.click(`#c >>> .day[data-date="2026-11-20"]`);
    t.assert.equal(await t.prop("#c", "selected"), "2026-11-20");
    await t.click(`#c >>> .prev`);
    t.assert.equal(await t.prop("#c", "month"), 10);
    t.assert.equal(await t.eval(`__q("#c >>> .month").textContent`), "OCTOBER 2026");
    await t.click(`#c >>> .next`);
    await t.click(`#c >>> .next`);
    t.assert.equal(await t.prop("#c", "month"), 12);
    t.assert.equal(await t.eval(`__q("#c >>> .prev").getAttribute("tooltip")`), "Previous month");
  },

  async sidebar_walks_sections_and_chooses(t) {
    await t.mount(`<omarchy-sidebar id="s" sections='[{"title":"Library","items":[{"id":"all","label":"All files"},{"id":"recent","label":"Recent"}]},{"title":"Places","items":[{"id":"docs","label":"Documents"}]}]'></omarchy-sidebar>`);
    await t.focus("#s");
    await t.key("ArrowDown");
    await t.key("ArrowDown");
    await t.key("ArrowDown");
    t.assert.equal(await t.prop("#s", "cursorId"), "docs", "Down walks across sections");
    await t.key("ArrowDown");
    t.assert.equal(await t.prop("#s", "cursorId"), "docs", "and stops at the last row");
    await t.key("Enter");
    await t.click(`#s >>> .row[data-id="recent"] .label`);
    const acts = (await t.events()).filter((e) => e[1] === "activate").map((e) => e[2].id);
    t.assert.deepEqual(acts, ["docs", "recent"]);
    await t.eval(`__q("#s").collapsed = true`);
    await t.frame();
    const w = (await t.box("#s")).width;
    t.assert.equal(w, 4 * 2 + 10 * 2 + 14, "collapsed to the rail");
  },

  async split_drags_the_handle_within_limits(t) {
    await t.mount(`<omarchy-split-view id="v" style="width: 600px; height: 200px">
      <div id="a" preferred-width="200" minimum-width="120" maximum-width="400"></div><div id="b"></div></omarchy-split-view>`);
    await t.frame();
    // The harness doesn't record "resize" (a window event name too): listen here.
    await t.eval(`window.sizes = []; __q("#v").addEventListener("resize", (e) => sizes.push(e.detail.size))`);
    t.assert.equal((await t.box("#a")).width, 200);
    const h = await t.box("#v >>> .handle");
    t.assert.equal(h.x, (await t.box("#a")).x + 200, "the handle sits after the pane");
    t.assert.equal(h.width, 6, "md thick");
    const y = h.y + h.height / 2;
    await t.page.move(h.x + 3, y);
    t.assert.ok(await t.eval(`__q("#v >>> .handle").classList.contains("hot")`), "the hover glass under the pointer");
    await t.page.mouse("mousePressed", h.x + 3, y);
    await t.page.send("Input.dispatchMouseEvent", { type: "mouseMoved", x: h.x + 53, y, button: "left", buttons: 1 });
    await t.frame();
    t.assert.equal((await t.box("#a")).width, 250);
    await t.page.send("Input.dispatchMouseEvent", { type: "mouseMoved", x: h.x - 300, y, button: "left", buttons: 1 });
    await t.page.mouse("mouseReleased", h.x - 300, y);
    await t.frame();
    t.assert.equal((await t.box("#a")).width, 120, "clamped to the minimum");
    const sizes = await t.eval("window.sizes");
    t.assert.deepEqual([sizes[0], sizes[sizes.length - 1]], [250, 120]);
    await t.eval(`__q("#a").hidden = true`);
    await t.frame();
    t.assert.equal(await t.eval(`__q("#v").shadowRoot.querySelectorAll(".handle").length`), 0, "a hidden pane takes its handle");
  },
};

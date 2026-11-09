// Menu, Tabs and IconRail, as qt/tests/tst_navigation.qml drives Qt's (and the Rune gallery's
// menu_rows_run, menu_walks_with_vim_keys_and_arrows, tabs_and_rail_choose tests drive Rune's).

const MENU = `
<omarchy-menu id="m" held>
  <omarchy-menu-item text="New" icon="\u{F15B}"></omarchy-menu-item>
  <omarchy-menu-item text="Open…"></omarchy-menu-item>
  <omarchy-menu-item text="Save"></omarchy-menu-item>
  <omarchy-menu-item text="Duplicate" disabled></omarchy-menu-item>
  <omarchy-menu-separator></omarchy-menu-separator>
  <omarchy-menu-item text="Sidebar" checkable checked></omarchy-menu-item>
  <omarchy-menu-item id="export" text="Export">
    <omarchy-menu id="sub">
      <omarchy-menu-item text="PDF"></omarchy-menu-item>
      <omarchy-menu-item text="PNG"></omarchy-menu-item>
    </omarchy-menu>
  </omarchy-menu-item>
</omarchy-menu>`;

const row = (label) => `[...document.querySelectorAll("omarchy-menu-item")].find((r) => r.text === ${JSON.stringify(label)})`;

async function menu(t) {
  await t.mount(MENU);
  await t.eval(`document.getElementById("m").focus()`);
  await t.frame();
}

/** The texts of the rows run so far (the root menu's `trigger` events). */
const ran = async (t) => (await t.events()).filter((e) => e[0] === "omarchy-menu" && e[1] === "trigger").map((e) => e[2].text);
const current = (t, id = "m") => t.eval(`(document.getElementById("${id}").currentItem || {}).text`);

async function clickRow(t, label) {
  const b = await t.eval(`(() => { const b = ${row(label)}.getBoundingClientRect(); return { x: b.x + b.width / 2, y: b.y + b.height / 2 }; })()`);
  await t.page.click(b.x, b.y);
}

export const tests = {
  async menu_rows_run_on_click(t) {
    await menu(t);
    await clickRow(t, "New");
    t.assert.deepEqual(await ran(t), ["New"]);
    await clickRow(t, "Sidebar");
    t.assert.equal(await t.eval(`${row("Sidebar")}.checked`), false, "a check row flips");
    await clickRow(t, "Duplicate");
    t.assert.deepEqual(await ran(t), ["New", "Sidebar"], "a disabled row doesn't run");
  },

  async menu_walks_with_vim_keys_and_arrows(t) {
    await menu(t);
    await t.key("j");
    t.assert.equal(await current(t), "New", "the first key lands on the first row");
    await t.key("j");
    await t.key("Enter");
    t.assert.deepEqual(await ran(t), ["Open…"]);
    await t.key("ArrowDown");
    await t.key(" ");
    t.assert.deepEqual(await ran(t), ["Open…", "Save"]);
    await t.key("k");
    await t.key("Enter");
    t.assert.deepEqual(await ran(t), ["Open…", "Save", "Open…"]);
    await t.key("j");
    await t.key("j");
    t.assert.equal(await current(t), "Sidebar", "skips the disabled row");
    await t.key("Enter");
    t.assert.equal(await t.eval(`${row("Sidebar")}.checked`), false);
    await t.key("ArrowDown");
    await t.key("ArrowDown");
    t.assert.equal(await current(t), "New", "wraps past the last row");
    await t.key("ArrowUp");
    t.assert.equal(await current(t), "Export", "and back past the first");
    await t.key("Home");
    t.assert.equal(await current(t), "New");
    await t.key("End");
    t.assert.equal(await current(t), "Export");
  },

  async menu_submenu_opens_right_and_closes_left(t) {
    await menu(t);
    const open = () => t.eval(`document.getElementById("sub").matches(":popover-open")`);
    const focused = () => t.eval(`document.activeElement && document.activeElement.id`);
    await t.key("End");
    await t.key("l");
    t.assert.equal(await open(), true, "l opens the submenu");
    t.assert.equal(await focused(), "sub");
    t.assert.equal(await current(t, "sub"), "PDF");
    const sub = await t.box("#sub");
    const exp = await t.box("#export");
    t.assert.ok(sub.x >= exp.x + exp.width, "it opens to the right of its row");
    await t.key("j");
    await t.key("Enter");
    t.assert.deepEqual(await ran(t), ["PNG"]);
    t.assert.equal(await open(), false, "running a row closes the submenu");
    t.assert.equal(await focused(), "m", "and the held menu has the keys again");
    await t.key("End");
    await t.key("ArrowRight");
    t.assert.equal(await open(), true);
    await t.key("h");
    t.assert.equal(await open(), false, "h goes back out");
    t.assert.equal(await focused(), "m");
    await t.key("ArrowRight");
    t.assert.equal(await open(), true);
    await t.key("ArrowLeft");
    t.assert.equal(await open(), false, "so does Left");
    await t.key("ArrowRight");
    await t.key("Escape");
    t.assert.equal(await open(), false, "and Escape");
  },

  async menu_escape_closes_an_open_menu(t) {
    await menu(t);
    await t.eval(`(() => { const m = document.getElementById("m"); m.held = false; m.open(); })()`);
    t.assert.equal(await t.eval(`document.getElementById("m").opened`), true);
    await t.key("Escape");
    t.assert.equal(await t.eval(`document.getElementById("m").opened`), false);
  },

  async menu_closes_on_a_press_outside(t) {
    await t.mount(`<omarchy-button id="b" text="Open" bordered></omarchy-button>
      <omarchy-menu id="m" trigger="b"><omarchy-menu-item text="One"></omarchy-menu-item></omarchy-menu>`);
    await t.eval(`document.getElementById("m").open()`);
    const m = await t.box("#m");
    const b = await t.box("#b");
    t.assert.ok(m.y >= b.y + b.height, "opens below its trigger");
    await t.page.click(600, 600);
    t.assert.equal(await t.eval(`document.getElementById("m").opened`), false);
  },

  async menu_context_menu_opens_at_the_pointer(t) {
    await t.mount(`<div id="area" style="width: 400px; height: 300px"></div>
      <omarchy-context-menu id="c" for="area"><omarchy-menu-item text="Cut"></omarchy-menu-item></omarchy-context-menu>`);
    await t.page.click(120, 90, { button: "right" });
    await t.frame();
    t.assert.equal(await t.eval(`document.getElementById("c").opened`), true);
    const c = await t.box("#c");
    t.assert.deepEqual([Math.round(c.x), Math.round(c.y)], [120, 90]);
    await t.key("j");
    await t.key("Enter");
    t.assert.deepEqual((await t.events()).filter((e) => e[1] === "trigger" && e[0] === "omarchy-context-menu").map((e) => e[2].text), ["Cut"]);
    t.assert.equal(await t.eval(`document.getElementById("c").opened`), false, "running a row closes it");
  },

  async tabs_choose_on_click_and_keys(t) {
    await t.mount(`<omarchy-tabs id="tabs" current-index="1" model='["General","Display","Audio","Network"]'></omarchy-tabs>`);
    await t.click("#tabs >>> .tab[data-i='3']");
    t.assert.equal(await t.prop("#tabs", "currentIndex"), 3);
    t.assert.equal((await t.events()).filter((e) => e[1] === "activate").length, 1);
    await t.eval(`(() => { const e = document.getElementById("tabs"); e.focus(); e.currentIndex = 1; })()`);
    for (const [k, want] of [["ArrowRight", 2], ["l", 3], ["ArrowRight", 3], ["h", 2], ["ArrowLeft", 1], ["Home", 0], ["ArrowLeft", 0], ["End", 3], ["ArrowUp", 3], ["ArrowDown", 3]]) {
      await t.key(k);
      t.assert.equal(await t.prop("#tabs", "currentIndex"), want, `tabs after ${k}`);
    }
    const bar = await t.box("#tabs >>> .bar");
    const tab = await t.box("#tabs >>> .tab[data-i='3']");
    t.assert.equal(Math.round(bar.x), Math.round(tab.x), "the underline sits under the selected tab");
  },

  async rail_chooses_on_click_and_keys(t) {
    const model = JSON.stringify(["Files", "Search", "Source control", "Debug", "Settings"].map((label) => ({ icon: "\u{F07B}", label })));
    await t.mount(`<omarchy-icon-rail id="rail" model='${model}'></omarchy-icon-rail>`);
    const r = await t.box("#rail");
    const size = await t.prop("#rail", "itemSize");
    await t.page.click(r.x + r.width / 2, r.y + r.height - size / 2 - 4);
    t.assert.equal(await t.prop("#rail", "currentIndex"), 4, "a press on the last button");
    t.assert.equal((await t.events()).filter((e) => e[1] === "activate").length, 1);
    await t.eval(`(() => { const e = document.getElementById("rail"); e.focus(); e.currentIndex = 0; })()`);
    for (const [k, want] of [["ArrowDown", 1], ["j", 2], ["k", 1], ["ArrowUp", 0], ["ArrowUp", 0], ["End", 4], ["ArrowDown", 4], ["Home", 0], ["ArrowRight", 0], ["ArrowLeft", 0]]) {
      await t.key(k);
      t.assert.equal(await t.prop("#rail", "currentIndex"), want, `rail after ${k}`);
    }
  },

  async toolbar_overflow_runs(t) {
    await t.mount(`<omarchy-toolbar id="tb" style="width: 500px" overflow><omarchy-icon-button icon="\u{F032}"></omarchy-icon-button><omarchy-heading slot="center" text="notes.md"></omarchy-heading></omarchy-toolbar>`);
    await t.eval(`window.overflows = 0; document.getElementById("tb").addEventListener("overflow", () => window.overflows++)`);
    await t.click("#tb >>> .more");
    t.assert.equal(await t.eval("window.overflows"), 1, "the overflow button emits overflow");
    const tb = await t.box("#tb");
    const h = await t.box("#tb >>> .center");
    t.assert.ok(Math.abs(h.x + h.width / 2 - (tb.x + tb.width / 2)) <= 1, "the center slot centers on the bar");
  },

  async toolbar_keycaps_normalize_names(t) {
    await t.mount(`<omarchy-key-combo id="k" keys="ctrl+shift++"></omarchy-key-combo>`);
    const labels = await t.eval(`[...__q("#k").shadowRoot.querySelectorAll("omarchy-keycap")].map((c) => c.label)`);
    t.assert.deepEqual(labels, ["Ctrl", "⇧", "+"]);
  },
};

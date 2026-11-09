// Lists: CursorModel case for case with qt/tests/tst_cursormodel.qml, the lists section's
// pointer-and-keys test from the Rune gallery (lists_cursor_follows_pointer_and_keys), and ListRow,
// ScrollArea, TileGrid and Avatar behavior.

// A focused panel handing every key to a CursorModel, as a panel's Keys.onPressed does; its events
// recorded in window.cm.
async function panel(t, opts = {}) {
  await t.mount(`<div id="panel" tabindex="0" style="width: 100px; height: 100px; outline: none"></div>`);
  await t.eval(`(() => {
    const m = new kit.CursorModel(${JSON.stringify({ sections: [{ name: "header", count: 1 }, { name: "connected", count: 2 }, { name: "known", count: 0 }, { name: "discovered", count: 3 }], ...opts })});
    window.cm = { model: m, activated: [], deleted: [], closed: 0, typed: [] };
    m.addEventListener("activated", (e) => cm.activated.push(e.detail));
    m.addEventListener("deleterequested", (e) => cm.deleted.push(e.detail));
    m.addEventListener("closerequested", () => cm.closed++);
    m.addEventListener("texttyped", (e) => cm.typed.push(e.detail.text));
    const p = document.getElementById("panel");
    p.addEventListener("keydown", (e) => m.handleKey(e));
    p.focus();
  })()`);
}
const at = (t, s, i) => t.eval(`cm.model.isAt(${JSON.stringify(s)}, ${i})`);
const index = (t) => t.eval("cm.model.selectedIndex");
const set = (t, s, i) => t.eval(`cm.model.set(${JSON.stringify(s)}, ${i})`);

export const tests = {
  async cursor_first_key_only_shows_the_cursor(t) {
    await panel(t);
    t.assert.ok(!(await at(t, "header", 0)));
    await t.key("j");
    t.assert.ok(await at(t, "header", 0));
    await t.key("j");
    t.assert.ok(await at(t, "connected", 0));
  },

  async cursor_rows_cross_sections_and_skip_empty_ones(t) {
    await panel(t);
    await set(t, "connected", 1);
    await t.key("ArrowDown");
    t.assert.ok(await at(t, "discovered", 0), "skips the empty known section");
    await t.key("k");
    t.assert.ok(await at(t, "connected", 1), "back to the previous section's last row");
    await t.key("End");
    t.assert.ok(await at(t, "discovered", 2));
    await t.key("ArrowDown");
    t.assert.ok(await at(t, "discovered", 2), "stops at the last row");
    await t.key("Home");
    t.assert.ok(await at(t, "header", 0));
    await t.key("ArrowUp");
    t.assert.ok(await at(t, "header", 0), "stops at the first row");
  },

  async cursor_tab_jumps_sections(t) {
    await panel(t);
    await set(t, "header", 0);
    await t.key("Tab");
    t.assert.ok(await at(t, "connected", 0));
    await t.key("Tab");
    t.assert.ok(await at(t, "discovered", 0), "skips the empty section");
    await t.key("Tab", { modifiers: t.SHIFT });
    t.assert.ok(await at(t, "connected", 0));
  },

  async cursor_pages_stay_in_their_section(t) {
    await panel(t, { sections: [{ name: "a", count: 12 }, { name: "b", count: 4 }] });
    await set(t, "a", 0);
    await t.key("PageDown");
    t.assert.deepEqual(await t.eval("[cm.model.focusSection, cm.model.selectedIndex]"), ["a", 5]);
    await t.key("PageDown");
    await t.key("PageDown");
    t.assert.deepEqual(await t.eval("[cm.model.focusSection, cm.model.selectedIndex]"), ["a", 11]);
    await t.key("PageUp");
    t.assert.equal(await index(t), 6);
  },

  async cursor_grids_move_by_columns(t) {
    await panel(t, { sections: [{ name: "", count: 8 }], columns: 4 });
    await set(t, "", 1);
    await t.key("l");
    t.assert.equal(await index(t), 2);
    await t.key("ArrowDown");
    t.assert.equal(await index(t), 6);
    await t.key("ArrowRight");
    await t.key("ArrowRight");
    t.assert.equal(await index(t), 7, "stays on its line");
    await t.key("k");
    t.assert.equal(await index(t), 3);
    await t.key("h");
    t.assert.equal(await index(t), 2);

    await panel(t, { sections: [{ name: "", count: 3 }] });
    await set(t, "", 1);
    await t.key("ArrowRight");
    t.assert.equal(await index(t), 1, "a list ignores column moves");
  },

  async cursor_short_last_line(t) {
    await panel(t, { sections: [{ name: "", count: 6 }], columns: 4 });
    await set(t, "", 3);
    await t.key("ArrowDown");
    t.assert.equal(await index(t), 5, "lands on the short line's last tile");
  },

  async cursor_actions_and_type_ahead(t) {
    await panel(t, { sections: [{ name: "", count: 3 }] });
    await set(t, "", 1);
    await t.key("Enter");
    await t.key(" ");
    t.assert.equal(await t.eval("cm.activated.length"), 2);
    t.assert.equal(await t.eval("cm.activated[0].index"), 1);
    await t.key("Escape");
    t.assert.equal(await t.eval("cm.closed"), 1);
    await t.key("x");
    await t.key("Delete");
    t.assert.equal(await t.eval("cm.deleted.length"), 2);
    await t.key("q");
    await t.key("7");
    await t.key("é");
    t.assert.deepEqual(await t.eval("cm.typed"), ["q", "7", "é"]);
    await t.key("q", { modifiers: 2 });
    t.assert.equal(await t.eval("cm.typed.length"), 3, "Ctrl+Q is not text");
  },

  // Rune gallery: lists_cursor_follows_pointer_and_keys, on the gallery's own wiring.
  async list_cursor_follows_pointer_and_keys(t) {
    await t.page.goto(await t.eval(`new URL("../../gallery/index.html?section=lists", location.href).href`));
    await t.page.waitFor("window.__galleryReady || window.__galleryError", { timeout: 15000 });
    t.assert.equal(await t.eval("window.__galleryError || ''"), "");
    const rows = "document.querySelectorAll('#lists-scroll omarchy-list-row')";
    t.assert.equal(await t.eval(`[...${rows}].findIndex((r) => r.cursor)`), 1);
    const b = await t.eval(`(() => { const r = ${rows}[4].getBoundingClientRect(); return { x: r.x + r.width / 2, y: r.y + r.height / 2 }; })()`);
    await t.page.move(b.x, b.y);
    await t.page.move(b.x, b.y + 3);
    t.assert.equal(await t.eval(`[...${rows}].findIndex((r) => r.cursor)`), 4, "the pointer moves the cursor");
    await t.page.click(b.x, b.y + 3);
    await t.eval("document.getElementById('lists-scroll').focus()");
    await t.key("ArrowDown");
    await t.key("j");
    t.assert.equal(await t.eval(`[...${rows}].findIndex((r) => r.cursor)`), 6, "the still pointer doesn't pull the cursor back");
    // Down past the cap scrolls the cursor row into view.
    await t.key("j");
    await t.key("j");
    t.assert.equal(await t.eval(`[...${rows}].findIndex((r) => r.cursor)`), 8);
    const shown = await t.eval(`(() => { const v = document.getElementById("lists-scroll").shadowRoot.querySelector(".view").getBoundingClientRect(); const r = ${rows}[8].getBoundingClientRect(); return r.bottom <= v.bottom + 0.5 && r.top >= v.top - 0.5; })()`);
    t.assert.ok(shown, "the cursor row is scrolled into view");
  },

  async list_row_clicks_and_action(t) {
    await t.mount(`
      <omarchy-list-row id="a" style="width: 400px" glyph="x" title="Office" trailing-glyph="L" action-glyph="F" action-urgent></omarchy-list-row>
      <omarchy-list-row id="d" style="width: 400px" title="Neighbor" disabled action-glyph="F"></omarchy-list-row>`);
    // As an app listens: bubbling, on the rows' container.
    await t.eval(`window.seen = []; for (const type of ["click", "action"]) document.body.addEventListener(type, (e) => e.target.tagName === "OMARCHY-LIST-ROW" && seen.push(e.target.id + ":" + type))`);
    t.assert.ok(await t.eval(`__q("#a >>> .act").classList.contains("hidden")`), "the action hides at rest");
    t.assert.ok(!(await t.eval(`__q("#a >>> .tg").classList.contains("hidden")`)), "the trailing glyph shows at rest");
    await t.click("#a >>> .title");
    t.assert.ok(!(await t.eval(`__q("#a >>> .act").classList.contains("hidden")`)), "the pointer reveals the action");
    t.assert.ok(await t.eval(`__q("#a >>> .tg").classList.contains("hidden")`), "in the trailing glyph's place");
    await t.click("#a >>> .slot");
    await t.click("#d >>> .title");
    t.assert.deepEqual(await t.eval("seen"), ["a:click", "a:action"], "the action isn't the row's click; a disabled row takes none");
  },

  async list_row_cursor_and_current_looks(t) {
    await t.mount(`<omarchy-list-row id="c" title="A" cursor current></omarchy-list-row><omarchy-list-row id="s" title="B" current></omarchy-list-row>`);
    const fill = (sel) => t.eval(`getComputedStyle(__q("${sel} >>> .surface")).backgroundColor`);
    const tokens = await t.eval("omarchy.tokens.controls");
    const rgba = (hex) => {
      const n = (i) => parseInt(hex.slice(i, i + 2), 16);
      const a = +(n(7) / 255).toFixed(3);
      return [n(1), n(3), n(5), a];
    };
    const parse = (s) => s.match(/[\d.]+/g).map(Number);
    const near = (a, b) => a.every((v, i) => Math.abs(v - b[i]) < (i === 3 ? 0.01 : 1));
    t.assert.ok(near(parse(await fill("#c")), rgba(tokens.hover.fill)), "the cursor wins over current");
    t.assert.ok(near(parse(await fill("#s")), rgba(tokens.selected.fill)), "current takes the selected fill");
  },

  async tile_grid_activates_on_click(t) {
    await t.mount(`<omarchy-tile-grid id="g" columns="2" tiles='[{"glyph":"a","label":"One"},{"glyph":"b","label":"Two"},{"glyph":"c","label":"Three"}]'></omarchy-tile-grid>`);
    await t.click("#g >>> .tile[data-index='2']");
    const ev = (await t.events()).filter((e) => e[1] === "activate");
    t.assert.deepEqual(ev[0][2], { index: 2 });
    const [a, b] = await t.eval(`[...__q("#g").shadowRoot.querySelectorAll(".tile")].slice(0, 2).map((e) => e.getBoundingClientRect().height)`);
    t.assert.equal(a, b, "every tile is the same height");
  },

  async scroll_area_bar_drags(t) {
    await t.mount(`<omarchy-scroll-area id="s" style="width: 200px" max-height="100">${Array.from({ length: 10 }, (_, i) => `<div style="height: 40px">${i}</div>`).join("")}</omarchy-scroll-area>`);
    await t.frame();
    t.assert.equal(await t.prop("#s", "overflowing"), true);
    const bar = await t.box("#s >>> .bar");
    t.assert.equal(bar.height, 100, "the bar is the viewport's height");
    await t.page.move(bar.x + 2, bar.y + 5);
    await t.page.mouse("mousePressed", bar.x + 2, bar.y + 5);
    await t.page.mouse("mouseMoved", bar.x + 2, bar.y + 200);
    await t.page.mouse("mouseReleased", bar.x + 2, bar.y + 200);
    const top = await t.eval(`__q("#s >>> .view").scrollTop`);
    t.assert.equal(top, 300, "dragging the thumb to the end scrolls to the end, no overshoot");
    await t.eval(`__q("#s").ensureVisible(__q("#s").children[0])`);
    t.assert.equal(await t.eval(`__q("#s >>> .view").scrollTop`), 0);
  },

  async avatar_falls_back_in_order(t) {
    await t.mount(`<omarchy-avatar id="m" name="missing" source="file:///nope/missing.png"></omarchy-avatar><omarchy-avatar id="g" name="x" glyph="G"></omarchy-avatar>`);
    await t.page.waitFor(`!(window.__omarchyPending > 0)`);
    t.assert.equal(await t.eval(`__q("#m >>> .mark").textContent`), "M", "a picture that fails gives way to the initial");
    t.assert.ok(await t.eval(`__q("#m >>> .tile").classList.contains("show")`));
    t.assert.equal(await t.eval(`__q("#g >>> .mark").textContent`), "G");
  },
};

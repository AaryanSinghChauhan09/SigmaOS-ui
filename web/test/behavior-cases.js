// The behavior cases: each mounts elements into the harness page and drives keys and clicks.
// Named after the qt/tests cases they mirror where there is one.

export const tests = {
  async button_clicks_on_click_enter_and_space(t) {
    await t.mount(`<omarchy-button id="b" text="Go" bordered></omarchy-button>`);
    await t.click("#b");
    await t.focus("#b");
    await t.key("Enter");
    await t.key(" ");
    const clicks = (await t.events()).filter((e) => e[0] === "omarchy-button" && e[1] === "click");
    t.assert.equal(clicks.length, 3, JSON.stringify(await t.events()));
  },

  async button_focus_looks_like_hover(t) {
    await t.mount(`<omarchy-button id="b" text="Go" bordered></omarchy-button><omarchy-button id="c" text="Hover" bordered cursor></omarchy-button>`);
    await t.key("Tab");
    t.assert.equal(await t.eval("document.activeElement.id"), "b");
    const look = (sel) => t.eval(`(() => { const b = __q("${sel} >>> .box"); const s = getComputedStyle(b); return [s.backgroundColor, s.borderTopColor, b.dataset.fill]; })()`);
    const [fFill, fBorder, state] = await look("#b");
    const [hFill, hBorder] = await look("#c");
    t.assert.equal(state, "focus");
    t.assert.equal(fFill, hFill, "focus fills as hover");
    t.assert.equal(fBorder, hBorder, "focus borders as hover");
  },

  async button_group_chooses_on_click_and_keys(t) {
    await t.mount(`<omarchy-button-group id="g" value="center" options='[{"value":"left","label":"Left"},{"value":"center","label":"Center"},{"value":"right","label":"Right"}]'></omarchy-button-group>`);
    await t.click("#g >>> omarchy-button[data-value=right]");
    t.assert.equal(await t.prop("#g", "value"), "right");
    await t.focus("#g");
    await t.key("ArrowLeft");
    await t.key("Enter");
    t.assert.equal(await t.prop("#g", "value"), "center");
    await t.key("h");
    await t.key(" ");
    t.assert.equal(await t.prop("#g", "value"), "left");
    await t.key("ArrowLeft");
    await t.key("Enter");
    t.assert.equal(await t.prop("#g", "value"), "left", "stops at the first");
    await t.key("l");
    await t.key("l");
    await t.key("l");
    await t.key("Enter");
    t.assert.equal(await t.prop("#g", "value"), "right", "stops at the last");
  },

  async toggle_switch_flips_on_click_and_space(t) {
    await t.mount(`<omarchy-toggle-switch id="s"></omarchy-toggle-switch>`);
    await t.click("#s");
    t.assert.equal(await t.prop("#s", "checked"), true);
    await t.focus("#s");
    await t.key(" ");
    t.assert.equal(await t.prop("#s", "checked"), false);
    await t.key("Enter");
    t.assert.equal(await t.prop("#s", "checked"), true);
  },

  // tst_surfaces: test_toggle_row_flips_on_click_and_space
  async toggle_row_flips_on_click_and_space(t) {
    await t.mount(`<omarchy-toggle id="t" label="Wi-Fi" description="Connected"></omarchy-toggle>`);
    await t.click("#t >>> .desc");
    t.assert.equal(await t.prop("#t", "checked"), true, "the whole row is the control");
    await t.focus("#t");
    await t.key(" ");
    t.assert.equal(await t.prop("#t", "checked"), false);
    const changes = (await t.events()).filter((e) => e[0] === "omarchy-toggle" && e[1] === "change");
    t.assert.equal(changes.length, 2);
  },

  async text_field_types_and_accepts(t) {
    await t.mount(`<omarchy-text-field id="f" placeholder="Search…"></omarchy-text-field>`);
    await t.click("#f");
    await t.type("omarchy");
    t.assert.equal(await t.prop("#f", "value"), "omarchy");
    await t.key("Enter");
    const acc = (await t.events()).filter((e) => e[1] === "accept");
    t.assert.deepEqual(acc[0][2], { value: "omarchy" });
    t.assert.equal(await t.eval(`__q("#f >>> .box").dataset.fill`), "focus");
  },

  async number_field_steps_and_clamps(t) {
    await t.mount(`<omarchy-number-field id="n" label="Gaps" value="10" from="0" to="40"></omarchy-number-field>`);
    await t.click("#n >>> .up");
    t.assert.equal(await t.prop("#n", "value"), 11);
    await t.click("#n >>> .down");
    await t.click("#n >>> .down");
    t.assert.equal(await t.prop("#n", "value"), 9);
    await t.click("#n >>> input");
    await t.key("ArrowUp");
    t.assert.equal(await t.prop("#n", "value"), 10);
    await t.eval(`__q("#n >>> input").select()`);
    await t.type("99");
    await t.key("Enter");
    t.assert.equal(await t.prop("#n", "value"), 40, "clamps to the range");
    await t.key("PageDown");
    t.assert.equal(await t.prop("#n", "value"), 30);
  },

  async dropdown_opens_walks_and_chooses(t) {
    await t.mount(`<omarchy-dropdown id="d" label="Theme" value="b" options='[{"value":"a","label":"Alpha"},{"value":"b","label":"Beta"},{"value":"c","label":"Gamma"}]'></omarchy-dropdown>`);
    await t.click("#d >>> .trigger");
    t.assert.equal(await t.prop("#d", "popupOpen"), true);
    t.assert.equal(await t.eval(`__q("#d >>> .opt.cur").textContent`), "Beta", "opens on the current value");
    await t.key("j");
    await t.key("Enter");
    t.assert.equal(await t.prop("#d", "value"), "c");
    t.assert.equal(await t.prop("#d", "popupOpen"), false);
    await t.key("ArrowDown");
    t.assert.equal(await t.prop("#d", "popupOpen"), true, "Down opens it from the trigger");
    await t.key("k");
    await t.key("k");
    await t.key("k");
    await t.key(" ");
    t.assert.equal(await t.prop("#d", "value"), "a", "k walks up and stops at the first");
    await t.key("Enter");
    await t.key("Escape");
    t.assert.equal(await t.prop("#d", "popupOpen"), false, "Escape closes");
    t.assert.equal(await t.prop("#d", "value"), "a");
    await t.click("#d >>> .trigger");
    await t.click("#d >>> .opt[data-i='1']");
    t.assert.equal(await t.prop("#d", "value"), "b", "a click chooses");
  },

  async slider_drags_clicks_and_steps(t) {
    await t.mount(`<omarchy-slider id="s" style="width: 200px" value="0.5"></omarchy-slider>`);
    const b = await t.box("#s");
    await t.page.click(b.x + b.width - 2, b.y + b.height / 2);
    t.assert.equal(await t.prop("#s", "value"), 1, "a click at the end");
    await t.page.click(b.x + 2, b.y + b.height / 2);
    t.assert.equal(await t.prop("#s", "value"), 0, "a click at the start");
    await t.focus("#s");
    await t.key("ArrowRight");
    await t.key("l");
    t.assert.equal(await t.prop("#s", "value"), 0.1);
    await t.key("End");
    t.assert.equal(await t.prop("#s", "value"), 1);
    await t.key("ArrowLeft");
    t.assert.equal(await t.prop("#s", "value"), 0.95);
  },

  // tst_surfaces: test_pane_action_ends_the_header_and_runs
  async pane_action_ends_the_header_and_runs(t) {
    await t.mount(`<omarchy-pane id="p" style="width: 240px" title="Night light" subtitle="Sunset to sunrise" bordered>
      <omarchy-toggle-switch id="sw" slot="actions" checked></omarchy-toggle-switch>
      <omarchy-setting-row label="Strength" style="width: auto"><omarchy-slider style="width: 100px" value="0.6"></omarchy-slider></omarchy-setting-row>
    </omarchy-pane>`);
    const p = await t.box("#p");
    const sw = await t.box("#sw");
    const inner = p.x + p.width - 18;
    t.assert.ok(sw.x + sw.width <= inner + 1.5 && sw.x + sw.width > inner - 40, `the action ends the header (${sw.x + sw.width} vs ${inner})`);
    const title = await t.box("#p >>> omarchy-heading >>> .t");
    t.assert.ok(title.x < sw.x, "the title comes first");
    await t.click("#sw");
    t.assert.equal(await t.prop("#sw", "checked"), false);
  },

  async confirm_dialog_keys(t) {
    await t.mount(`<div style="position: relative; width: 400px; height: 200px"><omarchy-confirm-dialog id="d" open message="Remove?"></omarchy-confirm-dialog></div>`);
    await t.frame();
    t.assert.equal(await t.eval("document.activeElement.id"), "d", "an open dialog takes focus");
    t.assert.equal(await t.eval(`__q("#d >>> .btn.sel").textContent`), "Confirm");
    await t.key("h");
    t.assert.equal(await t.prop("#d", "selectedIndex"), 0);
    await t.key("Tab");
    t.assert.equal(await t.prop("#d", "selectedIndex"), 1);
    await t.key("Enter");
    await t.key("ArrowLeft");
    await t.key("Enter");
    await t.key("Escape");
    const ev = (await t.events()).filter((e) => e[0] === "omarchy-confirm-dialog" && ["accept", "reject"].includes(e[1])).map((e) => e[1]);
    t.assert.deepEqual(ev, ["accept", "reject", "reject"]);
  },

  async strings_follow_the_language(t) {
    await t.mount(`<div style="position: relative; width: 400px; height: 200px"><omarchy-confirm-dialog id="d" open message="?"></omarchy-confirm-dialog></div>`);
    await t.eval(`window.__omarchyDeliver({ ...omarchy.state, language: "de", strings: { ...omarchy.strings, "common.cancel": "Abbrechen" } })`);
    await t.frame();
    t.assert.equal(await t.eval(`__q("#d >>> .btn").textContent`), "Abbrechen");
  },
};

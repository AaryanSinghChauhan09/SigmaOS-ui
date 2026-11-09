// Forms and pickers, mirroring qt/tests/tst_forms.qml case by case, plus the swatch strip and the
// password field's reveal.

const changes = async (t, tag) => (await t.events()).filter((e) => e[0] === tag && e[1] === "change");

export const tests = {
  // tst_forms: test_checkbox_flips_on_click_and_keys_unless_disabled
  async checkbox_flips_on_click_and_keys_unless_disabled(t) {
    await t.mount(`<omarchy-checkbox id="c" label="Show hidden files"></omarchy-checkbox>`);
    await t.click("#c");
    t.assert.equal(await t.prop("#c", "checked"), true);
    await t.focus("#c");
    await t.key(" ");
    t.assert.equal(await t.prop("#c", "checked"), false);
    await t.key("Enter");
    t.assert.equal(await t.prop("#c", "checked"), true);
    await t.eval(`__q("#c").disabled = true`);
    await t.click("#c").catch(() => {});
    await t.eval(`__q("#c").click()`);
    t.assert.equal(await t.prop("#c", "checked"), true, "a disabled checkbox doesn't flip");
    t.assert.equal((await changes(t, "omarchy-checkbox")).length, 3);
  },

  async checkbox_focus_shows_the_cursor_look(t) {
    await t.mount(`<omarchy-checkbox id="c" label="A"></omarchy-checkbox><omarchy-checkbox id="d" label="B" cursor></omarchy-checkbox>`);
    await t.key("Tab");
    t.assert.equal(await t.eval("document.activeElement.id"), "c");
    const look = (sel) => t.eval(`(() => { const s = getComputedStyle(__q("${sel} >>> .box")); return [s.backgroundColor, s.borderTopColor]; })()`);
    t.assert.deepEqual(await look("#c"), await look("#d"));
  },

  // tst_forms: test_radios_choose_on_click_and_keys
  async radio_group_chooses_on_click_and_keys(t) {
    await t.mount(`<omarchy-radio-group id="g" value="bottom" options='[{"value":"top","label":"Top"},{"value":"bottom","label":"Bottom"},{"value":"hidden","label":"Hidden"}]'></omarchy-radio-group>`);
    await t.click("#g >>> omarchy-radio[label=Hidden]");
    t.assert.equal(await t.prop("#g", "value"), "hidden");
    const first = (await changes(t, "omarchy-radio-group"))[0];
    t.assert.deepEqual(first[2], { value: "hidden" });
    await t.click("#g >>> omarchy-radio[label=Top]");
    t.assert.equal(await t.prop("#g", "value"), "top");
    await t.focus("#g");
    for (const [key, want] of [["ArrowDown", "bottom"], ["j", "hidden"], ["ArrowDown", "hidden"], ["k", "bottom"], ["ArrowUp", "top"],
      ["ArrowUp", "top"], ["ArrowRight", "bottom"], ["ArrowLeft", "top"]]) {
      await t.key(key);
      t.assert.equal(await t.prop("#g", "value"), want, `radios after ${key}`);
    }
    t.assert.equal(await t.eval(`__q("#g >>> omarchy-radio[label=Top]").hasAttribute("cursor")`), true, "the focused group's choice shows the cursor");
  },

  async radio_alone_is_a_round_checkbox(t) {
    await t.mount(`<omarchy-radio id="r" label="Only"></omarchy-radio>`);
    await t.click("#r");
    t.assert.equal(await t.prop("#r", "checked"), true);
    t.assert.equal(await t.eval(`getComputedStyle(__q("#r >>> .box")).borderRadius`), "50%");
  },

  // tst_forms: test_multiselect_toggles_on_click
  async multiselect_toggles_on_click(t) {
    await t.mount(`<omarchy-multi-select id="m" style="width: 260px" label="Notify for" options='["Calendar","Mail","Messages","Updates","Battery"]' values='["Mail","Updates"]'></omarchy-multi-select>`);
    await t.click("#m >>> .trigger");
    t.assert.equal(await t.prop("#m", "popupOpen"), true);
    t.assert.equal(await t.eval(`__q("#m").shadowRoot.activeElement.tagName`), "OMARCHY-TEXT-FIELD", "the search field takes the keys");
    await t.click("#m >>> .row[data-i='0']");
    t.assert.ok((await t.prop("#m", "values")).includes("Calendar"));
    await t.click("#m >>> .row[data-i='1']");
    t.assert.ok(!(await t.prop("#m", "values")).includes("Mail"));
    t.assert.deepEqual(await t.prop("#m", "values"), ["Updates", "Calendar"]);
    t.assert.equal(await t.eval(`__q("#m >>> .value").textContent`), "Updates, Calendar");
  },

  // tst_forms: test_multiselect_keys
  async multiselect_keys(t) {
    await t.mount(`<omarchy-multi-select id="m" style="width: 260px" label="Notify for" options='["Calendar","Mail","Messages","Updates","Battery"]' values='["Mail","Updates"]'></omarchy-multi-select>`);
    await t.eval(`__q("#m").focus()`);
    await t.key("j");
    t.assert.equal(await t.prop("#m", "popupOpen"), true, "j opens it from the trigger");
    await t.type("mes");
    await t.key("Enter");
    t.assert.deepEqual(await t.prop("#m", "values"), ["Mail", "Updates", "Messages"], "Return toggles the first match");
    await t.key("Backspace");
    await t.key("Backspace");
    await t.key("Backspace");
    await t.key("ArrowDown");
    await t.key(" ");
    t.assert.deepEqual(await t.prop("#m", "values"), ["Mail", "Updates", "Messages", "Calendar"], "Down into the list, Space toggles");
    await t.key("j");
    await t.key("Enter");
    t.assert.deepEqual(await t.prop("#m", "values"), ["Updates", "Messages", "Calendar"], "j walks, Return toggles");
    t.assert.equal((await changes(t, "omarchy-multi-select")).length, 3);
    await t.key("k");
    await t.key("k");
    t.assert.equal(await t.eval(`__q("#m").shadowRoot.activeElement.tagName`), "OMARCHY-TEXT-FIELD", "k off the first row goes back to the search");
    await t.key("Escape");
    t.assert.equal(await t.prop("#m", "popupOpen"), false, "Escape closes it");
    t.assert.equal(await t.eval(`__q("#m >>> .value").textContent`), "Updates, Messages, Calendar", "three or fewer read as their labels");
  },

  async multiselect_reads_the_count_past_three_and_none(t) {
    await t.mount(`<omarchy-multi-select id="a" options='["A","B","C","D"]' values='["A","B","C","D"]'></omarchy-multi-select><omarchy-multi-select id="b" options='["A"]'></omarchy-multi-select>`);
    t.assert.equal(await t.eval(`__q("#a >>> .value").textContent`), "4 selected");
    t.assert.equal(await t.eval(`__q("#b >>> .value").textContent`), "None selected");
  },

  async swatch_strip_picks_on_click_and_keys(t) {
    await t.mount(`<omarchy-swatch-strip id="s" current-index="2" colors='["red","green","blue","white"]'></omarchy-swatch-strip>`);
    await t.click("#s >>> .cell[data-i='0']");
    t.assert.equal(await t.prop("#s", "currentIndex"), 0);
    await t.focus("#s");
    await t.key("l");
    await t.key("ArrowRight");
    t.assert.equal(await t.prop("#s", "cursorIndex"), 2);
    t.assert.equal(await t.eval(`__q("#s >>> .cell[data-i='2']").classList.contains("hot")`), true);
    await t.key(" ");
    t.assert.equal(await t.prop("#s", "currentIndex"), 2);
    await t.key("h");
    await t.key("h");
    await t.key("h");
    await t.key("Enter");
    t.assert.equal(await t.prop("#s", "currentIndex"), 0, "stops at the first");
    const picks = (await t.events()).filter((e) => e[1] === "pick").map((e) => e[2].index);
    t.assert.deepEqual(picks, [0, 2, 0]);
  },

  async color_well_presses_on_click_and_keys(t) {
    await t.mount(`<omarchy-color-well id="w" swatch="red"></omarchy-color-well>`);
    await t.click("#w");
    await t.focus("#w");
    await t.key(" ");
    await t.key("Enter");
    t.assert.equal((await t.events()).filter((e) => e[0] === "omarchy-color-well" && e[1] === "click").length, 3);
    t.assert.equal(await t.eval(`__q("#w >>> .ring").classList.contains("on")`), true, "focus rings it");
  },

  async password_hides_reveals_and_accepts(t) {
    await t.mount(`<omarchy-password-field id="p" style="width: 220px" revealable></omarchy-password-field>`);
    await t.click("#p >>> omarchy-text-field");
    await t.type("hunter22");
    t.assert.equal(await t.prop("#p", "text"), "hunter22");
    t.assert.equal(await t.eval(`__q("#p >>> omarchy-text-field >>> input").type`), "password");
    await t.click("#p >>> .eye");
    t.assert.equal(await t.prop("#p", "revealed"), true);
    t.assert.equal(await t.eval(`__q("#p >>> omarchy-text-field >>> input").type`), "text");
    t.assert.equal(await t.eval(`document.activeElement.id`), "p", "the eye keeps focus in the field");
    await t.key("Enter");
    const acc = (await t.events()).filter((e) => e[0] === "omarchy-password-field" && e[1] === "accept");
    t.assert.deepEqual(acc.map((e) => e[2]), [{ value: "hunter22" }]);
  },

  async password_error_borders_in_polkit_error(t) {
    await t.mount(`<omarchy-password-field id="p" style="width: 220px" text="x" error="Wrong password"></omarchy-password-field>`);
    const [border, want, msg] = await t.eval(`(() => {
      const box = __q("#p >>> omarchy-text-field >>> .box");
      const probe = document.createElement("div"); probe.style.color = "var(--omarchy-polkit-border-error)"; document.body.append(probe);
      return [getComputedStyle(box).borderTopColor, getComputedStyle(probe).color, __q("#p >>> .message").textContent];
    })()`);
    t.assert.equal(border, want);
    t.assert.equal(msg, "Wrong password");
    t.assert.equal(await t.eval(`__q("#p >>> .wrap").getAnimations().length`), 0, "no shake under reduced motion");
  },

  async text_area_grows_from_min_to_max_rows(t) {
    await t.mount(`<omarchy-text-area id="a" style="width: 300px"></omarchy-text-area>`);
    const h = () => t.eval(`__q("#a").getBoundingClientRect().height`);
    const three = await h();
    await t.eval(`__q("#a").text = "1\\n2\\n3\\n4"`);
    await t.frame();
    const four = await h();
    await t.eval(`__q("#a").text = "1\\n2\\n3\\n4\\n5\\n6\\n7\\n8"`);
    await t.frame();
    const six = await h();
    t.assert.equal(four - three, 16, `a fourth line adds one line (${three} → ${four})`);
    t.assert.equal(six - three, 48, `capped at six lines (${six})`);
    await t.click("#a");
    await t.key("End");
    await t.type("x");
    t.assert.ok((await t.prop("#a", "text")).length > 15);
  },
};

// The plain-HTML restyle: a page's own controls take the tokens, the states and the opt-outs.

const css = (t, sel, prop) => t.eval(`getComputedStyle(document.querySelector(${JSON.stringify(sel)}))[${JSON.stringify(prop)}]`);
const token = (t, name) => t.eval(`(() => { const d = document.createElement("div"); d.style.color = "var(${name})"; document.body.append(d); const c = getComputedStyle(d).color; d.remove(); return c; })()`);

export const tests = {
  async restyle_draws_plain_controls_with_the_tokens(t) {
    await t.mount(`<button id="b">Go</button><input id="i" placeholder="x"><input id="c" type="checkbox" checked><select id="s"><option>a</option></select><progress id="p" value="0.5"></progress>`);
    t.assert.equal(await css(t, "#b", "borderTopColor"), await token(t, "--omarchy-control-normal-border"));
    t.assert.equal(await css(t, "#b", "fontFamily"), await t.eval(`getComputedStyle(document.documentElement).getPropertyValue("--omarchy-font-family").trim()`));
    t.assert.equal(await css(t, "#i", "backgroundColor"), await token(t, "--omarchy-control-normal-fill"));
    t.assert.equal(await css(t, "#i", "borderTopLeftRadius"), "0px");
    t.assert.equal(await css(t, "#c", "backgroundColor"), await token(t, "--omarchy-control-selected-fill"));
    t.assert.equal(await css(t, "html", "backgroundColor"), await token(t, "--omarchy-background"));
  },

  async restyle_hover_and_focus_look_alike(t) {
    await t.mount(`<input id="a"><input id="b">`);
    const box = await t.box("#a");
    await t.page.move(box.x + 5, box.y + 5);
    const hover = await css(t, "#a", "backgroundColor");
    await t.page.move(500, 500);
    await t.click("#b");
    await t.page.move(500, 500);
    t.assert.equal(await css(t, "#b", "backgroundColor"), hover, "focus fills as hover");
    t.assert.equal(hover, await token(t, "--omarchy-control-hover-fill"));
  },

  async restyle_loses_to_the_pages_own_rules(t) {
    await t.mount(`<style>button { border-color: rgb(255, 0, 0); }</style><button id="b">Go</button>`);
    t.assert.equal(await css(t, "#b", "borderTopColor"), "rgb(255, 0, 0)");
  },
};

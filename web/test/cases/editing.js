// Editing: Timeline, Scrubber, TrimHandle, CropFrame, DragHandle, ColorPicker, driven by the keys and
// drags spec/components/editing.md gives them (Qt's Keys.onPressed and MouseAreas).

async function mount(t, html) {
  await t.mount(html);
}

const evs = async (t, tag, type) => (await t.events()).filter((e) => e[0] === tag && e[1] === type).map((e) => e[2]);

async function drag(t, from, to, steps = 4) {
  await t.page.move(from[0], from[1]);
  await t.page.mouse("mousePressed", from[0], from[1]);
  for (let i = 1; i <= steps; i++) {
    const x = from[0] + ((to[0] - from[0]) * i) / steps;
    const y = from[1] + ((to[1] - from[1]) * i) / steps;
    await t.page.send("Input.dispatchMouseEvent", { type: "mouseMoved", x, y, button: "left", buttons: 1 });
  }
  await t.page.mouse("mouseReleased", to[0], to[1]);
}

const TRACKS = JSON.stringify([
  [{ start: 0, end: 7.5, label: "Intro" }, { start: 8, end: 18, label: "Demo" }, { start: 18.5, end: 30, label: "Outro" }],
  [{ start: 2, end: 12, label: "Voiceover" }],
]);

export const tests = {
  async timeline_seeks_on_keys_and_ruler_presses(t) {
    await mount(t, `<omarchy-timeline id="tl" style="width: 600px; height: 160px" duration="30" position="12" tracks='${TRACKS}'></omarchy-timeline>`);
    await t.focus("#tl");
    await t.key("ArrowRight");
    t.assert.equal(await t.prop("#tl", "position"), 13, "Right: +1 s");
    await t.key("ArrowLeft");
    await t.key("ArrowLeft");
    t.assert.equal(await t.prop("#tl", "position"), 11);
    const step = await t.eval(`__q("#tl").tickStep()`);
    await t.key("ArrowRight", { modifiers: t.SHIFT });
    t.assert.equal(await t.prop("#tl", "position"), 11 + step, "Shift: a major step");
    await t.key("End");
    t.assert.equal(await t.prop("#tl", "position"), 30);
    await t.key("Home");
    t.assert.equal(await t.prop("#tl", "position"), 0);
    const b = await t.box("#tl");
    await t.page.click(b.x + b.width / 2, b.y + 5);
    t.assert.equal(Math.round(await t.prop("#tl", "position")), 15, "a press on the ruler seeks there");
    const seeks = await evs(t, "omarchy-timeline", "seek");
    t.assert.equal(seeks.length, 7);
  },

  async timeline_selects_a_clip_and_trims_it(t) {
    await mount(t, `<omarchy-timeline id="tl" style="width: 600px; height: 160px" duration="30" tracks='${TRACKS}'></omarchy-timeline>`);
    await t.click("#tl >>> .clip[data-track='0'][data-clip='1']");
    t.assert.equal(await t.prop("#tl", "selectedTrack"), 0);
    t.assert.equal(await t.prop("#tl", "selectedClip"), 1);
    t.assert.deepEqual((await evs(t, "omarchy-timeline", "clip-press"))[0], { track: 0, clip: 1 });
    await t.frame();
    // Drag the right handle 100 px (5 s at 20 px/s) right.
    const h = await t.box("#tl >>> omarchy-trim-handle[edge=right]");
    const cx = h.x + h.width / 2;
    const cy = h.y + h.height / 2;
    await drag(t, [cx, cy], [cx + 100, cy]);
    const end = await t.eval(`__q("#tl").tracks[0][1].end`);
    t.assert.ok(Math.abs(end - 23) < 0.06, `the clip ends at ${end}, not 23`);
    // Nudge the left handle 1 pt left (Shift: 10 pt), 1/20 s per pt.
    await t.focus("#tl >>> omarchy-trim-handle[edge=left]");
    await t.key("ArrowLeft");
    await t.key("ArrowLeft", { modifiers: t.SHIFT });
    const start = await t.eval(`__q("#tl").tracks[0][1].start`);
    t.assert.ok(Math.abs(start - 7.45) < 1e-6, `the clip starts at ${start}, not 7.45`);
    const trims = await evs(t, "omarchy-timeline", "trim");
    t.assert.ok(trims.length >= 3 && trims.every((d) => d.track === 0 && d.clip === 1));
    t.assert.equal(await t.eval("document.activeElement.id"), "tl", "the handle keeps the focus inside the timeline");
    t.assert.equal(await t.prop("#tl", "position"), 0, "trimming doesn't seek");
  },

  async timeline_wheel_zooms_around_the_pointer(t) {
    await mount(t, `<omarchy-timeline id="tl" style="width: 600px; height: 160px" duration="30" tracks='${TRACKS}'></omarchy-timeline>`);
    const b = await t.box("#tl");
    await t.page.send("Input.dispatchMouseEvent", { type: "mouseWheel", x: b.x + 300, y: b.y + 80, deltaX: 0, deltaY: -100 });
    await t.frame();
    const zoom = await t.prop("#tl", "zoom");
    const start = await t.prop("#tl", "viewStart");
    t.assert.ok(Math.abs(zoom - 1.25) < 1e-9, `zoom ${zoom}`);
    t.assert.ok(Math.abs(start + 15 / 1.25 - 15) < 0.05, `the time under the pointer stays (${start})`);
    t.assert.equal((await evs(t, "omarchy-timeline", "view-change")).length, 1);
  },

  async scrubber_seeks_on_keys_and_clicks(t) {
    await mount(t, `<omarchy-scrubber id="s" style="width: 360px" position="42" duration="130"></omarchy-scrubber>`);
    await t.focus("#s");
    await t.key("ArrowRight");
    t.assert.equal(await t.prop("#s", "position"), 47, "Right: +5 s");
    await t.key("ArrowLeft", { modifiers: t.SHIFT });
    t.assert.equal(await t.prop("#s", "position"), 17, "Shift-Left: -30 s");
    await t.key("End");
    t.assert.equal(await t.prop("#s", "position"), 130);
    await t.key("Home");
    t.assert.equal(await t.prop("#s", "position"), 0);
    t.assert.equal(await t.eval(`__q("#s >>> .elapsed").textContent`), "0:00");
    const bar = await t.box("#s >>> .bar");
    await t.page.click(bar.x + bar.width / 2, bar.y + bar.height / 2);
    t.assert.equal(Math.round(await t.prop("#s", "position")), 65, "a click seeks there");
    t.assert.equal(await t.eval(`__q("#s >>> .total").textContent`), "2:10");
  },

  async trim_handle_drags_and_nudges_along_its_axis(t) {
    await mount(t, `<omarchy-trim-handle id="h" edge="left"></omarchy-trim-handle><omarchy-trim-handle id="c" crop edge="top"></omarchy-trim-handle>`);
    const b = await t.box("#h");
    await drag(t, [b.x + b.width / 2, b.y + b.height / 2], [b.x + b.width / 2 + 30, b.y + b.height / 2 + 10]);
    const moves = await evs(t, "omarchy-trim-handle", "move");
    t.assert.deepEqual(moves[moves.length - 1], { edge: "left", dx: 30, dy: 10 }, "travel since the press");
    t.assert.equal((await evs(t, "omarchy-trim-handle", "release")).length, 1);
    await t.clearEvents();
    await t.focus("#h");
    await t.key("ArrowRight");
    await t.key("ArrowLeft", { modifiers: t.SHIFT });
    await t.key("ArrowUp");
    await t.focus("#c");
    await t.key("ArrowDown");
    await t.key("ArrowLeft");
    t.assert.deepEqual(await evs(t, "omarchy-trim-handle", "nudge"), [
      { edge: "left", dx: 1, dy: 0 },
      { edge: "left", dx: -10, dy: 0 },
      { edge: "top", dx: 0, dy: 1 },
    ]);
  },

  async crop_frame_reports_the_edge(t) {
    await mount(t, `<omarchy-crop-frame id="f" style="width: 240px; height: 140px"></omarchy-crop-frame>`);
    await t.focus("#f >>> omarchy-trim-handle[edge=bottom-right]");
    await t.key("ArrowLeft");
    await t.key("ArrowDown", { modifiers: t.SHIFT });
    t.assert.deepEqual(await evs(t, "omarchy-crop-frame", "nudge"), [
      { edge: "bottom-right", dx: -1, dy: 0 },
      { edge: "bottom-right", dx: 0, dy: 10 },
    ]);
    t.assert.equal((await evs(t, "omarchy-trim-handle", "nudge")).length, 0, "the handles' own events stay inside");
    const h = await t.box("#f >>> omarchy-trim-handle[edge=left]");
    const f = await t.box("#f");
    t.assert.equal(h.x, f.x);
    t.assert.equal(h.y, f.y + Math.round((140 - h.height) / 2), "a side bar is centered on its side");
    await drag(t, [h.x + 2, h.y + h.height / 2], [h.x + 22, h.y + h.height / 2]);
    t.assert.deepEqual((await evs(t, "omarchy-crop-frame", "press"))[0], { edge: "left" });
    const moves = await evs(t, "omarchy-crop-frame", "move");
    t.assert.deepEqual(moves[moves.length - 1], { edge: "left", dx: 20, dy: 0 });
    t.assert.deepEqual((await evs(t, "omarchy-crop-frame", "release"))[0], { edge: "left" });
  },

  async drag_handle_drags_and_asks_to_move(t) {
    await mount(t, `<omarchy-drag-handle id="d"></omarchy-drag-handle>`);
    const b = await t.box("#d");
    await drag(t, [b.x + 8, b.y + 11], [b.x + 8, b.y + 51]);
    t.assert.equal((await evs(t, "omarchy-drag-handle", "press")).length, 1);
    const moves = await evs(t, "omarchy-drag-handle", "move");
    t.assert.deepEqual(moves[moves.length - 1], { dx: 0, dy: 40 });
    t.assert.equal((await evs(t, "omarchy-drag-handle", "release")).length, 1);
    await t.key("ArrowUp");
    await t.key("ArrowDown");
    t.assert.deepEqual(await evs(t, "omarchy-drag-handle", "move-request"), [{ delta: -1 }, { delta: 1 }]);
    t.assert.equal(await t.eval(`getComputedStyle(__q("#d >>> .dot")).backgroundColor === getComputedStyle(document.documentElement).getPropertyValue("--omarchy-foreground").trim() || __q("#d").classList.contains("hot")`), true, "focused, its dots are the foreground");
  },

  async color_picker_keys_drags_hex_and_palette(t) {
    await mount(t, `<omarchy-color-picker id="p" style="width: 300px; height: 260px" color="#7aa2f7"></omarchy-color-picker>`);
    t.assert.equal(await t.prop("#p", "hex"), "#7aa2f7");
    // Arrows move the square's knob 1% (Shift 10%).
    await t.focus("#p >>> .square");
    const s0 = await t.prop("#p", "saturation");
    await t.key("ArrowRight");
    await t.key("ArrowDown", { modifiers: t.SHIFT });
    t.assert.ok(Math.abs((await t.prop("#p", "saturation")) - (s0 + 0.01)) < 1e-9);
    t.assert.ok(Math.abs((await t.prop("#p", "value")) - (0.9686 - 0.1)) < 0.001);
    // A press in the square's corner: s 1, v 1.
    const sq = await t.box("#p >>> .square");
    await t.page.click(sq.x + sq.width - 0.5, sq.y + 0.5);
    t.assert.ok((await t.prop("#p", "saturation")) > 0.99 && (await t.prop("#p", "value")) > 0.99);
    // The hue strip: a drag to its bottom is red again (clamped below 1); Up/Down move it.
    const hs = await t.box("#p >>> .huestrip");
    await drag(t, [hs.x + 7, hs.y + hs.height / 2], [hs.x + 7, hs.y + hs.height + 20]);
    t.assert.ok((await t.prop("#p", "hue")) > 0.999);
    await t.key("ArrowUp", { modifiers: t.SHIFT });
    t.assert.ok(Math.abs((await t.prop("#p", "hue")) - 0.8999) < 1e-6);
    // The alpha strip: Down takes 1% off; the hex grows an alpha byte.
    await t.focus("#p >>> .alpha");
    await t.key("ArrowDown");
    t.assert.ok(Math.abs((await t.prop("#p", "alpha")) - 0.99) < 1e-9);
    t.assert.equal((await t.prop("#p", "hex")).length, 9);
    // A valid hex typed into the field applies at once.
    await t.eval(`__q("#p >>> omarchy-text-field").input.select()`);
    await t.click("#p >>> omarchy-text-field");
    await t.eval(`__q("#p >>> omarchy-text-field").input.select()`);
    await t.type("#9ece6a");
    t.assert.equal(await t.prop("#p", "hex"), "#9ece6a");
    // A palette swatch picks its color and keeps the alpha; Return on a focused one too.
    await t.focus("#p >>> .alpha");
    await t.key("ArrowDown");
    await t.click("#p >>> .swatch");
    const first = await t.eval(`__q("#p >>> .swatch").dataset.color`);
    t.assert.equal((await t.prop("#p", "hex")).slice(0, 7), first);
    t.assert.ok(Math.abs((await t.prop("#p", "alpha")) - 0.99) < 1e-9, "keeps the alpha");
    t.assert.equal(await t.eval(`__q("#p >>> .swatch").classList.contains("chosen")`), true);
    await t.focus("#p >>> .swatch:nth-child(2)");
    await t.key("Enter");
    t.assert.equal((await t.prop("#p", "hex")).slice(0, 7), await t.eval(`__q("#p >>> .swatch:nth-child(2)").dataset.color`));
    t.assert.ok((await evs(t, "omarchy-color-picker", "change")).length >= 8);
  },
};

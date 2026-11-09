// The lists section (spec/components/lists.md, Gallery; qt/gallery/sections/lists.qml).
import { CursorModel } from "../../src/elements/lists.js";

const g = (cp) => String.fromCodePoint(cp);

export default {
  title: "Lists",
  html: `
<omarchy-scroll-area id="lists-scroll" tabindex="0" style="width: 420px; outline: none" max-height="340" spacing="var(--omarchy-space-xxs)">
  <omarchy-list-row glyph="${g(0xf0928)}" title="Coffee Shop" subtitle="WPA2" value="5 GHz" trailing-glyph="${g(0xf033e)}"></omarchy-list-row>
  <omarchy-list-row glyph="${g(0xf0925)}" title="Basecamp Guest" subtitle="Open"></omarchy-list-row>
  <omarchy-list-row glyph="${g(0xf0928)}" title="Basecamp" status="Connected" status-tone="success" current></omarchy-list-row>
  <omarchy-list-row glyph="${g(0xf091f)}" title="Neighbor 5G" subtitle="Out of range" disabled></omarchy-list-row>
  <omarchy-list-row title="Messages" subtitle="From Basecamp">
    <omarchy-avatar slot="leading" name="Messages" size="small"></omarchy-avatar>
    <omarchy-badge slot="trailing" text="3" tone="accent" filled></omarchy-badge>
  </omarchy-list-row>
  <omarchy-list-row glyph="${g(0xf00af)}" title="Headphones" subtitle="Battery 80%">
    <omarchy-toggle-switch slot="trailing" checked passive></omarchy-toggle-switch>
  </omarchy-list-row>
  <omarchy-list-row glyph="${g(0xf0922)}" title="Office" subtitle="Saved" trailing-glyph="${g(0xf033e)}" action-glyph="${g(0xf0159)}" action-urgent action-cursor></omarchy-list-row>
  <omarchy-list-row glyph="${g(0xf0922)}" title="Library" subtitle="WPA2" trailing-glyph="${g(0xf033e)}" action-glyph="${g(0xf0159)}" action-urgent></omarchy-list-row>
  <omarchy-list-row glyph="${g(0xf091f)}" title="Airport Free WiFi" subtitle="Open"></omarchy-list-row>
</omarchy-scroll-area>
<div class="col" style="width: 420px; gap: var(--omarchy-space-xxs); align-items: stretch">
  <omarchy-list-row glyph="${g(0xf03d8)}" title="Appearance" subtitle="Theme, font, rounding" trailing-glyph="${g(0xf0142)}"></omarchy-list-row>
  <omarchy-list-row glyph="${g(0xf0379)}" title="Display" subtitle="2560 × 1440" trailing-glyph="${g(0xf0142)}"></omarchy-list-row>
  <omarchy-list-row glyph="${g(0xf02cb)}" title="Headphones" subtitle="Output" current trailing-glyph="${g(0xf012c)}" trailing-tone="accent"></omarchy-list-row>
</div>
<omarchy-hero style="width: 420px" glyph="${g(0xf0928)}" title="Wi-Fi" status="Connected to Basecamp">
  <omarchy-toggle-switch slot="trailing" checked></omarchy-toggle-switch>
</omarchy-hero>
<omarchy-hero style="width: 420px" glyph="${g(0xf00b2)}" title="Bluetooth" status="No adapter found" error glyph-opacity="0.5"></omarchy-hero>
<omarchy-key-value pair-columns="2" pairs='[{"label":"Ping","value":"18 ms"},{"label":"Packet loss","value":"4%","tone":"urgent"},{"label":"Receiving","value":"1.2 MB/s"},{"label":"Sending","value":"240 KB/s"},{"label":"IP address","value":"192.168.1.24"},{"label":"Gateway","value":"192.168.1.1"}]'></omarchy-key-value>`,

  // The scroll area's rows follow a CursorModel: the keys (focus the list) and the pointer move it.
  setup(body) {
    const scroll = body.querySelector("#lists-scroll");
    const rows = [...scroll.querySelectorAll("omarchy-list-row")];
    const cursor = new CursorModel({ sections: [{ name: "rows", count: rows.length }] });
    const render = () => rows.forEach((r, i) => (r.cursor = cursor.isAt("rows", i)));
    cursor.addEventListener("change", render);
    cursor.addEventListener("moved", (e) => {
      if (e.detail.byKey) scroll.ensureVisible(rows[e.detail.index]);
    });
    scroll.addEventListener("keydown", (e) => cursor.handleKey(e));
    rows.forEach((r, i) => {
      if (!r.disabled) r.addEventListener("pointermove", (e) => cursor.pointerMoved(e, "rows", i));
    });
    cursor.set("rows", 1);
    scroll.cursorModel = cursor;
  },
};

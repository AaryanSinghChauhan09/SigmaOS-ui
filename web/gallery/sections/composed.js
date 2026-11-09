// Real UIs built only from kit elements and tokens (spec/components/composed.md, Gallery;
// qt/gallery/sections/composed.qml): the gallery adds nothing but layout.
import { SearchList } from "../../src/elements/composed.js";

const g = (cp) => String.fromCodePoint(cp);
const searchItems = [
  { glyph: g(0xf120), title: "Terminal", subtitle: "App", detail: "Open a new terminal window." },
  { glyph: g(0xf031), title: "Terminal font", subtitle: "Style", detail: "Pick the monospace font every terminal uses." },
  { glyph: g(0xf1fc), title: "Terminal theme", subtitle: "Style", detail: "Match the terminal colors to the desktop theme." },
  { glyph: g(0xf0a3), title: "Intermediate CA", subtitle: "Certificate", detail: "Trust a certificate authority for this machine." },
];
const json = (v) => JSON.stringify(v).replace(/'/g, "&#39;");

export default {
  title: "Composed",
  html: `
<div class="row top" style="gap: calc(var(--omarchy-space-huge) * 2)">
  <omarchy-card style="width: 360px">
    <omarchy-hero glyph="${g(0xf0928)}" title="Wi-Fi" status="Connected to Basecamp">
      <omarchy-toggle-switch slot="trailing" checked></omarchy-toggle-switch>
    </omarchy-hero>
    <omarchy-section-header text="Networks"></omarchy-section-header>
    <omarchy-scroll-area max-height="280" spacing="var(--omarchy-space-xxs)">
      <omarchy-list-row glyph="${g(0xf0928)}" title="Basecamp" status="Connected" status-tone="success" current></omarchy-list-row>
      <omarchy-list-row glyph="${g(0xf0925)}" title="Basecamp Guest" subtitle="Open" action-glyph="${g(0xf0159)}" action-urgent cursor></omarchy-list-row>
      <omarchy-list-row glyph="${g(0xf0922)}" title="Coffee Shop" subtitle="WPA2" trailing-glyph="${g(0xf033e)}">
        <div slot="expansion" class="row" style="gap: var(--omarchy-space-md)">
          <omarchy-password-field style="flex: 1; width: auto" placeholder="Password"></omarchy-password-field>
          <omarchy-button text="Connect" bordered></omarchy-button>
        </div>
      </omarchy-list-row>
      <omarchy-list-row glyph="${g(0xf091f)}" title="Library" subtitle="Saved" trailing-glyph="${g(0xf033e)}" action-glyph="${g(0xf0159)}" action-urgent></omarchy-list-row>
    </omarchy-scroll-area>
    <omarchy-separator></omarchy-separator>
    <omarchy-key-value pairs='[{"label":"IP","value":"192.168.1.24"},{"label":"Gateway","value":"192.168.1.1"},{"label":"DNS","value":"1.1.1.1"}]'></omarchy-key-value>
  </omarchy-card>

  <omarchy-pane style="width: 300px" title="Recording 1" subtitle="1920 × 1080 · 0:42" divider="left">
    <omarchy-icon-button slot="actions" icon="${g(0xf0e2)}"></omarchy-icon-button>
    <omarchy-icon-button slot="actions" icon="${g(0xf01e)}"></omarchy-icon-button>
    <omarchy-button slot="actions" text="Export" bordered></omarchy-button>
    <omarchy-tabs model='["Shape","Cursor","Zoom"]' current-index="0"></omarchy-tabs>
    <omarchy-section-header text="Frame"></omarchy-section-header>
    <div class="col fill" style="align-items: stretch">
      <omarchy-setting-row style="width: auto" label="Radius"><omarchy-slider style="width: 110px" value="0.4"></omarchy-slider></omarchy-setting-row>
      <omarchy-setting-row style="width: auto" label="Border"><omarchy-dropdown style="width: 110px" hide-label value="solid" options='[{"value":"solid","label":"Solid"},{"value":"none","label":"None"}]'></omarchy-dropdown></omarchy-setting-row>
      <omarchy-setting-row style="width: auto" label="Shadow" cursor><omarchy-toggle-switch checked></omarchy-toggle-switch></omarchy-setting-row>
    </div>
    <omarchy-section-header text="Background"></omarchy-section-header>
    <div class="col fill" style="align-items: stretch">
      <omarchy-setting-row style="width: auto" label="Fill"><omarchy-color-well swatch="var(--omarchy-accent)"></omarchy-color-well></omarchy-setting-row>
      <omarchy-setting-row style="width: auto" label="Padding"><omarchy-number-field value="24" from="0" to="96"></omarchy-number-field></omarchy-setting-row>
    </div>
  </omarchy-pane>
</div>

<div class="row top" style="gap: calc(var(--omarchy-space-huge) * 2)">
  <omarchy-search-list id="composed-search" style="width: 360px; height: 300px" query="term" show-detail items='${json(searchItems)}'></omarchy-search-list>
  <omarchy-search-list style="width: 300px; height: 200px" query="zzz" items='${json(searchItems.filter((i) => SearchList.matches(i.title, "zzz")))}'></omarchy-search-list>
</div>`,

  setup(body) {
    const s = body.querySelector("#composed-search");
    s.cursor.set("results", 1);
    s.renderCursor();
  },
};

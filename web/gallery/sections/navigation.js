// qt/gallery/sections/navigation.qml: a toolbar, tabs, and an icon rail beside its page.
const rail = [
  { icon: "\u{F07B}", label: "Files" },
  { icon: "\u{F002}", label: "Search" },
  { icon: "\u{F126}", label: "Source control" },
  { icon: "\u{F188}", label: "Debug" },
  { icon: "\u{F013}", label: "Settings" },
];

export default {
  title: "Navigation",
  html: `
<omarchy-toolbar class="fill" overflow>
  <omarchy-icon-button icon="\u{F032}"></omarchy-icon-button>
  <omarchy-icon-button icon="\u{F033}" cursor></omarchy-icon-button>
  <omarchy-icon-button icon="\u{F0CD}"></omarchy-icon-button>
  <omarchy-toolbar-separator></omarchy-toolbar-separator>
  <omarchy-icon-button icon="\u{F0CA}"></omarchy-icon-button>
  <omarchy-icon-button icon="\u{F0CB}"></omarchy-icon-button>
  <omarchy-toolbar-separator></omarchy-toolbar-separator>
  <omarchy-icon-button icon="\u{F03E}"></omarchy-icon-button>
  <omarchy-icon-button icon="\u{F0C1}"></omarchy-icon-button>
  <omarchy-heading slot="center" text="notes.md" centered></omarchy-heading>
  <omarchy-button slot="trailing" text="Share" bordered></omarchy-button>
</omarchy-toolbar>
<omarchy-tabs class="fill" current-index="1" cursor-index="2" model='["General","Display","Audio","Network"]'></omarchy-tabs>
<div class="row" style="gap: 0">
  <omarchy-icon-rail id="nav-rail" current-index="1" model='${JSON.stringify(rail)}'></omarchy-icon-rail>
  <div class="nav-page"><span class="text">Search</span></div>
</div>
<style>
  .nav-page { width: round(calc(320px * var(--omarchy-space-scale)), 1px); align-self: stretch; position: relative; }
  .nav-page > span { position: absolute; left: 50%; top: 50%; transform: translate(-50%, -50%); color: var(--omarchy-text-dim); }
</style>`,
};

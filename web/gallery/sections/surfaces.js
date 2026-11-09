export default {
  title: "Surfaces",
  html: `
<div class="row top" style="gap: var(--omarchy-space-huge)">
  <omarchy-card title="Network" style="width: 300px">
    <omarchy-toggle label="Wi-Fi" description="Basecamp" checked style="width: auto"></omarchy-toggle>
    <omarchy-separator></omarchy-separator>
    <div class="col fill">
      <omarchy-menu-row text="Basecamp" checked></omarchy-menu-row>
      <omarchy-menu-row text="Basecamp Guest" highlighted></omarchy-menu-row>
      <omarchy-menu-row text="Coffee Shop"></omarchy-menu-row>
    </div>
  </omarchy-card>
  <div class="col" style="gap: var(--omarchy-space-huge)">
    <omarchy-tooltip held text="Restart Wi-Fi"></omarchy-tooltip>
    <div style="position: relative; width: 340px; height: 170px">
      <omarchy-confirm-dialog open data-no-autofocus message="Remove the Coffee Shop network?"></omarchy-confirm-dialog>
    </div>
  </div>
</div>
<div class="row top" style="gap: var(--omarchy-space-huge)">
  <omarchy-pane title="Layers" subtitle="3 items" divider="left" style="width: 240px">
    <omarchy-icon-button slot="actions" icon="\u{F067}"></omarchy-icon-button>
    <div class="col fill">
      <omarchy-menu-row text="Background"></omarchy-menu-row>
      <omarchy-menu-row text="Title" checked></omarchy-menu-row>
      <omarchy-menu-row text="Logo"></omarchy-menu-row>
    </div>
  </omarchy-pane>
  <omarchy-pane title="Night light" subtitle="Sunset to sunrise" bordered style="width: 240px">
    <omarchy-toggle-switch slot="actions" checked></omarchy-toggle-switch>
    <omarchy-setting-row label="Strength" style="width: auto">
      <omarchy-slider style="width: 100px" value="0.6"></omarchy-slider>
    </omarchy-setting-row>
  </omarchy-pane>
</div>
<div class="row top" style="gap: var(--omarchy-space-huge)">
  <div class="col" style="gap: var(--omarchy-space-huge); width: 240px">
    <omarchy-heading class="fill" text="Settings" subtitle="Omarchy" level="display"></omarchy-heading>
    <omarchy-heading class="fill" text="Appearance" subtitle="Theme, font and rounding" level="heading"></omarchy-heading>
  </div>
  <div class="col" style="gap: var(--omarchy-space-huge); width: 240px">
    <omarchy-heading class="fill" text="Frame"><omarchy-icon-button icon="\u{F141}"></omarchy-icon-button></omarchy-heading>
    <omarchy-heading class="fill" text="notes.md" subtitle="~/Documents" centered></omarchy-heading>
  </div>
</div>`,
};

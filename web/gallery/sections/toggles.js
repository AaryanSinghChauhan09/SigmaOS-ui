export default {
  title: "Toggles",
  html: `
<div class="row" style="gap: var(--omarchy-space-xxl)">
  <omarchy-toggle-switch></omarchy-toggle-switch>
  <omarchy-toggle-switch checked></omarchy-toggle-switch>
  <omarchy-toggle-switch cursor></omarchy-toggle-switch>
  <omarchy-toggle-switch checked cursor></omarchy-toggle-switch>
</div>
<omarchy-toggle style="width: 360px" label="Wi-Fi" description="Connected to Basecamp" checked></omarchy-toggle>
<omarchy-toggle style="width: 360px" label="Bluetooth" description="Off" cursor></omarchy-toggle>`,
};

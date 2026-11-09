export default {
  title: "Inputs",
  html: `
<div class="row" style="gap: var(--omarchy-space-control-gap)">
  <omarchy-text-field style="width: 220px" placeholder="Search…"></omarchy-text-field>
  <omarchy-text-field style="width: 220px" value="Omarchy"></omarchy-text-field>
  <omarchy-text-field style="width: 220px" value="Hovered" cursor></omarchy-text-field>
</div>
<div class="row" style="gap: var(--omarchy-space-xxl)">
  <omarchy-number-field label="Gaps" value="10" from="0" to="40"></omarchy-number-field>
  <omarchy-dropdown label="Theme" value="tokyo-night" options='[{"value":"tokyo-night","label":"Tokyo Night"},{"value":"nord","label":"Nord"}]'></omarchy-dropdown>
</div>`,
};

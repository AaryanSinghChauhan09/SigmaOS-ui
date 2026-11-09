// qt/gallery/sections/forms.qml
export default {
  title: "Forms",
  html: `
<div class="col" style="width: 480px">
  <omarchy-setting-row style="width: 480px" label="Night light" description="Warm the screen after sunset">
    <omarchy-toggle-switch checked></omarchy-toggle-switch>
  </omarchy-setting-row>
  <omarchy-setting-row style="width: 480px" label="Theme" description="Applies to every app">
    <omarchy-dropdown hide-label style="width: 180px" value="tokyo-night" options='[{"value":"tokyo-night","label":"Tokyo Night"},{"value":"nord","label":"Nord"}]'></omarchy-dropdown>
  </omarchy-setting-row>
  <omarchy-setting-row style="width: 480px" label="Gaps" description="Space between windows">
    <omarchy-number-field value="10" from="0" to="40"></omarchy-number-field>
  </omarchy-setting-row>
</div>
<div class="row" style="gap: calc(var(--omarchy-space-huge) * 2); align-items: flex-start">
  <div class="col" style="gap: var(--omarchy-space-lg)">
    <omarchy-checkbox label="Show seconds"></omarchy-checkbox>
    <omarchy-checkbox label="Show date" checked></omarchy-checkbox>
    <omarchy-checkbox label="Show weekday" cursor></omarchy-checkbox>
    <omarchy-checkbox label="Show week number" disabled></omarchy-checkbox>
    <omarchy-checkbox label="Twelve-hour clock" description="Show AM and PM" checked></omarchy-checkbox>
  </div>
  <omarchy-radio-group value="bottom" options='[{"value":"top","label":"Top"},{"value":"bottom","label":"Bottom"},{"value":"hidden","label":"Hidden"}]'></omarchy-radio-group>
</div>
<div class="row" style="gap: var(--omarchy-space-control-gap); align-items: flex-start">
  <omarchy-password-field style="width: 220px" placeholder="Password"></omarchy-password-field>
  <omarchy-password-field style="width: 220px" text="hunter22" revealable></omarchy-password-field>
  <omarchy-password-field style="width: 220px" text="letmein" error="Wrong password, try again"></omarchy-password-field>
</div>
<omarchy-text-area id="forms-notes" style="width: 480px"></omarchy-text-area>`,
  setup(body) {
    body.querySelector("#forms-notes").text = "Back up your dotfiles\nbefore switching themes,\nthen run omarchy update.";
  },
};

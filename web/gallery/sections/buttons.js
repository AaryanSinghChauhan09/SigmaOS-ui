export default {
  title: "Buttons",
  html: `
<div class="row" style="gap: var(--omarchy-space-control-gap)">
  <omarchy-button text="Plain"></omarchy-button>
  <omarchy-button text="Bordered" bordered></omarchy-button>
  <omarchy-button text="Hover" bordered cursor></omarchy-button>
  <omarchy-button text="Selected" bordered selected></omarchy-button>
  <omarchy-button text="Icon" icon="\u{F013}" bordered></omarchy-button>
  <omarchy-icon-button icon="\u{F011}" bordered></omarchy-icon-button>
  <omarchy-icon-button icon="\u{F021}" bordered cursor></omarchy-icon-button>
</div>
<omarchy-button-group value="center" options='[{"value":"left","label":"Left"},{"value":"center","label":"Center"},{"value":"right","label":"Right"}]'></omarchy-button-group>`,
};

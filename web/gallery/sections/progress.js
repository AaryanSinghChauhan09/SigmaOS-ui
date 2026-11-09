// qt/gallery/sections/progress.qml
export default {
  title: "Progress",
  html: `
<div class="row" style="gap: var(--omarchy-space-huge)">
  <omarchy-meter style="width: 220px" value="0.2" markers="[0.5, 0.8]"></omarchy-meter>
  <omarchy-meter style="width: 220px" value="0.65" markers="[0.5, 0.8]"></omarchy-meter>
  <omarchy-meter style="width: 220px" value="0.92" alarming markers="[0.5, 0.8]"></omarchy-meter>
</div>
<omarchy-meter style="width: 360px" value="0.65" label="Battery" value-text="65%"></omarchy-meter>
<div class="row" style="gap: var(--omarchy-space-huge)">
  <omarchy-progress-bar style="width: 340px" value="0.4"></omarchy-progress-bar>
  <omarchy-progress-bar style="width: 340px" indeterminate></omarchy-progress-bar>
</div>
<div class="row" style="gap: var(--omarchy-space-huge)">
  <omarchy-spinner></omarchy-spinner>
  <omarchy-spinner size="icon-large"></omarchy-spinner>
  <omarchy-spinner pulse></omarchy-spinner>
  <omarchy-spinner pulse size="icon-large"></omarchy-spinner>
</div>`,
};

// Popups in the top layer (the Popover API), so no overflow or stacking context clips them: the
// dropdowns' lists, menus, tooltips. Positioned from their anchor's box when they open.

import { surfaceBorder } from "./base.js";

/** CSS for a [popover] drawn on a surface (popups, tooltip, menu) of spec/tokens.md. */
export function popoverCss(surface, sel = ".pop") {
  return `
${sel} {
  position: fixed; inset: auto; margin: 0; overflow: visible;
  box-sizing: border-box;
  background: var(--omarchy-${surface}-background);
  color: var(--omarchy-${surface}-text);
  ${surfaceBorder(surface)}
  border-radius: var(--omarchy-radius);
  font-family: var(--omarchy-font-family);
}
${sel}:not(:popover-open) { display: none; }
`;
}

/** Place a fixed-position popup below (or above, when it doesn't fit) `anchor`, `gap` px away,
 *  left-aligned (or centered). */
export function place(pop, anchor, { gap = 2, align = "start", side = "below", width } = {}) {
  const a = anchor.getBoundingClientRect();
  if (width !== undefined) pop.style.width = `${width}px`;
  const p = pop.getBoundingClientRect();
  const vw = document.documentElement.clientWidth;
  const vh = document.documentElement.clientHeight;
  let x = align === "center" ? a.left + (a.width - p.width) / 2 : align === "end" ? a.right - p.width : a.left;
  let y = side === "above" ? a.top - gap - p.height : a.bottom + gap;
  if (side === "right") {
    x = a.right + gap;
    y = a.top;
  }
  if (side === "below" && y + p.height > vh && a.top - gap - p.height >= 0) y = a.top - gap - p.height;
  if (side === "above" && y < 0) y = a.bottom + gap;
  x = Math.max(0, Math.min(x, vw - p.width));
  y = Math.max(0, Math.min(y, vh - p.height));
  pop.style.left = `${Math.round(x)}px`;
  pop.style.top = `${Math.round(y)}px`;
}

/** Whether the Popover API is open on `el`. */
export const isOpen = (el) => {
  try {
    return el.matches(":popover-open");
  } catch {
    return false;
  }
};

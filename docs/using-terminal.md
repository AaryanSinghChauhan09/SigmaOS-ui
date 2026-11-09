# Omarchy UI in the terminal

Terminal apps get the same tokens and components as GUI apps, drawn on the terminal's cell grid. [`spec/terminal.md`](../spec/terminal.md) is the contract: how colors composite and degrade (24-bit, 256, 16), what the metrics are in cells, how corners follow Hyprland's rounding, how states, type, icons and motion map, every component's cell form, and every gallery section's size. Bindings:

| Toolkit | Read | Status |
|---|---|---|
| Ratatui (Rust) | [using-ratatui.md](using-ratatui.md) | `omarchy-ratatui`: one `omarchy_ratatui::run(app)`; the reference of the terminal family |
| Bubble Tea + Lip Gloss (Go) | | planned |
| Textual (Python) | [using-textual.md](using-textual.md) | `omarchy-textual`: `OmarchyApp` (or `follow(app)`), the kit as Textual widgets, stock widgets restyled |
| Anything else | [using-c.md](using-c.md) | tokens from `libomarchy-theme` or `omarchy-theme --watch --state`; draw per `spec/terminal.md` |

## What reads as Omarchy in a terminal

- **The page is the terminal's.** Never paint the theme background: cells that would be `palette.background` are written with the default background, so the terminal's opacity shows. Surfaces are their borders.
- **Controls are one row of foreground glass.** A faint fill across the control and thin `▏ ▕` edges on its outer sides; stronger under the cursor; accent only for what is chosen; bold only for selection and titles.
- **Corners come from Hyprland**: `┌┐└┘` at radius 0, `╭╮╰╯` above. Light lines always.
- **Dense**: lists are a row per item, gaps are one row or one or two columns.
- **Keyboard first**, with the mouse (clicks, hover) where the terminal reports it.
- **Follow the theme live**, and fade colors on a switch in truecolor.

## A new terminal binding

1. **Tokens from the core**: link `libomarchy-theme` (cgo, cffi/ctypes) or spawn `omarchy-theme --watch --state` and read a JSON line per change (the first line is the initial state). Never a resolver of your own.
2. **Follow live** ([`spec/following.md`](../spec/following.md)): one call for the app, a public changed hook (once per change, after the new tokens are in effect, on the UI loop, never for the initial load), a revision, the 280 ms fade at 24-bit by redrawing.
3. **Every component** under its name, drawn as `spec/terminal.md` says, and a **gallery** with every section at its size: `--list-sections`; `--snap out.ans --section NAME --size COLSxROWS` writing the canonical snapshot from your own frame buffer; the interactive window (`[`/`]`, Tab, mouse, `q`).
4. **Check yourself against Ratatui** while you work: `tui/ratatui/target/release/omarchy-ratatui-gallery --snap ref.ans --section lists`, then yours at `$(target/release/omarchy-ansi size ref.ans)`, then `target/release/omarchy-ansi diff ref.ans mine.ans --png diff.png` (and `omarchy-ansi png` both to look). `omarchy-ansi canon mine.ans | cmp - mine.ans` checks your writer is canonical.
5. **Tests** in your registry entry: behavior (the keys and clicks the other galleries' tests drive), a live switch under `tools/live/run`, with and without `OMARCHY_UI_REDUCE_MOTION=1`.
6. **The registry file** with `"family": "terminal"`, `"tokens": "core"`, `"icons": "none"` ([`spec/terminal.md`](../spec/terminal.md#harness)); then `bin/review --bindings <name>` shows it against Ratatui, cell by cell and pixel by pixel, with Qt beside them.

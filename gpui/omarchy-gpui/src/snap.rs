//! Stills with no window on screen (the `snap` feature): a view rendered offscreen to a PNG at 2×,
//! in the theme as it is now (or `OMARCHY_UI_*`'s fixture), for review and docs.
//!
//! GPUI's headless app context: the test platform's windows (never shown), Linux's real text
//! system, and the wgpu headless renderer reading the frame back. It needs a GPU adapter (any
//! Vulkan or GL driver, llvmpipe included) but no display server.
//!
//! ```ignore
//! fn main() {
//!     // `app --snap out.png [--size WxH]` renders and exits; otherwise run as usual.
//!     if omarchy_gpui::snap::from_args((400.0, 283.0), |_, cx| Network::new(cx)) {
//!         return;
//!     }
//!     omarchy_gpui::gpui_platform::application().run(|cx| { /* ... */ });
//! }
//! ```

use std::path::Path;
use std::sync::Arc;

use gpui::{AppContext as _, Context, HeadlessAppContext, PlatformHeadlessRenderer, Render, Window, px, size};

/// Render the view `build` makes to the PNG `out`, `w`×`h` points at 2× (GPUI's test windows are
/// 2×), with [`crate::install`]ed tokens (the current theme, not followed).
pub fn render<V: Render + 'static>(out: &Path, (w, h): (f32, f32), build: impl FnOnce(&mut Window, &mut Context<V>) -> V) -> anyhow::Result<()> {
    let text = Arc::new(gpui_wgpu::CosmicTextSystem::new("IBM Plex Sans"));
    let mut cx = HeadlessAppContext::with_platform(text, Arc::new(()), || {
        gpui_wgpu::WgpuHeadlessRenderer::new().map(|r| Some(Box::new(r) as Box<dyn PlatformHeadlessRenderer>))
    });
    cx.update(|cx| crate::install(cx, omarchy_theme::load()));
    let window = cx.open_window(size(px(w), px(h)), |window, cx| cx.new(|cx| build(window, cx)))?;
    cx.run_until_parked();
    // A second frame, for anything laid out from the first (anchored popups, measured text).
    cx.update_window(window.into(), |_, window, _| window.refresh())?;
    cx.run_until_parked();
    let img = cx.capture_screenshot(window.into())?;
    img.save(out)?;
    Ok(())
}

/// `--snap OUT [--size WxH]` on the command line: render the view to OUT at that size
/// (`default` otherwise) and return true (exiting 1 on failure); without `--snap`, false.
pub fn from_args<V: Render + 'static>(default: (f32, f32), build: impl FnOnce(&mut Window, &mut Context<V>) -> V) -> bool {
    let args: Vec<String> = std::env::args().collect();
    let arg = |name: &str| args.iter().position(|a| a == name).and_then(|i| args.get(i + 1).cloned());
    let Some(out) = arg("--snap") else { return false };
    let size = arg("--size")
        .and_then(|s| s.split_once('x').and_then(|(w, h)| Some((w.parse().ok()?, h.parse().ok()?))))
        .unwrap_or(default);
    if let Err(e) = render(Path::new(&out), size, build) {
        eprintln!("snap: {e:#}");
        std::process::exit(1);
    }
    true
}

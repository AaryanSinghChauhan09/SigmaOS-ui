//! The live-switch test (spec/following.md), run under `tools/live/run`: a GPUI app (Linux's
//! headless platform: the real executor, no window) calls `omarchy_gpui::init`, the theme is
//! switched the way `omarchy theme set` does it, and the app must see catppuccin-latte through
//! the public API, the changed hook (and `observe_global::<Theme>`) once with the new tokens,
//! revision 1, and a fade that ran and landed exactly; under `OMARCHY_UI_REDUCE_MOTION=1`, no
//! fade at all.

use std::cell::RefCell;
use std::rc::Rc;
use std::time::{Duration, Instant};

use omarchy_gpui::gpui::App;
use omarchy_gpui::omarchy_theme::Rgba;
use omarchy_gpui::{Paint, Theme};

const LATTE: &str = "#eff1f5ff";

fn fail(msg: String) -> ! {
    eprintln!("live switch: FAIL: {msg}");
    std::process::exit(1);
}

fn main() {
    let home = std::env::var("OMARCHY_UI_LIVE_HOME").unwrap_or_else(|_| fail("run me under tools/live/run".into()));
    let switch = std::env::var("OMARCHY_UI_LIVE_SWITCH").unwrap_or_else(|_| fail("no OMARCHY_UI_LIVE_SWITCH".into()));
    let reduced = std::env::var("OMARCHY_UI_REDUCE_MOTION").is_ok_and(|v| v == "1");

    omarchy_gpui::gpui_platform::headless().run(move |cx: &mut App| {
        omarchy_gpui::init(cx);
        let start_bg = Theme::get(cx).tokens().palette.background;
        let hooks: Rc<RefCell<Vec<String>>> = Rc::default();
        let observed = Rc::new(RefCell::new(0usize));
        let h = hooks.clone();
        omarchy_gpui::on_change(cx, move |tokens, cx| {
            // After the new tokens are in effect: the public API already reads them.
            assert_eq!(Theme::get(cx).tokens().palette.background, tokens.palette.background);
            h.borrow_mut().push(tokens.palette.background.hex());
        })
        .detach();
        let o = observed.clone();
        cx.observe_global::<Theme>(move |_| *o.borrow_mut() += 1).detach();

        cx.spawn(async move |cx| {
            let timer = |ms: u64| cx.background_executor().timer(Duration::from_millis(ms));
            timer(300).await;
            let status = std::process::Command::new(&switch).arg(&home).arg("catppuccin-latte").status();
            if !status.is_ok_and(|s| s.success()) {
                fail(format!("{switch} failed"));
            }
            let began = Instant::now();
            let (mut faded, mut between) = (false, false);
            let target = loop {
                let (bg, painted, fading) = cx.update(|cx| {
                    (Theme::get(cx).tokens().palette.background, Paint::of(cx).tokens().palette.background, Paint::fading(cx))
                });
                faded |= fading;
                between |= painted != bg && painted != start_bg && bg.hex() == LATTE;
                if bg.hex() == LATTE && !fading {
                    break bg;
                }
                if began.elapsed() > Duration::from_secs(5) {
                    fail(format!("background is {} after 5 s, not {LATTE}", bg.hex()));
                }
                timer(4).await;
            };
            let painted: Rgba = cx.update(|cx| Paint::of(cx).tokens().palette.background);
            if painted != target {
                fail(format!("the fade landed on {}, not {LATTE}", painted.hex()));
            }
            timer(600).await;
            let (revision, hook, seen) = (cx.update(|cx| Theme::get(cx).revision()), hooks.borrow().clone(), *observed.borrow());
            if hook != [LATTE] {
                fail(format!("changed hook fired {hook:?}, want once with {LATTE}"));
            }
            if seen != 1 {
                fail(format!("observe_global::<Theme> fired {seen} times, want 1"));
            }
            if revision != 1 {
                fail(format!("revision {revision}, want 1"));
            }
            if reduced && (faded || between) {
                fail("a fade ran under reduced motion".into());
            }
            if !reduced && !(faded && between) {
                fail(format!("no fade seen (fading {faded}, a color between {between})"));
            }
            println!(
                "live switch: {LATTE} in {} ms, hook once, revision 1, {}",
                began.elapsed().as_millis(),
                if reduced { "no fade (reduced motion)" } else { "faded and landed exactly" }
            );
            std::process::exit(0);
        })
        .detach();
    });
}

//! Box drawing, block elements and braille drawn as pixels, as terminals draw them, so lines
//! join across cells and blocks meet their neighbours without gaps.

use crate::render::Image;

/// Arm weights: left, right, up, down. 0 none, 1 light, 2 heavy.
fn arms(c: char) -> Option<[u8; 4]> {
    let a = |l, r, u, d| Some([l, r, u, d]);
    match c as u32 {
        0x2500 => a(1, 1, 0, 0),
        0x2501 => a(2, 2, 0, 0),
        0x2502 => a(0, 0, 1, 1),
        0x2503 => a(0, 0, 2, 2),
        0x250C => a(0, 1, 0, 1),
        0x250D => a(0, 2, 0, 1),
        0x250E => a(0, 1, 0, 2),
        0x250F => a(0, 2, 0, 2),
        0x2510 => a(1, 0, 0, 1),
        0x2511 => a(2, 0, 0, 1),
        0x2512 => a(1, 0, 0, 2),
        0x2513 => a(2, 0, 0, 2),
        0x2514 => a(0, 1, 1, 0),
        0x2515 => a(0, 2, 1, 0),
        0x2516 => a(0, 1, 2, 0),
        0x2517 => a(0, 2, 2, 0),
        0x2518 => a(1, 0, 1, 0),
        0x2519 => a(2, 0, 1, 0),
        0x251A => a(1, 0, 2, 0),
        0x251B => a(2, 0, 2, 0),
        0x251C => a(0, 1, 1, 1),
        0x251D => a(0, 2, 1, 1),
        0x251E => a(0, 1, 2, 1),
        0x251F => a(0, 1, 1, 2),
        0x2520 => a(0, 1, 2, 2),
        0x2521 => a(0, 2, 2, 1),
        0x2522 => a(0, 2, 1, 2),
        0x2523 => a(0, 2, 2, 2),
        0x2524 => a(1, 0, 1, 1),
        0x2525 => a(2, 0, 1, 1),
        0x2526 => a(1, 0, 2, 1),
        0x2527 => a(1, 0, 1, 2),
        0x2528 => a(1, 0, 2, 2),
        0x2529 => a(2, 0, 2, 1),
        0x252A => a(2, 0, 1, 2),
        0x252B => a(2, 0, 2, 2),
        0x252C => a(1, 1, 0, 1),
        0x252D => a(2, 1, 0, 1),
        0x252E => a(1, 2, 0, 1),
        0x252F => a(2, 2, 0, 1),
        0x2530 => a(1, 1, 0, 2),
        0x2531 => a(2, 1, 0, 2),
        0x2532 => a(1, 2, 0, 2),
        0x2533 => a(2, 2, 0, 2),
        0x2534 => a(1, 1, 1, 0),
        0x2535 => a(2, 1, 1, 0),
        0x2536 => a(1, 2, 1, 0),
        0x2537 => a(2, 2, 1, 0),
        0x2538 => a(1, 1, 2, 0),
        0x2539 => a(2, 1, 2, 0),
        0x253A => a(1, 2, 2, 0),
        0x253B => a(2, 2, 2, 0),
        0x253C => a(1, 1, 1, 1),
        0x253D => a(2, 1, 1, 1),
        0x253E => a(1, 2, 1, 1),
        0x253F => a(2, 2, 1, 1),
        0x2540 => a(1, 1, 2, 1),
        0x2541 => a(1, 1, 1, 2),
        0x2542 => a(1, 1, 2, 2),
        0x2543 => a(2, 1, 2, 1),
        0x2544 => a(1, 2, 2, 1),
        0x2545 => a(2, 1, 1, 2),
        0x2546 => a(1, 2, 1, 2),
        0x2547 => a(2, 2, 2, 1),
        0x2548 => a(2, 2, 1, 2),
        0x2549 => a(2, 1, 2, 2),
        0x254A => a(1, 2, 2, 2),
        0x254B => a(2, 2, 2, 2),
        0x2574 => a(1, 0, 0, 0),
        0x2575 => a(0, 0, 1, 0),
        0x2576 => a(0, 1, 0, 0),
        0x2577 => a(0, 0, 0, 1),
        0x2578 => a(2, 0, 0, 0),
        0x2579 => a(0, 0, 2, 0),
        0x257A => a(0, 2, 0, 0),
        0x257B => a(0, 0, 0, 2),
        0x257C => a(1, 2, 0, 0),
        0x257D => a(0, 0, 1, 2),
        0x257E => a(2, 1, 0, 0),
        0x257F => a(0, 0, 2, 1),
        _ => None,
    }
}

/// Dashed lines: (horizontal, segments, heavy).
fn dashes(c: char) -> Option<(bool, i64, bool)> {
    Some(match c as u32 {
        0x2504 => (true, 3, false),
        0x2505 => (true, 3, true),
        0x2506 => (false, 3, false),
        0x2507 => (false, 3, true),
        0x2508 => (true, 4, false),
        0x2509 => (true, 4, true),
        0x250A => (false, 4, false),
        0x250B => (false, 4, true),
        0x254C => (true, 2, false),
        0x254D => (true, 2, true),
        0x254E => (false, 2, false),
        0x254F => (false, 2, true),
        _ => return None,
    })
}

/// Draw `c` into the cell at (`x0`, `y0`), `w` × `h` pixels, if it's one of the characters drawn
/// as pixels; false leaves it to the font. `t` is the light line thickness.
#[allow(clippy::too_many_arguments)]
pub(crate) fn draw(img: &mut Image, c: char, x0: i64, y0: i64, w: i64, h: i64, t: i64, ink: [u8; 3]) -> bool {
    let thick = |weight: u8| if weight >= 2 { 2 * t } else { t };
    // A stroke of thickness `s` centered across a length `len`: its start.
    let center = |len: i64, s: i64| (len - s) / 2;
    let rect = |img: &mut Image, l: i64, tp: i64, r: i64, b: i64, a: f32| img.fill(x0 + l, y0 + tp, x0 + r, y0 + b, ink, a);

    if let Some([l, r, u, d]) = arms(c) {
        // The stroke the horizontal arms meet the vertical ones in.
        let vs = thick(u.max(d)).max(if u.max(d) == 0 { thick(l.max(r)) } else { 0 });
        let hs = thick(l.max(r)).max(if l.max(r) == 0 { thick(u.max(d)) } else { 0 });
        let vx = center(w, vs);
        let hy = center(h, hs);
        for (weight, horizontal, first) in [(l, true, true), (r, true, false), (u, false, true), (d, false, false)] {
            if weight == 0 {
                continue;
            }
            let s = thick(weight);
            if horizontal {
                let y = center(h, s);
                let (a, b) = if first { (0, vx + vs) } else { (vx, w) };
                rect(img, a, y, b, y + s, 1.0);
            } else {
                let x = center(w, s);
                let (a, b) = if first { (0, hy + hs) } else { (hy, h) };
                rect(img, x, a, x + s, b, 1.0);
            }
        }
        return true;
    }

    if let Some((horizontal, n, heavy)) = dashes(c) {
        let s = if heavy { 2 * t } else { t };
        let len = if horizontal { w } else { h };
        let gap = (len / (n * 4)).max(1);
        for i in 0..n {
            let a = i * len / n + gap / 2 + gap % 2;
            let b = (i + 1) * len / n - gap / 2;
            if horizontal {
                let y = center(h, s);
                rect(img, a, y, b, y + s, 1.0);
            } else {
                let x = center(w, s);
                rect(img, x, a, x + s, b, 1.0);
            }
        }
        return true;
    }

    if matches!(c as u32, 0x256D..=0x2570) {
        // A quarter circle joining two arms: the vertical arm's center x, the horizontal arm's
        // center y, radius the distance to the cell's nearer side edge.
        let vx = center(w, t);
        let hy = center(h, t);
        let cx = vx as f32 + t as f32 / 2.0;
        let cy = hy as f32 + t as f32 / 2.0;
        let r = cx.min(w as f32 - cx);
        let (right, down) = match c as u32 {
            0x256D => (true, true),   // ╭
            0x256E => (false, true),  // ╮
            0x256F => (false, false), // ╯
            _ => (true, false),       // ╰
        };
        let ccx = if right { cx + r } else { cx - r };
        let ccy = if down { cy + r } else { cy - r };
        // The straight part of the vertical arm, beyond the arc.
        if down {
            rect(img, vx, ccy.round() as i64, vx + t, h, 1.0);
        } else {
            rect(img, vx, 0, vx + t, ccy.round() as i64, 1.0);
        }
        let half = t as f32 / 2.0;
        for py in 0..h {
            for px in 0..w {
                let (fx, fy) = (px as f32 + 0.5, py as f32 + 0.5);
                let in_x = if right { fx <= ccx } else { fx >= ccx };
                let in_y = if down { fy <= ccy } else { fy >= ccy };
                if !(in_x && in_y) {
                    continue;
                }
                let d = ((fx - ccx).powi(2) + (fy - ccy).powi(2)).sqrt();
                let cov = (half + 0.5 - (d - r).abs()).clamp(0.0, 1.0);
                if cov > 0.0 {
                    img.blend(x0 + px, y0 + py, ink, cov);
                }
            }
        }
        return true;
    }

    let n = c as u32;
    if (0x2580..=0x259F).contains(&n) {
        // Eighths from the bottom and the left.
        let lower = |k: i64| h - (h * k + 4) / 8;
        let left = |k: i64| (w * k + 4) / 8;
        let (hx, hy) = (left(4), lower(4));
        match n {
            0x2580 => rect(img, 0, 0, w, hy, 1.0),
            0x2581..=0x2587 => rect(img, 0, lower((n - 0x2580) as i64), w, h, 1.0),
            0x2588 => rect(img, 0, 0, w, h, 1.0),
            0x2589..=0x258F => rect(img, 0, 0, left((0x2590 - n) as i64), h, 1.0),
            0x2590 => rect(img, hx, 0, w, h, 1.0),
            0x2591 => rect(img, 0, 0, w, h, 0.25),
            0x2592 => rect(img, 0, 0, w, h, 0.5),
            0x2593 => rect(img, 0, 0, w, h, 0.75),
            0x2594 => rect(img, 0, 0, w, h - lower(1), 1.0),
            0x2595 => rect(img, w - left(1), 0, w, h, 1.0),
            _ => {
                // Quadrants: upper left, upper right, lower left, lower right.
                let q = match n {
                    0x2596 => [false, false, true, false],
                    0x2597 => [false, false, false, true],
                    0x2598 => [true, false, false, false],
                    0x2599 => [true, false, true, true],
                    0x259A => [true, false, false, true],
                    0x259B => [true, true, true, false],
                    0x259C => [true, true, false, true],
                    0x259D => [false, true, false, false],
                    0x259E => [false, true, true, false],
                    _ => [false, true, true, true],
                };
                let boxes = [(0, 0, hx, hy), (hx, 0, w, hy), (0, hy, hx, h), (hx, hy, w, h)];
                for (on, (l, tp, r, b)) in q.iter().zip(boxes) {
                    if *on {
                        rect(img, l, tp, r, b, 1.0);
                    }
                }
            }
        }
        return true;
    }

    if (0x2800..=0x28FF).contains(&n) {
        let bits = n - 0x2800;
        // Dot (column, row) for bits 0..8: 1 2 3 then 4 5 6 down the columns, then 7 and 8.
        const DOTS: [(i64, i64); 8] = [(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2), (0, 3), (1, 3)];
        let d = (w as f32 / 4.0).round().max(1.0);
        let rad = d / 2.0;
        for (i, (col, row)) in DOTS.iter().enumerate() {
            if bits & (1 << i) == 0 {
                continue;
            }
            let cx = w as f32 * (1 + 2 * col) as f32 / 4.0;
            let cy = h as f32 * (1 + 2 * row) as f32 / 8.0;
            let (l, r) = ((cx - rad).floor() as i64 - 1, (cx + rad).ceil() as i64 + 1);
            let (tp, b) = ((cy - rad).floor() as i64 - 1, (cy + rad).ceil() as i64 + 1);
            for py in tp..b {
                for px in l..r {
                    let dist = ((px as f32 + 0.5 - cx).powi(2) + (py as f32 + 0.5 - cy).powi(2)).sqrt();
                    let cov = (rad + 0.5 - dist).clamp(0.0, 1.0);
                    if cov > 0.0 && px >= 0 && px < w && py >= 0 && py < h {
                        img.blend(x0 + px, y0 + py, ink, cov);
                    }
                }
            }
        }
        return true;
    }
    false
}

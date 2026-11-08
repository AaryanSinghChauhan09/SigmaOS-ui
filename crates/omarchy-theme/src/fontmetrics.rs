//! A font's line height as a multiple of its size, read from its `hhea` and `head` tables: the
//! ascent plus descent (plus line gap) that Qt's `Text` uses for its height, so frameworks that
//! default to another ratio (Rune's 1.25) can match Qt's control heights.

use std::path::Path;

fn u16_at(b: &[u8], o: usize) -> Option<u16> {
    Some(u16::from_be_bytes([*b.get(o)?, *b.get(o + 1)?]))
}

fn i16_at(b: &[u8], o: usize) -> Option<i16> {
    u16_at(b, o).map(|v| v as i16)
}

fn table<'a>(b: &'a [u8], tag: &[u8; 4]) -> Option<&'a [u8]> {
    let n = u16_at(b, 4)? as usize;
    for i in 0..n {
        let rec = 12 + i * 16;
        if b.get(rec..rec + 4)? == tag {
            let off = u32::from_be_bytes(b.get(rec + 8..rec + 12)?.try_into().ok()?) as usize;
            let len = u32::from_be_bytes(b.get(rec + 12..rec + 16)?.try_into().ok()?) as usize;
            return b.get(off..off + len);
        }
    }
    None
}

/// `(ascent + descent + line gap) / units per em`, or None for a file this can't read (a
/// collection, a WOFF).
pub fn line_height_ratio(path: &Path) -> Option<f64> {
    let b = std::fs::read(path).ok()?;
    let head = table(&b, b"head")?;
    let hhea = table(&b, b"hhea")?;
    let upem = u16_at(head, 18)? as f64;
    let ascent = i16_at(hhea, 4)? as f64;
    let descent = i16_at(hhea, 6)? as f64;
    let gap = i16_at(hhea, 8)? as f64;
    if upem <= 0.0 {
        return None;
    }
    let r = (ascent - descent + gap.max(0.0)) / upem;
    (0.8..3.0).contains(&r).then_some(r)
}

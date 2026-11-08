//! The editing pieces apps need (roadmap Tier 3): Timeline, Scrubber, TrimHandle, CropFrame,
//! DragHandle, DropIndicator and ColorPicker. Same names, anatomy and metrics as the Qt files
//! of the same names; `spec/components/editing.md` lists them.

mod color;
mod handles;
mod timeline;

pub use color::{ColorPicker, Hsva};
pub use handles::{CropFrame, DragHandle, DropIndicator, Edge, TrimHandle};
pub use timeline::{Clip, Scrubber, Timeline, TimelineView};

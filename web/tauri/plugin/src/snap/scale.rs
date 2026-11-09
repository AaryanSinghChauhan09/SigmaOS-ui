//! The device scale WebKit renders at, in headless runs.
//!
//! GTK 3's broadway backend is always 1×, and a snap is 2×. WebKitGTK takes its device scale
//! factor from `gtk_widget_get_scale_factor` on its view and nowhere else, so this binary exports
//! its own (build.rs) that answers the requested scale for WebKit views and forwards everything
//! else to GTK's. The page then sees `devicePixelRatio` 2 and lays out exactly as on a 2× monitor;
//! GTK itself (window sizes, broadway) stays at 1×. Off unless [`set`] was called.

use std::ffi::{c_int, c_void};
use std::sync::OnceLock;
use std::sync::atomic::{AtomicI32, Ordering};

static SCALE: AtomicI32 = AtomicI32::new(0);

pub fn set(scale: i32) {
    SCALE.store(scale.max(1), Ordering::SeqCst);
}

type Real = unsafe extern "C" fn(*mut c_void) -> c_int;

fn real() -> Option<Real> {
    static REAL: OnceLock<Option<Real>> = OnceLock::new();
    *REAL.get_or_init(|| {
        // SAFETY: dlsym with a NUL-terminated name; RTLD_NEXT finds GTK's own definition.
        let p = unsafe { libc::dlsym(libc::RTLD_NEXT, c"gtk_widget_get_scale_factor".as_ptr()) };
        (!p.is_null()).then(|| unsafe { std::mem::transmute::<*mut c_void, Real>(p) })
    })
}

/// GTK's `gtk_widget_get_scale_factor`, with WebKit's views at [`set`]'s scale.
///
/// # Safety
/// Called by GTK and WebKit with a `GtkWidget *`, as GTK's own is.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn gtk_widget_get_scale_factor(widget: *mut c_void) -> c_int {
    let scale = SCALE.load(Ordering::Relaxed);
    if scale > 0 && !widget.is_null() {
        // SAFETY: widget is a GObject instance (GTK's contract for this function).
        let is_webkit = unsafe {
            gobject_sys::g_type_check_instance_is_a(widget.cast(), webkit2gtk_sys::webkit_web_view_get_type()) != 0
        };
        if is_webkit {
            return scale;
        }
    }
    match real() {
        Some(f) => unsafe { f(widget) },
        None => 1,
    }
}

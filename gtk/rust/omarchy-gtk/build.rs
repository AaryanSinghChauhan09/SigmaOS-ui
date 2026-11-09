// Link libomarchy-gtk-1 through pkg-config (`omarchy-gtk-1`). From a checkout, before installing:
//   PKG_CONFIG_PATH=<repo>/build/gtk/meson-uninstalled:<repo>/build/ffi/lib/pkgconfig cargo build
fn main() {
    if let Err(e) = pkg_config::Config::new().atleast_version("0.1").probe("omarchy-gtk-1") {
        panic!("omarchy-gtk: libomarchy-gtk-1 not found by pkg-config ({e}); build it with gtk/build and set PKG_CONFIG_PATH");
    }
}

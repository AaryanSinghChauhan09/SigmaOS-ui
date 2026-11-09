fn main() {
    // src/scale.rs replaces GTK's gtk_widget_get_scale_factor for WebKit's views in headless runs:
    // export it so libwebkit2gtk binds to ours.
    println!("cargo:rustc-link-arg-bins=-Wl,--export-dynamic-symbol=gtk_widget_get_scale_factor");
    tauri_build::build()
}

fn main() {
    // No Node step in this example: the page loads @omarchy/ui's ES modules as they are, from
    // dist/omarchy-ui (git-ignored). With Vite or another bundler, `import "@omarchy/ui"` in the
    // page and let the bundler put it in your frontendDist instead.
    let web = std::path::Path::new("../../web/src");
    let dest = std::path::Path::new("dist/omarchy-ui");
    let _ = std::fs::remove_dir_all(dest);
    copy(web, dest).expect("copy web/src into dist/omarchy-ui");
    println!("cargo:rerun-if-changed=../../web/src");

    // Lets `--snap` render at 2×: WebKitGTK takes its scale from this GTK function, which the
    // plugin's snap feature answers for headless runs. An app that never snaps leaves it out.
    println!("cargo:rustc-link-arg-bins=-Wl,--export-dynamic-symbol=gtk_widget_get_scale_factor");
    tauri_build::build()
}

fn copy(from: &std::path::Path, to: &std::path::Path) -> std::io::Result<()> {
    std::fs::create_dir_all(to)?;
    for entry in std::fs::read_dir(from)? {
        let entry = entry?;
        let target = to.join(entry.file_name());
        if entry.file_type()?.is_dir() {
            copy(&entry.path(), &target)?;
        } else {
            std::fs::copy(entry.path(), target)?;
        }
    }
    Ok(())
}

//! A private GTK 3 broadway display, so snaps and tests never put a window on the desktop.
//!
//! Tauri on Linux is GTK 3 + WebKitGTK, and a WebKit view only lays out and paints once its
//! window is mapped. Broadway is GTK's own in-memory backend: `broadwayd` keeps the windows and
//! shows them to a browser that connects, and none does. Its client socket goes in a temp
//! directory, as GLib's runtime dir (GLib caches it on first use) while the environment keeps the
//! real `XDG_RUNTIME_DIR`, which the core reads to find Hyprland. Nothing is left behind.
//!
//! The pixels come from WebKit, not from broadway: `webkit_web_view_get_snapshot` has the web
//! process paint the page into a bitmap at the view's device scale ([`crate::scale`] makes that
//! the requested one; broadway itself is always 1×).

use std::path::PathBuf;
use std::os::unix::process::CommandExt;
use std::process::{Child, Command, Stdio};

pub struct Display {
    child: Child,
    dir: PathBuf,
}

impl Drop for Display {
    fn drop(&mut self) {
        let _ = self.child.kill();
        let _ = self.child.wait();
        let _ = std::fs::remove_dir_all(&self.dir);
    }
}

/// Start `broadwayd` on a free display and point GDK at it. Call before anything touches GLib or
/// GTK (the first thing in `main`).
pub fn start() -> Result<Display, String> {
    let dir = std::env::temp_dir().join(format!("omarchy-tauri-{}", std::process::id()));
    std::fs::create_dir_all(&dir).map_err(|e| format!("headless: {e}"))?;
    let real = std::env::var_os("XDG_RUNTIME_DIR");
    // SAFETY: single-threaded, before GTK, GLib or Tauri start.
    unsafe { std::env::set_var("XDG_RUNTIME_DIR", &dir) };
    let runtime = glib::user_runtime_dir();
    unsafe {
        match &real {
            Some(r) => std::env::set_var("XDG_RUNTIME_DIR", r),
            None => std::env::remove_var("XDG_RUNTIME_DIR"),
        }
    }
    if runtime != dir {
        eprintln!("headless: GLib read its runtime dir first; the socket goes to {}", runtime.display());
    }
    for attempt in 0..8u32 {
        let n = 2000 + (std::process::id().wrapping_mul(7).wrapping_add(attempt * 7919) % 40000);
        // Its web server on a unix socket too (no TCP port). The display socket is an abstract
        // one named after GLib's runtime dir, so it leaves no file either.
        let http = runtime.join("http.sock");
        let _ = std::fs::remove_file(&http);
        let mut command = Command::new("broadwayd");
        // SAFETY: prctl is async-signal-safe. broadwayd ends with this process, even on a crash.
        unsafe {
            command.pre_exec(|| {
                libc::prctl(libc::PR_SET_PDEATHSIG, libc::SIGTERM);
                Ok(())
            });
        }
        let mut child = command
            .arg("--unixsocket")
            .arg(&http)
            .arg(format!(":{n}"))
            .env("XDG_RUNTIME_DIR", &runtime)
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(if std::env::var_os("OMARCHY_TAURI_DEBUG").is_some() { Stdio::inherit() } else { Stdio::null() })
            .spawn()
            .map_err(|e| format!("headless: broadwayd: {e} (it comes with gtk3)"))?;
        // Ready once it says where it listens; an exit means the number was taken.
        let mut line = String::new();
        let listening = child.stdout.take().is_some_and(|out| {
            std::io::BufRead::read_line(&mut std::io::BufReader::new(out), &mut line).is_ok() && line.starts_with("Listening on")
        });
        if listening && child.try_wait().ok().flatten().is_none() {
            // SAFETY: still single-threaded (broadwayd is a child process, not a thread).
            unsafe {
                std::env::set_var("GDK_BACKEND", "broadway");
                std::env::set_var("BROADWAY_DISPLAY", format!(":{n}"));
                std::env::remove_var("WAYLAND_DISPLAY");
                std::env::remove_var("DISPLAY");
                // No accessibility bus, no portal, no user settings: renders the same everywhere.
                std::env::set_var("NO_AT_BRIDGE", "1");
                std::env::set_var("GTK_USE_PORTAL", "0");
                std::env::set_var("GSETTINGS_BACKEND", "memory");
                // Software rendering in the web process: no GPU, no DMA-BUF, nothing to share
                // with a compositor that isn't there.
                std::env::set_var("WEBKIT_DISABLE_COMPOSITING_MODE", "1");
                std::env::set_var("WEBKIT_DISABLE_DMABUF_RENDERER", "1");
                std::env::set_var("LIBGL_ALWAYS_SOFTWARE", "1");
            }
            return Ok(Display { child, dir });
        }
        let _ = child.kill();
        let _ = child.wait();
    }
    let _ = std::fs::remove_dir_all(&dir);
    Err("headless: broadwayd didn't start".into())
}

/* headless.c: a private broadway display, so snaps and tests never put a window on the desktop.
 *
 * GTK 4 has no offscreen backend, and a widget only draws once it is mapped. Broadway is GTK's
 * own HTML5 backend: gtk4-broadwayd keeps the windows in memory and shows them to a browser that
 * connects, and none does. It listens on a unix socket in a temp directory (no TCP port), and
 * GDK talks to it through $XDG_RUNTIME_DIR/broadway<N+1>.socket. Rendering is ours: the widget's
 * render nodes go through a Cairo GskRenderer into a texture at the requested scale. */
#include "headless.h"

#include <errno.h>
#include <glib/gstdio.h>
#include <signal.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static GPid daemon_pid;
static char *daemon_dir;
static char *client_socket;
static int viewer_fd = -1;
static GThread *viewer_thread;
static GMutex viewer_lock;
static guint32 last_serial;

/* ---- the viewer: a stand-in for the browser broadway expects ------------------------------
 *
 * GDK's broadway backend freezes a surface's frame clock after every frame until the browser
 * answers the server's ROUNDTRIP op. With no browser, a window paints once and never again (no
 * relayout, no restyle, no fade). This is the smallest browser that keeps frames coming: a
 * websocket client on the daemon's unix socket that walks the binary command stream
 * (broadway.js handleCommands) and answers each ROUNDTRIP, after telling the server the screen's
 * size. It draws nothing. */

enum {
  OP_GRAB_POINTER = 0, OP_UNGRAB_POINTER, OP_NEW_SURFACE, OP_SHOW_SURFACE, OP_HIDE_SURFACE, OP_RAISE_SURFACE,
  OP_LOWER_SURFACE, OP_DESTROY_SURFACE, OP_MOVE_RESIZE, OP_SET_TRANSIENT_FOR, OP_DISCONNECTED, OP_SURFACE_UPDATE,
  OP_SET_SHOW_KEYBOARD, OP_UPLOAD_TEXTURE, OP_RELEASE_TEXTURE, OP_SET_NODES, OP_ROUNDTRIP,
};
enum { EV_SCREEN_SIZE_CHANGED = 12, EV_ROUNDTRIP_NOTIFY = 14 };

static gboolean
write_all (int fd, const guint8 *buf, gsize len)
{
  while (len) {
    ssize_t n = write (fd, buf, len);
    if (n < 0 && errno == EINTR)
      continue;
    if (n <= 0)
      return FALSE;
    buf += n;
    len -= (gsize) n;
  }
  return TRUE;
}

static gboolean
read_all (int fd, guint8 *buf, gsize len)
{
  while (len) {
    ssize_t n = read (fd, buf, len);
    if (n < 0 && errno == EINTR)
      continue;
    if (n <= 0)
      return FALSE;
    buf += n;
    len -= (gsize) n;
  }
  return TRUE;
}

/* sendInput(cmd, args): big-endian int32s [cmd, serial, time, args...], in a masked frame. */
static void
send_input (int cmd, const gint32 *args, int n)
{
  guint8 payload[64];
  gint32 all[16] = { cmd, (gint32) last_serial, (gint32) (g_get_monotonic_time () / 1000) };
  for (int i = 0; i < n; i++)
    all[3 + i] = args[i];
  int len = (3 + n) * 4;
  for (int i = 0; i < 3 + n; i++) {
    guint32 be = GUINT32_TO_BE ((guint32) all[i]);
    memcpy (payload + i * 4, &be, 4);
  }
  guint8 frame[80] = { 0x82, (guint8) (0x80 | len), 0, 0, 0, 0 };
  for (int i = 0; i < len; i++)
    frame[6 + i] = payload[i] ^ frame[2 + (i & 3)];
  g_mutex_lock (&viewer_lock);
  write_all (viewer_fd, frame, 6 + len);
  g_mutex_unlock (&viewer_lock);
}

static guint16 u16 (const guint8 *p) { return (guint16) (p[0] | p[1] << 8); }
static guint32 u32 (const guint8 *p) { return (guint32) p[0] | (guint32) p[1] << 8 | (guint32) p[2] << 16 | (guint32) p[3] << 24; }

/* Walk one message's commands; answer the roundtrips. */
static void
handle_commands (const guint8 *p, gsize len)
{
  gsize pos = 0;
#define NEED(n) if (pos + (n) > len) return
  while (pos < len) {
    NEED (5);
    guint8 op = p[pos];
    last_serial = u32 (p + pos + 1);
    pos += 5;
    switch (op) {
    case OP_GRAB_POINTER: NEED (3); pos += 3; break;
    case OP_UNGRAB_POINTER: case OP_DISCONNECTED: break;
    case OP_NEW_SURFACE: NEED (10); pos += 10; break;
    case OP_SHOW_SURFACE: case OP_HIDE_SURFACE: case OP_RAISE_SURFACE: case OP_LOWER_SURFACE:
    case OP_DESTROY_SURFACE: case OP_SET_SHOW_KEYBOARD: NEED (2); pos += 2; break;
    case OP_SET_TRANSIENT_FOR: NEED (4); pos += 4; break;
    case OP_MOVE_RESIZE: {
      NEED (3);
      guint8 flags = p[pos + 2];
      pos += 3 + ((flags & 1) ? 4 : 0) + ((flags & 2) ? 4 : 0);
      break;
    }
    case OP_UPLOAD_TEXTURE: NEED (8); pos += 8 + u32 (p + pos + 4); break;
    case OP_RELEASE_TEXTURE: NEED (4); pos += 4; break;
    case OP_SET_NODES: NEED (6); pos += 6 + (gsize) u32 (p + pos + 2) * 4; break;
    case OP_ROUNDTRIP: {
      NEED (6);
      gint32 args[2] = { u16 (p + pos), (gint32) u32 (p + pos + 2) };
      pos += 6;
      /* At most 60 frames a second, as a display would pace them. */
      static gint64 last;
      gint64 now = g_get_monotonic_time ();
      if (now - last < 16000)
        g_usleep (16000 - (now - last));
      last = g_get_monotonic_time ();
      send_input (EV_ROUNDTRIP_NOTIFY, args, 2);
      break;
    }
    default:
      return; /* an op this viewer doesn't know: skip the rest of the message */
    }
  }
#undef NEED
}

static gpointer
viewer_main (gpointer data)
{
  int fd = viewer_fd;
  GByteArray *message = g_byte_array_new ();
  for (;;) {
    guint8 head[2];
    if (!read_all (fd, head, 2))
      break;
    guint64 len = head[1] & 0x7f;
    if (len == 126) {
      guint8 ext[2];
      if (!read_all (fd, ext, 2))
        break;
      len = (guint64) ext[0] << 8 | ext[1];
    } else if (len == 127) {
      guint8 ext[8];
      if (!read_all (fd, ext, 8))
        break;
      len = 0;
      for (int i = 0; i < 8; i++)
        len = len << 8 | ext[i];
    }
    guint8 mask[4] = { 0 };
    if ((head[1] & 0x80) && !read_all (fd, mask, 4))
      break;
    guint8 *buf = g_malloc (len ? len : 1);
    if (len && !read_all (fd, buf, len)) {
      g_free (buf);
      break;
    }
    if (head[1] & 0x80)
      for (guint64 i = 0; i < len; i++)
        buf[i] ^= mask[i & 3];
    int opcode = head[0] & 0x0f;
    if (opcode == 8) { /* close */
      g_free (buf);
      break;
    }
    if (opcode == 1 || opcode == 2 || opcode == 0)
      g_byte_array_append (message, buf, (guint) len);
    g_free (buf);
    if ((head[0] & 0x80) && (opcode == 0 || opcode == 1 || opcode == 2)) {
      handle_commands (message->data, message->len);
      g_byte_array_set_size (message, 0);
    }
  }
  g_byte_array_unref (message);
  return NULL;
}

/* Broadway's frame timing warns when a roundtrip answer arrives for a frame it already
 * timed; harmless here (frames are paced by this viewer, not a display). */
static GLogWriterOutput
log_writer (GLogLevelFlags level, const GLogField *fields, gsize n, gpointer data)
{
  for (gsize i = 0; i < n; i++)
    if (!strcmp (fields[i].key, "MESSAGE") && fields[i].length < 0
        && strstr (fields[i].value, "gdk_frame_timings_submitted"))
      return G_LOG_WRITER_HANDLED;
  return g_log_writer_default (level, fields, n, data);
}

static gboolean
viewer_connect (const char *http_socket, int scale)
{
  int fd = socket (AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  struct sockaddr_un addr = { .sun_family = AF_UNIX };
  g_strlcpy (addr.sun_path, http_socket, sizeof addr.sun_path);
  if (fd < 0 || connect (fd, (struct sockaddr *) &addr, sizeof addr) != 0) {
    if (fd >= 0)
      close (fd);
    return FALSE;
  }
  const char *req = "GET /socket HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                    "Sec-WebSocket-Key: b21hcmNoeS12aWV3ZXIhIQ==\r\nSec-WebSocket-Protocol: broadway\r\n"
                    "Sec-WebSocket-Version: 13\r\nOrigin: http://localhost\r\n\r\n";
  if (!write_all (fd, (const guint8 *) req, strlen (req))) {
    close (fd);
    return FALSE;
  }
  /* The 101 response, up to its blank line. */
  char resp[2048];
  gsize got = 0;
  while (got < sizeof resp - 1) {
    if (!read_all (fd, (guint8 *) resp + got, 1)) {
      close (fd);
      return FALSE;
    }
    got++;
    resp[got] = 0;
    if (got >= 4 && !memcmp (resp + got - 4, "\r\n\r\n", 4))
      break;
  }
  if (!strstr (resp, " 101 ")) {
    close (fd);
    return FALSE;
  }
  viewer_fd = fd;
  gint32 size[3] = { 3840, 2160, scale };
  send_input (EV_SCREEN_SIZE_CHANGED, size, 3);
  viewer_thread = g_thread_new ("broadway-viewer", viewer_main, NULL);
  return TRUE;
}

static void
die_with_parent (gpointer data)
{
  prctl (PR_SET_PDEATHSIG, SIGTERM);
}

void
headless_stop (void)
{
  if (viewer_fd >= 0) {
    shutdown (viewer_fd, SHUT_RDWR);
    if (viewer_thread)
      g_thread_join (viewer_thread);
    viewer_thread = NULL;
    close (viewer_fd);
    viewer_fd = -1;
  }
  if (daemon_pid) {
    kill (daemon_pid, SIGTERM);
    g_spawn_close_pid (daemon_pid);
    daemon_pid = 0;
  }
  if (client_socket) {
    g_unlink (client_socket);
    g_clear_pointer (&client_socket, g_free);
  }
  if (daemon_dir) {
    g_autofree char *http = g_build_filename (daemon_dir, "http.sock", NULL);
    g_unlink (http);
    g_rmdir (daemon_dir);
    g_clear_pointer (&daemon_dir, g_free);
  }
}

static void
at_exit (void)
{
  headless_stop ();
}

gboolean
headless_start (int scale)
{
  g_log_set_writer_func (log_writer, NULL, NULL);
  g_autofree char *daemon = g_find_program_in_path ("gtk4-broadwayd");
  if (!daemon) {
    g_printerr ("headless: gtk4-broadwayd is not on PATH (it comes with gtk4)\n");
    return FALSE;
  }
  g_autoptr (GError) error = NULL;
  daemon_dir = g_dir_make_tmp ("omarchy-gtk-XXXXXX", &error);
  if (!daemon_dir) {
    g_printerr ("headless: %s\n", error->message);
    return FALSE;
  }
  /* The daemon's client socket lands in GLib's runtime dir: make that our temp directory (GLib
   * caches it on first use) while the environment keeps the real one, which libomarchy-theme
   * reads to find Hyprland. Nothing is left in $XDG_RUNTIME_DIR, even after a crash. */
  g_autofree char *real_runtime = g_strdup (g_getenv ("XDG_RUNTIME_DIR"));
  g_setenv ("XDG_RUNTIME_DIR", daemon_dir, TRUE);
  const char *runtime = g_get_user_runtime_dir ();
  if (real_runtime)
    g_setenv ("XDG_RUNTIME_DIR", real_runtime, TRUE);
  else
    g_unsetenv ("XDG_RUNTIME_DIR");
  if (g_strcmp0 (runtime, daemon_dir) != 0)
    g_printerr ("headless: GLib's runtime dir was read before headless_start(); the socket goes to %s\n", runtime);
  g_auto (GStrv) daemon_env = g_environ_setenv (g_get_environ (), "XDG_RUNTIME_DIR", runtime, TRUE);
  for (int attempt = 0; attempt < 8; attempt++) {
    int n = 2000 + (int) ((getpid () * 7 + attempt * 7919 + g_random_int_range (0, 997)) % 40000);
    g_autofree char *display = g_strdup_printf (":%d", n);
    g_autofree char *http = g_build_filename (daemon_dir, "http.sock", NULL);
    g_autofree char *sock_name = g_strdup_printf ("broadway%d.socket", n + 1);
    g_autofree char *sock = g_build_filename (runtime, sock_name, NULL);
    if (g_file_test (sock, G_FILE_TEST_EXISTS))
      continue;
    char *argv[] = { daemon, "--unixsocket", http, display, NULL };
    g_clear_error (&error);
    if (!g_spawn_async (NULL, argv, daemon_env, G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                        die_with_parent, NULL, &daemon_pid, &error)) {
      g_printerr ("headless: %s\n", error->message);
      return FALSE;
    }
    /* Ready once its client socket exists; gone means the number was taken. */
    for (int i = 0; i < 300; i++) {
      if (g_file_test (sock, G_FILE_TEST_EXISTS))
        break;
      g_usleep (10000);
    }
    if (g_file_test (sock, G_FILE_TEST_EXISTS) && kill (daemon_pid, 0) == 0) {
      client_socket = g_steal_pointer (&sock);
      if (!viewer_connect (http, MAX (1, scale)))
        g_printerr ("headless: no viewer; windows will paint once\n");
      g_setenv ("GDK_BACKEND", "broadway", TRUE);
      g_setenv ("BROADWAY_DISPLAY", display, TRUE);
      /* No accessibility bus, no portal, no user settings: renders the same everywhere. */
      g_setenv ("GTK_A11Y", "none", TRUE);
      g_setenv ("ADW_DISABLE_PORTAL", "1", TRUE);
      g_setenv ("GSETTINGS_BACKEND", "memory", TRUE);
      g_unsetenv ("WAYLAND_DISPLAY");
      g_unsetenv ("DISPLAY");
      atexit (at_exit);
      return TRUE;
    }
    kill (daemon_pid, SIGTERM);
    g_spawn_close_pid (daemon_pid);
    daemon_pid = 0;
    g_unlink (http);
  }
  g_printerr ("headless: gtk4-broadwayd didn't start\n");
  return FALSE;
}

char **
headless_child_env (void)
{
  char **env = g_get_environ ();
  env = g_environ_setenv (env, "XDG_RUNTIME_DIR", g_get_user_runtime_dir (), TRUE);
  return env;
}

static gboolean
quit_loop (gpointer loop)
{
  g_main_loop_quit (loop);
  return G_SOURCE_REMOVE;
}

void
headless_wait (guint ms)
{
  GMainLoop *loop = g_main_loop_new (NULL, FALSE);
  g_timeout_add (ms, quit_loop, loop);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);
}

void
headless_present (GtkWindow *window, int width, int height)
{
  /* The viewer's screen size reaches GDK a moment after the display opens; until it does, a
   * toplevel is bounded by broadway's default 1024 × 768. */
  GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (window));
  for (int i = 0; i < 100; i++) {
    GListModel *monitors = gdk_display_get_monitors (display);
    g_autoptr (GdkMonitor) m = g_list_model_get_n_items (monitors) ? g_list_model_get_item (monitors, 0) : NULL;
    GdkRectangle r = { 0 };
    if (m)
      gdk_monitor_get_geometry (m, &r);
    if (r.height > 768)
      break;
    headless_wait (10);
  }
  gtk_window_set_decorated (window, FALSE);
  gtk_window_set_default_size (window, width, height);
  gtk_window_present (window);
  for (int i = 0; i < 200; i++) {
    GtkWidget *w = GTK_WIDGET (window);
    if (gtk_widget_get_mapped (w) && gtk_widget_get_width (w) > 0)
      break;
    headless_wait (10);
  }
  /* A few frames for layout, fonts and the first paint to settle. */
  headless_wait (120);
}

GdkTexture *
headless_render (GtkWidget *widget, double scale)
{
  int w = gtk_widget_get_width (widget), h = gtk_widget_get_height (widget);
  GdkPaintable *paintable = gtk_widget_paintable_new (widget);
  GtkSnapshot *s = gtk_snapshot_new ();
  gtk_snapshot_scale (s, (float) scale, (float) scale);
  gdk_paintable_snapshot (paintable, s, w, h);
  GskRenderNode *node = gtk_snapshot_free_to_node (s);
  g_object_unref (paintable);
  GskRenderer *renderer = gsk_cairo_renderer_new ();
  g_autoptr (GError) error = NULL;
  if (!gsk_renderer_realize_for_display (renderer, gtk_widget_get_display (widget), &error)) {
    g_printerr ("headless: %s\n", error->message);
    g_object_unref (renderer);
    if (node)
      gsk_render_node_unref (node);
    return NULL;
  }
  graphene_rect_t viewport = GRAPHENE_RECT_INIT (0, 0, (float) (w * scale), (float) (h * scale));
  GdkTexture *texture;
  if (node) {
    texture = gsk_renderer_render_texture (renderer, node, &viewport);
    gsk_render_node_unref (node);
  } else {
    GskRenderNode *empty = gsk_color_node_new (&(GdkRGBA) { 0, 0, 0, 0 }, &viewport);
    texture = gsk_renderer_render_texture (renderer, empty, &viewport);
    gsk_render_node_unref (empty);
  }
  gsk_renderer_unrealize (renderer);
  g_object_unref (renderer);
  return texture;
}

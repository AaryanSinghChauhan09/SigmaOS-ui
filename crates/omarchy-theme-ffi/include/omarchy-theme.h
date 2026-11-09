/* omarchy-theme.h: the live Omarchy theme as Omarchy UI tokens, for C and anything that can call C.
 *
 * libomarchy-theme is the Rust resolver every Omarchy UI binding shares (crates/omarchy-theme),
 * built as a C library, so a toolkit binding reads exactly the tokens the Qt and Rune kits do and
 * follows the desktop the same way. The tokens are JSON in the canonical shape of
 * spec/tokens.md; parse them with your toolkit's JSON reader (json-glib, QJsonDocument, cJSON).
 * How they change while an app runs is spec/following.md; docs/using-c.md has worked examples.
 *
 *   pkg-config --cflags --libs omarchy-theme
 *
 * Conventions:
 *   - Every `char *` returned is UTF-8, NUL-terminated, owned by the caller: free it with
 *     omarchy_theme_free(). NULL means "none" (no such icon, no such string, nothing new).
 *   - Every function may be called from any thread.
 *   - The OMARCHY_UI_* overrides of spec/tokens.md apply (OMARCHY_UI_THEME_DIR pins a theme).
 */
#ifndef OMARCHY_THEME_H
#define OMARCHY_THEME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bumped, with the library's soname (libomarchy-theme.so.N), when a signature changes. */
#define OMARCHY_THEME_ABI 0

/* The ABI the loaded library was built with; compare with OMARCHY_THEME_ABI. */
uint32_t omarchy_theme_abi_version(void);

/* Free a string this library returned. NULL is ignored. */
void omarchy_theme_free(char *s);

/* ---- The theme now ------------------------------------------------------------------------ */

/* The tokens, resolved now, as the canonical JSON of spec/tokens.md on one line:
 * {"schema":1,"mode":"dark","palette":{"background":"#1a1b26ff",...},"radius":0,...}.
 * Colors are "#rrggbbaa". */
char *omarchy_theme_tokens_json(void);

/* Everything at once, on one line: {"schema":1,"revision":0,"theme":"tokyo-night",
 * "iconTheme":"Yaru-blue","language":"en","tokens":{...},"colors":{"red":"#f7768eff",...},"strings":{...}}. */
char *omarchy_theme_state_json(void);

/* The theme's name ("tokyo-night"). */
char *omarchy_theme_name(void);

/* The freedesktop icon theme the Omarchy theme names ("Yaru-blue" when it names none). */
char *omarchy_theme_icon_theme(void);

/* The file icon `name` ("firefox", "org.gnome.Nautilus") resolves to at `size` device pixels
 * in that icon theme, by the lookup Omarchy's app launcher uses (app and device icons first,
 * then loose pixmaps, then any context; an SVG over a PNG). NULL when no theme has it. */
char *omarchy_theme_icon_lookup(const char *name, int size);

/* The kit's own strings (Cancel, Search..., "No matches for “%1”") in the display language
 * (`omarchy language`), with English for anything untranslated:
 * {"language":"de","strings":{"common.cancel":"Abbrechen",...}}. */
char *omarchy_theme_strings_json(void);

/* One of those strings, or NULL when the catalog has no `key`. Placeholders stay as "%1". */
char *omarchy_theme_string(const char *key);

/* ---- Following the desktop (spec/following.md) ------------------------------------------- */

/* A running watch. It follows theme switches, edits in the theme, ~/.config/omarchy/shell.toml,
 * the display language, fontconfig and Hyprland reloads, coalesced over 150 ms, and delivers a
 * change only when the tokens (or the icon theme, language or theme name) actually differ. The
 * initial load is never a change. */
typedef struct OmarchyThemeWatch OmarchyThemeWatch;

/* For a main loop. Add omarchy_theme_watch_fd() to it, and when the fd is readable call
 * omarchy_theme_watch_take(). With GLib:
 *
 *   static gboolean on_theme(gint fd, GIOCondition c, gpointer w) {
 *     char *json = omarchy_theme_watch_take(w);
 *     if (json) { retheme(json); omarchy_theme_free(json); }
 *     return G_SOURCE_CONTINUE;
 *   }
 *   OmarchyThemeWatch *w = omarchy_theme_watch_new();
 *   g_unix_fd_add(omarchy_theme_watch_fd(w), G_IO_IN, on_theme, w);
 *
 * With Qt: a QSocketNotifier(fd, QSocketNotifier::Read) whose activated() calls take.
 * NULL if the watch can't be started. */
OmarchyThemeWatch *omarchy_theme_watch_new(void);

/* The fd to poll for readability; -1 for a callback watch. Owned by the watch: don't close or
 * read it. */
int omarchy_theme_watch_fd(const OmarchyThemeWatch *watch);

/* The latest change's tokens JSON, or NULL when there's none since the last take. Clears the
 * fd's readability. Several changes between two takes give the latest only. */
char *omarchy_theme_watch_take(OmarchyThemeWatch *watch);

/* The tokens JSON the watch has now: what it loaded when it started, then the latest change.
 * Read the starting tokens here, after creating the watch, so no change can fall between. */
char *omarchy_theme_watch_current_json(const OmarchyThemeWatch *watch);

/* 0 for the initial load, plus one for every change delivered: a cache key. */
uint64_t omarchy_theme_watch_revision(const OmarchyThemeWatch *watch);

/* Called once per change, on a background thread (not your UI thread: hand it over, with
 * g_idle_add or QMetaObject::invokeMethod). `tokens_json` is valid during the call only. */
typedef void (*OmarchyThemeChanged)(const char *tokens_json, uint64_t revision, void *user_data);

/* Called with user_data once the watch is freed. */
typedef void (*OmarchyThemeDestroy)(void *user_data);

/* The callback form. `destroy` may be NULL. NULL if `callback` is. */
OmarchyThemeWatch *omarchy_theme_watch_callback(OmarchyThemeChanged callback, void *user_data,
                                                OmarchyThemeDestroy destroy);

/* Stop and free a watch of either kind. Waits for a callback that is running, so never call it
 * from inside the callback. NULL is ignored. */
void omarchy_theme_watch_free(OmarchyThemeWatch *watch);

#ifdef __cplusplus
}
#endif

#endif /* OMARCHY_THEME_H */

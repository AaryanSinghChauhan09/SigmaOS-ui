/* The C library's live-switch test (spec/following.md), and a check of the rest of its API.
 * bin/test builds it against the staged library through pkg-config and runs it under
 * tools/live/run, which gives it a HOME of its own with tokyo-night current:
 *
 *   cc live.c $(pkg-config --cflags --libs omarchy-theme) -lpthread -o live
 *   tools/live/run ./live
 */
#include <omarchy-theme.h>

#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, ...)                                    \
  do {                                                      \
    if (!(cond)) {                                          \
      fprintf(stderr, "c library: FAIL %s:%d: ", __FILE__, __LINE__); \
      fprintf(stderr, __VA_ARGS__);                         \
      fputc('\n', stderr);                                  \
      failures++;                                           \
    }                                                       \
  } while (0)

static int contains(const char *s, const char *needle) { return s && strstr(s, needle) != NULL; }

static int readable(int fd, int ms) {
  struct pollfd p = {.fd = fd, .events = POLLIN};
  return poll(&p, 1, ms) == 1 && (p.revents & POLLIN);
}

/* The callback watch's deliveries, from its background thread. */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int calls = 0, destroyed = 0;
static uint64_t last_revision = 0;
static char last_json[65536];

static void on_change(const char *json, uint64_t revision, void *data) {
  pthread_mutex_lock(&lock);
  calls++;
  last_revision = revision;
  snprintf(last_json, sizeof last_json, "%s", json);
  pthread_mutex_unlock(&lock);
  CHECK(data == (void *)&calls, "user_data is passed through");
}

static void on_destroy(void *data) {
  CHECK(data == (void *)&calls, "destroy gets user_data");
  destroyed++;
}

static int callback_calls(void) {
  pthread_mutex_lock(&lock);
  int n = calls;
  pthread_mutex_unlock(&lock);
  return n;
}

int main(void) {
  const char *home = getenv("OMARCHY_UI_LIVE_HOME"), *sw = getenv("OMARCHY_UI_LIVE_SWITCH");
  if (!home || !sw) {
    fprintf(stderr, "run under tools/live/run\n");
    return 2;
  }
  CHECK(omarchy_theme_abi_version() == OMARCHY_THEME_ABI, "abi %u", omarchy_theme_abi_version());

  /* The theme now. */
  char *json = omarchy_theme_tokens_json();
  CHECK(contains(json, "\"background\":\"#1a1b26ff\""), "tokyo-night's background in %.80s", json);
  CHECK(contains(json, "\"theme\":280"), "the fade's duration is a token");
  omarchy_theme_free(json);
  char *name = omarchy_theme_name();
  CHECK(name && strcmp(name, "tokyo-night") == 0, "theme name %s", name);
  omarchy_theme_free(name);
  char *state = omarchy_theme_state_json();
  CHECK(contains(state, "\"theme\":\"tokyo-night\"") && contains(state, "\"tokens\":{"), "state %.80s", state);
  omarchy_theme_free(state);

  /* Strings and icons (bin/test points the icon lookup at fixtures/icons). */
  char *strings = omarchy_theme_strings_json();
  CHECK(contains(strings, "\"language\":\"en\"") && contains(strings, "\"common.cancel\":\"Cancel\""), "strings %.80s", strings);
  omarchy_theme_free(strings);
  char *cancel = omarchy_theme_string("common.cancel");
  CHECK(cancel && strcmp(cancel, "Cancel") == 0, "common.cancel is %s", cancel);
  omarchy_theme_free(cancel);
  CHECK(omarchy_theme_string("no.such.key") == NULL, "an unknown key is NULL");
  char *icons = omarchy_theme_icon_theme();
  CHECK(icons && *icons, "an icon theme");
  omarchy_theme_free(icons);
  if (getenv("OMARCHY_UI_ICON_DIRS")) {
    char *path = omarchy_theme_icon_lookup("firefox", 48);
    CHECK(path && strstr(path, "firefox"), "firefox resolves: %s", path);
    omarchy_theme_free(path);
    CHECK(omarchy_theme_icon_lookup("no-such-app", 48) == NULL, "a missing icon is NULL");
  }

  /* Both watches, then the initial load: not a change. */
  OmarchyThemeWatch *w = omarchy_theme_watch_new();
  CHECK(w != NULL, "a watch");
  OmarchyThemeWatch *cw = omarchy_theme_watch_callback(on_change, &calls, on_destroy);
  CHECK(cw != NULL, "a callback watch");
  CHECK(omarchy_theme_watch_fd(cw) == -1, "a callback watch has no fd");
  int fd = omarchy_theme_watch_fd(w);
  CHECK(fd >= 0, "an fd");
  char *current = omarchy_theme_watch_current_json(w);
  CHECK(contains(current, "#1a1b26ff"), "the watch starts on tokyo-night");
  omarchy_theme_free(current);
  CHECK(!readable(fd, 400), "the initial load is not a change");
  CHECK(omarchy_theme_watch_take(w) == NULL, "nothing to take yet");
  CHECK(omarchy_theme_watch_revision(w) == 0, "revision 0");

  /* omarchy theme set catppuccin-latte, simulated. */
  char cmd[4096];
  snprintf(cmd, sizeof cmd, "'%s' '%s' catppuccin-latte", sw, home);
  CHECK(system(cmd) == 0, "the switch ran");

  CHECK(readable(fd, 5000), "the fd turns readable");
  json = omarchy_theme_watch_take(w);
  CHECK(contains(json, "\"background\":\"#eff1f5ff\""), "take gives catppuccin-latte: %.80s", json);
  CHECK(contains(json, "\"mode\":\"light\""), "a light theme");
  omarchy_theme_free(json);
  CHECK(omarchy_theme_watch_revision(w) == 1, "revision 1, not %llu", (unsigned long long)omarchy_theme_watch_revision(w));
  CHECK(omarchy_theme_watch_take(w) == NULL, "taken once");
  CHECK(!readable(fd, 600), "one switch is one change");
  current = omarchy_theme_watch_current_json(w);
  CHECK(contains(current, "#eff1f5ff"), "current follows");
  omarchy_theme_free(current);

  for (int i = 0; i < 50 && callback_calls() == 0; i++) usleep(100 * 1000);
  usleep(600 * 1000);
  pthread_mutex_lock(&lock);
  CHECK(calls == 1, "the callback ran once, not %d times", calls);
  CHECK(last_revision == 1, "with revision 1");
  CHECK(contains(last_json, "\"background\":\"#eff1f5ff\""), "and the new tokens");
  pthread_mutex_unlock(&lock);

  omarchy_theme_watch_free(w);
  omarchy_theme_watch_free(cw);
  CHECK(destroyed == 1, "destroy ran once");
  omarchy_theme_watch_free(NULL);
  omarchy_theme_free(NULL);

  if (failures) return 1;
  printf("c library: a switch reaches the fd watch and the callback once, with the new tokens\n");
  return 0;
}

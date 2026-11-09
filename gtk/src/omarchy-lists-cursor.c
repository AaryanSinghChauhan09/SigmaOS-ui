/* omarchy-lists-cursor.c: CursorModel (CursorModel.qml and PointerMoveGate.qml), the keyboard
 * cursor every panel re-implements, as one non-visual GObject. */
#include "omarchy-private.h"

#include <math.h>
#include <string.h>

/**
 * OmarchyCursorModel:
 *
 * The keyboard cursor of a panel: a section and a row within it, and whether it shows.
 *
 * - j/k and Down/Up move a row, crossing into the next or previous non-empty section at the
 *   ends and stopping at the first and last row; h/l and Left/Right move a column when
 *   #OmarchyCursorModel:columns is above 1 (a grid).
 * - Page Up/Down move #OmarchyCursorModel:page-size lines within the section; Home/End go to the
 *   first or last row; Tab/Shift+Tab jump to the next or previous section's first row.
 * - Return, Enter and Space emit ::activated, Escape ::close-requested, Delete and x
 *   ::delete-requested; any other printable key without Ctrl, Alt or Super ::text-typed.
 * - The first key after the cursor was hidden only shows it.
 * - The pointer moves the cursor through omarchy_cursor_model_pointer_moved(), which ignores
 *   samples that didn't travel more than 1 point (rows moving under a still pointer). Set
 *   #OmarchyCursorModel:reference-item to something that doesn't scroll, like the panel.
 *
 * Hand it a widget's keys with omarchy_cursor_model_attach(), and a #OmarchyListRow's cursor
 * flag and pointer with omarchy_cursor_model_bind(). On ::moved with @by_key, reveal the row
 * (omarchy_scroll_area_ensure_visible()).
 */

typedef struct {
  char *name;
  int count;
} Section;

struct _OmarchyCursorModel {
  GObject parent_instance;
  GArray *sections; /* Section */
  char *focus_section;
  int selected_index;
  gboolean cursor_active;
  int columns, page_size;
  GtkWidget *reference_item; /* weak */

  /* PointerMoveGate */
  gboolean primed, initial_allowed;
  double last_x, last_y;
};

enum {
  PROP_0,
  PROP_FOCUS_SECTION,
  PROP_SELECTED_INDEX,
  PROP_CURSOR_ACTIVE,
  PROP_COLUMNS,
  PROP_PAGE_SIZE,
  PROP_REFERENCE_ITEM,
  N_PROPS
};
static GParamSpec *props[N_PROPS];

enum { MOVED, ACTIVATED, DELETE_REQUESTED, CLOSE_REQUESTED, TEXT_TYPED, N_SIGNALS };
static guint signals[N_SIGNALS];

G_DEFINE_FINAL_TYPE (OmarchyCursorModel, omarchy_cursor_model, G_TYPE_OBJECT)

#define SEC(self, i) (&g_array_index ((self)->sections, Section, (i)))

/* ---- state ------------------------------------------------------------------------------- */

static void
put_section (OmarchyCursorModel *self, const char *name)
{
  if (!g_strcmp0 (self->focus_section, name))
    return;
  g_free (self->focus_section);
  self->focus_section = g_strdup (name ? name : "");
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_FOCUS_SECTION]);
}

static void
put_index (OmarchyCursorModel *self, int index)
{
  if (self->selected_index == index)
    return;
  self->selected_index = index;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SELECTED_INDEX]);
}

static void
put_active (OmarchyCursorModel *self, gboolean active)
{
  if (self->cursor_active == !!active)
    return;
  self->cursor_active = !!active;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CURSOR_ACTIVE]);
}

static int
section_index (OmarchyCursorModel *self, const char *name)
{
  for (guint i = 0; i < self->sections->len; i++)
    if (!g_strcmp0 (SEC (self, i)->name, name ? name : ""))
      return (int) i;
  return -1;
}

static int
count_of (OmarchyCursorModel *self, const char *name)
{
  int i = section_index (self, name);
  return i < 0 ? 0 : SEC (self, i)->count;
}

static int
neighbour (OmarchyCursorModel *self, int from, int dir)
{
  for (int i = from + dir; i >= 0 && i < (int) self->sections->len; i += dir)
    if (SEC (self, i)->count > 0)
      return i;
  return -1;
}

/* Keep the cursor on a real row: off an empty section, within the count. */
static void
settle (OmarchyCursorModel *self)
{
  int s = section_index (self, self->focus_section);
  if (s < 0 || SEC (self, s)->count == 0) {
    int n = neighbour (self, MAX (0, s), 1);
    if (n < 0)
      n = neighbour (self, MAX (0, s), -1);
    if (n < 0 && s < 0 && self->sections->len > 0)
      n = 0;
    if (n >= 0) {
      put_section (self, SEC (self, n)->name);
      put_index (self, 0);
    }
  }
  put_index (self, MAX (0, MIN (self->selected_index, count_of (self, self->focus_section) - 1)));
}

static void
gate_reset (OmarchyCursorModel *self)
{
  self->primed = FALSE;
  self->initial_allowed = FALSE;
  self->last_x = self->last_y = 0;
}

/* Show a hidden cursor; FALSE when it was hidden (the key only woke it). */
static gboolean
wake (OmarchyCursorModel *self)
{
  gate_reset (self);
  if (self->cursor_active)
    return TRUE;
  put_active (self, TRUE);
  settle (self);
  return FALSE;
}

static void
announce (OmarchyCursorModel *self, gboolean by_key)
{
  g_signal_emit (self, signals[MOVED], 0, self->focus_section, self->selected_index, by_key);
}

/* ---- API --------------------------------------------------------------------------------- */

/**
 * omarchy_cursor_model_new:
 *
 * Returns: (transfer full): a cursor model with no sections; add them with
 *   omarchy_cursor_model_add_section()
 */
OmarchyCursorModel *
omarchy_cursor_model_new (void)
{
  return g_object_new (OMARCHY_TYPE_CURSOR_MODEL, NULL);
}

/**
 * omarchy_cursor_model_new_list:
 * @count: the rows
 *
 * A plain list: one section named `""` of @count rows.
 *
 * Returns: (transfer full): a cursor model
 */
OmarchyCursorModel *
omarchy_cursor_model_new_list (int count)
{
  OmarchyCursorModel *self = omarchy_cursor_model_new ();
  omarchy_cursor_model_add_section (self, "", count);
  return self;
}

/**
 * omarchy_cursor_model_add_section:
 * @self: the model
 * @name: the section's name, unique in the model
 * @count: its rows
 *
 * Adds a section after the others (Qt `sections: [{ name, count }]`).
 */
void
omarchy_cursor_model_add_section (OmarchyCursorModel *self, const char *name, int count)
{
  g_return_if_fail (OMARCHY_IS_CURSOR_MODEL (self));
  if (section_index (self, name) >= 0) {
    omarchy_cursor_model_set_count (self, name, count);
    return;
  }
  Section s = { g_strdup (name ? name : ""), MAX (0, count) };
  g_array_append_val (self->sections, s);
  if (self->sections->len == 1 && !self->cursor_active)
    put_section (self, s.name);
  if (self->cursor_active)
    settle (self);
}

/**
 * omarchy_cursor_model_clear_sections:
 * @self: the model
 */
void
omarchy_cursor_model_clear_sections (OmarchyCursorModel *self)
{
  g_return_if_fail (OMARCHY_IS_CURSOR_MODEL (self));
  g_array_set_size (self->sections, 0);
}

/**
 * omarchy_cursor_model_set_count:
 * @self: the model
 * @section: a section's name
 * @count: its rows now
 *
 * Changes a section's rows, keeping the cursor on a real row.
 */
void
omarchy_cursor_model_set_count (OmarchyCursorModel *self, const char *section, int count)
{
  g_return_if_fail (OMARCHY_IS_CURSOR_MODEL (self));
  int i = section_index (self, section);
  if (i < 0)
    return;
  SEC (self, i)->count = MAX (0, count);
  settle (self);
}

int
omarchy_cursor_model_get_count (OmarchyCursorModel *self, const char *section)
{
  g_return_val_if_fail (OMARCHY_IS_CURSOR_MODEL (self), 0);
  return count_of (self, section);
}

guint
omarchy_cursor_model_get_n_sections (OmarchyCursorModel *self)
{
  g_return_val_if_fail (OMARCHY_IS_CURSOR_MODEL (self), 0);
  return self->sections->len;
}

const char *
omarchy_cursor_model_get_focus_section (OmarchyCursorModel *self)
{
  return self->focus_section;
}

int
omarchy_cursor_model_get_selected_index (OmarchyCursorModel *self)
{
  return self->selected_index;
}

gboolean
omarchy_cursor_model_get_cursor_active (OmarchyCursorModel *self)
{
  return self->cursor_active;
}

int
omarchy_cursor_model_get_columns (OmarchyCursorModel *self)
{
  return self->columns;
}

/**
 * omarchy_cursor_model_set_columns:
 * @self: the model
 * @columns: tiles to a line (a grid), or 1 for a list
 */
void
omarchy_cursor_model_set_columns (OmarchyCursorModel *self, int columns)
{
  columns = MAX (1, columns);
  if (self->columns == columns)
    return;
  self->columns = columns;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_COLUMNS]);
}

int
omarchy_cursor_model_get_page_size (OmarchyCursorModel *self)
{
  return self->page_size;
}

void
omarchy_cursor_model_set_page_size (OmarchyCursorModel *self, int page_size)
{
  if (self->page_size == page_size)
    return;
  self->page_size = page_size;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_PAGE_SIZE]);
}

/**
 * omarchy_cursor_model_get_reference_item:
 * @self: the model
 *
 * Returns: (transfer none) (nullable): what pointer samples are measured against
 */
GtkWidget *
omarchy_cursor_model_get_reference_item (OmarchyCursorModel *self)
{
  return self->reference_item;
}

/**
 * omarchy_cursor_model_set_reference_item:
 * @self: the model
 * @item: (nullable): a widget that doesn't scroll (the panel), or %NULL for the row itself
 */
void
omarchy_cursor_model_set_reference_item (OmarchyCursorModel *self, GtkWidget *item)
{
  if (self->reference_item == item)
    return;
  if (self->reference_item)
    g_object_remove_weak_pointer (G_OBJECT (self->reference_item), (gpointer *) &self->reference_item);
  self->reference_item = item;
  if (item)
    g_object_add_weak_pointer (G_OBJECT (item), (gpointer *) &self->reference_item);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_REFERENCE_ITEM]);
}

/**
 * omarchy_cursor_model_is_at:
 * @self: the model
 * @section: a section's name
 * @index: a row in it
 *
 * Returns: whether the cursor shows on that row (a row's #OmarchyListRow:has-cursor)
 */
gboolean
omarchy_cursor_model_is_at (OmarchyCursorModel *self, const char *section, int index)
{
  g_return_val_if_fail (OMARCHY_IS_CURSOR_MODEL (self), FALSE);
  return self->cursor_active && !g_strcmp0 (self->focus_section, section ? section : "") && self->selected_index == index;
}

/**
 * omarchy_cursor_model_set:
 * @self: the model
 * @section: a section's name
 * @index: a row in it
 *
 * Shows the cursor on a row.
 */
void
omarchy_cursor_model_set (OmarchyCursorModel *self, const char *section, int index)
{
  g_return_if_fail (OMARCHY_IS_CURSOR_MODEL (self));
  if (section_index (self, section) < 0)
    return;
  g_object_freeze_notify (G_OBJECT (self));
  put_section (self, section);
  put_index (self, index);
  put_active (self, TRUE);
  settle (self);
  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * omarchy_cursor_model_reset:
 * @self: the model
 *
 * Hides the cursor (it stays where it was for the next key).
 */
void
omarchy_cursor_model_reset (OmarchyCursorModel *self)
{
  g_return_if_fail (OMARCHY_IS_CURSOR_MODEL (self));
  put_active (self, FALSE);
  gate_reset (self);
}

/**
 * omarchy_cursor_model_move:
 * @self: the model
 * @dx: columns (a grid's only)
 * @dy: lines
 */
void
omarchy_cursor_model_move (OmarchyCursorModel *self, int dx, int dy)
{
  g_return_if_fail (OMARCHY_IS_CURSOR_MODEL (self));
  g_object_freeze_notify (G_OBJECT (self));
  if (!wake (self)) {
    announce (self, TRUE);
    g_object_thaw_notify (G_OBJECT (self));
    return;
  }
  int s = section_index (self, self->focus_section);
  if (s < 0) {
    g_object_thaw_notify (G_OBJECT (self));
    return;
  }
  int columns = self->columns;
  int count = SEC (self, s)->count;
  int index = self->selected_index;
  if (dx != 0 && columns > 1) {
    int col = index % columns + dx;
    if (col >= 0 && col < columns && index + dx >= 0 && index + dx < count)
      index += dx;
  }
  for (int step = 0; step < ABS (dy); step++) {
    int next = index + columns * (dy > 0 ? 1 : -1);
    if (next >= 0 && next < count) {
      index = next;
      continue;
    }
    int n = neighbour (self, s, dy > 0 ? 1 : -1);
    if (n >= 0) {
      int c = SEC (self, n)->count;
      if (dy > 0)
        index = MIN (index % columns, c - 1);
      else
        index = MIN ((c - 1) / columns * columns + index % columns, c - 1);
      s = n;
      count = c;
    } else if (dy > 0 && columns > 1 && index / columns < (count - 1) / columns) {
      index = count - 1;
    }
  }
  put_section (self, SEC (self, s)->name);
  put_index (self, index);
  announce (self, TRUE);
  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * omarchy_cursor_model_move_section:
 * @self: the model
 * @delta: 1 for the next non-empty section's first row, -1 for the previous one's
 */
void
omarchy_cursor_model_move_section (OmarchyCursorModel *self, int delta)
{
  g_return_if_fail (OMARCHY_IS_CURSOR_MODEL (self));
  g_object_freeze_notify (G_OBJECT (self));
  if (!wake (self)) {
    announce (self, TRUE);
  } else {
    int n = neighbour (self, section_index (self, self->focus_section), delta > 0 ? 1 : -1);
    if (n >= 0) {
      put_section (self, SEC (self, n)->name);
      put_index (self, 0);
      announce (self, TRUE);
    }
  }
  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * omarchy_cursor_model_page:
 * @self: the model
 * @delta: 1 for Page Down, -1 for Page Up
 */
void
omarchy_cursor_model_page (OmarchyCursorModel *self, int delta)
{
  g_return_if_fail (OMARCHY_IS_CURSOR_MODEL (self));
  g_object_freeze_notify (G_OBJECT (self));
  if (wake (self)) {
    int last = count_of (self, self->focus_section) - 1;
    int step = MAX (1, self->page_size) * self->columns * (delta > 0 ? 1 : -1);
    put_index (self, MAX (0, MIN (last, self->selected_index + step)));
  }
  announce (self, TRUE);
  g_object_thaw_notify (G_OBJECT (self));
}

void
omarchy_cursor_model_home (OmarchyCursorModel *self)
{
  g_return_if_fail (OMARCHY_IS_CURSOR_MODEL (self));
  g_object_freeze_notify (G_OBJECT (self));
  put_active (self, TRUE);
  gate_reset (self);
  int n = neighbour (self, -1, 1);
  if (n >= 0) {
    put_section (self, SEC (self, n)->name);
    put_index (self, 0);
  }
  announce (self, TRUE);
  g_object_thaw_notify (G_OBJECT (self));
}

void
omarchy_cursor_model_end (OmarchyCursorModel *self)
{
  g_return_if_fail (OMARCHY_IS_CURSOR_MODEL (self));
  g_object_freeze_notify (G_OBJECT (self));
  put_active (self, TRUE);
  gate_reset (self);
  int n = neighbour (self, self->sections->len, -1);
  if (n >= 0) {
    put_section (self, SEC (self, n)->name);
    put_index (self, SEC (self, n)->count - 1);
  }
  announce (self, TRUE);
  g_object_thaw_notify (G_OBJECT (self));
}

#define NOT_PLAIN (GDK_CONTROL_MASK | GDK_ALT_MASK | GDK_SUPER_MASK | GDK_META_MASK)

/**
 * omarchy_cursor_model_printable:
 * @keyval: a key
 * @state: its modifiers
 *
 * The text a key types for a type-ahead filter: one printable character without Ctrl, Alt or
 * Super. Space is not filter text.
 *
 * Returns: (transfer full) (nullable): the text, or %NULL
 */
char *
omarchy_cursor_model_printable (guint keyval, GdkModifierType state)
{
  if (state & NOT_PLAIN)
    return NULL;
  gunichar c = gdk_keyval_to_unicode (keyval);
  if (!c || g_unichar_isspace (c) || c < 0x20 || c == 0x7f || !g_unichar_isprint (c))
    return NULL;
  char buf[8];
  int n = g_unichar_to_utf8 (c, buf);
  buf[n] = 0;
  return g_strdup (buf);
}

/**
 * omarchy_cursor_model_handle_key:
 * @self: the model
 * @keyval: a key
 * @state: its modifiers
 *
 * Handles a key the shell's way (see #OmarchyCursorModel).
 *
 * Returns: %TRUE when it was the model's key
 */
gboolean
omarchy_cursor_model_handle_key (OmarchyCursorModel *self, guint keyval, GdkModifierType state)
{
  g_return_val_if_fail (OMARCHY_IS_CURSOR_MODEL (self), FALSE);
  gboolean plain = !(state & NOT_PLAIN);
  gunichar t = plain ? gdk_keyval_to_unicode (keyval) : 0;
  switch (keyval) {
  case GDK_KEY_Down: case GDK_KEY_KP_Down: omarchy_cursor_model_move (self, 0, 1); return TRUE;
  case GDK_KEY_Up: case GDK_KEY_KP_Up: omarchy_cursor_model_move (self, 0, -1); return TRUE;
  case GDK_KEY_Right: case GDK_KEY_KP_Right: omarchy_cursor_model_move (self, 1, 0); return TRUE;
  case GDK_KEY_Left: case GDK_KEY_KP_Left: omarchy_cursor_model_move (self, -1, 0); return TRUE;
  case GDK_KEY_Page_Down: case GDK_KEY_KP_Page_Down: omarchy_cursor_model_page (self, 1); return TRUE;
  case GDK_KEY_Page_Up: case GDK_KEY_KP_Page_Up: omarchy_cursor_model_page (self, -1); return TRUE;
  case GDK_KEY_Home: case GDK_KEY_KP_Home: omarchy_cursor_model_home (self); return TRUE;
  case GDK_KEY_End: case GDK_KEY_KP_End: omarchy_cursor_model_end (self); return TRUE;
  case GDK_KEY_Tab: case GDK_KEY_KP_Tab:
    omarchy_cursor_model_move_section (self, state & GDK_SHIFT_MASK ? -1 : 1);
    return TRUE;
  case GDK_KEY_ISO_Left_Tab: omarchy_cursor_model_move_section (self, -1); return TRUE;
  case GDK_KEY_Escape: g_signal_emit (self, signals[CLOSE_REQUESTED], 0); return TRUE;
  case GDK_KEY_Delete: case GDK_KEY_KP_Delete:
    g_signal_emit (self, signals[DELETE_REQUESTED], 0, self->focus_section, self->selected_index);
    return TRUE;
  default: break;
  }
  if (omarchy_key_is_activate (keyval)) {
    g_signal_emit (self, signals[ACTIVATED], 0, self->focus_section, self->selected_index);
    return TRUE;
  }
  switch (t) {
  case 'j': omarchy_cursor_model_move (self, 0, 1); return TRUE;
  case 'k': omarchy_cursor_model_move (self, 0, -1); return TRUE;
  case 'l': omarchy_cursor_model_move (self, 1, 0); return TRUE;
  case 'h': omarchy_cursor_model_move (self, -1, 0); return TRUE;
  case 'x': case 'X':
    g_signal_emit (self, signals[DELETE_REQUESTED], 0, self->focus_section, self->selected_index);
    return TRUE;
  default: break;
  }
  g_autofree char *typed = omarchy_cursor_model_printable (keyval, state);
  if (!typed)
    return FALSE;
  g_signal_emit (self, signals[TEXT_TYPED], 0, typed);
  return TRUE;
}

/* PointerMoveGate.moved */
static gboolean
gate_moved (OmarchyCursorModel *self, GtkWidget *item, double x, double y)
{
  if (!item) {
    gate_reset (self);
    return FALSE;
  }
  GtkWidget *target = self->reference_item ? self->reference_item : item;
  graphene_point_t p = GRAPHENE_POINT_INIT (x, y);
  if (target != item && !gtk_widget_compute_point (item, target, &GRAPHENE_POINT_INIT (x, y), &p))
    p = GRAPHENE_POINT_INIT (x, y);
  gboolean first = !self->primed;
  gboolean did = !first ? fabs (p.x - self->last_x) > 1 || fabs (p.y - self->last_y) > 1 : self->initial_allowed;
  /* Keep the last accepted position while filtering jitter, so slow steps add up. */
  if (first || did) {
    self->last_x = p.x;
    self->last_y = p.y;
  }
  self->primed = TRUE;
  self->initial_allowed = FALSE;
  return did;
}

/**
 * omarchy_cursor_model_pointer_moved:
 * @self: the model
 * @item: the row the pointer moved over
 * @x: the pointer in @item's coordinates
 * @y: the pointer in @item's coordinates
 * @section: the row's section
 * @index: the row's index
 *
 * Moves the cursor to the row, when the pointer really moved (see #OmarchyCursorModel).
 *
 * Returns: whether the sample counted
 */
gboolean
omarchy_cursor_model_pointer_moved (OmarchyCursorModel *self, GtkWidget *item, double x, double y,
                                    const char *section, int index)
{
  g_return_val_if_fail (OMARCHY_IS_CURSOR_MODEL (self), FALSE);
  if (!gate_moved (self, item, x, y))
    return FALSE;
  if (omarchy_cursor_model_is_at (self, section, index))
    return TRUE;
  omarchy_cursor_model_set (self, section, index);
  announce (self, FALSE);
  return TRUE;
}

/**
 * omarchy_cursor_model_allow_initial_sample:
 * @self: the model
 *
 * A view opened by the pointer keeps the row under it: the next sample counts.
 */
void
omarchy_cursor_model_allow_initial_sample (OmarchyCursorModel *self)
{
  gate_reset (self);
  self->initial_allowed = TRUE;
}

/* ---- wiring ------------------------------------------------------------------------------ */

static gboolean
attached_key (GtkWidget *w, guint keyval, GdkModifierType state)
{
  OmarchyCursorModel *self = g_object_get_data (G_OBJECT (w), "omarchy-cursor-model");
  return self ? omarchy_cursor_model_handle_key (self, keyval, state) : FALSE;
}

/**
 * omarchy_cursor_model_attach:
 * @self: the model
 * @widget: the panel (or the #OmarchyScrollArea) whose keys move the cursor
 *
 * Hands @widget's keys to omarchy_cursor_model_handle_key() (Qt's
 * `Keys.onPressed: cursor.handleKey(event)`) and makes it focusable.
 */
void
omarchy_cursor_model_attach (OmarchyCursorModel *self, GtkWidget *widget)
{
  g_return_if_fail (OMARCHY_IS_CURSOR_MODEL (self));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  gboolean had = g_object_get_data (G_OBJECT (widget), "omarchy-cursor-model") != NULL;
  g_object_set_data_full (G_OBJECT (widget), "omarchy-cursor-model", g_object_ref (self), g_object_unref);
  if (!had)
    omarchy_widget_add_keys (widget, attached_key);
  gtk_widget_set_focusable (widget, TRUE);
}

typedef struct {
  OmarchyCursorModel *model; /* weak */
  char *section;
  int index;
} Binding;

static void
binding_free (gpointer data)
{
  Binding *b = data;
  if (b->model)
    g_object_remove_weak_pointer (G_OBJECT (b->model), (gpointer *) &b->model);
  g_free (b->section);
  g_free (b);
}

static void
binding_sync (GtkWidget *row)
{
  Binding *b = g_object_get_data (G_OBJECT (row), "omarchy-cursor-binding");
  if (!b || !b->model)
    return;
  g_object_set (row, "has-cursor", omarchy_cursor_model_is_at (b->model, b->section, b->index), NULL);
}

static void
binding_pointer (GtkWidget *row, double x, double y, gpointer data)
{
  Binding *b = g_object_get_data (G_OBJECT (row), "omarchy-cursor-binding");
  if (b && b->model)
    omarchy_cursor_model_pointer_moved (b->model, row, x, y, b->section, b->index);
}

/**
 * omarchy_cursor_model_bind:
 * @self: the model
 * @row: a row with a `has-cursor` property and a `pointer-moved` signal (#OmarchyListRow)
 * @section: the row's section
 * @index: the row's index in it
 *
 * Keeps the row's cursor flag on omarchy_cursor_model_is_at() and hands its pointer to
 * omarchy_cursor_model_pointer_moved(), as the Qt gallery's
 * `hasCursor: cursor.isAt(..)`, `onPointerMoved: cursor.pointerMoved(..)` do.
 */
void
omarchy_cursor_model_bind (OmarchyCursorModel *self, GtkWidget *row, const char *section, int index)
{
  g_return_if_fail (OMARCHY_IS_CURSOR_MODEL (self));
  g_return_if_fail (GTK_IS_WIDGET (row));
  gboolean had = g_object_get_data (G_OBJECT (row), "omarchy-cursor-binding") != NULL;
  Binding *b = g_new0 (Binding, 1);
  b->model = self;
  g_object_add_weak_pointer (G_OBJECT (self), (gpointer *) &b->model);
  b->section = g_strdup (section ? section : "");
  b->index = index;
  g_object_set_data_full (G_OBJECT (row), "omarchy-cursor-binding", b, binding_free);
  if (!had && g_signal_lookup ("pointer-moved", G_OBJECT_TYPE (row)))
    g_signal_connect (row, "pointer-moved", G_CALLBACK (binding_pointer), NULL);
  g_signal_connect_object (self, "notify::focus-section", G_CALLBACK (binding_sync), row, G_CONNECT_SWAPPED);
  g_signal_connect_object (self, "notify::selected-index", G_CALLBACK (binding_sync), row, G_CONNECT_SWAPPED);
  g_signal_connect_object (self, "notify::cursor-active", G_CALLBACK (binding_sync), row, G_CONNECT_SWAPPED);
  binding_sync (row);
}

/* ---- GObject ----------------------------------------------------------------------------- */

static void
section_clear (gpointer p)
{
  g_free (((Section *) p)->name);
}

static void
omarchy_cursor_model_init (OmarchyCursorModel *self)
{
  self->sections = g_array_new (FALSE, TRUE, sizeof (Section));
  g_array_set_clear_func (self->sections, section_clear);
  self->focus_section = g_strdup ("");
  self->columns = 1;
  self->page_size = 5;
}

static void
cursor_model_dispose (GObject *o)
{
  omarchy_cursor_model_set_reference_item (OMARCHY_CURSOR_MODEL (o), NULL);
  G_OBJECT_CLASS (omarchy_cursor_model_parent_class)->dispose (o);
}

static void
cursor_model_finalize (GObject *o)
{
  OmarchyCursorModel *self = OMARCHY_CURSOR_MODEL (o);
  g_array_unref (self->sections);
  g_free (self->focus_section);
  G_OBJECT_CLASS (omarchy_cursor_model_parent_class)->finalize (o);
}

static void
cursor_model_set_property (GObject *o, guint id, const GValue *v, GParamSpec *pspec)
{
  OmarchyCursorModel *self = OMARCHY_CURSOR_MODEL (o);
  switch (id) {
  case PROP_COLUMNS: omarchy_cursor_model_set_columns (self, g_value_get_int (v)); break;
  case PROP_PAGE_SIZE: omarchy_cursor_model_set_page_size (self, g_value_get_int (v)); break;
  case PROP_REFERENCE_ITEM: omarchy_cursor_model_set_reference_item (self, g_value_get_object (v)); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
cursor_model_get_property (GObject *o, guint id, GValue *v, GParamSpec *pspec)
{
  OmarchyCursorModel *self = OMARCHY_CURSOR_MODEL (o);
  switch (id) {
  case PROP_FOCUS_SECTION: g_value_set_string (v, self->focus_section); break;
  case PROP_SELECTED_INDEX: g_value_set_int (v, self->selected_index); break;
  case PROP_CURSOR_ACTIVE: g_value_set_boolean (v, self->cursor_active); break;
  case PROP_COLUMNS: g_value_set_int (v, self->columns); break;
  case PROP_PAGE_SIZE: g_value_set_int (v, self->page_size); break;
  case PROP_REFERENCE_ITEM: g_value_set_object (v, self->reference_item); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
omarchy_cursor_model_class_init (OmarchyCursorModelClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  oc->dispose = cursor_model_dispose;
  oc->finalize = cursor_model_finalize;
  oc->set_property = cursor_model_set_property;
  oc->get_property = cursor_model_get_property;
  const GParamFlags ro = G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY;
  const GParamFlags rw = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY;
  /**
   * OmarchyCursorModel:focus-section:
   *
   * The cursor's section.
   */
  props[PROP_FOCUS_SECTION] = g_param_spec_string ("focus-section", NULL, NULL, "", ro);
  /**
   * OmarchyCursorModel:selected-index:
   *
   * The cursor's row in its section.
   */
  props[PROP_SELECTED_INDEX] = g_param_spec_int ("selected-index", NULL, NULL, 0, G_MAXINT, 0, ro);
  /**
   * OmarchyCursorModel:cursor-active:
   *
   * Whether the cursor shows.
   */
  props[PROP_CURSOR_ACTIVE] = g_param_spec_boolean ("cursor-active", NULL, NULL, FALSE, ro);
  /**
   * OmarchyCursorModel:columns:
   *
   * Tiles to a line, for a grid (#OmarchyTileGrid); 1 for a list.
   */
  props[PROP_COLUMNS] = g_param_spec_int ("columns", NULL, NULL, 1, G_MAXINT, 1, rw);
  /**
   * OmarchyCursorModel:page-size:
   *
   * The lines Page Up and Page Down move.
   */
  props[PROP_PAGE_SIZE] = g_param_spec_int ("page-size", NULL, NULL, 0, G_MAXINT, 5, rw);
  /**
   * OmarchyCursorModel:reference-item:
   *
   * A widget that doesn't scroll (the panel), that pointer samples are measured against.
   */
  props[PROP_REFERENCE_ITEM] = g_param_spec_object ("reference-item", NULL, NULL, GTK_TYPE_WIDGET, rw);
  g_object_class_install_properties (oc, N_PROPS, props);

  /**
   * OmarchyCursorModel::moved:
   * @self: the model
   * @section: the cursor's section
   * @index: its row
   * @by_key: whether a key moved it (reveal the row then)
   */
  signals[MOVED] = g_signal_new ("moved", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                 G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);
  /**
   * OmarchyCursorModel::activated:
   * @self: the model
   * @section: the cursor's section
   * @index: its row
   */
  signals[ACTIVATED] = g_signal_new ("activated", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                     G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_INT);
  /**
   * OmarchyCursorModel::delete-requested:
   * @self: the model
   * @section: the cursor's section
   * @index: its row
   */
  signals[DELETE_REQUESTED] = g_signal_new ("delete-requested", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL,
                                            NULL, NULL, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_INT);
  /**
   * OmarchyCursorModel::close-requested:
   * @self: the model
   */
  signals[CLOSE_REQUESTED] = g_signal_new ("close-requested", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL,
                                           NULL, NULL, G_TYPE_NONE, 0);
  /**
   * OmarchyCursorModel::text-typed:
   * @self: the model
   * @text: a printable key nothing else took (type-ahead filter text)
   */
  signals[TEXT_TYPED] = g_signal_new ("text-typed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                      G_TYPE_NONE, 1, G_TYPE_STRING);
}

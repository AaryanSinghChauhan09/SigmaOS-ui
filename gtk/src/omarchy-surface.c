/* omarchy-surface.c: SectionHeader (PanelSectionHeader.qml), Separator (PanelSeparator.qml),
 * Card (Card.qml), MenuRow (PopupMenuRow.qml) and Tooltip (PanelToolTip.qml), drawn as the Qt
 * kit draws them. */
#include "omarchy-private.h"

#include <math.h>

/* ========================================================================= SectionHeader === */

/**
 * OmarchySectionHeader:
 *
 * The caption that introduces a panel section: `caption`, bold, `textDim`, with room above for
 * the font's overshoot (`ceil(caption × 0.15)`).
 */
struct _OmarchySectionHeader {
  GtkWidget parent_instance;
  char *text;
};

enum { H_PROP_0, H_PROP_TEXT, H_N_PROPS };
static GParamSpec *h_props[H_N_PROPS];

G_DEFINE_FINAL_TYPE (OmarchySectionHeader, omarchy_section_header, GTK_TYPE_WIDGET)

static int
header_pad (void)
{
  return (int) ceil (omarchy_tokens ()->caption * 0.15);
}

static void
header_measure (GtkWidget *w, GtkOrientation o, int for_size, int *min, int *nat, int *mb, int *nb)
{
  OmarchySectionHeader *self = OMARCHY_SECTION_HEADER (w);
  const OmarchyTokens *t = omarchy_tokens ();
  if (o == GTK_ORIENTATION_HORIZONTAL) {
    g_autoptr (PangoLayout) l = omarchy_layout (w, self->text, t->caption, TRUE);
    int tw;
    omarchy_layout_size (l, &tw, NULL);
    *min = 0;
    *nat = tw;
  } else {
    *min = *nat = omarchy_line_height (t->caption) + header_pad ();
  }
}

static void
header_snapshot (GtkWidget *w, GtkSnapshot *s)
{
  OmarchySectionHeader *self = OMARCHY_SECTION_HEADER (w);
  const OmarchyTokens *p = omarchy_paint ();
  g_autoptr (PangoLayout) l = omarchy_layout (w, self->text, p->caption, TRUE);
  int width = gtk_widget_get_width (w), tw;
  omarchy_layout_size (l, &tw, NULL);
  float x = gtk_widget_get_direction (w) == GTK_TEXT_DIR_RTL ? width - tw : 0;
  omarchy_draw_layout (s, l, x, header_pad (), &p->text_dim);
}

static void
omarchy_section_header_init (OmarchySectionHeader *self)
{
  self->text = g_strdup ("");
  omarchy_widget_follow (GTK_WIDGET (self));
}

static void
header_finalize (GObject *o)
{
  g_free (OMARCHY_SECTION_HEADER (o)->text);
  G_OBJECT_CLASS (omarchy_section_header_parent_class)->finalize (o);
}

static void
header_set_property (GObject *o, guint id, const GValue *v, GParamSpec *pspec)
{
  if (id == H_PROP_TEXT)
    omarchy_section_header_set_text (OMARCHY_SECTION_HEADER (o), g_value_get_string (v));
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
}

static void
header_get_property (GObject *o, guint id, GValue *v, GParamSpec *pspec)
{
  if (id == H_PROP_TEXT)
    g_value_set_string (v, OMARCHY_SECTION_HEADER (o)->text);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
}

static void
omarchy_section_header_class_init (OmarchySectionHeaderClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  oc->finalize = header_finalize;
  oc->set_property = header_set_property;
  oc->get_property = header_get_property;
  wc->measure = header_measure;
  wc->snapshot = header_snapshot;
  h_props[H_PROP_TEXT] = g_param_spec_string ("text", NULL, NULL, "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (oc, H_N_PROPS, h_props);
  gtk_widget_class_set_css_name (wc, "omarchy-section-header");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_HEADING);
}

/**
 * omarchy_section_header_new:
 * @text: (nullable): the caption
 *
 * Returns: (transfer floating): a section header
 */
GtkWidget *
omarchy_section_header_new (const char *text)
{
  return g_object_new (OMARCHY_TYPE_SECTION_HEADER, "text", text ? text : "", NULL);
}

OMARCHY_STRING_ACCESSORS (OmarchySectionHeader, section_header, text, h_props, H_PROP_TEXT, TRUE)

/* ============================================================================= Separator === */

/**
 * OmarchySeparator:
 *
 * A 1 px rule in the foreground at 12% (the `track` role).
 */
struct _OmarchySeparator {
  GtkWidget parent_instance;
  gboolean vertical;
};

G_DEFINE_FINAL_TYPE (OmarchySeparator, omarchy_separator, GTK_TYPE_WIDGET)

static void
separator_measure (GtkWidget *w, GtkOrientation o, int for_size, int *min, int *nat, int *mb, int *nb)
{
  gboolean along = (o == GTK_ORIENTATION_HORIZONTAL) != OMARCHY_SEPARATOR (w)->vertical;
  *min = along ? 0 : 1;
  *nat = along ? 100 : 1;
}

static void
separator_snapshot (GtkWidget *w, GtkSnapshot *s)
{
  const GdkRGBA *c = &omarchy_paint ()->track;
  gtk_snapshot_append_color (s, c, &GRAPHENE_RECT_INIT (0, 0, gtk_widget_get_width (w), gtk_widget_get_height (w)));
}

static void
omarchy_separator_init (OmarchySeparator *self)
{
  omarchy_widget_follow (GTK_WIDGET (self));
  gtk_widget_set_hexpand (GTK_WIDGET (self), FALSE);
}

static void
omarchy_separator_class_init (OmarchySeparatorClass *klass)
{
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  wc->measure = separator_measure;
  wc->snapshot = separator_snapshot;
  gtk_widget_class_set_css_name (wc, "omarchy-separator");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_SEPARATOR);
}

/**
 * omarchy_separator_new:
 *
 * Returns: (transfer floating): a horizontal rule
 */
GtkWidget *
omarchy_separator_new (void)
{
  return g_object_new (OMARCHY_TYPE_SEPARATOR, NULL);
}

void
omarchy_separator_set_vertical (OmarchySeparator *self, gboolean vertical)
{
  self->vertical = !!vertical;
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

/* ================================================================================== Card === */

/**
 * OmarchyCard:
 *
 * The raised surface panels, popups and flyouts are drawn on: the popups surface (background,
 * text, border, which follows Hyprland's active border), Hyprland's rounding, `panelPadding`
 * inside and children `panelGap` apart. #OmarchyCard:title adds a section header above them.
 */
struct _OmarchyCard {
  GtkWidget parent_instance;
  GtkWidget *box;
  GtkWidget *header;
  int spacing; /* -1: panelGap */
  int padding; /* -1: panelPadding */
};

enum { C_PROP_0, C_PROP_TITLE, C_N_PROPS };
static GParamSpec *c_props[C_N_PROPS];

static void card_buildable_init (GtkBuildableIface *iface);
G_DEFINE_FINAL_TYPE_WITH_CODE (OmarchyCard, omarchy_card, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, card_buildable_init))

static OmarchyBorder
card_border (void)
{
  const OmarchyTokens *p = omarchy_paint ();
  return p->popups.border;
}

static void
card_insets (OmarchyCard *self, float in[4])
{
  OmarchyBorder b = card_border ();
  int pad = self->padding >= 0 ? self->padding : omarchy_tokens ()->panel_padding;
  for (int i = 0; i < 4; i++)
    in[i] = b.widths[i] + pad;
}

static void
card_restyle (OmarchyCard *self)
{
  gtk_box_set_spacing (GTK_BOX (self->box), self->spacing >= 0 ? self->spacing : omarchy_tokens ()->panel_gap);
}

static void
card_measure (GtkWidget *w, GtkOrientation o, int for_size, int *min, int *nat, int *mb, int *nb)
{
  OmarchyCard *self = OMARCHY_CARD (w);
  float in[4];
  card_insets (self, in);
  int extra = o == GTK_ORIENTATION_HORIZONTAL ? (int) (in[1] + in[3]) : (int) (in[0] + in[2]);
  int other = o == GTK_ORIENTATION_HORIZONTAL ? (int) (in[0] + in[2]) : (int) (in[1] + in[3]);
  gtk_widget_measure (self->box, o, for_size >= 0 ? MAX (0, for_size - other) : -1, min, nat, NULL, NULL);
  *min += extra;
  *nat += extra;
}

static void
card_allocate (GtkWidget *w, int width, int height, int baseline)
{
  OmarchyCard *self = OMARCHY_CARD (w);
  float in[4];
  card_insets (self, in);
  GtkAllocation a = { (int) in[3], (int) in[0], MAX (0, width - (int) (in[1] + in[3])), MAX (0, height - (int) (in[0] + in[2])) };
  gtk_widget_size_allocate (self->box, &a, -1);
}

static void
card_snapshot (GtkWidget *w, GtkSnapshot *s)
{
  const OmarchyTokens *p = omarchy_paint ();
  OmarchyBorder b = card_border ();
  omarchy_draw_surface (s, &GRAPHENE_RECT_INIT (0, 0, gtk_widget_get_width (w), gtk_widget_get_height (w)), p->radius,
                        &p->popups.background, &b);
  GTK_WIDGET_CLASS (omarchy_card_parent_class)->snapshot (w, s);
}

static void
omarchy_card_init (OmarchyCard *self)
{
  self->spacing = -1;
  self->padding = -1;
  self->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_parent (self->box, GTK_WIDGET (self));
  self->header = omarchy_section_header_new ("");
  gtk_widget_set_visible (self->header, FALSE);
  gtk_box_append (GTK_BOX (self->box), self->header);
  card_restyle (self);
  g_signal_connect_object (omarchy_theme_get_default (), "restyle", G_CALLBACK (card_restyle), self, G_CONNECT_SWAPPED);
  omarchy_widget_follow (GTK_WIDGET (self));
}

static void
card_dispose (GObject *o)
{
  OmarchyCard *self = OMARCHY_CARD (o);
  g_clear_pointer (&self->box, gtk_widget_unparent);
  G_OBJECT_CLASS (omarchy_card_parent_class)->dispose (o);
}

static void
card_set_property (GObject *o, guint id, const GValue *v, GParamSpec *pspec)
{
  if (id == C_PROP_TITLE)
    omarchy_card_set_title (OMARCHY_CARD (o), g_value_get_string (v));
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
}

static void
card_get_property (GObject *o, guint id, GValue *v, GParamSpec *pspec)
{
  if (id == C_PROP_TITLE)
    g_value_set_string (v, omarchy_card_get_title (OMARCHY_CARD (o)));
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
}

static GtkBuildableIface *card_parent_buildable;

static void
card_add_child (GtkBuildable *b, GtkBuilder *builder, GObject *child, const char *type)
{
  if (GTK_IS_WIDGET (child))
    omarchy_card_append (OMARCHY_CARD (b), GTK_WIDGET (child));
  else
    card_parent_buildable->add_child (b, builder, child, type);
}

static void
card_buildable_init (GtkBuildableIface *iface)
{
  card_parent_buildable = g_type_interface_peek_parent (iface);
  iface->add_child = card_add_child;
}

static void
omarchy_card_class_init (OmarchyCardClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  oc->dispose = card_dispose;
  oc->set_property = card_set_property;
  oc->get_property = card_get_property;
  wc->measure = card_measure;
  wc->size_allocate = card_allocate;
  wc->snapshot = card_snapshot;
  c_props[C_PROP_TITLE] = g_param_spec_string ("title", NULL, NULL, "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (oc, C_N_PROPS, c_props);
  gtk_widget_class_set_css_name (wc, "omarchy-card");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_GROUP);
}

/**
 * omarchy_card_new:
 * @title: (nullable): a section header above the children, or %NULL
 *
 * Returns: (transfer floating): a card
 */
GtkWidget *
omarchy_card_new (const char *title)
{
  return g_object_new (OMARCHY_TYPE_CARD, "title", title ? title : "", NULL);
}

const char *
omarchy_card_get_title (OmarchyCard *self)
{
  return omarchy_section_header_get_text (OMARCHY_SECTION_HEADER (self->header));
}

void
omarchy_card_set_title (OmarchyCard *self, const char *title)
{
  omarchy_section_header_set_text (OMARCHY_SECTION_HEADER (self->header), title);
  gtk_widget_set_visible (self->header, title && *title);
  g_object_notify_by_pspec (G_OBJECT (self), c_props[C_PROP_TITLE]);
}

/**
 * omarchy_card_append:
 * @self: the card
 * @child: (transfer floating): a child, stacked below the others
 */
void
omarchy_card_append (OmarchyCard *self, GtkWidget *child)
{
  gtk_box_append (GTK_BOX (self->box), child);
}

void
omarchy_card_remove (OmarchyCard *self, GtkWidget *child)
{
  gtk_box_remove (GTK_BOX (self->box), child);
}

/**
 * omarchy_card_set_spacing:
 * @self: the card
 * @spacing: between children, or -1 for `panelGap`
 */
void
omarchy_card_set_spacing (OmarchyCard *self, int spacing)
{
  self->spacing = spacing;
  card_restyle (self);
}

/**
 * omarchy_card_set_padding:
 * @self: the card
 * @padding: inside the border, or -1 for `panelPadding`
 */
void
omarchy_card_set_padding (OmarchyCard *self, int padding)
{
  self->padding = padding;
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

/* =============================================================================== MenuRow === */

/**
 * OmarchyMenuRow:
 *
 * One row of a small popup menu: a label, an accent check when #OmarchyMenuRow:checked, the
 * hover fill under the pointer or when #OmarchyMenuRow:highlighted (a menu walked with the
 * keyboard). ::triggered on a click, or Return/Space while focused.
 */
struct _OmarchyMenuRow {
  GtkWidget parent_instance;
  char *text;
  gboolean checked, highlighted;
};

enum { M_PROP_0, M_PROP_TEXT, M_PROP_CHECKED, M_PROP_HIGHLIGHTED, M_N_PROPS };
static GParamSpec *m_props[M_N_PROPS];
enum { M_TRIGGERED, M_N_SIGNALS };
static guint m_signals[M_N_SIGNALS];

G_DEFINE_FINAL_TYPE (OmarchyMenuRow, omarchy_menu_row, GTK_TYPE_WIDGET)

static void
menu_row_measure (GtkWidget *w, GtkOrientation o, int for_size, int *min, int *nat, int *mb, int *nb)
{
  OmarchyMenuRow *self = OMARCHY_MENU_ROW (w);
  const OmarchyTokens *t = omarchy_tokens ();
  if (o == GTK_ORIENTATION_VERTICAL) {
    *min = *nat = t->popup_row_height;
    return;
  }
  g_autoptr (PangoLayout) l = omarchy_layout (w, self->text, t->body, FALSE);
  int tw;
  omarchy_layout_size (l, &tw, NULL);
  *min = 0;
  *nat = tw + t->control_padding_x * 2 + (self->checked ? omarchy_line_height (t->body) + t->control_padding_x : 0);
}

static void
menu_row_snapshot (GtkWidget *w, GtkSnapshot *s)
{
  OmarchyMenuRow *self = OMARCHY_MENU_ROW (w);
  const OmarchyTokens *p = omarchy_paint ();
  float W = gtk_widget_get_width (w), H = gtk_widget_get_height (w);
  gboolean rtl = gtk_widget_get_direction (w) == GTK_TEXT_DIR_RTL;
  if (omarchy_widget_hovered (w) || self->highlighted || omarchy_widget_focus_shown (w))
    omarchy_draw_fill (s, &GRAPHENE_RECT_INIT (0, 0, W, H), p->radius, &p->hover.fill);
  int line = omarchy_line_height (p->body);
  g_autoptr (PangoLayout) l = omarchy_layout (w, self->text, p->body, FALSE);
  int tw;
  omarchy_layout_size (l, &tw, NULL);
  omarchy_draw_layout (s, l, rtl ? W - p->control_padding_x - tw : p->control_padding_x, roundf ((H - line) / 2), &p->popups.text);
  if (self->checked) {
    g_autoptr (PangoLayout) c = omarchy_layout (w, "✓", p->body, FALSE);
    int cw;
    omarchy_layout_size (c, &cw, NULL);
    omarchy_draw_layout (s, c, rtl ? p->control_padding_x : W - p->control_padding_x - cw, roundf ((H - line) / 2), &p->accent);
  }
}

static void
menu_row_clicked (GtkWidget *w, guint button, double x, double y)
{
  if (button == GDK_BUTTON_PRIMARY)
    g_signal_emit (w, m_signals[M_TRIGGERED], 0);
}

static gboolean
menu_row_key (GtkWidget *w, guint keyval, GdkModifierType state)
{
  if (omarchy_key_is_activate (keyval)) {
    g_signal_emit (w, m_signals[M_TRIGGERED], 0);
    return TRUE;
  }
  return FALSE;
}

static void
omarchy_menu_row_init (OmarchyMenuRow *self)
{
  GtkWidget *w = GTK_WIDGET (self);
  self->text = g_strdup ("");
  gtk_widget_set_focusable (w, TRUE);
  gtk_widget_set_focus_on_click (w, FALSE);
  omarchy_widget_follow (w);
  omarchy_widget_add_click (w, menu_row_clicked);
  omarchy_widget_add_keys (w, menu_row_key);
}

static void
menu_row_finalize (GObject *o)
{
  g_free (OMARCHY_MENU_ROW (o)->text);
  G_OBJECT_CLASS (omarchy_menu_row_parent_class)->finalize (o);
}

static void
menu_row_set_property (GObject *o, guint id, const GValue *v, GParamSpec *pspec)
{
  OmarchyMenuRow *self = OMARCHY_MENU_ROW (o);
  switch (id) {
  case M_PROP_TEXT: omarchy_menu_row_set_text (self, g_value_get_string (v)); break;
  case M_PROP_CHECKED: omarchy_menu_row_set_checked (self, g_value_get_boolean (v)); break;
  case M_PROP_HIGHLIGHTED: omarchy_menu_row_set_highlighted (self, g_value_get_boolean (v)); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
menu_row_get_property (GObject *o, guint id, GValue *v, GParamSpec *pspec)
{
  OmarchyMenuRow *self = OMARCHY_MENU_ROW (o);
  switch (id) {
  case M_PROP_TEXT: g_value_set_string (v, self->text); break;
  case M_PROP_CHECKED: g_value_set_boolean (v, self->checked); break;
  case M_PROP_HIGHLIGHTED: g_value_set_boolean (v, self->highlighted); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
omarchy_menu_row_class_init (OmarchyMenuRowClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  oc->finalize = menu_row_finalize;
  oc->set_property = menu_row_set_property;
  oc->get_property = menu_row_get_property;
  wc->measure = menu_row_measure;
  wc->snapshot = menu_row_snapshot;
  const GParamFlags f = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY;
  m_props[M_PROP_TEXT] = g_param_spec_string ("text", NULL, NULL, "", f);
  m_props[M_PROP_CHECKED] = g_param_spec_boolean ("checked", NULL, NULL, FALSE, f);
  m_props[M_PROP_HIGHLIGHTED] = g_param_spec_boolean ("highlighted", NULL, NULL, FALSE, f);
  g_object_class_install_properties (oc, M_N_PROPS, m_props);
  m_signals[M_TRIGGERED] = g_signal_new ("triggered", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, 0,
                                         NULL, NULL, NULL, G_TYPE_NONE, 0);
  gtk_widget_class_set_activate_signal (wc, m_signals[M_TRIGGERED]);
  gtk_widget_class_set_css_name (wc, "omarchy-menu-row");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_MENU_ITEM);
}

/**
 * omarchy_menu_row_new:
 * @text: (nullable): the label
 *
 * Returns: (transfer floating): a menu row
 */
GtkWidget *
omarchy_menu_row_new (const char *text)
{
  return g_object_new (OMARCHY_TYPE_MENU_ROW, "text", text ? text : "", NULL);
}

OMARCHY_STRING_ACCESSORS (OmarchyMenuRow, menu_row, text, m_props, M_PROP_TEXT, TRUE)
OMARCHY_BOOL_ACCESSORS (OmarchyMenuRow, menu_row, checked, m_props, M_PROP_CHECKED, FALSE)
OMARCHY_BOOL_ACCESSORS (OmarchyMenuRow, menu_row, highlighted, m_props, M_PROP_HIGHLIGHTED, FALSE)

/* =============================================================================== Tooltip === */

/**
 * OmarchyTooltip:
 *
 * A tooltip as it draws, held open: the tooltip surface, `bodySmall` text, padding
 * `controlPaddingX` × `controlPaddingY`, Hyprland's rounding. With a #OmarchyTooltip:shortcut,
 * an #OmarchyKeyCombo follows the text `controlGap` after it, both vertically centered (Qt's
 * Tooltip). GTK's own tooltips (gtk_widget_set_tooltip_text()) get the plain look from
 * omarchy_gtk_init()'s CSS.
 */
struct _OmarchyTooltip {
  GtkWidget parent_instance;
  char *text;
  char *shortcut;
  GtkWidget *combo; /* the shortcut's keycaps, when there is one */
};

enum { T_PROP_0, T_PROP_TEXT, T_PROP_SHORTCUT, T_N_PROPS };
static GParamSpec *t_props[T_N_PROPS];

G_DEFINE_FINAL_TYPE (OmarchyTooltip, omarchy_tooltip, GTK_TYPE_WIDGET)

static OmarchyBorder
tooltip_border (void)
{
  const OmarchyTokens *p = omarchy_paint ();
  return p->tooltip.border;
}

/* The row inside the padding: the text, then the shortcut's keycaps. With a shortcut the text
 * keeps its fractional advance, so the keycaps land where Qt's Row puts them. */
static void
tooltip_row (OmarchyTooltip *self, double *text_w, int *combo_w, int *combo_h, int *row_h)
{
  const OmarchyTokens *t = omarchy_tokens ();
  g_autoptr (PangoLayout) l = omarchy_layout (GTK_WIDGET (self), self->text, t->body_small, FALSE);
  int rounded;
  omarchy_layout_size (l, &rounded, NULL);
  *text_w = rounded;
  *combo_w = *combo_h = 0;
  if (self->combo) {
    PangoRectangle logical;
    pango_layout_get_extents (l, NULL, &logical);
    *text_w = logical.width / (double) PANGO_SCALE;
    gtk_widget_measure (self->combo, GTK_ORIENTATION_HORIZONTAL, -1, NULL, combo_w, NULL, NULL);
    gtk_widget_measure (self->combo, GTK_ORIENTATION_VERTICAL, -1, NULL, combo_h, NULL, NULL);
  }
  *row_h = MAX (omarchy_line_height (t->body_small), *combo_h);
}

/* Qt's anchors.verticalCenter (alignWhenCentered): an odd height centers on its upper middle. */
static float
tooltip_center (float outer, float inner)
{
  float a = (int) outer % 2 ? (outer + 1) / 2 : outer / 2;
  float b = (int) inner % 2 ? (inner + 1) / 2 : inner / 2;
  return a - b;
}

static void
tooltip_measure (GtkWidget *w, GtkOrientation o, int for_size, int *min, int *nat, int *mb, int *nb)
{
  OmarchyTooltip *self = OMARCHY_TOOLTIP (w);
  const OmarchyTokens *t = omarchy_tokens ();
  OmarchyBorder b = tooltip_border ();
  double tw;
  int cw, ch, rh;
  tooltip_row (self, &tw, &cw, &ch, &rh);
  if (o == GTK_ORIENTATION_HORIZONTAL)
    *min = *nat = (int) ceil (tw + (self->combo ? t->control_gap + cw : 0) + t->control_padding_x * 2 + b.widths[1] + b.widths[3] - 1e-3);
  else
    *min = *nat = rh + t->control_padding_y * 2 + (int) (b.widths[0] + b.widths[2]);
}

static double
tooltip_combo_x (OmarchyTooltip *self, double tw)
{
  OmarchyBorder b = tooltip_border ();
  const OmarchyTokens *t = omarchy_tokens ();
  return b.widths[3] + t->control_padding_x + tw + t->control_gap;
}

static void
tooltip_allocate (GtkWidget *w, int width, int height, int baseline)
{
  OmarchyTooltip *self = OMARCHY_TOOLTIP (w);
  if (!self->combo)
    return;
  const OmarchyTokens *t = omarchy_tokens ();
  OmarchyBorder b = tooltip_border ();
  double tw;
  int cw, ch, rh;
  tooltip_row (self, &tw, &cw, &ch, &rh);
  GtkAllocation a = { (int) floor (tooltip_combo_x (self, tw)),
                      (int) b.widths[0] + t->control_padding_y + (int) tooltip_center (rh, ch), cw, ch };
  gtk_widget_size_allocate (self->combo, &a, -1);
}

static void
tooltip_snapshot (GtkWidget *w, GtkSnapshot *s)
{
  OmarchyTooltip *self = OMARCHY_TOOLTIP (w);
  const OmarchyTokens *p = omarchy_paint ();
  OmarchyBorder b = tooltip_border ();
  omarchy_draw_surface (s, &GRAPHENE_RECT_INIT (0, 0, gtk_widget_get_width (w), gtk_widget_get_height (w)), p->radius,
                        &p->tooltip.background, &b);
  double tw;
  int cw, ch, rh;
  tooltip_row (self, &tw, &cw, &ch, &rh);
  g_autoptr (PangoLayout) l = omarchy_layout (w, self->text, p->body_small, FALSE);
  omarchy_draw_layout (s, l, b.widths[3] + p->control_padding_x,
                       b.widths[0] + p->control_padding_y + tooltip_center (rh, omarchy_line_height (p->body_small)),
                       &p->tooltip.text);
  if (self->combo) {
    /* The keycaps at the text's fractional end, as Qt's Row places them. */
    double x = tooltip_combo_x (self, tw);
    gtk_snapshot_save (s);
    gtk_snapshot_translate (s, &GRAPHENE_POINT_INIT ((float) (x - floor (x)), 0));
    gtk_widget_snapshot_child (w, self->combo, s);
    gtk_snapshot_restore (s);
  }
}

static void
omarchy_tooltip_init (OmarchyTooltip *self)
{
  self->text = g_strdup ("");
  self->shortcut = g_strdup ("");
  omarchy_widget_follow (GTK_WIDGET (self));
}

static void
tooltip_dispose (GObject *o)
{
  g_clear_pointer (&OMARCHY_TOOLTIP (o)->combo, gtk_widget_unparent);
  G_OBJECT_CLASS (omarchy_tooltip_parent_class)->dispose (o);
}

static void
tooltip_finalize (GObject *o)
{
  g_free (OMARCHY_TOOLTIP (o)->text);
  g_free (OMARCHY_TOOLTIP (o)->shortcut);
  G_OBJECT_CLASS (omarchy_tooltip_parent_class)->finalize (o);
}

static void
tooltip_set_property (GObject *o, guint id, const GValue *v, GParamSpec *pspec)
{
  if (id == T_PROP_TEXT)
    omarchy_tooltip_set_text (OMARCHY_TOOLTIP (o), g_value_get_string (v));
  else if (id == T_PROP_SHORTCUT)
    omarchy_tooltip_set_shortcut (OMARCHY_TOOLTIP (o), g_value_get_string (v));
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
}

static void
tooltip_get_property (GObject *o, guint id, GValue *v, GParamSpec *pspec)
{
  if (id == T_PROP_TEXT)
    g_value_set_string (v, OMARCHY_TOOLTIP (o)->text);
  else if (id == T_PROP_SHORTCUT)
    g_value_set_string (v, OMARCHY_TOOLTIP (o)->shortcut);
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
}

static void
omarchy_tooltip_class_init (OmarchyTooltipClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  oc->dispose = tooltip_dispose;
  oc->finalize = tooltip_finalize;
  oc->set_property = tooltip_set_property;
  oc->get_property = tooltip_get_property;
  wc->measure = tooltip_measure;
  wc->size_allocate = tooltip_allocate;
  wc->snapshot = tooltip_snapshot;
  t_props[T_PROP_TEXT] = g_param_spec_string ("text", NULL, NULL, "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * OmarchyTooltip:shortcut:
   *
   * A key combination ("Ctrl+S") drawn as keycaps after the text, or "" for none.
   */
  t_props[T_PROP_SHORTCUT] = g_param_spec_string ("shortcut", NULL, NULL, "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (oc, T_N_PROPS, t_props);
  gtk_widget_class_set_css_name (wc, "omarchy-tooltip");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_TOOLTIP);
}

/**
 * omarchy_tooltip_new:
 * @text: (nullable): the text
 *
 * Returns: (transfer floating): a tooltip surface
 */
GtkWidget *
omarchy_tooltip_new (const char *text)
{
  return g_object_new (OMARCHY_TYPE_TOOLTIP, "text", text ? text : "", NULL);
}

OMARCHY_STRING_ACCESSORS (OmarchyTooltip, tooltip, text, t_props, T_PROP_TEXT, TRUE)

const char *
omarchy_tooltip_get_shortcut (OmarchyTooltip *self)
{
  return self->shortcut;
}

/**
 * omarchy_tooltip_set_shortcut:
 * @self: the tooltip
 * @shortcut: (nullable): a key combination ("Ctrl+S"), or %NULL or "" for none
 */
void
omarchy_tooltip_set_shortcut (OmarchyTooltip *self, const char *shortcut)
{
  if (!g_strcmp0 (self->shortcut, shortcut ? shortcut : ""))
    return;
  g_free (self->shortcut);
  self->shortcut = g_strdup (shortcut ? shortcut : "");
  if (!*self->shortcut) {
    g_clear_pointer (&self->combo, gtk_widget_unparent);
  } else if (self->combo) {
    omarchy_key_combo_set_keys (OMARCHY_KEY_COMBO (self->combo), self->shortcut);
  } else {
    self->combo = omarchy_key_combo_new (self->shortcut);
    gtk_widget_set_parent (self->combo, GTK_WIDGET (self));
  }
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), t_props[T_PROP_SHORTCUT]);
}

/* =============================================================================== Surface === */

/**
 * OmarchySurface:
 *
 * Qt's BorderSurface: a fill and a border spec from the tokens, named by their canonical JSON
 * paths (`controls.hover.fill`, `surfaces.popups.border`), on Hyprland's rounding, around an
 * optional child. For an app's own panels and chips; follows the theme and its fade.
 */
struct _OmarchySurface {
  GtkWidget parent_instance;
  GtkWidget *child;
  char *fill, *border;
  int border_width; /* -1: the spec's */
  gboolean small;
  int pad_x, pad_y;
};

static void surface_buildable_init (GtkBuildableIface *iface);
G_DEFINE_FINAL_TYPE_WITH_CODE (OmarchySurface, omarchy_surface, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_BUILDABLE, surface_buildable_init))

static OmarchyBorder
surface_border (OmarchySurface *self)
{
  const OmarchyBorder *b = self->border ? omarchy_tokens_border_at (omarchy_paint (), self->border) : NULL;
  if (!b)
    return omarchy_border_none ();
  return self->border_width >= 0 ? omarchy_border_with_width (b, self->border_width) : *b;
}

static void
surface_measure (GtkWidget *w, GtkOrientation o, int for_size, int *min, int *nat, int *mb, int *nb)
{
  OmarchySurface *self = OMARCHY_SURFACE (w);
  OmarchyBorder b = surface_border (self);
  int extra = o == GTK_ORIENTATION_HORIZONTAL ? (int) (b.widths[1] + b.widths[3]) + 2 * self->pad_x
                                              : (int) (b.widths[0] + b.widths[2]) + 2 * self->pad_y;
  *min = *nat = extra;
  if (self->child) {
    int cmin, cnat;
    gtk_widget_measure (self->child, o, -1, &cmin, &cnat, NULL, NULL);
    *min += cmin;
    *nat += cnat;
  }
}

static void
surface_allocate (GtkWidget *w, int width, int height, int baseline)
{
  OmarchySurface *self = OMARCHY_SURFACE (w);
  if (!self->child)
    return;
  OmarchyBorder b = surface_border (self);
  GtkAllocation a = { (int) b.widths[3] + self->pad_x, (int) b.widths[0] + self->pad_y,
                      MAX (0, width - (int) (b.widths[1] + b.widths[3]) - 2 * self->pad_x),
                      MAX (0, height - (int) (b.widths[0] + b.widths[2]) - 2 * self->pad_y) };
  gtk_widget_size_allocate (self->child, &a, -1);
}

static void
surface_snapshot (GtkWidget *w, GtkSnapshot *s)
{
  OmarchySurface *self = OMARCHY_SURFACE (w);
  const OmarchyTokens *p = omarchy_paint ();
  const GdkRGBA *fill = self->fill ? omarchy_tokens_color_at (p, self->fill) : NULL;
  OmarchyBorder b = surface_border (self);
  omarchy_draw_surface (s, &GRAPHENE_RECT_INIT (0, 0, gtk_widget_get_width (w), gtk_widget_get_height (w)),
                        self->small ? MIN (4, p->radius) : p->radius, fill, &b);
  if (self->child)
    gtk_widget_snapshot_child (w, self->child, s);
}

static void
omarchy_surface_init (OmarchySurface *self)
{
  self->border_width = -1;
  omarchy_widget_follow (GTK_WIDGET (self));
}

static void
surface_dispose (GObject *o)
{
  OmarchySurface *self = OMARCHY_SURFACE (o);
  g_clear_pointer (&self->child, gtk_widget_unparent);
  G_OBJECT_CLASS (omarchy_surface_parent_class)->dispose (o);
}

static void
surface_finalize (GObject *o)
{
  OmarchySurface *self = OMARCHY_SURFACE (o);
  g_free (self->fill);
  g_free (self->border);
  G_OBJECT_CLASS (omarchy_surface_parent_class)->finalize (o);
}

static GtkBuildableIface *surface_parent_buildable;

static void
surface_add_child (GtkBuildable *b, GtkBuilder *builder, GObject *child, const char *type)
{
  if (GTK_IS_WIDGET (child))
    omarchy_surface_set_child (OMARCHY_SURFACE (b), GTK_WIDGET (child));
  else
    surface_parent_buildable->add_child (b, builder, child, type);
}

static void
surface_buildable_init (GtkBuildableIface *iface)
{
  surface_parent_buildable = g_type_interface_peek_parent (iface);
  iface->add_child = surface_add_child;
}

static void
omarchy_surface_class_init (OmarchySurfaceClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  oc->dispose = surface_dispose;
  oc->finalize = surface_finalize;
  wc->measure = surface_measure;
  wc->size_allocate = surface_allocate;
  wc->snapshot = surface_snapshot;
  gtk_widget_class_set_css_name (wc, "omarchy-surface");
}

/**
 * omarchy_surface_new:
 * @fill: (nullable): a color's path (`controls.normal.fill`, `surfaces.popups.background`)
 * @border: (nullable): a border spec's path (`controls.normal.border`, `surfaces.popups.border`)
 *
 * Returns: (transfer floating): a surface
 */
GtkWidget *
omarchy_surface_new (const char *fill, const char *border)
{
  OmarchySurface *self = g_object_new (OMARCHY_TYPE_SURFACE, NULL);
  omarchy_surface_set_fill (self, fill);
  omarchy_surface_set_border (self, border);
  return GTK_WIDGET (self);
}

void
omarchy_surface_set_fill (OmarchySurface *self, const char *fill)
{
  g_free (self->fill);
  self->fill = g_strdup (fill);
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
omarchy_surface_set_border (OmarchySurface *self, const char *border)
{
  g_free (self->border);
  self->border = g_strdup (border);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

/**
 * omarchy_surface_set_border_width:
 * @self: the surface
 * @width: every side's width, or -1 for the spec's own
 */
void
omarchy_surface_set_border_width (OmarchySurface *self, int width)
{
  self->border_width = width;
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

/**
 * omarchy_surface_set_small:
 * @self: the surface
 * @small: corners `min(4, radius)`, for small parts
 */
void
omarchy_surface_set_small (OmarchySurface *self, gboolean small)
{
  self->small = !!small;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
omarchy_surface_set_padding (OmarchySurface *self, int x, int y)
{
  self->pad_x = MAX (0, x);
  self->pad_y = MAX (0, y);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

/**
 * omarchy_surface_set_child:
 * @self: the surface
 * @child: (nullable) (transfer floating): the content
 */
void
omarchy_surface_set_child (OmarchySurface *self, GtkWidget *child)
{
  g_clear_pointer (&self->child, gtk_widget_unparent);
  self->child = child;
  if (child)
    gtk_widget_set_parent (child, GTK_WIDGET (self));
}

/**
 * omarchy_surface_get_child:
 * @self: the surface
 *
 * Returns: (transfer none) (nullable): the content
 */
GtkWidget *
omarchy_surface_get_child (OmarchySurface *self)
{
  return self->child;
}

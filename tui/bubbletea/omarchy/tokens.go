package omarchy

import (
	"bytes"
	"encoding/json"
)

// Tokens are the core's resolved tokens (spec/tokens.md), read from `omarchy-theme --state`:
// never re-derived here. Named colors (`colors.toml`'s keys) and the kit's strings in the display
// language come with them.
type Tokens struct {
	Schema       int                `json:"schema"`
	Mode         string             `json:"mode"`
	Radius       int64              `json:"radius"`
	GapsOut      float64            `json:"gapsOut"`
	ReduceMotion bool               `json:"reduceMotion"`
	Palette      Palette            `json:"palette"`
	Roles        Roles              `json:"roles"`
	Controls     Controls           `json:"controls"`
	Surfaces     Surfaces           `json:"surfaces"`
	Font         Font               `json:"font"`
	Motion       Motion             `json:"motion"`
	Spacing      map[string]float64 `json:"spacing"`

	// Theme is the Omarchy theme's name, Language the display language, IconTheme the icon theme
	// it names (from the state line, not the token JSON).
	Theme     string `json:"-"`
	Language  string `json:"-"`
	IconTheme string `json:"-"`
	// Strings are the kit's strings in the display language (`common.cancel`, ...).
	Strings map[string]string `json:"-"`
}

// Palette is the theme's colors; Named holds every key of its colors.toml (red, orange, ...).
type Palette struct {
	Foreground Rgba            `json:"foreground"`
	Background Rgba            `json:"background"`
	Accent     Rgba            `json:"accent"`
	Urgent     Rgba            `json:"urgent"`
	Muted      Rgba            `json:"muted"`
	Success    Rgba            `json:"success"`
	Warning    Rgba            `json:"warning"`
	Named      map[string]Rgba `json:"-"`
}

type Roles struct {
	TextDim   Rgba `json:"textDim"`
	TextFaint Rgba `json:"textFaint"`
	Track     Rgba `json:"track"`
	Scrim     Rgba `json:"scrim"`
}

type Gradient struct {
	Colors []Rgba  `json:"colors"`
	Angle  float64 `json:"angle"`
}

// BorderSpec is a border: its color (a gradient's first stop), widths (top, right, bottom,
// left) and the gradient, if any.
type BorderSpec struct {
	Color    Rgba       `json:"color"`
	Widths   [4]float64 `json:"widths"`
	Gradient *Gradient  `json:"gradient"`
}

// None is true when every width is 0: the state shows no border.
func (b BorderSpec) None() bool {
	m := 0.0
	for _, w := range b.Widths {
		m = max(m, w)
	}
	return m <= 0
}

type State struct {
	Fill   Rgba       `json:"fill"`
	Text   Rgba       `json:"text"`
	Border BorderSpec `json:"border"`
}

type FillOnly struct {
	Fill Rgba `json:"fill"`
}

type Controls struct {
	Normal    State    `json:"normal"`
	Hover     State    `json:"hover"`
	Focus     State    `json:"focus"`
	Selected  State    `json:"selected"`
	Pressed   FillOnly `json:"pressed"`
	Selection FillOnly `json:"selection"`
}

type Surface struct {
	Background Rgba       `json:"background"`
	Text       Rgba       `json:"text"`
	Border     BorderSpec `json:"border"`
}

type Menu struct {
	Surface
	Scrim              Rgba       `json:"scrim"`
	SelectedBackground Rgba       `json:"selectedBackground"`
	SelectedText       Rgba       `json:"selectedText"`
	SelectedBorder     BorderSpec `json:"selectedBorder"`
}

type Notifications struct {
	Surface
	Countdown Rgba `json:"countdown"`
}

type Polkit struct {
	Surface
	TextError   Rgba       `json:"textError"`
	BorderError BorderSpec `json:"borderError"`
	Accent      Rgba       `json:"accent"`
	Scrim       Rgba       `json:"scrim"`
}

type Surfaces struct {
	Popups        Surface       `json:"popups"`
	Tooltip       Surface       `json:"tooltip"`
	Menu          Menu          `json:"menu"`
	Notifications Notifications `json:"notifications"`
	Polkit        Polkit        `json:"polkit"`
}

// Font is the type scale. The terminal has one font at one size: the sizes only name the role
// (spec/terminal.md, Type).
type Font struct {
	Family         string  `json:"family"`
	ResolvedFamily string  `json:"resolvedFamily"`
	BaseSize       float64 `json:"baseSize"`
	Caption        float64 `json:"caption"`
	BodySmall      float64 `json:"bodySmall"`
	Body           float64 `json:"body"`
	Subtitle       float64 `json:"subtitle"`
	Title          float64 `json:"title"`
	Heading        float64 `json:"heading"`
	Display        float64 `json:"display"`
	DisplayLarge   float64 `json:"displayLarge"`
	Icon           float64 `json:"icon"`
	IconSmall      float64 `json:"iconSmall"`
	IconLarge      float64 `json:"iconLarge"`
}

// Motion: durations in ms, curves as cubic-bezier control points.
type Motion struct {
	Ease    [4]float64 `json:"ease"`
	EaseOut [4]float64 `json:"easeOut"`
	Fast    int64      `json:"fast"`
	Press   int64      `json:"press"`
	Theme   int64      `json:"theme"`
}

// String is the kit's string `key` in the display language, else `fallback`.
func (t *Tokens) String(key, fallback string) string {
	if s, ok := t.Strings[key]; ok && s != "" {
		return s
	}
	return fallback
}

// IsDark is true for a dark theme.
func (t *Tokens) IsDark() bool { return t.Mode == "dark" }

// stateLine is one line of `omarchy-theme --state` / `--watch --state`.
type stateLine struct {
	Schema    int               `json:"schema"`
	Revision  int64             `json:"revision"`
	Theme     string            `json:"theme"`
	IconTheme string            `json:"iconTheme"`
	Language  string            `json:"language"`
	Tokens    json.RawMessage   `json:"tokens"`
	Colors    map[string]Rgba   `json:"colors"`
	Strings   map[string]string `json:"strings"`
}

// ParseState reads one state line (`omarchy-theme --state`) into Tokens.
func ParseState(line []byte) (Tokens, error) {
	var s stateLine
	if err := json.Unmarshal(line, &s); err != nil {
		return Tokens{}, err
	}
	var t Tokens
	if err := json.Unmarshal(s.Tokens, &t); err != nil {
		return Tokens{}, err
	}
	t.Palette.Named = s.Colors
	if t.Palette.Named == nil {
		t.Palette.Named = map[string]Rgba{}
	}
	t.Strings = s.Strings
	t.Theme, t.Language, t.IconTheme = s.Theme, s.Language, s.IconTheme
	return t, nil
}

// Same is true when two token sets are equal in every field (a change that alters nothing is no
// change).
func (t *Tokens) Same(o *Tokens) bool {
	key := func(x *Tokens) []byte {
		b, _ := json.Marshal(struct {
			T                          *Tokens
			Named                      map[string]Rgba
			Strings                    map[string]string
			Theme, Language, IconTheme string
		}{x, x.Palette.Named, x.Strings, x.Theme, x.Language, x.IconTheme})
		return b
	}
	return bytes.Equal(key(t), key(o))
}

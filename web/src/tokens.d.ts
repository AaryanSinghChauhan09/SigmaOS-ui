/** A border token: a color, four widths (top, right, bottom, left) and an optional gradient. */
export interface OmarchyBorder {
  color: string;
  widths: [number, number, number, number];
  gradient: { colors: string[]; angle: number } | null;
}

export interface OmarchyControlState {
  fill: string;
  border?: OmarchyBorder;
  text?: string;
}

export interface OmarchySurface {
  background: string;
  text: string;
  border: OmarchyBorder;
  [extra: string]: string | OmarchyBorder;
}

/** The canonical token JSON of spec/tokens.md, as the core resolves it. Colors are "#rrggbbaa". */
export interface OmarchyTokens {
  schema: number;
  mode: "dark" | "light";
  palette: { foreground: string; background: string; accent: string; urgent: string; muted: string; success: string; warning: string };
  roles: { textDim: string; textFaint: string; track: string; scrim: string };
  radius: number;
  gapsOut: number;
  reduceMotion: boolean;
  font: {
    family: string; resolvedFamily?: string; baseSize: number;
    caption: number; bodySmall: number; body: number; subtitle: number; title: number; heading: number;
    display: number; displayLarge: number; iconSmall: number; icon: number; iconLarge: number;
  };
  spacing: { scale: number } & Record<string, number>;
  controls: { normal: OmarchyControlState; hover: OmarchyControlState; focus: OmarchyControlState; selected: OmarchyControlState; pressed: OmarchyControlState; selection: OmarchyControlState };
  surfaces: { popups: OmarchySurface; tooltip: OmarchySurface; menu: OmarchySurface; notifications: OmarchySurface; polkit: OmarchySurface };
  motion: { fast: number; press: number; theme: number; easeOut: [number, number, number, number]; ease: [number, number, number, number] };
}

/** What `omarchy-theme --state` prints: one line now, one per change with --watch. */
export interface OmarchyState {
  schema: number;
  revision: number;
  theme: string;
  iconTheme: string;
  language: string;
  tokens: OmarchyTokens;
  strings: Record<string, string>;
}

/** Every --omarchy-* custom property for a token set, and the names that hold colors (they fade). */
export function tokensToCss(tokens: OmarchyTokens): { props: Record<string, string>; colors: string[] };
/** The same as a rule: `:root { color-scheme: dark; --omarchy-…: …; }`. */
export function tokensToCssText(tokens: OmarchyTokens, selector?: string): string;
/** A border token's gradient as CSS, or "none". */
export function borderGradient(border: OmarchyBorder): string;

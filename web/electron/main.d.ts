import type { OmarchyState, OmarchyTokens } from "../src/tokens.js";

export interface FollowOmarchyOptions {
  /** The omarchy-theme CLI (default: $OMARCHY_THEME_BIN, then PATH, then this checkout's build). */
  bin?: string;
  /** Its environment (default process.env, so OMARCHY_UI_* overrides apply). */
  env?: NodeJS.ProcessEnv;
  /** How icon lookups answer the renderer: data: URLs (default), file: URLs or plain paths. */
  icons?: "data" | "file" | "path";
  /** Set every window's background to the theme's (default true). */
  windowBackground?: boolean;
  /** Set nativeTheme.themeSource to the theme's mode (default true). */
  nativeTheme?: boolean;
}

export interface OmarchyFollower {
  readonly state: OmarchyState;
  readonly tokens: OmarchyTokens;
  /** 0, plus one per change. */
  readonly revision: number;
  /** The main process's changed hook: once per change. Returns an unsubscribe. */
  onChange(cb: (tokens: OmarchyTokens, state: OmarchyState) => void): () => void;
  stop(): void;
}

/** Follow the Omarchy theme for every window (one call, in main). Idempotent. */
export function followOmarchy(options?: FollowOmarchyOptions): OmarchyFollower;
/** The preload's path, for `webPreferences.preload`. */
export const omarchyPreload: string;

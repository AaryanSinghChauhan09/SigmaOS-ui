import type { OmarchyState, OmarchyTokens } from "./tokens.js";
export type { OmarchyBorder, OmarchyState, OmarchyTokens } from "./tokens.js";
export { tokensToCss, tokensToCssText, borderGradient } from "./tokens.js";
export * from "./elements/index.js";

/** The renderer's view of the theme (also `window.omarchy`). */
export interface Omarchy {
  /** The whole state, or null before the first arrives (a page with no transport yet). */
  readonly state: OmarchyState | null;
  /** The canonical tokens: `omarchy.tokens.palette.background` is "#1a1b26ff". */
  readonly tokens: OmarchyTokens | null;
  /** 0 for the tokens the page started with, plus one per change since. */
  readonly revision: number;
  readonly theme: string;
  readonly iconTheme: string;
  readonly language: string;
  readonly strings: Record<string, string>;
  /** One of the kit's strings in the display language, %1… filled in. */
  string(key: string, fallback?: string, ...args: unknown[]): string;
  /** The changed hook: once per real change, after the new CSS variables are in effect, never for
   *  the initial load. Returns an unsubscribe. */
  onChange(cb: (tokens: OmarchyTokens, state: OmarchyState) => void): () => void;
  /** Icon names to URLs (data: or served) by the core's lookup, null when none. */
  icons(size: number, names: string[]): Promise<Record<string, string | null>>;
  icon(name: string, size?: number): Promise<string | null>;
}

export const omarchy: Omarchy;

/** Feed a state yourself (a snapshot, or each state of your own transport). True when it was a change. */
export function setOmarchyState(state: OmarchyState | string): boolean;
/** Follow the dev helper's Server-Sent Events (web/dev/serve.js). Returns a function that stops. */
export function connectOmarchy(url?: string): () => void;
/** Apply a state to the page's CSS yourself (what the transports do). */
export function applyState(state: OmarchyState, isChange: boolean): void;
/** Whether a theme-switch fade is running. */
export function fading(): boolean;
/** Define every <omarchy-*> element (importing the package does it). */
export function defineElements(): void;

declare global {
  interface Window {
    omarchy: Omarchy;
  }
}

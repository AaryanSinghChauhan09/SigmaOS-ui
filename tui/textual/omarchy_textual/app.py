"""The one call (spec/following.md, Adoption): `OmarchyApp` (subclass it instead of `App`) or
`follow(app)` on any Textual app.

It reads the tokens from the core (`omarchy-theme --watch --state`, an asyncio subprocess on the
app's loop), maps them onto a Textual `Theme` named "omarchy" and the kit's `$omarchy-*` TCSS
variables, restyles Textual's stock widgets to the one-row forms of spec/terminal.md, writes the
theme background as the terminal's default (and maps colors down below truecolor) on every line
Textual draws, follows the desktop live, fades colors over `motion.theme` on `motion.ease` by
redrawing (truecolor only; none under reduced motion), and posts `OmarchyThemeChanged` to the app
once per real change, after the new tokens are in effect: the public changed hook.
"""

from __future__ import annotations

import asyncio
from functools import lru_cache
from importlib import resources
from typing import Sequence

from rich.color import Color as RichColor
from rich.color import ColorSystem, ColorType
from rich.segment import Segment
from rich.style import Style
from textual.app import App
from textual.color import Color
from textual.filter import LineFilter
from textual.message import Message
from textual.theme import Theme as TextualTheme

from .color import Rgba, over
from .core import watch
from .depth import Depth, nearest256, nearest_role, tables
from .theme import Theme
from .tokens import Tokens

THEME_NAME = "omarchy"


class OmarchyThemeChanged(Message):
    """The changed hook (spec/following.md): posted to the app once per change that alters the
    tokens, after they are in effect (`kit.theme.tokens` has them, the revision counts it), handled
    on the app's event loop; never for the tokens the app started with. Handle it with
    `on_omarchy_theme_changed` (or `@on(OmarchyThemeChanged)`) for what the kit can't reach: a
    cache, a child process to re-theme."""

    def __init__(self, tokens: Tokens, revision: int) -> None:
        super().__init__()
        self.tokens = tokens
        self.revision = revision


# The page is the terminal's -----------------------------------------------------------------------


class PageFilter(LineFilter):
    """The last step of every line Textual draws (spec/terminal.md, Color): the theme background
    becomes the terminal's default, and below truecolor each color maps to the nearest xterm-256
    index (16–255) or the ANSI index of the nearest theme role."""

    def __init__(self, kit: "Kit") -> None:
        super().__init__(enabled=True)
        self.kit = kit
        self._key = None

    def apply(self, segments: list[Segment], background: Color) -> list[Segment]:
        t = self.kit.theme
        key = (id(t.shown), t.depth)
        if key != self._key:
            self._key = key
            self._map_style.cache_clear()
            bg = t.bg()
            self._bg = (bg.r, bg.g, bg.b)
            fg = t.fg()
            self._fg = (fg.r, fg.g, fg.b)
            self._tables = tables(t.shown) if t.depth == Depth.ANSI16 else None
        out = []
        for text, style, control in segments:
            out.append(Segment(text, self._map_style(style) if style is not None else None, control))
        return out

    @lru_cache(maxsize=4096)
    def _map_style(self, style: Style) -> Style:
        depth = self.kit.theme.depth
        fgc, bgc = style.color, style.bgcolor
        fg = fgc.get_truecolor() if fgc is not None and fgc.type != ColorType.DEFAULT else None
        bg = bgc.get_truecolor() if bgc is not None and bgc.type != ColorType.DEFAULT else None
        fg = tuple(fg) if fg is not None else None
        bg = tuple(bg) if bg is not None else None
        if bg == self._bg:
            bg = None
        dim = None
        if depth == Depth.TRUE:
            if bg is None and bgc is not None:
                return style + Style(bgcolor=RichColor.default())
            return style
        if depth == Depth.ANSI256:
            nbg = RichColor.default() if bg is None else RichColor.from_ansi(nearest256(bg))
            nfg = RichColor.default() if fg is None or fg == self._fg else RichColor.from_ansi(nearest256(fg))
        else:
            ink, fill = self._tables
            nbg = RichColor.default()
            if bg is not None:
                r = nearest_role(bg, fill)
                nbg = RichColor.default() if r.index is None else RichColor.from_ansi(r.index)
            nfg = RichColor.default()
            if fg is not None:
                r = nearest_role(fg, ink)
                nfg = RichColor.default() if r.index is None else RichColor.from_ansi(r.index)
                if r.faint:
                    dim = True
            if nfg.type != ColorType.DEFAULT and nfg == nbg:
                nfg = RichColor.default()
        return style + Style(color=nfg, bgcolor=nbg, dim=dim)


# The Textual theme and the kit's TCSS variables ----------------------------------------------------


def css_variables(tokens: Tokens) -> dict[str, str]:
    """The kit's `$omarchy-*` TCSS variables: every color composited over the theme background
    (opaque, so Textual's own blending never rounds them), the scrims with their alpha."""
    p, r, c, s = tokens.palette, tokens.roles, tokens.controls, tokens.surfaces
    bg = p.background

    def o(x: Rgba) -> str:
        return over(x, bg).hex6()

    return {
        "omarchy-foreground": o(p.foreground),
        "omarchy-background": o(bg),
        "omarchy-accent": o(p.accent),
        "omarchy-urgent": o(p.urgent),
        "omarchy-success": o(p.success),
        "omarchy-warning": o(p.warning),
        "omarchy-muted": o(p.muted),
        "omarchy-text-dim": o(r.text_dim),
        "omarchy-text-faint": o(r.text_faint),
        "omarchy-track": o(r.track),
        "omarchy-scrim": r.scrim.hex(),
        "omarchy-normal-fill": o(c.normal.fill),
        "omarchy-normal-border": o(c.normal.border.color),
        "omarchy-hover-fill": o(c.hover.fill),
        "omarchy-hover-border": o(c.hover.border.color),
        "omarchy-focus-fill": o(c.focus.fill),
        "omarchy-focus-border": o(c.focus.border.color),
        "omarchy-selected-fill": o(c.selected.fill),
        "omarchy-selected-text": o(c.selected.text),
        "omarchy-pressed-fill": o(c.pressed.fill),
        "omarchy-selection-fill": o(c.selection.fill),
        "omarchy-popup-border": o(s.popups.border.color),
        "omarchy-menu-border": o(s.menu.border.color),
        "omarchy-menu-selected-background": o(s.menu.selected_background),
        "omarchy-menu-selected-text": o(s.menu.selected_text),
        "omarchy-tooltip-border": o(s.tooltip.border.color),
        "omarchy-notification-border": o(s.notifications.border.color),
        "omarchy-error-border": o(s.polkit.border_error.color),
        "omarchy-error-text": o(s.polkit.text_error),
    }


def textual_theme(tokens: Tokens) -> TextualTheme:
    """The tokens as a Textual `Theme` named "omarchy" (stock widgets read `$primary`,
    `$surface`, ... from it), carrying the `$omarchy-*` variables."""
    v = css_variables(tokens)
    variables = dict(v)
    variables.update(
        {
            "border": v["omarchy-hover-border"],
            "border-blurred": v["omarchy-normal-border"],
            "scrollbar": v["omarchy-normal-border"],
            "scrollbar-hover": v["omarchy-hover-border"],
            "scrollbar-active": v["omarchy-foreground"],
            "scrollbar-background": v["omarchy-background"],
            "scrollbar-background-hover": v["omarchy-background"],
            "scrollbar-background-active": v["omarchy-background"],
            "scrollbar-corner-color": v["omarchy-background"],
            "block-cursor-background": v["omarchy-hover-fill"],
            "block-cursor-foreground": v["omarchy-foreground"],
            "block-cursor-text-style": "bold",
            "block-cursor-blurred-background": v["omarchy-selected-fill"],
            "block-cursor-blurred-foreground": v["omarchy-foreground"],
            "block-cursor-blurred-text-style": "none",
            "block-hover-background": v["omarchy-hover-fill"],
            "input-cursor-background": v["omarchy-foreground"],
            "input-cursor-foreground": v["omarchy-background"],
            "input-selection-background": v["omarchy-selection-fill"],
            "footer-background": v["omarchy-background"],
            "footer-key-foreground": v["omarchy-accent"],
            "footer-description-foreground": v["omarchy-text-dim"],
            "link-color": v["omarchy-accent"],
            "text-muted": v["omarchy-text-dim"],
            "text-disabled": v["omarchy-text-faint"],
        }
    )
    return TextualTheme(
        name=THEME_NAME,
        primary=v["omarchy-accent"],
        secondary=v["omarchy-accent"],
        accent=v["omarchy-accent"],
        warning=v["omarchy-warning"],
        error=v["omarchy-urgent"],
        success=v["omarchy-success"],
        foreground=v["omarchy-foreground"],
        background=v["omarchy-background"],
        surface=v["omarchy-background"],
        panel=v["omarchy-normal-fill"],
        boost=v["omarchy-normal-fill"],
        dark=tokens.mode != "light",
        variables=variables,
    )


def stock_css() -> str:
    """The restyle of Textual's stock widgets (`omarchy_textual/stock.tcss`)."""
    return resources.files("omarchy_textual").joinpath("stock.tcss").read_text()


# The controller ---------------------------------------------------------------------------------


class Kit:
    """The kit on one app: the `Theme` (tokens in effect, what is on screen, the revision), the
    watch, the fade. `follow(app)` makes it; `kit_of(app)` finds it."""

    def __init__(self, app: App, theme: Theme | None = None, live: bool = True):
        self.app = app
        self.theme = theme or Theme.load()
        self.live = live
        self._task: asyncio.Task | None = None
        self._timer = None
        self._started = False
        self.filter = PageFilter(self)

    @property
    def tokens(self) -> Tokens:
        return self.theme.tokens

    @property
    def revision(self) -> int:
        return self.theme.revision

    def install(self) -> None:
        """Map the tokens onto the app (theme, variables, stock restyle, the page filter, the color
        depth); safe before the app runs."""
        app = self.app
        app.register_theme(textual_theme(self.theme.shown))
        app.theme = THEME_NAME
        if self.filter not in app._filters:
            app._filters.append(self.filter)
        app.console._color_system = {
            Depth.TRUE: ColorSystem.TRUECOLOR,
            Depth.ANSI256: ColorSystem.EIGHT_BIT,
            Depth.ANSI16: ColorSystem.STANDARD,
        }[self.theme.depth]
        if not getattr(app, "_omarchy_css", False):
            app.stylesheet.add_source(stock_css(), read_from=("omarchy_textual", "stock.tcss"), is_default_css=True)
            app._omarchy_css = True

    def start(self) -> None:
        """Start following the desktop (on the app's loop)."""
        if self._started or not self.live:
            return
        self._started = True
        self._task = asyncio.get_running_loop().create_task(self._follow())

    async def _follow(self) -> None:
        try:
            await watch(self._delivered)
        except asyncio.CancelledError:
            raise
        except Exception as e:  # pragma: no cover - the CLI missing or failing
            self.app.log.error(f"omarchy: not following the theme: {e}")

    def _delivered(self, tokens: Tokens, is_change: bool) -> None:
        # The first line is the state the app started with, unless the theme changed meanwhile.
        self.set_tokens(tokens)

    def set_tokens(self, tokens: Tokens) -> bool:
        """Deliver a change (the watch does; tests and previews can): the new tokens take effect,
        colors fade, everything redraws, and `OmarchyThemeChanged` is posted. Tokens equal to the
        current ones change nothing and return False."""
        if not self.theme.set_tokens(tokens):
            return False
        self._apply_textual(self.theme.shown)
        self.redraw()
        if self.theme.fading:
            self._start_fade()
        self.app.post_message(OmarchyThemeChanged(self.theme.tokens, self.theme.revision))
        return True

    def _start_fade(self) -> None:
        if self._timer is None:
            self._timer = self.app.set_interval(1 / 60, self._fade_frame, name="omarchy-fade")

    def _fade_frame(self) -> None:
        running = self.theme.tick()
        self._apply_textual(self.theme.shown)
        self.redraw()
        if not running and self._timer is not None:
            self._timer.stop()
            self._timer = None

    def _apply_textual(self, tokens: Tokens) -> None:
        app = self.app
        app.register_theme(textual_theme(tokens))
        if app.theme != THEME_NAME:
            app.theme = THEME_NAME
        else:
            app._invalidate_css()
            app.refresh_css(animate=False)

    def redraw(self) -> None:
        """Draw every kit widget again from what is on screen now."""
        from .widget import KitWidget

        for screen in self.app.screen_stack:
            for w in screen.walk_children(KitWidget):
                w._frame = None
                w.refresh(layout=True)
        self.app.refresh()

    def stop(self) -> None:
        if self._task is not None:
            self._task.cancel()
            self._task = None


def kit_of(app: App) -> Kit:
    """The app's kit: the one `follow(app)` made, else a theme read once (not following)."""
    kit = getattr(app, "_omarchy_kit", None)
    if kit is None:
        kit = Kit(app, live=False)
        app._omarchy_kit = kit
        kit.install()
    return kit


def follow(app: App, theme: Theme | None = None) -> Kit:
    """The one call for any Textual app: the Omarchy theme on it, followed live. Call it in the
    app's `__init__` (or before `run()`); `OmarchyApp` does it for you. Returns the `Kit`
    (`kit.theme`, `kit.revision`, `kit.set_tokens`)."""
    kit = getattr(app, "_omarchy_kit", None)
    if kit is None or not kit.live:
        kit = Kit(app, theme=theme or (kit.theme if kit else None), live=True)
        app._omarchy_kit = kit
    kit.install()
    try:
        asyncio.get_running_loop()
        running = app.is_running
    except RuntimeError:
        running = False
    if running:
        kit.start()
    else:
        app.call_later(kit.start)
    return kit


class OmarchyApp(App):
    """A Textual `App` with Omarchy's look, followed live: subclass it instead of `App`.
    `self.kit` is the kit (`self.kit.theme`, `self.kit.revision`); handle `OmarchyThemeChanged`
    for the changed hook."""

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        follow(self)

    @property
    def kit(self) -> Kit:
        return kit_of(self)

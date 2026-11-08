pragma Singleton
import QtQuick
import Omarchy.Theme as Commons

// The resolved tokens as the canonical JSON in spec/tokens.md, read back from the live Color,
// Style and Border singletons. bin/review compares this with the Rust resolver's output for
// every fixture theme, so the two bindings can't drift apart unnoticed.
QtObject {
  id: root

  function hex(c) {
    var q = typeof c === "string" ? Qt.color(c) : c
    function h(v) {
      var n = Math.max(0, Math.min(255, Math.round(v * 255)))
      return (n < 16 ? "0" : "") + n.toString(16)
    }
    return "#" + h(q.r) + h(q.g) + h(q.b) + h(q.a)
  }

  function border(spec) {
    var g = spec.gradient && spec.gradient.enabled ? spec.gradient : null
    return {
      color: hex(spec.color),
      widths: [spec.widths.top, spec.widths.right, spec.widths.bottom, spec.widths.left],
      gradient: g ? { colors: g.colors.map(hex), angle: g.angle } : null,
    }
  }

  function canonical() {
    var C = Commons.Color, S = Commons.Style, B = Commons.Border
    var fg = C.foreground, ac = C.accent
    var card = Math.max(1, S.space(2))
    var sp = S.spacing, f = S.font
    function state(name, fill, text) {
      return { border: border(B.controlSpec(name, fg, ac)), fill: hex(fill), text: hex(text) }
    }
    function surface(s, name, width) {
      return { background: hex(s.background), text: hex(s.text), border: border(B.surfaceSpec(name, "border", s.border, width)) }
    }
    var menu = surface(C.menu, "menu", card)
    menu.scrim = hex(C.menu.scrim)
    menu.selectedBackground = hex(C.menu.selectedBackground)
    menu.selectedText = hex(C.menu.selectedText)
    menu.selectedBorder = border(B.surfaceSpec("menu", "selected-border", C.menu.selectedBorder, 0))
    var notifications = surface(C.notifications, "notifications", card)
    notifications.countdown = hex(C.notifications.countdown)
    var polkit = {
      background: hex(C.polkit.background),
      text: hex(C.polkit.text),
      border: border(B.surfaceSpec("polkit", "border", C.polkit.border, card, "border-alpha")),
      textError: hex(C.polkit.textError),
      borderError: border(B.surfaceSpec("polkit", "border-error", C.polkit.borderError, card, "border-alpha")),
      accent: hex(C.polkit.accent),
      scrim: hex(C.polkit.scrim),
    }
    return {
      schema: 1,
      mode: C.dark ? "dark" : "light",
      palette: { foreground: hex(fg), background: hex(C.background), accent: hex(ac), urgent: hex(C.urgent), muted: hex(C.muted) },
      radius: S.cornerRadius,
      gapsOut: S.gapsOut,
      reduceMotion: S.reduceMotion,
      font: {
        family: f.family, resolvedFamily: f.resolvedFamily, baseSize: f.baseSize,
        caption: f.caption, bodySmall: f.bodySmall, body: f.body, subtitle: f.subtitle, title: f.title,
        heading: f.heading, display: f.display, displayLarge: f.displayLarge,
        iconSmall: f.iconSmall, icon: f.icon, iconLarge: f.iconLarge,
      },
      spacing: {
        scale: Math.round(sp.scale * 10000) / 10000, hairline: sp.hairline, xxs: sp.xxs, xs: sp.xs, sm: sp.sm, md: sp.md,
        lg: sp.lg, xl: sp.xl, xxl: sp.xxl, xxxl: sp.xxxl, huge: sp.huge,
        controlGap: sp.controlGap, controlPaddingX: sp.controlPaddingX, controlPaddingY: sp.controlPaddingY,
        inputPaddingY: sp.inputPaddingY, controlHeight: sp.controlHeight, popupRowHeight: sp.popupRowHeight,
        rowGap: sp.rowGap, rowPaddingX: sp.rowPaddingX, labelGap: sp.labelGap, panelGap: sp.panelGap,
        panelPadding: sp.panelPadding, popupPadding: sp.popupPadding,
      },
      controls: {
        normal: state("normal", S.normalFillFor(fg, ac), fg),
        hover: state("hover-cursor", S.hoverFillFor(fg, ac), fg),
        focus: state("focus", S.focusFillFor(fg, ac), fg),
        selected: state("selected", S.selectedFillFor(fg, ac), S.selectedStateColor(fg, ac)),
        pressed: { fill: hex(S.pressedFillFor(fg, ac)) },
        selection: { fill: hex(S.selectionFillFor(fg, ac)) },
      },
      surfaces: {
        popups: surface(C.popups, "popups", card),
        tooltip: surface(C.tooltip, "tooltip", Math.max(1, S.normalBorderWidth)),
        menu: menu,
        notifications: notifications,
        polkit: polkit,
      },
      motion: { fast: 120, press: 160, easeOut: [S.easeOut[0], S.easeOut[1], S.easeOut[2], S.easeOut[3]] },
    }
  }
}

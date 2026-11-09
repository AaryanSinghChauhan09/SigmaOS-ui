"""omarchy-textual-gallery

  omarchy-textual-gallery                                   the gallery in this terminal
  omarchy-textual-gallery --section lists                   ...starting on a section
  omarchy-textual-gallery --list-sections                   the sections, in order
  omarchy-textual-gallery --snap out.ans --section buttons [--size 96x0]
                                                            one section as an ANSI snapshot
"""

from __future__ import annotations

import sys

from .sections import SECTIONS


def _arg(args: list[str], name: str) -> str | None:
    if name in args:
        i = args.index(name)
        if i + 1 < len(args):
            return args[i + 1]
    return None


def main(argv: list[str] | None = None) -> int:
    args = sys.argv[1:] if argv is None else argv
    if "-h" in args or "--help" in args:
        print(__doc__.strip())
        return 0
    if "--list-sections" in args:
        for n, _ in SECTIONS:
            print(n)
        return 0
    section = _arg(args, "--section")
    if section is not None and section not in [n for n, _ in SECTIONS]:
        print(f"no section {section} (--list-sections)", file=sys.stderr)
        return 2
    out = _arg(args, "--snap")
    if out is not None:
        from ..snap import parse_size, snap_app
        from .app import GalleryApp

        cols, rows = parse_size(_arg(args, "--size"))
        app = GalleryApp(section=section or "buttons", snap=True)
        text = snap_app(app, cols, rows, natural=lambda a: a.query_one("#section").get_content_height(None, None, cols))
        with open(out, "w", encoding="utf-8") as f:
            f.write(text)
        return 0
    from .app import GalleryApp

    GalleryApp(section=section or "buttons").run(mouse=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Render every gallery section in every framework, for a set of themes and roundings, check
token parity, and write a comparison page. Driven by bin/review; see its --help."""

import argparse
import html
import json
import os
import shutil
import struct
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_THEMES = ["live", "tokyo-night", "catppuccin-latte", "gruvbox", "rose-pine", "retro-82"]
QT = ROOT / "build/qt/omarchy-qt-gallery"
RUNE = ROOT / "target/release/omarchy-rune-gallery"
TOKENS = ROOT / "target/release/omarchy-theme"


def gallery_sections():
    """The sections, in order, as the Rune gallery lists them (the Qt gallery has the same)."""
    return subprocess.run([str(RUNE), "--list-sections"], capture_output=True, text=True, check=True).stdout.split()


def png_size(path):
    with open(path, "rb") as f:
        head = f.read(24)
    return struct.unpack(">II", head[16:24])


def env_for(theme, radius):
    env = dict(os.environ)
    for k in ("OMARCHY_UI_THEME_DIR", "OMARCHY_UI_ROUNDING", "OMARCHY_UI_REDUCE_MOTION", "OMARCHY_UI_ICON_DIRS", "OMARCHY_UI_ICON_THEME"):
        env.pop(k, None)
    if theme != "live":
        env["OMARCHY_UI_THEME_DIR"] = str(ROOT / "fixtures/themes" / theme)
    if radius is not None:
        env["OMARCHY_UI_ROUNDING"] = str(radius)
    # Stills, not mid-animation frames.
    env["OMARCHY_UI_REDUCE_MOTION"] = "1"
    # Icons by name from the fixture icon tree, whatever is installed (live theme included).
    icons = ROOT / "fixtures/icons"
    env["OMARCHY_UI_ICON_DIRS"] = ":".join(str(icons / d) for d in ("local", "system", "pixmaps"))
    env["OMARCHY_UI_ICON_THEME"] = "Fixture-Accent"
    return env


def run(cmd, env):
    return subprocess.run(cmd, env=env, capture_output=True, text=True)


def token_parity(theme, radius):
    env = env_for(theme, radius)
    qt = run([str(QT), "--dump-tokens"], env)
    rs = run([str(TOKENS)], env)
    try:
        a, b = json.loads(qt.stdout), json.loads(rs.stdout)
    except json.JSONDecodeError:
        return [f"could not read dumps: {qt.stderr.strip()} {rs.stderr.strip()}"]
    diffs = []

    def walk(x, y, path):
        if isinstance(x, dict) and isinstance(y, dict):
            for k in sorted(set(x) | set(y)):
                if k not in x or k not in y:
                    diffs.append(f"{path}.{k}: only in {'rune' if k not in x else 'qt'}")
                else:
                    walk(x[k], y[k], f"{path}.{k}")
        elif x != y:
            diffs.append(f"{path}: qt={x} rune={y}")

    walk(a, b, "")
    return diffs


def render(job):
    theme, radius, section, out = job
    env = env_for(theme, radius)
    out.mkdir(parents=True, exist_ok=True)
    qt_png, rune_png, diff_png = out / "qt.png", out / "rune.png", out / "diff.png"
    r = run([str(QT), "--snap", str(qt_png), "--section", section, "--scale", "2"], env)
    if r.returncode != 0 or not qt_png.exists():
        return {"error": f"qt: {r.stderr.strip()[-400:]}"}
    w, h = png_size(qt_png)
    r = run([str(RUNE), "--snap", str(rune_png), "--scene", section, "--size", f"{w // 2}x{h // 2}", "--scale", "2"], env)
    if r.returncode != 0 or not rune_png.exists():
        return {"error": f"rune: {r.stderr.strip()[-400:]}"}
    # Share of pixels that differ by more than a hair, and a picture of where.
    c = run(["magick", "compare", "-metric", "AE", "-fuzz", "3%", str(qt_png), str(rune_png),
             "-highlight-color", "#ff3b6b", "-lowlight-color", "#00000000", str(diff_png)], env)
    try:
        differing = float(c.stderr.split()[0])
    except (ValueError, IndexError):
        differing = -1
    return {"diff": round(100 * differing / (w * h), 2) if differing >= 0 else None, "w": w // 2, "h": h // 2}


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--themes", default=",".join(DEFAULT_THEMES))
    p.add_argument("--all-themes", action="store_true")
    p.add_argument("--radii", default="0,8")
    p.add_argument("--sections", default="")
    p.add_argument("--out", default=str(ROOT / "out/review"))
    args = p.parse_args()

    themes = args.themes.split(",")
    if args.all_themes:
        themes = ["live"] + sorted(d.name for d in (ROOT / "fixtures/themes").iterdir() if d.is_dir())
    radii = [int(r) for r in args.radii.split(",")]
    sections = args.sections.split(",") if args.sections else gallery_sections()
    out = Path(args.out)
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)

    # Token parity across every fixture (cheap), not just the rendered ones.
    parity = {}
    all_fixtures = ["live"] + sorted(d.name for d in (ROOT / "fixtures/themes").iterdir() if d.is_dir())
    for theme in all_fixtures:
        for radius in radii:
            parity[f"{theme}@{radius}"] = token_parity(theme, radius)
    failing = {k: v for k, v in parity.items() if v}
    print(f"tokens: {len(parity) - len(failing)}/{len(parity)} theme×radius dumps match between Qt and Rune")
    for k, v in failing.items():
        print(f"  {k}: {len(v)} differences, e.g. {v[0]}")

    jobs = [(t, r, s, out / t / f"r{r}" / s) for t in themes for r in radii for s in sections]
    with ThreadPoolExecutor(max_workers=max(2, (os.cpu_count() or 4) // 2)) as pool:
        results = list(pool.map(render, jobs))
    rows = []
    for (t, r, s, path), res in zip(jobs, results):
        rows.append({"theme": t, "radius": r, "section": s, "path": str(path.relative_to(out)), **res})
        if "error" in res:
            print(f"  {t} r{r} {s}: {res['error']}", file=sys.stderr)
    print(f"renders: {sum('error' not in r for r in rows)}/{len(rows)} section pairs")

    live_name = ""
    try:
        live_name = (Path.home() / ".local/state/omarchy/current/theme.name").read_text().strip()
    except OSError:
        pass
    data = {"rows": rows, "parity": parity, "themes": themes, "radii": radii, "sections": sections, "live": live_name,
            "rev": subprocess.run(["git", "-C", str(ROOT), "rev-parse", "--short", "HEAD"], capture_output=True, text=True).stdout.strip()}
    page = (ROOT / "tools/review.html").read_text().replace("/*DATA*/null", json.dumps(data))
    (out / "index.html").write_text(page)
    print(out / "index.html")


if __name__ == "__main__":
    main()

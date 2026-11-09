#!/usr/bin/env python3
"""Render every gallery section in every binding of the registry (tools/bindings.d), for a set of
themes and roundings, check token parity with the core, diff each binding against the reference
(Qt), and write a comparison page. Driven by bin/review; see its --help.

Terminal bindings ("family": "terminal", spec/terminal.md) are a second family: each section is
an ANSI snapshot at TERMINAL_COLS columns, rendered to PNG by one rasterizer (omarchy-ansi) and
diffed cell by cell and pixel by pixel against the terminal reference (Ratatui), never against Qt;
the page shows Qt beside them only to eyeball."""

import argparse
import json
import os
import shutil
import struct
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bindings as registry  # noqa: E402

ROOT = registry.ROOT
ANSI = ROOT / "target/release/omarchy-ansi"
DEFAULT_THEMES = ["live", "tokyo-night", "catppuccin-latte", "gruvbox", "rose-pine", "retro-82"]


def png_size(path):
    with open(path, "rb") as f:
        head = f.read(24)
    return struct.unpack(">II", head[16:24])


def diff_percent(ref_png, png, diff_png, env):
    """Share of pixels that differ by more than a hair, and a picture of where."""
    c = subprocess.run(["magick", "compare", "-metric", "AE", "-fuzz", "3%", str(ref_png), str(png),
                        "-highlight-color", "#ff3b6b", "-lowlight-color", "#00000000", str(diff_png)],
                       env=env, capture_output=True, text=True)
    try:
        differing = float(c.stderr.split()[0])
    except (ValueError, IndexError):
        return None
    w, h = png_size(ref_png)
    return round(100 * differing / (w * h), 2)


def render(job):
    """One section, theme and radius in every binding: the GUI family, then the terminal one."""
    theme, radius, section, out, bindings, term, has = job
    env = registry.fixture_env(theme, radius)
    out.mkdir(parents=True, exist_ok=True)
    res = render_gui(section, out, bindings, has, env)
    if term:
        res["term"] = render_term(section, out, term, has, env)
    return res


def ansi(*args, env):
    return subprocess.run([str(ANSI), *map(str, args)], env=env, capture_output=True, text=True)


def render_term(section, out, term, has, env):
    """The terminal family: the reference snaps at TERMINAL_COLS and sizes its rows; every other
    terminal binding snaps at the reference's cells and is diffed against it, by cells (omarchy-ansi
    diff) and by pixels (the same compare as the GUI, on PNGs from the same rasterizer)."""
    ref = term[0]
    res = {"images": {}, "diffs": {}, "errors": {}, "missing": []}
    if section not in has[ref.name]:
        res["errors"][ref.name] = f"{ref['title']} has no {section} section"
        return res
    ref_ans = out / f"{ref.name}.ans"
    r = ref.snap(ref_ans, section, registry.TERMINAL_COLS, 0, env=env)
    if r.returncode != 0 or not ref_ans.exists():
        res["errors"][ref.name] = r.stderr.strip()[-400:] or f"exit {r.returncode}"
        return res
    size = ansi("size", ref_ans, env=env)
    if size.returncode != 0:
        res["errors"][ref.name] = size.stderr.strip()[-400:] or "not a valid snapshot"
        return res
    cols, rows = (int(n) for n in size.stdout.strip().split("x"))
    ref_png = out / f"{ref.name}.png"
    p = ansi("png", ref_ans, ref_png, "--scale", 2, env=env)
    if p.returncode != 0 or not ref_png.exists():
        res["errors"][ref.name] = p.stderr.strip()[-400:] or "omarchy-ansi png failed"
        return res
    w, h = png_size(ref_png)
    res.update(w=w // 2, h=h // 2, cols=cols, rows=rows)
    res["images"][ref.name] = ref_png.name
    for b in term[1:]:
        if section not in has[b.name]:
            res["missing"].append(b.name)
            continue
        ans = out / f"{b.name}.ans"
        r = b.snap(ans, section, cols, rows, env=env)
        if r.returncode != 0 or not ans.exists():
            res["errors"][b.name] = r.stderr.strip()[-400:] or f"exit {r.returncode}"
            continue
        c = ansi("check", ans, "--size", f"{cols}x{rows}", env=env)
        if c.returncode != 0:
            res["errors"][b.name] = (c.stdout + c.stderr).strip()[-400:] or "not a valid snapshot"
            continue
        png = out / f"{b.name}.png"
        p = ansi("png", ans, png, "--scale", 2, env=env)
        if p.returncode != 0 or not png.exists():
            res["errors"][b.name] = p.stderr.strip()[-400:] or "omarchy-ansi png failed"
            continue
        res["images"][b.name] = png.name
        cells_png = out / f"diffcells-{b.name}.png"
        d = ansi("diff", ref_ans, ans, "--png", cells_png, "--scale", 2, env=env)
        try:
            cells = json.loads(d.stdout)["percent"]
        except (ValueError, KeyError):
            cells = None
        if cells_png.exists():
            res["images"][f"diffcells-{b.name}"] = cells_png.name
        diff_png = out / f"diff-{b.name}.png"
        pixels = diff_percent(ref_png, png, diff_png, env)
        if diff_png.exists():
            res["images"][f"diff-{b.name}"] = diff_png.name
        res["diffs"][b.name] = {"cells": cells, "pixels": pixels}
    return res


def render_gui(section, out, bindings, has, env):
    """The GUI family: the reference first, at the gallery width; every other binding at the
    reference's size, diffed against it."""
    ref = bindings[0]
    res = {"images": {}, "diffs": {}, "errors": {}, "missing": []}
    ref_png = out / f"{ref.name}.png"
    r = ref.snap(ref_png, section, registry.GALLERY_WIDTH, 0, env=env)
    if r.returncode != 0 or not ref_png.exists():
        res["errors"][ref.name] = r.stderr.strip()[-400:] or f"exit {r.returncode}"
        return res
    w, h = png_size(ref_png)
    res.update(w=w // 2, h=h // 2)
    res["images"][ref.name] = ref_png.name
    for b in bindings[1:]:
        if section not in has[b.name]:
            res["missing"].append(b.name)
            continue
        png = out / f"{b.name}.png"
        r = b.snap(png, section, w // 2, h // 2, env=env)
        if r.returncode != 0 or not png.exists():
            res["errors"][b.name] = r.stderr.strip()[-400:] or f"exit {r.returncode}"
            continue
        res["images"][b.name] = png.name
        diff_png = out / f"diff-{b.name}.png"
        res["diffs"][b.name] = diff_percent(ref_png, png, diff_png, env)
        if diff_png.exists():
            res["images"][f"diff-{b.name}"] = diff_png.name
    return res


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--themes", default=",".join(DEFAULT_THEMES))
    p.add_argument("--all-themes", action="store_true")
    p.add_argument("--radii", default="0,8")
    p.add_argument("--sections", default="")
    p.add_argument("--bindings", default="", help="only these registry names (the GUI reference always renders; the terminal one when a terminal binding is named)")
    p.add_argument("--out", default=str(ROOT / "out/review"))
    args = p.parse_args()

    every = registry.load()
    found = registry.available(every)
    bindings = [b for b in found if not b.terminal]
    term = [b for b in found if b.terminal]
    if not bindings or not bindings[0].reference:
        sys.exit("review: the reference binding (tools/bindings.d, \"reference\": true) isn't available here")
    if args.bindings:
        wanted = set(args.bindings.split(","))
        bindings = [b for b in bindings if b.reference or b.name in wanted]
        term = [b for b in term if b.reference or b.name in wanted] if any(b.name in wanted for b in term) else []
    if term and not term[0].reference:
        print(f"review: the terminal reference isn't available here; skipping {', '.join(b['title'] for b in term)}", file=sys.stderr)
        term = []
    if term and not ANSI.exists():
        print(f"review: no {ANSI.relative_to(ROOT)} (bin/build core); skipping the terminal bindings", file=sys.stderr)
        term = []

    themes = args.themes.split(",")
    if args.all_themes:
        themes = ["live"] + registry.fixtures()
    radii = [int(r) for r in args.radii.split(",")]
    has = {b.name: b.sections() for b in bindings + term}
    sections = args.sections.split(",") if args.sections else has[bindings[0].name]
    out = Path(args.out)
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)

    # Token parity with the core across every fixture (cheap), not just the rendered ones.
    parity = {}
    for b in bindings:
        if not b.own_tokens():
            continue
        jobs = [(t, r) for t in ["live"] + registry.fixtures() for r in radii]
        with ThreadPoolExecutor(max_workers=max(2, (os.cpu_count() or 4) // 2)) as pool:
            diffs = list(pool.map(lambda j: registry.token_diffs(b, *j), jobs))
        parity[b.name] = {f"{t}@{r}": d for (t, r), d in zip(jobs, diffs)}
        failing = {k: v for k, v in parity[b.name].items() if v}
        print(f"tokens: {len(jobs) - len(failing)}/{len(jobs)} theme×radius dumps match between {b['title']} and the core")
        for k, v in failing.items():
            print(f"  {k}: {len(v)} differences, e.g. {v[0]}")
    for b in bindings:
        if not b.own_tokens():
            print(f"tokens: {b['title']} reads the core's tokens directly")

    jobs = [(t, r, s, out / t / f"r{r}" / s, bindings, term, has) for t in themes for r in radii for s in sections]
    with ThreadPoolExecutor(max_workers=max(2, (os.cpu_count() or 4) // 2)) as pool:
        results = list(pool.map(render, jobs))
    rows = []
    for (t, r, s, path, _, _, _), res in zip(jobs, results):
        rows.append({"theme": t, "radius": r, "section": s, "path": str(path.relative_to(out)), **res})
        for name, err in res["errors"].items():
            print(f"  {t} r{r} {s}: {name}: {err}", file=sys.stderr)
        for name, err in res.get("term", {}).get("errors", {}).items():
            print(f"  {t} r{r} {s}: {name}: {err}", file=sys.stderr)
    ref = bindings[0]
    for b in bindings[1:]:
        scores = [row["diffs"][b.name] for row in rows if row["diffs"].get(b.name) is not None]
        worst = max(scores) if scores else None
        missing = sum(b.name in row["missing"] for row in rows)
        print(f"renders: {len(scores)}/{len(rows)} sections in {b['title']} against {ref['title']}"
              + (f", at most {worst}% of pixels differ" if worst is not None else "")
              + (f", {missing} not in this binding" if missing else ""))
    if len(bindings) == 1:
        print(f"renders: {sum(ref.name in row['images'] for row in rows)}/{len(rows)} sections in {ref['title']} (no other binding to compare)")
    if term:
        tref = term[0]
        print(f"terminal renders: {sum(tref.name in row['term']['images'] for row in rows)}/{len(rows)} sections in {tref['title']} at {registry.TERMINAL_COLS} columns")
        for b in term[1:]:
            scores = [row["term"]["diffs"][b.name] for row in rows if b.name in row["term"]["diffs"]]
            cells = [d["cells"] for d in scores if d["cells"] is not None]
            pixels = [d["pixels"] for d in scores if d["pixels"] is not None]
            missing = sum(b.name in row["term"]["missing"] for row in rows)
            print(f"terminal renders: {len(scores)}/{len(rows)} sections in {b['title']} against {tref['title']}"
                  + (f", at most {max(cells)}% of cells" if cells else "")
                  + (f" and {max(pixels)}% of pixels differ" if pixels else (" differ" if cells else ""))
                  + (f", {missing} not in this binding" if missing else ""))

    live_name = ""
    try:
        live_name = (Path.home() / ".local/state/omarchy/current/theme.name").read_text().strip()
    except OSError:
        pass
    data = {
        "bindings": [{"name": b.name, "title": b["title"], "reference": b.reference} for b in bindings],
        "terminal": [{"name": b.name, "title": b["title"], "reference": b.reference} for b in term],
        "skipped": [{"name": b.name, "title": b["title"], "why": b.availability()[1]} for b in every if b not in bindings and not b.availability()[0]],
        "rows": rows, "parity": parity, "themes": themes, "radii": radii, "sections": sections, "live": live_name,
        "rev": subprocess.run(["git", "-C", str(ROOT), "rev-parse", "--short", "HEAD"], capture_output=True, text=True).stdout.strip(),
    }
    page = (ROOT / "tools/review.html").read_text().replace("/*DATA*/null", json.dumps(data))
    (out / "index.html").write_text(page)
    print(out / "index.html")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""The binding registry: tools/bindings.d/<name>.json, one file per framework binding.

bin/build, bin/test, bin/review and tools/review.py read it, so a new binding plugs in by adding
its file, without editing them. AGENTS.md ("A new framework") documents the format.

    tools/bindings.py list              the available bindings, reference first (skips to stderr):
                                        the gui family, then the terminal family
    tools/bindings.py references [NAME...]
                                        the references NAMEs need beside them: the gui reference
                                        always, the terminal one when any NAME is a terminal binding
    tools/bindings.py build [NAME...]   each binding's build step
    tools/bindings.py test [NAME...]    each binding's tests
    tools/bindings.py parity            token and icon parity with the core, for every binding
                                        that resolves them itself
    tools/bindings.py windows           every gallery as a window, following the desktop

A binding whose toolchain is missing (its `available` check fails) is skipped with a message,
never failed. Commands run with bash from the repository root; {placeholders} are filled in
shell-quoted.
"""

import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REGISTRY = ROOT / "tools/bindings.d"
CORE_CLI = ROOT / "target/release/omarchy-theme"
FIXTURES = ROOT / "fixtures/themes"
ICONS = ROOT / "fixtures/icons"
# The gallery width every snap starts from (spec/gallery.md); others match the reference's size.
GALLERY_WIDTH = 760
# The terminal family's gallery width in cells (spec/terminal.md); the reference sizes its rows.
TERMINAL_COLS = 96
FAMILIES = ("gui", "terminal")
REQUIRED = ("name", "title", "build", "gallery", "tokens", "tests")


class Binding(dict):
    """One registry entry, with helpers. Keys are the JSON's."""

    @property
    def name(self):
        return self["name"]

    @property
    def reference(self):
        """The reference of its family: Qt for gui, Ratatui for terminal."""
        return bool(self.get("reference"))

    @property
    def family(self):
        return self.get("family", "gui")

    @property
    def terminal(self):
        return self.family == "terminal"

    def availability(self):
        """(True, "") or (False, why): its commands are on PATH and its check passes."""
        a = self.get("available") or {}
        for cmd in a.get("commands", []):
            if shutil.which(cmd) is None:
                return False, f"no {cmd}"
        check = a.get("check")
        if check and run(check, capture=True).returncode != 0:
            return False, a.get("reason") or f"`{check}` failed"
        return True, ""

    def own_tokens(self):
        """The command that dumps its tokens, or None when it consumes the core's directly."""
        t = self["tokens"]
        return None if t == "core" else t["dump"]

    def own_icons(self):
        i = self.get("icons", "core")
        return None if i in (None, "core", "none") else i["lookup"]

    def no_icons(self):
        """True for a binding that draws no icons by name (a terminal)."""
        return self.get("icons") == "none"

    def sections(self, env=None):
        r = run(self["gallery"]["sections"], env=env, capture=True)
        return r.stdout.split() if r.returncode == 0 else []

    def snap(self, out, section, width, height, scale=2, env=None):
        cmd = expand(self["gallery"]["snap"], out=out, section=section, width=width, height=height, scale=scale)
        return run(cmd, env=env, capture=True)


def load(names=None):
    """Every entry, reference first, then by `order` and name. Exits on a malformed entry."""
    out = []
    for path in sorted(REGISTRY.glob("*.json")):
        try:
            b = Binding(json.loads(path.read_text()))
        except json.JSONDecodeError as e:
            sys.exit(f"{path.relative_to(ROOT)}: {e}")
        missing = [k for k in REQUIRED if k not in b]
        if missing or b.get("name") != path.stem:
            sys.exit(f"{path.relative_to(ROOT)}: needs {', '.join(missing) or 'name = ' + path.stem}")
        for k in ("snap", "sections"):
            if k not in b["gallery"]:
                sys.exit(f"{path.relative_to(ROOT)}: gallery needs {k}")
        if b.family not in FAMILIES:
            sys.exit(f"{path.relative_to(ROOT)}: family is {' or '.join(FAMILIES)}, not {b.family}")
        out.append(b)
    # The gui family first (so the first entry is always its reference), then the terminal family;
    # each with its reference first.
    out.sort(key=lambda b: (FAMILIES.index(b.family), not b.reference, b.get("order", 100), b.name))
    if names:
        unknown = set(names) - {b.name for b in out}
        if unknown:
            sys.exit(f"no binding named {', '.join(sorted(unknown))} in tools/bindings.d")
        out = [b for b in out if b.name in names]
    return out


def available(bindings, quiet=False):
    """The bindings that can run here; the rest are reported as skipped."""
    ok = []
    for b in bindings:
        yes, why = b.availability()
        if yes:
            ok.append(b)
        elif not quiet:
            print(f"{b.name}: skipped ({why})", file=sys.stderr)
    return ok


def expand(cmd, **values):
    """`cmd` with each {key} replaced by its shell-quoted value; other braces are left alone."""
    return re.sub(r"\{(\w+)\}", lambda m: shlex.quote(str(values[m[1]])) if m[1] in values else m[0], cmd)


def run(cmd, env=None, capture=False, extra=None):
    e = dict(os.environ if env is None else env)
    e.update(extra or {})
    return subprocess.run(["bash", "-c", cmd], cwd=ROOT, env=e, capture_output=capture, text=True)


def fixture_env(theme, radius, base=None):
    """What a still render or a dump runs under: a fixture theme (or the live desktop), a
    rounding, no motion, and the fixture icon tree."""
    env = dict(os.environ if base is None else base)
    for k in ("OMARCHY_UI_THEME_DIR", "OMARCHY_UI_ROUNDING", "OMARCHY_UI_REDUCE_MOTION", "OMARCHY_UI_ICON_DIRS", "OMARCHY_UI_ICON_THEME"):
        env.pop(k, None)
    if theme != "live":
        env["OMARCHY_UI_THEME_DIR"] = str(FIXTURES / theme)
    if radius is not None:
        env["OMARCHY_UI_ROUNDING"] = str(radius)
    env["OMARCHY_UI_REDUCE_MOTION"] = "1"
    env.update(icon_env())
    return env


def icon_env():
    return {"OMARCHY_UI_ICON_DIRS": ":".join(str(ICONS / d) for d in ("local", "system", "pixmaps")), "OMARCHY_UI_ICON_THEME": "Fixture-Accent"}


def canonical(text):
    return json.dumps(json.loads(text), sort_keys=True)


def token_diffs(binding, theme, radius):
    """Where `binding`'s dump differs from the core's for one fixture, as `path: a vs b` lines."""
    env = fixture_env(theme, radius)
    mine = run(binding.own_tokens(), env=env, capture=True)
    core = subprocess.run([str(CORE_CLI)], env=env, capture_output=True, text=True)
    try:
        a, b = json.loads(mine.stdout), json.loads(core.stdout)
    except json.JSONDecodeError:
        return [f"could not read the dumps: {mine.stderr.strip()[-200:]} {core.stderr.strip()[-200:]}"]
    diffs = []

    def walk(x, y, path):
        if isinstance(x, dict) and isinstance(y, dict):
            for k in sorted(set(x) | set(y)):
                if k not in x or k not in y:
                    diffs.append(f"{path}.{k}: only in {'core' if k not in x else binding.name}")
                else:
                    walk(x[k], y[k], f"{path}.{k}")
        elif x != y:
            diffs.append(f"{path}: {binding.name}={x} core={y}")

    walk(a, b, "")
    return diffs


ICON_NAMES = ["utilities-terminal", "org.gnome.Nautilus", "firefox", "chromium", "btop", "steam", "zoom", "printer", "edit-copy", "no-such-app"]
ICON_SIZES = [8, 32, 56, 200]


def icon_diffs(binding):
    env = dict(os.environ)
    env.update(icon_env())
    diffs = []
    for size in ICON_SIZES:
        core = json.loads(subprocess.run([str(CORE_CLI), "--icons", str(size), *ICON_NAMES], env=env, capture_output=True, text=True).stdout)
        for name in ICON_NAMES:
            mine = run(expand(binding.own_icons(), name=name, size=size), env=env, capture=True).stdout.strip()
            if mine != (core.get(name) or ""):
                diffs.append(f"{name} at {size}: {binding.name}={mine or 'none'} core={core.get(name) or 'none'}")
    return diffs


def fixtures():
    return sorted(d.name for d in FIXTURES.iterdir() if d.is_dir())


def cmd_list(args):
    for b in available(load(args)):
        print(b.name)


def cmd_references(args):
    """The references the named bindings are compared against (built beside them by bin/review)."""
    every = load()
    named = load(args) if args else []
    families = {"gui"} | {b.family for b in named}
    for b in available(every, quiet=True):
        if b.reference and b.family in families:
            print(b.name)


def cmd_build(args):
    failed = False
    for b in available(load(args)):
        r = run(b["build"])
        failed |= r.returncode != 0
    return 1 if failed else 0


def cmd_test(args):
    failed = False
    for b in available(load(args)):
        for t in b["tests"]:
            r = run(t["run"], capture=True, extra=t.get("env"))
            out = (r.stdout + r.stderr).strip().splitlines()
            if r.returncode == 0:
                last = next((l for l in reversed(out) if l.strip()), "")
                print(f"{b.name}: {t['name']}: ok" + (f" ({last.strip()})" if last else ""))
            else:
                failed = True
                print(f"{b.name}: {t['name']}: FAILED (exit {r.returncode})")
                for line in out[-40:]:
                    print(f"  {line}")
    return 1 if failed else 0


def cmd_parity(args):
    bindings = available(load(args))
    failed = False
    jobs = [(b, t, r) for b in bindings if b.own_tokens() for t in fixtures() for r in (0, 8)]
    with ThreadPoolExecutor(max_workers=max(2, (os.cpu_count() or 4) // 2)) as pool:
        results = list(pool.map(lambda j: token_diffs(*j), jobs))
    for (b, t, r), diffs in zip(jobs, results):
        if diffs:
            failed = True
            print(f"token parity: {b.name} differs from the core for {t} at radius {r}: {diffs[0]}" + (f" (+{len(diffs) - 1})" if len(diffs) > 1 else ""))
    for b in bindings:
        if b.own_tokens() and not any(d for (bb, _, _), d in zip(jobs, results) if bb is b):
            print(f"token parity: {b.name} dumps the core's tokens for every fixture at radius 0 and 8")
        elif not b.own_tokens():
            print(f"token parity: {b.name} reads the core's tokens directly")
    for b in bindings:
        if b.no_icons():
            print(f"icon parity: {b.name} draws no icons by name ({b.family})")
        elif b.own_icons():
            diffs = icon_diffs(b)
            for d in diffs:
                print(f"icon parity: {d}")
            failed |= bool(diffs)
            if not diffs:
                print(f"icon parity: {b.name} resolves every fixture icon as the core does")
    return 1 if failed else 0


def cmd_windows(args):
    procs = [subprocess.Popen(["bash", "-c", b["gallery"]["window"]], cwd=ROOT) for b in available(load(args)) if b["gallery"].get("window")]
    for p in procs:
        p.wait()
    return 0


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        print(__doc__.strip())
        return 0
    cmd, args = sys.argv[1], sys.argv[2:]
    handler = {"list": cmd_list, "references": cmd_references, "build": cmd_build, "test": cmd_test, "parity": cmd_parity, "windows": cmd_windows}.get(cmd)
    if not handler:
        sys.exit(f"unknown command {cmd} (list, references, build, test, parity, windows)")
    return handler(args) or 0


if __name__ == "__main__":
    sys.exit(main())

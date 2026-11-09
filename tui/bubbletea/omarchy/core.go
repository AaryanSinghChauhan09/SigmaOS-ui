package omarchy

import (
	"bufio"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"sync"
)

// The core, from Go: the `omarchy-theme` CLI resolves the tokens (spec/tokens.md); this package
// only reads its JSON. No cgo.

// ThemeBin finds the omarchy-theme CLI: $OMARCHY_THEME_BIN, else on PATH, else this checkout's
// build (target/release or target/debug, from the module's own directory).
func ThemeBin() (string, error) {
	if p := os.Getenv("OMARCHY_THEME_BIN"); p != "" {
		return p, nil
	}
	if p, err := exec.LookPath("omarchy-theme"); err == nil {
		return p, nil
	}
	if _, file, _, ok := runtime.Caller(0); ok {
		// tui/bubbletea/omarchy/core.go → the checkout's root.
		root := filepath.Join(filepath.Dir(file), "..", "..", "..")
		for _, p := range []string{"target/release/omarchy-theme", "target/debug/omarchy-theme"} {
			if fi, err := os.Stat(filepath.Join(root, p)); err == nil && !fi.IsDir() {
				return filepath.Join(root, p), nil
			}
		}
	}
	return "", errors.New("omarchy-theme not found: install the omarchy-ui package, put it on PATH, or set OMARCHY_THEME_BIN")
}

// ReadState runs `omarchy-theme --state` once: the theme as it is now (or the
// OMARCHY_UI_THEME_DIR fixture, under the caller's OMARCHY_UI_* overrides).
func ReadState() (Tokens, error) {
	bin, err := ThemeBin()
	if err != nil {
		return Tokens{}, err
	}
	out, err := exec.Command(bin, "--state").Output()
	if err != nil {
		return Tokens{}, fmt.Errorf("%s --state: %w", bin, err)
	}
	return ParseState(out)
}

// watcher follows `omarchy-theme --watch --state`: the first line is the state now, every later
// one a change (coalesced and deduplicated by the core).
type watcher struct {
	cmd   *exec.Cmd
	lines chan Tokens
	once  sync.Once
}

func startWatch() (*watcher, error) {
	bin, err := ThemeBin()
	if err != nil {
		return nil, err
	}
	cmd := exec.Command(bin, "--watch", "--state")
	cmd.Stderr = nil
	dieWithParent(cmd)
	out, err := cmd.StdoutPipe()
	if err != nil {
		return nil, err
	}
	if err := cmd.Start(); err != nil {
		return nil, err
	}
	w := &watcher{cmd: cmd, lines: make(chan Tokens, 16)}
	go func() {
		sc := bufio.NewScanner(out)
		sc.Buffer(make([]byte, 1<<20), 16<<20)
		for sc.Scan() {
			if t, err := ParseState(sc.Bytes()); err == nil {
				w.lines <- t
			}
		}
		close(w.lines)
	}()
	return w, nil
}

func (w *watcher) stop() {
	w.once.Do(func() {
		if w.cmd.Process != nil {
			_ = w.cmd.Process.Kill()
			_ = w.cmd.Wait()
		}
	})
}

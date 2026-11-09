//go:build !linux

package omarchy

import "os/exec"

func dieWithParent(*exec.Cmd) {}

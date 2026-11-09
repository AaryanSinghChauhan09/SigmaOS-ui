package omarchy

import (
	"os/exec"
	"syscall"
)

// dieWithParent ends the watch with this process, however it exits.
func dieWithParent(cmd *exec.Cmd) {
	cmd.SysProcAttr = &syscall.SysProcAttr{Pdeathsig: syscall.SIGTERM}
}

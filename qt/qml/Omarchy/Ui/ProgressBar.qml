import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// Progress of a task: a Meter whose fill is the accent (as the OSD and app export bars draw
// it), with an indeterminate mode for work of unknown length: a quarter-track segment that sweeps
// across, held still in the middle under reduced motion.
//
//   ProgressBar { value: 0.4; label: "Exporting"; valueText: "40%" }
//   ProgressBar { indeterminate: true }
Meter {
  tone: "info"
}

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Plain Kirigami: no Omarchy imports. Its colors (Kirigami.Theme) and controls follow the theme
// because main.cpp made the one call.
Kirigami.ApplicationWindow {
  id: root
  width: 420
  height: 380
  visible: true
  title: "Network"

  property bool wifi: true

  pageStack.initialPage: Kirigami.ScrollablePage {
    title: "Network"

    ColumnLayout {
      spacing: Kirigami.Units.largeSpacing

      Kirigami.Heading { text: "Wi-Fi and saved networks"; level: 3 }

      Controls.Switch {
        text: "Wi-Fi"
        checked: root.wifi
        onToggled: root.wifi = checked
      }

      Kirigami.FormLayout {
        Layout.fillWidth: true
        Controls.TextField { Kirigami.FormData.label: "Network:"; text: "Basecamp" }
        Controls.ComboBox { Kirigami.FormData.label: "Band:"; model: ["5 GHz", "2.4 GHz"] }
        Controls.CheckBox { Kirigami.FormData.label: "Options:"; text: "Connect automatically"; checked: true }
      }

      Controls.Slider { Layout.fillWidth: true; value: 0.6 }

      RowLayout {
        Controls.Button { text: "Forget network" }
        Controls.Button { text: "Connect"; highlighted: true }
      }

      Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: true
        text: root.wifi ? "Connected to Basecamp" : "Wi-Fi is off"
        type: root.wifi ? Kirigami.MessageType.Positive : Kirigami.MessageType.Information
      }
    }
  }
}

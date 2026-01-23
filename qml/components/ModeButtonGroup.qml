import QtQuick
import QtQuick.Layouts
import "../theme"

RowLayout {
    id: root

    property int currentIndex: 0
    property var model: []
    property bool enabled: true

    signal modeSelected(int index)

    spacing: Theme.spacingSmall

    Repeater {
        model: root.model

        ModeButton {
            text: modelData.name
            icon: modelData.icon || ""
            selected: root.currentIndex === index
            accentColor: modelData.color || Theme.accent
            enabled: root.enabled

            onClicked: {
                if (root.enabled) {
                    root.currentIndex = index
                    root.modeSelected(index)
                }
            }
        }
    }
}

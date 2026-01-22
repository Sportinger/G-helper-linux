import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../theme"

RowLayout {
    id: root

    property color selectedColor: "#00a0e0"
    property string label: ""

    signal colorSelected(color newColor)

    spacing: Theme.spacingSmall

    Text {
        visible: root.label !== ""
        text: root.label
        font.pixelSize: Theme.fontSizeMedium
        color: Theme.textPrimary
    }

    Rectangle {
        width: 32
        height: 32
        radius: Theme.radiusSmall
        color: root.selectedColor
        border.color: mouseArea.containsMouse ? Theme.borderHover : Theme.border
        border.width: 2

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: colorDialog.open()
        }
    }

    // Preset colors
    Row {
        spacing: 4

        Repeater {
            model: [
                "#ff0000", "#ff8000", "#ffff00", "#00ff00",
                "#00ffff", "#0080ff", "#8000ff", "#ff00ff", "#ffffff"
            ]

            Rectangle {
                width: 20
                height: 20
                radius: 2
                color: modelData
                border.color: root.selectedColor === modelData ? Theme.accent : "transparent"
                border.width: 2

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.selectedColor = modelData
                        root.colorSelected(modelData)
                    }
                }
            }
        }
    }

    ColorDialog {
        id: colorDialog
        selectedColor: root.selectedColor
        onAccepted: {
            root.selectedColor = selectedColor
            root.colorSelected(selectedColor)
        }
    }
}

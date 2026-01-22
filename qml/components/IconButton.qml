import QtQuick
import QtQuick.Controls
import "../theme"

ToolButton {
    id: root

    implicitWidth: 36
    implicitHeight: 36

    icon.width: Theme.iconSize
    icon.height: Theme.iconSize
    icon.color: hovered ? Theme.textPrimary : Theme.textSecondary

    background: Rectangle {
        radius: Theme.radiusSmall
        color: root.pressed ? Theme.surfaceLight :
               root.hovered ? Theme.colorWithAlpha(Theme.accent, 0.1) : "transparent"

        Behavior on color {
            ColorAnimation { duration: Theme.animationFast }
        }
    }

    ToolTip.visible: hovered && ToolTip.text !== ""
    ToolTip.delay: 500
}

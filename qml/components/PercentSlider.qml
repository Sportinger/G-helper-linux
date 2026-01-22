import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

ColumnLayout {
    id: root

    property alias value: slider.value
    property alias from: slider.from
    property alias to: slider.to
    property alias stepSize: slider.stepSize
    property string label: ""
    property string unit: "%"
    property bool showValue: true

    signal moved()

    spacing: Theme.spacingSmall

    RowLayout {
        Layout.fillWidth: true
        visible: root.label !== "" || root.showValue

        Text {
            visible: root.label !== ""
            text: root.label
            font.pixelSize: Theme.fontSizeMedium
            color: Theme.textPrimary
        }

        Item { Layout.fillWidth: true }

        Text {
            visible: root.showValue
            text: Math.round(slider.value) + root.unit
            font.pixelSize: Theme.fontSizeMedium
            font.bold: true
            color: Theme.accent
        }
    }

    Slider {
        id: slider
        Layout.fillWidth: true
        from: 0
        to: 100
        stepSize: 1

        onMoved: root.moved()

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 200
            implicitHeight: 6
            width: slider.availableWidth
            height: implicitHeight
            radius: 3
            color: Theme.surface

            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                radius: 3
                color: Theme.accent
            }
        }

        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            implicitWidth: 18
            implicitHeight: 18
            radius: 9
            color: slider.pressed ? Theme.accentLight : Theme.accent
            border.color: Theme.accentDark
            border.width: 2

            Behavior on color {
                ColorAnimation { duration: Theme.animationFast }
            }
        }
    }
}

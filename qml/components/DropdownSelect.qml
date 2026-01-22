import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

ComboBox {
    id: root

    property string label: ""

    implicitHeight: Theme.buttonHeight

    background: Rectangle {
        color: root.pressed ? Theme.surfaceLight : Theme.surface
        border.color: root.hovered ? Theme.borderHover : Theme.border
        radius: Theme.radiusSmall
    }

    contentItem: Text {
        leftPadding: Theme.spacingSmall
        rightPadding: root.indicator.width + Theme.spacingSmall
        text: root.displayText
        font.pixelSize: Theme.fontSizeMedium
        color: Theme.textPrimary
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    indicator: Canvas {
        id: canvas
        x: root.width - width - Theme.spacingSmall
        y: root.topPadding + (root.availableHeight - height) / 2
        width: 12
        height: 8
        contextType: "2d"

        Connections {
            target: root
            function onPressedChanged() { canvas.requestPaint() }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            ctx.moveTo(0, 0)
            ctx.lineTo(width, 0)
            ctx.lineTo(width / 2, height)
            ctx.closePath()
            ctx.fillStyle = Theme.textSecondary
            ctx.fill()
        }
    }

    popup: Popup {
        y: root.height
        width: root.width
        implicitHeight: contentItem.implicitHeight
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.delegateModel
            currentIndex: root.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: Theme.radiusSmall
        }
    }

    delegate: ItemDelegate {
        width: root.width
        height: Theme.buttonHeight

        contentItem: Text {
            text: root.textRole ? (modelData[root.textRole] || modelData) : modelData
            font.pixelSize: Theme.fontSizeMedium
            color: highlighted ? Theme.accent : Theme.textPrimary
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        highlighted: root.highlightedIndex === index

        background: Rectangle {
            color: highlighted ? Theme.colorWithAlpha(Theme.accent, 0.2) : "transparent"
        }
    }
}

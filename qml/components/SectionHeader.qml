import QtQuick
import QtQuick.Layouts
import "../theme"

RowLayout {
    id: root

    property string title: ""
    property string subtitle: ""
    property string icon: ""

    spacing: Theme.spacingSmall

    Image {
        visible: root.icon !== ""
        source: root.icon
        width: Theme.iconSize
        height: Theme.iconSize
        sourceSize: Qt.size(width, height)
    }

    ColumnLayout {
        spacing: 2

        Text {
            text: root.title
            font.pixelSize: Theme.fontSizeLarge
            font.bold: true
            color: Theme.textPrimary
        }

        Text {
            visible: root.subtitle !== ""
            text: root.subtitle
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
        }
    }

    Item { Layout.fillWidth: true }
}

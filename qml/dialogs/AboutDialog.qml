import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Dialog {
    id: root
    title: qsTr("About G-Helper Linux")
    modal: true
    width: 350
    height: 300

    background: Rectangle {
        color: Theme.background
        border.color: Theme.border
        radius: Theme.radiusMedium
    }

    header: Rectangle {
        color: Theme.surface
        height: 50
        radius: Theme.radiusMedium

        Text {
            anchors.centerIn: parent
            text: root.title
            font.pixelSize: Theme.fontSizeLarge
            font.bold: true
            color: Theme.textPrimary
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingLarge

        // Logo/Icon placeholder
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 64
            height: 64
            radius: Theme.radiusMedium
            color: Theme.accent

            Text {
                anchors.centerIn: parent
                text: "G"
                font.pixelSize: 36
                font.bold: true
                color: Theme.textPrimary
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "G-Helper Linux"
                font.pixelSize: Theme.fontSizeXLarge
                font.bold: true
                color: Theme.textPrimary
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Version 0.1.0")
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textSecondary
            }
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("A lightweight control tool for ASUS ROG laptops on Linux.\n\nInspired by G-Helper for Windows.")
            font.pixelSize: Theme.fontSizeMedium
            color: Theme.textSecondary
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingTiny

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Built with Qt 6 and asusctl/supergfxctl")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "© 2024 G-Helper Linux Contributors"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.spacingMedium

            Button {
                text: qsTr("GitHub")
                flat: true

                onClicked: Qt.openUrlExternally("https://github.com/g-helper-linux/g-helper-linux")

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.accent
                    horizontalAlignment: Text.AlignHCenter
                }

                background: Rectangle {
                    color: parent.hovered ? Theme.colorWithAlpha(Theme.accent, 0.1) : "transparent"
                    radius: Theme.radiusSmall
                }
            }

            Button {
                text: qsTr("License")
                flat: true

                onClicked: Qt.openUrlExternally("https://www.gnu.org/licenses/gpl-3.0.html")

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.accent
                    horizontalAlignment: Text.AlignHCenter
                }

                background: Rectangle {
                    color: parent.hovered ? Theme.colorWithAlpha(Theme.accent, 0.1) : "transparent"
                    radius: Theme.radiusSmall
                }
            }
        }
    }

    footer: DialogButtonBox {
        background: Rectangle {
            color: Theme.surface
        }

        Button {
            text: qsTr("Close")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole

            background: Rectangle {
                implicitWidth: 80
                implicitHeight: Theme.buttonHeight
                radius: Theme.radiusSmall
                color: parent.pressed ? Theme.accentDark :
                       parent.hovered ? Theme.accentLight : Theme.accent
            }

            contentItem: Text {
                text: parent.text
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}

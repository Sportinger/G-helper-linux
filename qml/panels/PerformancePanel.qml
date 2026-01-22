import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import GHelperLinux
import "../theme"
import "../components"

Rectangle {
    id: root
    color: Theme.surface
    radius: Theme.radiusMedium
    border.color: Theme.border
    implicitHeight: layout.implicitHeight + Theme.spacingMedium * 2

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Theme.spacingMedium
        spacing: Theme.spacingMedium

        SectionHeader {
            title: qsTr("Performance Mode")
            subtitle: PerformanceController.currentProfileName
            icon: "qrc:/icons/performance.svg"
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.spacingMedium

            ModeButton {
                text: qsTr("Silent")
                icon: "qrc:/icons/quiet.svg"
                selected: PerformanceController.currentProfile === 0
                accentColor: Theme.quietColor
                enabled: PerformanceController.available

                onClicked: PerformanceController.setProfile(0)

                ToolTip.visible: containsMouse
                ToolTip.text: PerformanceController.profileDescription(0)
                ToolTip.delay: 500
            }

            ModeButton {
                text: qsTr("Balanced")
                icon: "qrc:/icons/balanced.svg"
                selected: PerformanceController.currentProfile === 1
                accentColor: Theme.balancedColor
                enabled: PerformanceController.available

                onClicked: PerformanceController.setProfile(1)

                ToolTip.visible: containsMouse
                ToolTip.text: PerformanceController.profileDescription(1)
                ToolTip.delay: 500
            }

            ModeButton {
                text: qsTr("Turbo")
                icon: "qrc:/icons/turbo.svg"
                selected: PerformanceController.currentProfile === 2
                accentColor: Theme.performanceColor
                enabled: PerformanceController.available

                onClicked: PerformanceController.setProfile(2)

                ToolTip.visible: containsMouse
                ToolTip.text: PerformanceController.profileDescription(2)
                ToolTip.delay: 500
            }
        }

        // Not available message
        Text {
            Layout.fillWidth: true
            visible: !PerformanceController.available
            text: qsTr("Performance control not available. Is asusd running?")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.warning
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }
}

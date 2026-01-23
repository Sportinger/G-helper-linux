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
            title: qsTr("GPU Mode")
            subtitle: GpuController.switchPending ?
                      qsTr("Pending: %1 (logout required)").arg(GpuController.modeName(GpuController.pendingMode)) :
                      GpuController.currentModeName
            icon: "qrc:/icons/gpu.svg"
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.spacingMedium

            ModeButton {
                text: qsTr("Eco")
                icon: "qrc:/icons/eco.svg"
                selected: GpuController.currentMode === 0
                accentColor: Theme.ecoColor
                enabled: GpuController.available

                onClicked: {
                    if (GpuController.requiresLogout(0)) {
                        logoutDialog.targetMode = 0
                        logoutDialog.open()
                    } else {
                        GpuController.setMode(0)
                    }
                }

                ToolTip.visible: containsMouse
                ToolTip.text: GpuController.modeDescription(0)
                ToolTip.delay: 500
            }

            ModeButton {
                text: qsTr("Standard")
                icon: "qrc:/icons/hybrid.svg"
                selected: GpuController.currentMode === 1
                accentColor: Theme.standardColor
                enabled: GpuController.available

                onClicked: {
                    if (GpuController.requiresLogout(1)) {
                        logoutDialog.targetMode = 1
                        logoutDialog.open()
                    } else {
                        GpuController.setMode(1)
                    }
                }

                ToolTip.visible: containsMouse
                ToolTip.text: GpuController.modeDescription(1)
                ToolTip.delay: 500
            }

            ModeButton {
                text: qsTr("Ultimate")
                icon: "qrc:/icons/dedicated.svg"
                selected: GpuController.currentMode === 2
                accentColor: Theme.ultimateColor
                enabled: GpuController.available

                onClicked: {
                    if (GpuController.requiresLogout(2)) {
                        logoutDialog.targetMode = 2
                        logoutDialog.open()
                    } else {
                        GpuController.setMode(2)
                    }
                }

                ToolTip.visible: containsMouse
                ToolTip.text: GpuController.modeDescription(2)
                ToolTip.delay: 500
            }

            ModeButton {
                visible: GpuController.supportedModes.some(m => m.mode === 3)
                text: qsTr("Optimized")
                icon: "qrc:/icons/vfio.svg"
                selected: GpuController.currentMode === 3
                accentColor: Theme.optimizedColor
                enabled: GpuController.available

                onClicked: {
                    if (GpuController.requiresLogout(3)) {
                        logoutDialog.targetMode = 3
                        logoutDialog.open()
                    } else {
                        GpuController.setMode(3)
                    }
                }

                ToolTip.visible: containsMouse
                ToolTip.text: GpuController.modeDescription(3)
                ToolTip.delay: 500
            }
        }

        // GPU Power and Usage status
        RowLayout {
            Layout.fillWidth: true
            visible: GpuController.available
            spacing: Theme.spacingMedium

            Text {
                text: qsTr("dGPU:")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }

            Text {
                text: GpuController.gpuPower
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: GpuController.gpuPower === "Off" ? Theme.success :
                       GpuController.gpuPower === "Active" ? Theme.warning : Theme.textSecondary
            }

            Item { Layout.fillWidth: true }

            // dGPU Usage (only show when active)
            Text {
                visible: GpuController.gpuPower === "Active" && SystemMonitor.dgpuUsage > 0
                text: qsTr("Usage: %1%").arg(Math.round(SystemMonitor.dgpuUsage))
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }

            // dGPU Temperature
            Text {
                visible: GpuController.gpuPower === "Active" && SystemMonitor.dgpuTemp > 0
                text: qsTr("%1°C").arg(SystemMonitor.dgpuTemp)
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: SystemMonitor.dgpuTemp > 80 ? Theme.error :
                       SystemMonitor.dgpuTemp > 70 ? Theme.warning : Theme.success
            }
        }

        // Not available message
        Text {
            Layout.fillWidth: true
            visible: !GpuController.available
            text: qsTr("GPU control not available. Is supergfxd running?")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.warning
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }

    // Logout confirmation dialog
    Dialog {
        id: logoutDialog
        property int targetMode: 0

        anchors.centerIn: parent
        title: qsTr("Logout Required")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: Theme.radiusMedium
        }

        header: Rectangle {
            color: Theme.surfaceLight
            height: 40
            radius: Theme.radiusMedium

            Text {
                anchors.centerIn: parent
                text: logoutDialog.title
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                color: Theme.textPrimary
            }
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingMedium

            Text {
                Layout.fillWidth: true
                text: qsTr("Switching to %1 mode requires logging out of your session. Save your work before proceeding.")
                      .arg(GpuController.modeName(logoutDialog.targetMode))
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textPrimary
                wrapMode: Text.WordWrap
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("The GPU mode will change after you log back in.")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }
        }

        onAccepted: {
            GpuController.setMode(targetMode)
        }
    }

    Connections {
        target: GpuController
        function onLogoutRequired(modeName) {
            TrayManager.showMessage(
                qsTr("GPU Mode Change"),
                qsTr("Switching to %1 mode. Please log out to complete the change.").arg(modeName),
                1, // Information icon
                10000
            )
        }
    }
}

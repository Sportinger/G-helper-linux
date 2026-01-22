import QtQuick
import QtQuick.Layouts
import GHelperLinux
import "../theme"

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
        spacing: Theme.spacingSmall

        SectionHeader {
            title: qsTr("System Status")
            icon: "qrc:/icons/monitor.svg"
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: Theme.spacingSmall
            columnSpacing: Theme.spacingLarge

            // CPU Temperature
            StatusItem {
                label: qsTr("CPU Temp")
                value: SystemMonitor.cpuTemp + "°C"
                color: tempColor(SystemMonitor.cpuTemp)
            }

            // GPU Temperature
            StatusItem {
                label: qsTr("GPU Temp")
                value: SystemMonitor.gpuTemp > 0 ? SystemMonitor.gpuTemp + "°C" : "N/A"
                color: tempColor(SystemMonitor.gpuTemp)
            }

            // CPU Fan
            StatusItem {
                label: qsTr("CPU Fan")
                value: SystemMonitor.cpuFanRpm > 0 ? SystemMonitor.cpuFanRpm + " RPM" : "Off"
                color: Theme.textSecondary
            }

            // GPU Fan
            StatusItem {
                label: qsTr("GPU Fan")
                value: SystemMonitor.gpuFanRpm > 0 ? SystemMonitor.gpuFanRpm + " RPM" : "Off"
                color: Theme.textSecondary
            }

            // CPU Usage
            StatusItem {
                label: qsTr("CPU Usage")
                value: SystemMonitor.cpuUsage.toFixed(1) + "%"
                color: usageColor(SystemMonitor.cpuUsage)
            }

            // Memory
            StatusItem {
                label: qsTr("Memory")
                value: (SystemMonitor.memoryUsed / 1024).toFixed(1) + " / " +
                       (SystemMonitor.memoryTotal / 1024).toFixed(1) + " GB"
                color: Theme.textSecondary
            }
        }
    }

    function tempColor(temp) {
        if (temp >= 90) return Theme.error
        if (temp >= 75) return Theme.warning
        return Theme.success
    }

    function usageColor(usage) {
        if (usage >= 90) return Theme.error
        if (usage >= 70) return Theme.warning
        return Theme.success
    }

    component StatusItem: RowLayout {
        property string label: ""
        property string value: ""
        property color color: Theme.textSecondary

        Layout.fillWidth: true
        spacing: Theme.spacingSmall

        Text {
            text: label + ":"
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
        }

        Text {
            text: value
            font.pixelSize: Theme.fontSizeSmall
            font.bold: true
            color: parent.color
        }
    }
}

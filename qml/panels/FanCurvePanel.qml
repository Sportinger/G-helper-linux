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

        RowLayout {
            Layout.fillWidth: true

            SectionHeader {
                title: qsTr("Fan Curves")
                subtitle: qsTr("Profile: %1").arg(PerformanceController.currentProfileName)
                icon: "qrc:/icons/fan.svg"
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Edit")
                enabled: FanController.available

                onClicked: fanCurveDialog.open()

                background: Rectangle {
                    implicitWidth: 60
                    implicitHeight: 28
                    radius: Theme.radiusSmall
                    color: parent.pressed ? Theme.surfaceLight :
                           parent.hovered ? Theme.colorWithAlpha(Theme.accent, 0.2) : "transparent"
                    border.color: parent.hovered ? Theme.accent : Theme.border
                }

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: Theme.fontSizeSmall
                    color: parent.hovered ? Theme.accent : Theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        // Mini fan curve preview
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLarge

            // CPU Fan curve
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                Text {
                    text: qsTr("CPU Fan")
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                    color: Theme.textPrimary
                }

                FanCurvePreview {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    curveData: FanController.cpuCurve
                    curveColor: Theme.quietColor
                    currentTemp: SystemMonitor.cpuTemp
                    enabled: FanController.cpuCurveEnabled
                }

                RowLayout {
                    spacing: Theme.spacingSmall

                    CheckBox {
                        id: cpuCurveEnabled
                        checked: FanController.cpuCurveEnabled
                        onCheckedChanged: {
                            if (checked !== FanController.cpuCurveEnabled) {
                                FanController.setCpuCurve(FanController.cpuCurve, checked)
                            }
                        }
                    }

                    Text {
                        text: qsTr("Custom curve")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }
                }
            }

            // GPU Fan curve
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                Text {
                    text: qsTr("GPU Fan")
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                    color: Theme.textPrimary
                }

                FanCurvePreview {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    curveData: FanController.gpuCurve
                    curveColor: Theme.performanceColor
                    currentTemp: SystemMonitor.gpuTemp
                    enabled: FanController.gpuCurveEnabled
                }

                RowLayout {
                    spacing: Theme.spacingSmall

                    CheckBox {
                        id: gpuCurveEnabled
                        checked: FanController.gpuCurveEnabled
                        onCheckedChanged: {
                            if (checked !== FanController.gpuCurveEnabled) {
                                FanController.setGpuCurve(FanController.gpuCurve, checked)
                            }
                        }
                    }

                    Text {
                        text: qsTr("Custom curve")
                        font.pixelSize: Theme.fontSizeSmall
                        color: Theme.textSecondary
                    }
                }
            }
        }

        // Not available message
        Text {
            Layout.fillWidth: true
            visible: !FanController.available
            text: qsTr("Fan curve control not available.")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.warning
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }

    // Mini fan curve preview component
    component FanCurvePreview: Canvas {
        id: canvas

        property var curveData: []
        property color curveColor: Theme.accent
        property int currentTemp: 0
        property bool enabled: true

        onCurveDataChanged: requestPaint()
        onCurrentTempChanged: requestPaint()
        onEnabledChanged: requestPaint()

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()

            var w = width
            var h = height
            var padding = 4

            // Background
            ctx.fillStyle = Theme.surfaceLight
            ctx.fillRect(0, 0, w, h)

            // Grid lines
            ctx.strokeStyle = Theme.border
            ctx.lineWidth = 0.5

            for (var i = 0; i <= 4; i++) {
                var y = padding + (h - padding * 2) * i / 4
                ctx.beginPath()
                ctx.moveTo(padding, y)
                ctx.lineTo(w - padding, y)
                ctx.stroke()
            }

            if (!curveData || curveData.length === 0) return

            // Draw curve
            ctx.strokeStyle = enabled ? curveColor : Theme.textDisabled
            ctx.lineWidth = 2
            ctx.beginPath()

            for (var j = 0; j < curveData.length; j++) {
                var point = curveData[j]
                var x = padding + (point.temp - 50) / 50 * (w - padding * 2)
                var y = h - padding - (point.fan / 100) * (h - padding * 2)

                if (j === 0) {
                    ctx.moveTo(x, y)
                } else {
                    ctx.lineTo(x, y)
                }
            }
            ctx.stroke()

            // Current temperature indicator
            if (currentTemp > 0 && enabled) {
                var tempX = padding + (currentTemp - 50) / 50 * (w - padding * 2)
                ctx.strokeStyle = Theme.warning
                ctx.lineWidth = 1
                ctx.setLineDash([2, 2])
                ctx.beginPath()
                ctx.moveTo(tempX, padding)
                ctx.lineTo(tempX, h - padding)
                ctx.stroke()
                ctx.setLineDash([])
            }
        }
    }
}

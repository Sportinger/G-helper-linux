import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import GHelperLinux
import "../theme"
import "../components"

Window {
    id: root
    title: qsTr("Fans and Power")
    width: 520
    height: 500
    minimumWidth: 480
    minimumHeight: 450
    color: Theme.background
    flags: Qt.Window

    property int selectedFan: 0 // 0 = CPU, 1 = GPU
    property var currentCurve: selectedFan === 0 ? FanController.cpuCurve : FanController.gpuCurve

    function open() {
        root.show()
        root.raise()
        root.requestActivate()
    }

    // Header
    Rectangle {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 50
        color: Theme.surface

        RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingMedium

            Text {
                text: root.title
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                color: Theme.textPrimary
            }

            Item { Layout.fillWidth: true }

            // Fan selector
            TabBar {
                id: fanTabs
                currentIndex: root.selectedFan

                background: Rectangle {
                    color: "transparent"
                }

                TabButton {
                    text: qsTr("CPU Fan")
                    width: implicitWidth

                    background: Rectangle {
                        color: fanTabs.currentIndex === 0 ?
                               Theme.colorWithAlpha(Theme.quietColor, 0.3) : "transparent"
                        radius: Theme.radiusSmall
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: Theme.fontSizeMedium
                        color: fanTabs.currentIndex === 0 ? Theme.quietColor : Theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: root.selectedFan = 0
                }

                TabButton {
                    text: qsTr("GPU Fan")
                    width: implicitWidth

                    background: Rectangle {
                        color: fanTabs.currentIndex === 1 ?
                               Theme.colorWithAlpha(Theme.performanceColor, 0.3) : "transparent"
                        radius: Theme.radiusSmall
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: Theme.fontSizeMedium
                        color: fanTabs.currentIndex === 1 ? Theme.performanceColor : Theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: root.selectedFan = 1
                }
            }
        }

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.border
        }
    }

    // Content
    ColumnLayout {
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spacingMedium
        spacing: Theme.spacingMedium

        // Fan curve canvas
        FanCurveCanvas {
            id: curveCanvas
            Layout.fillWidth: true
            Layout.fillHeight: true
            curveData: root.currentCurve
            curveColor: root.selectedFan === 0 ? Theme.quietColor : Theme.performanceColor

            onPointMoved: function(index, temp, fan) {
                var newCurve = root.currentCurve.slice()
                newCurve[index] = { temp: temp, fan: fan }

                if (root.selectedFan === 0) {
                    FanController.setCpuCurve(newCurve, FanController.cpuCurveEnabled)
                } else {
                    FanController.setGpuCurve(newCurve, FanController.gpuCurveEnabled)
                }
            }
        }

        // Preset buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            Text {
                text: qsTr("Presets:")
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textSecondary
            }

            Button {
                text: qsTr("Silent")
                onClicked: applyPreset(0)

                background: Rectangle {
                    implicitHeight: 28
                    radius: Theme.radiusSmall
                    color: parent.hovered ? Theme.colorWithAlpha(Theme.quietColor, 0.2) : "transparent"
                    border.color: Theme.quietColor
                }

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.quietColor
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Button {
                text: qsTr("Balanced")
                onClicked: applyPreset(1)

                background: Rectangle {
                    implicitHeight: 28
                    radius: Theme.radiusSmall
                    color: parent.hovered ? Theme.colorWithAlpha(Theme.balancedColor, 0.2) : "transparent"
                    border.color: Theme.balancedColor
                }

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.balancedColor
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Button {
                text: qsTr("Performance")
                onClicked: applyPreset(2)

                background: Rectangle {
                    implicitHeight: 28
                    radius: Theme.radiusSmall
                    color: parent.hovered ? Theme.colorWithAlpha(Theme.performanceColor, 0.2) : "transparent"
                    border.color: Theme.performanceColor
                }

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.performanceColor
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Reset")
                onClicked: FanController.resetToDefaults()

                background: Rectangle {
                    implicitHeight: 28
                    radius: Theme.radiusSmall
                    color: parent.hovered ? Theme.colorWithAlpha(Theme.warning, 0.2) : "transparent"
                    border.color: Theme.warning
                }

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.warning
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    function applyPreset(profile) {
        var presetCurve = FanController.defaultCurve(profile)
        if (root.selectedFan === 0) {
            FanController.setCpuCurve(presetCurve, true)
        } else {
            FanController.setGpuCurve(presetCurve, true)
        }
    }

    // Fan curve canvas component
    component FanCurveCanvas: Canvas {
        id: canvas

        property var curveData: []
        property color curveColor: Theme.accent
        property int dragIndex: -1
        property real dragX: 0
        property real dragY: 0

        signal pointMoved(int index, int temp, int fan)

        onCurveDataChanged: requestPaint()

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true

            onPressed: function(mouse) {
                canvas.dragIndex = findNearestPoint(mouse.x, mouse.y)
            }

            onPositionChanged: function(mouse) {
                if (canvas.dragIndex >= 0) {
                    var coords = screenToData(mouse.x, mouse.y)
                    canvas.pointMoved(canvas.dragIndex, coords.temp, coords.fan)
                }
            }

            onReleased: {
                canvas.dragIndex = -1
            }
        }

        function findNearestPoint(mx, my) {
            var padding = 40
            var w = width - padding * 2
            var h = height - padding * 2

            for (var i = 0; i < curveData.length; i++) {
                var point = curveData[i]
                var x = padding + (point.temp - 30) / 70 * w
                var y = height - padding - (point.fan / 100) * h

                var dist = Math.sqrt(Math.pow(mx - x, 2) + Math.pow(my - y, 2))
                if (dist < 15) return i
            }
            return -1
        }

        function screenToData(mx, my) {
            var padding = 40
            var w = width - padding * 2
            var h = height - padding * 2

            var temp = Math.round(30 + (mx - padding) / w * 70)
            var fan = Math.round(100 - (my - padding) / h * 100)

            temp = Math.max(30, Math.min(100, temp))
            fan = Math.max(0, Math.min(100, fan))

            return { temp: temp, fan: fan }
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()

            var padding = 40
            var w = width - padding * 2
            var h = height - padding * 2

            // Background
            ctx.fillStyle = Theme.surface
            ctx.fillRect(padding, padding, w, h)

            // Grid
            ctx.strokeStyle = Theme.border
            ctx.lineWidth = 0.5

            // Horizontal grid lines (fan %)
            for (var i = 0; i <= 10; i++) {
                var y = padding + h * i / 10
                ctx.beginPath()
                ctx.moveTo(padding, y)
                ctx.lineTo(padding + w, y)
                ctx.stroke()

                // Labels
                ctx.fillStyle = Theme.textSecondary
                ctx.font = "10px sans-serif"
                ctx.textAlign = "right"
                ctx.fillText((100 - i * 10) + "%", padding - 5, y + 4)
            }

            // Vertical grid lines (temp)
            for (var j = 0; j <= 7; j++) {
                var x = padding + w * j / 7
                ctx.beginPath()
                ctx.moveTo(x, padding)
                ctx.lineTo(x, padding + h)
                ctx.stroke()

                // Labels
                ctx.fillStyle = Theme.textSecondary
                ctx.textAlign = "center"
                ctx.fillText((30 + j * 10) + "°", x, height - padding + 15)
            }

            // Axis labels
            ctx.fillStyle = Theme.textPrimary
            ctx.font = "12px sans-serif"
            ctx.textAlign = "center"
            ctx.fillText("Temperature (°C)", width / 2, height - 5)

            ctx.save()
            ctx.translate(12, height / 2)
            ctx.rotate(-Math.PI / 2)
            ctx.fillText("Fan Speed (%)", 0, 0)
            ctx.restore()

            if (!curveData || curveData.length === 0) return

            // Draw curve
            ctx.strokeStyle = curveColor
            ctx.lineWidth = 2
            ctx.beginPath()

            for (var k = 0; k < curveData.length; k++) {
                var point = curveData[k]
                var px = padding + (point.temp - 30) / 70 * w
                var py = padding + h - (point.fan / 100) * h

                if (k === 0) {
                    ctx.moveTo(px, py)
                } else {
                    ctx.lineTo(px, py)
                }
            }
            ctx.stroke()

            // Draw points
            for (var m = 0; m < curveData.length; m++) {
                var pt = curveData[m]
                var ptx = padding + (pt.temp - 30) / 70 * w
                var pty = padding + h - (pt.fan / 100) * h

                ctx.beginPath()
                ctx.arc(ptx, pty, dragIndex === m ? 8 : 6, 0, Math.PI * 2)
                ctx.fillStyle = dragIndex === m ? Theme.accentLight : curveColor
                ctx.fill()
                ctx.strokeStyle = Theme.textPrimary
                ctx.lineWidth = 2
                ctx.stroke()
            }
        }
    }
}

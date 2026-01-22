import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import GHelperLinux
import "../theme"
import "../components"

Window {
    id: root
    title: qsTr("Fan Curves")
    width: 520
    height: 750
    minimumWidth: 480
    minimumHeight: 650
    color: Theme.background
    flags: Qt.Window

    property int selectedProfile: 0 // 0 = Silent, 1 = Balanced, 2 = Turbo
    property int mainWindowX: 0
    property int mainWindowY: 0

    // Profile colors
    function profileColor(profile) {
        switch(profile) {
            case 0: return Theme.quietColor      // Silent - cyan
            case 1: return Theme.balancedColor   // Balanced - green
            case 2: return Theme.performanceColor // Turbo - orange
            default: return Theme.accent
        }
    }

    function profileName(profile) {
        switch(profile) {
            case 0: return qsTr("Silent")
            case 1: return qsTr("Balanced")
            case 2: return qsTr("Turbo")
            default: return ""
        }
    }

    function open(mainX, mainY) {
        mainWindowX = mainX
        mainWindowY = mainY
        root.x = mainX - root.width - 8
        root.y = mainY
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

            // Profile selector
            Row {
                spacing: Theme.spacingSmall

                Repeater {
                    model: 3
                    Button {
                        text: profileName(index)
                        width: 80
                        height: 32

                        background: Rectangle {
                            radius: Theme.radiusSmall
                            color: root.selectedProfile === index ?
                                   Theme.colorWithAlpha(profileColor(index), 0.3) :
                                   (parent.hovered ? Theme.colorWithAlpha(profileColor(index), 0.15) : "transparent")
                            border.color: profileColor(index)
                            border.width: root.selectedProfile === index ? 2 : 1
                        }

                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: root.selectedProfile === index
                            color: profileColor(index)
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: {
                            root.selectedProfile = index
                            // TODO: Load curves for this profile from storage
                        }
                    }
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

    // Content - Two graphs stacked
    ColumnLayout {
        anchors.top: headerBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.spacingMedium
        spacing: Theme.spacingSmall

        // CPU Fan section
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("CPU Fan")
                font.pixelSize: Theme.fontSizeMedium
                font.bold: true
                color: Theme.quietColor
            }
            Item { Layout.fillWidth: true }
        }

        FanCurveCanvas {
            id: cpuCanvas
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 250
            curveData: FanController.cpuCurve
            curveColor: root.profileColor(root.selectedProfile)
            label: "CPU"

            onCurveChanged: function(newCurve) {
                FanController.setCpuCurve(newCurve, FanController.cpuCurveEnabled)
            }
        }

        // GPU Fan section
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("GPU Fan")
                font.pixelSize: Theme.fontSizeMedium
                font.bold: true
                color: Theme.performanceColor
            }
            Item { Layout.fillWidth: true }
        }

        FanCurveCanvas {
            id: gpuCanvas
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 250
            curveData: FanController.gpuCurve
            curveColor: root.profileColor(root.selectedProfile)
            label: "GPU"

            onCurveChanged: function(newCurve) {
                FanController.setGpuCurve(newCurve, FanController.gpuCurveEnabled)
            }
        }

        // Bottom buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            Button {
                text: qsTr("Reset to Defaults")
                onClicked: {
                    FanController.resetToDefaults()
                    cpuCanvas.reloadFromExternal()
                    gpuCanvas.reloadFromExternal()
                }

                background: Rectangle {
                    implicitHeight: 32
                    radius: Theme.radiusSmall
                    color: parent.hovered ? Theme.colorWithAlpha(Theme.warning, 0.2) : "transparent"
                    border.color: Theme.warning
                }

                contentItem: Text {
                    text: parent.text
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.warning
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: qsTr("Profile: ") + profileName(root.selectedProfile)
                font.pixelSize: Theme.fontSizeSmall
                color: profileColor(root.selectedProfile)
            }
        }
    }

    // Fan curve canvas component
    component FanCurveCanvas: Canvas {
        id: canvas

        property var curveData: []
        property color curveColor: Theme.accent
        property int dragIndex: -1
        property string label: ""

        // Internal curve data - completely local
        property var internalCurve: []
        property bool initialized: false

        signal curveChanged(var newCurve)

        onCurveDataChanged: {
            if (!initialized || internalCurve.length === 0) {
                initFromExternal()
            }
        }

        Component.onCompleted: {
            initFromExternal()
        }

        function initFromExternal() {
            internalCurve = []
            if (curveData && curveData.length > 0) {
                for (var i = 0; i < curveData.length; i++) {
                    internalCurve.push({
                        temp: curveData[i].temp,
                        fan: curveData[i].fan
                    })
                }
                initialized = true
            }
            requestPaint()
        }

        function reloadFromExternal() {
            initialized = false
            initFromExternal()
        }

        MouseArea {
            id: dragArea
            anchors.fill: parent
            hoverEnabled: true
            preventStealing: true

            onPressed: function(mouse) {
                var idx = canvas.findNearestPoint(mouse.x, mouse.y)
                canvas.dragIndex = idx
            }

            onPositionChanged: function(mouse) {
                if (pressed && canvas.dragIndex >= 0 && canvas.internalCurve.length > 0) {
                    var coords = canvas.screenToData(mouse.x, mouse.y)
                    canvas.internalCurve[canvas.dragIndex].temp = coords.temp
                    canvas.internalCurve[canvas.dragIndex].fan = coords.fan
                    canvas.requestPaint()
                }
            }

            onReleased: function(mouse) {
                if (canvas.dragIndex >= 0 && canvas.internalCurve.length > 0) {
                    var curveCopy = []
                    for (var i = 0; i < canvas.internalCurve.length; i++) {
                        curveCopy.push({
                            temp: canvas.internalCurve[i].temp,
                            fan: canvas.internalCurve[i].fan
                        })
                    }
                    canvas.curveChanged(curveCopy)
                }
                canvas.dragIndex = -1
            }
        }

        function findNearestPoint(mx, my) {
            var padding = 35
            var w = width - padding * 2
            var h = height - padding * 2

            if (!internalCurve || internalCurve.length === 0) return -1

            for (var i = 0; i < internalCurve.length; i++) {
                var point = internalCurve[i]
                var x = padding + (point.temp - 30) / 70 * w
                var y = height - padding - (point.fan / 100) * h

                var dist = Math.sqrt(Math.pow(mx - x, 2) + Math.pow(my - y, 2))
                if (dist < 12) return i
            }
            return -1
        }

        function screenToData(mx, my) {
            var padding = 35
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

            var padding = 35
            var w = width - padding * 2
            var h = height - padding * 2

            // Background
            ctx.fillStyle = Theme.surface
            ctx.fillRect(padding, padding, w, h)

            // Grid
            ctx.strokeStyle = Theme.border
            ctx.lineWidth = 0.5

            // Horizontal grid lines (fan %)
            for (var i = 0; i <= 5; i++) {
                var y = padding + h * i / 5
                ctx.beginPath()
                ctx.moveTo(padding, y)
                ctx.lineTo(padding + w, y)
                ctx.stroke()

                ctx.fillStyle = Theme.textSecondary
                ctx.font = "9px sans-serif"
                ctx.textAlign = "right"
                ctx.fillText((100 - i * 20) + "%", padding - 4, y + 3)
            }

            // Vertical grid lines (temp)
            for (var j = 0; j <= 7; j++) {
                var x = padding + w * j / 7
                ctx.beginPath()
                ctx.moveTo(x, padding)
                ctx.lineTo(x, padding + h)
                ctx.stroke()

                ctx.fillStyle = Theme.textSecondary
                ctx.textAlign = "center"
                ctx.fillText((30 + j * 10) + "°", x, height - padding + 12)
            }

            // Use internalCurve for drawing
            if (!internalCurve || internalCurve.length === 0) return
            var drawCurve = internalCurve

            // Draw curve
            ctx.strokeStyle = curveColor
            ctx.lineWidth = 2
            ctx.beginPath()

            for (var k = 0; k < drawCurve.length; k++) {
                var point = drawCurve[k]
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
            for (var m = 0; m < drawCurve.length; m++) {
                var pt = drawCurve[m]
                var ptx = padding + (pt.temp - 30) / 70 * w
                var pty = padding + h - (pt.fan / 100) * h

                ctx.beginPath()
                ctx.arc(ptx, pty, dragIndex === m ? 7 : 5, 0, Math.PI * 2)
                ctx.fillStyle = dragIndex === m ? Theme.accentLight : curveColor
                ctx.fill()
                ctx.strokeStyle = Theme.textPrimary
                ctx.lineWidth = 1.5
                ctx.stroke()
            }
        }
    }
}

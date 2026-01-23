import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import GHelperLinux
import "theme"
import "components"
import "dialogs"

ApplicationWindow {
    id: window
    visible: true
    width: 420
    height: 700
    minimumWidth: 400
    minimumHeight: 600
    x: Screen.desktopAvailableWidth - width - 12
    y: Screen.desktopAvailableHeight - height - 52
    title: qsTr("G-Helper Linux")
    color: Theme.background

    function positionBottomRight() {
        x = Screen.desktopAvailableWidth - width - 12
        y = Screen.desktopAvailableHeight - height - 52
    }

    onClosing: function(close) {
        if (Settings.minimizeToTray && TrayManager.visible) {
            close.accepted = false
            window.hide()
        }
    }

    Connections {
        target: TrayManager
        function onShowWindowRequested() {
            positionBottomRight()
            window.show()
            window.raise()
            window.requestActivate()
        }
        function onQuitRequested() {
            Qt.quit()
        }
    }

    // Header
    header: ToolBar {
        height: 50
        background: Rectangle {
            color: Theme.background
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Theme.border
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Label {
                text: "G-Helper"
                font.pixelSize: 20
                font.bold: true
                color: Theme.textPrimary
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 10
                height: 10
                radius: 5
                color: DBusWatcher.allConnected ? Theme.success : Theme.error
            }

            IconButton {
                icon.source: "qrc:/icons/info.svg"
                onClicked: aboutDialog.open()
            }

            IconButton {
                icon.source: "qrc:/icons/settings.svg"
                onClicked: settingsPopup.open()
            }
        }
    }

    // Main content
    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 0

            // === Performance Mode Section ===
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: sectionPerf.implicitHeight + 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                ColumnLayout {
                    id: sectionPerf
                    anchors.fill: parent
                    spacing: 12

                    // Header with status
                    RowLayout {
                        Layout.fillWidth: true

                        Image {
                            source: "qrc:/icons/performance.svg"
                            sourceSize: Qt.size(20, 20)
                        }
                        Label {
                            text: "Mode: " + PerformanceController.currentProfileName
                            font.pixelSize: 15
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: "CPU: " + SystemMonitor.cpuTemp + "°C Fan: " + SystemMonitor.cpuFanRpm + "RPM"
                            font.pixelSize: 13
                            color: Theme.textSecondary
                        }
                    }

                    // Mode buttons
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: [
                                { name: "Silent", icon: "qrc:/icons/quiet.svg", profile: 0 },
                                { name: "Balanced", icon: "qrc:/icons/balanced.svg", profile: 1 },
                                { name: "Turbo", icon: "qrc:/icons/turbo.svg", profile: 2 },
                                { name: "Fans + Power", icon: "qrc:/icons/fan.svg", profile: -1 }
                            ]

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 70
                                color: Theme.buttonBackground
                                border.color: PerformanceController.currentProfile === modelData.profile ? Theme.accent : Theme.border
                                border.width: PerformanceController.currentProfile === modelData.profile ? 2 : 1
                                radius: 4

                                ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 4

                                    Image {
                                        Layout.alignment: Qt.AlignHCenter
                                        source: modelData.icon
                                        sourceSize: Qt.size(24, 24)
                                    }
                                    Label {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: modelData.name
                                        font.pixelSize: 12
                                        color: PerformanceController.currentProfile === modelData.profile ? Theme.accent : Theme.textPrimary
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (modelData.profile >= 0) {
                                            PerformanceController.setProfile(modelData.profile)
                                        } else {
                                            openFanCurveWindow()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // === GPU Mode Section ===
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: sectionGpu.implicitHeight + 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                ColumnLayout {
                    id: sectionGpu
                    anchors.fill: parent
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        Image {
                            source: "qrc:/icons/gpu.svg"
                            sourceSize: Qt.size(20, 20)
                        }
                        Label {
                            text: "GPU Mode: " + GpuController.currentModeName
                            font.pixelSize: 15
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: "GPU: " + SystemMonitor.gpuTemp + "°C Fan: " + SystemMonitor.gpuFanRpm + "RPM"
                            font.pixelSize: 13
                            color: Theme.textSecondary
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: [
                                { name: "Eco", icon: "qrc:/icons/eco.svg", mode: 0 },
                                { name: "Standard", icon: "qrc:/icons/hybrid.svg", mode: 1 },
                                { name: "Ultimate", icon: "qrc:/icons/dedicated.svg", mode: 2 },
                                { name: "Optimized", icon: "qrc:/icons/vfio.svg", mode: 3 }
                            ]

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 70
                                color: Theme.buttonBackground
                                border.color: GpuController.currentMode === modelData.mode ? Theme.accent : Theme.border
                                border.width: GpuController.currentMode === modelData.mode ? 2 : 1
                                radius: 4

                                ColumnLayout {
                                    anchors.centerIn: parent
                                    spacing: 4

                                    Image {
                                        Layout.alignment: Qt.AlignHCenter
                                        source: modelData.icon
                                        sourceSize: Qt.size(24, 24)
                                    }
                                    Label {
                                        Layout.alignment: Qt.AlignHCenter
                                        text: modelData.name
                                        font.pixelSize: 12
                                        color: GpuController.currentMode === modelData.mode ? Theme.accent : Theme.textPrimary
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: GpuController.setMode(modelData.mode)
                                }
                            }
                        }
                    }

                    Label {
                        text: "dGPU: " + GpuController.gpuPower + (GpuController.gpuPower === "Active" && SystemMonitor.dgpuUsage > 0 ? " (" + Math.round(SystemMonitor.dgpuUsage) + "%)" : "")
                        font.pixelSize: 12
                        color: GpuController.gpuPower === "Active" ? Theme.warning :
                               GpuController.gpuPower === "Off" ? Theme.success : Theme.textSecondary
                    }
                }
            }

            // === Keyboard Section ===
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: sectionKb.implicitHeight + 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                ColumnLayout {
                    id: sectionKb
                    anchors.fill: parent
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        Image {
                            source: "qrc:/icons/keyboard.svg"
                            sourceSize: Qt.size(20, 20)
                        }
                        Label {
                            text: "Laptop Keyboard"
                            font.pixelSize: 15
                            font.bold: true
                            color: Theme.textPrimary
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        ComboBox {
                            Layout.preferredWidth: 120
                            model: ["Static", "Breathe", "Rainbow", "Strobe"]
                            currentIndex: AuraController.currentMode
                            onCurrentIndexChanged: AuraController.setMode(currentIndex)

                            background: Rectangle {
                                color: Theme.buttonBackground
                                border.color: Theme.border
                                radius: 4
                            }
                            contentItem: Label {
                                text: parent.displayText
                                color: Theme.textPrimary
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: 8
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 36
                            color: Theme.buttonBackground
                            border.color: Theme.border
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 8

                                Label {
                                    text: "Color"
                                    color: Theme.textPrimary
                                }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    width: 24
                                    height: 24
                                    color: AuraController.color1
                                    border.color: Theme.border
                                    radius: 2

                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: colorDialog.open()
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 80
                            Layout.preferredHeight: 36
                            color: Theme.buttonBackground
                            border.color: Theme.border
                            radius: 4

                            Label {
                                anchors.centerIn: parent
                                text: "Extra"
                                color: Theme.textPrimary
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: keyboardExtraPopup.open()
                            }
                        }
                    }

                    // Brightness slider
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: "Brightness"
                            color: Theme.textSecondary
                            font.pixelSize: 12
                        }

                        Slider {
                            Layout.fillWidth: true
                            from: 0
                            to: 3
                            stepSize: 1
                            value: AuraController.brightness
                            onValueChanged: AuraController.setBrightness(value)

                            background: Rectangle {
                                x: parent.leftPadding
                                y: parent.topPadding + parent.availableHeight / 2 - height / 2
                                width: parent.availableWidth
                                height: 4
                                radius: 2
                                color: Theme.border

                                Rectangle {
                                    width: parent.parent.visualPosition * parent.width
                                    height: parent.height
                                    color: Theme.accent
                                    radius: 2
                                }
                            }

                            handle: Rectangle {
                                x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                                y: parent.topPadding + parent.availableHeight / 2 - height / 2
                                width: 16
                                height: 16
                                radius: 8
                                color: Theme.accent
                            }
                        }
                    }
                }
            }

            // === Slash Lightbar Section ===
            Item {
                visible: SlashController.available
                Layout.fillWidth: true
                Layout.preferredHeight: sectionSlash.implicitHeight + 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                ColumnLayout {
                    id: sectionSlash
                    anchors.fill: parent
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        Image {
                            source: "qrc:/icons/led-rainbow.svg"
                            sourceSize: Qt.size(20, 20)
                        }
                        Label {
                            text: "Slash Lightbar"
                            font.pixelSize: 15
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Item { Layout.fillWidth: true }
                        Switch {
                            checked: SlashController.enabled
                            onCheckedChanged: SlashController.setEnabled(checked)
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        ComboBox {
                            id: slashModeCombo
                            Layout.preferredWidth: 140
                            model: SlashController.availableModes
                            currentIndex: SlashController.availableModes.indexOf(SlashController.currentMode)
                            onActivated: SlashController.setMode(currentText)

                            background: Rectangle {
                                color: Theme.buttonBackground
                                border.color: Theme.border
                                radius: 4
                            }
                            contentItem: Label {
                                text: parent.displayText
                                color: Theme.textPrimary
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: 8
                            }
                        }

                        Label {
                            text: "Brightness"
                            color: Theme.textSecondary
                            font.pixelSize: 12
                        }

                        Slider {
                            Layout.fillWidth: true
                            from: 0
                            to: 255
                            stepSize: 1
                            value: SlashController.brightness
                            onPressedChanged: {
                                if (!pressed) {
                                    SlashController.setBrightness(Math.round(value))
                                }
                            }

                            background: Rectangle {
                                x: parent.leftPadding
                                y: parent.topPadding + parent.availableHeight / 2 - height / 2
                                width: parent.availableWidth
                                height: 4
                                radius: 2
                                color: Theme.border

                                Rectangle {
                                    width: parent.parent.visualPosition * parent.width
                                    height: parent.height
                                    color: Theme.accent
                                    radius: 2
                                }
                            }

                            handle: Rectangle {
                                x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                                y: parent.topPadding + parent.availableHeight / 2 - height / 2
                                width: 16
                                height: 16
                                radius: 8
                                color: Theme.accent
                            }
                        }
                    }
                }
            }

            // === Battery Section ===
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: sectionBat.implicitHeight + 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                ColumnLayout {
                    id: sectionBat
                    anchors.fill: parent
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true

                        Image {
                            source: "qrc:/icons/battery.svg"
                            sourceSize: Qt.size(20, 20)
                        }
                        Label {
                            text: "Battery"
                            font.pixelSize: 15
                            font.bold: true
                            color: Theme.textPrimary
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: {
                                var systemText = "System: " + SystemMonitor.systemPower.toFixed(1) + "W"
                                if (SystemMonitor.onBattery) {
                                    return systemText + " (Batterie)"
                                } else {
                                    return systemText + " (geschätzt)"
                                }
                            }
                            font.pixelSize: 13
                            color: Theme.textSecondary

                            ToolTip.visible: powerMouseArea.containsMouse
                            ToolTip.delay: 500
                            ToolTip.text: {
                                var details = "APU: " + SystemMonitor.apuPower.toFixed(1) + "W\n"
                                details += "Display (~" + SystemMonitor.displayBrightness + "%): " + SystemMonitor.displayPower.toFixed(1) + "W"
                                if (SystemMonitor.onBattery) {
                                    details += "\nBatterie-Entladung: " + SystemMonitor.batteryPower.toFixed(1) + "W"
                                } else {
                                    details += "\nSonstige (SSD/WiFi/RAM): ~5W"
                                }
                                return details
                            }

                            MouseArea {
                                id: powerMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }
                    }

                    // Battery bar
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Rectangle {
                            Layout.preferredWidth: 50
                            Layout.preferredHeight: 24
                            color: "transparent"
                            border.color: Theme.textSecondary
                            border.width: 2
                            radius: 4

                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.margins: 3
                                width: Math.max(4, (parent.width - 6) * BatteryController.currentCharge / 100)
                                color: BatteryController.currentCharge > 20 ? Theme.success : Theme.error
                                radius: 2
                            }

                            Rectangle {
                                anchors.right: parent.right
                                anchors.rightMargin: -4
                                anchors.verticalCenter: parent.verticalCenter
                                width: 4
                                height: 10
                                color: Theme.textSecondary
                                radius: 1
                            }
                        }

                        Label {
                            text: BatteryController.currentCharge + "%"
                            font.pixelSize: 16
                            font.bold: true
                            color: Theme.textPrimary
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: "Charge Limit"
                            font.pixelSize: 13
                            color: Theme.textSecondary
                        }

                        Label {
                            text: BatteryController.chargeLimit + "%"
                            font.pixelSize: 16
                            font.bold: true
                            color: Theme.accent
                        }
                    }

                    Slider {
                        Layout.fillWidth: true
                        from: 20
                        to: 100
                        stepSize: 5
                        value: BatteryController.chargeLimit
                        onPressedChanged: {
                            if (!pressed) {
                                BatteryController.setChargeLimit(value)
                            }
                        }

                        background: Rectangle {
                            x: parent.leftPadding
                            y: parent.topPadding + parent.availableHeight / 2 - height / 2
                            width: parent.availableWidth
                            height: 6
                            radius: 3
                            color: Theme.border

                            Rectangle {
                                width: parent.parent.visualPosition * parent.width
                                height: parent.height
                                color: Theme.accent
                                radius: 3
                            }
                        }

                        handle: Rectangle {
                            x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                            y: parent.topPadding + parent.availableHeight / 2 - height / 2
                            width: 18
                            height: 18
                            radius: 9
                            color: Theme.accent
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Repeater {
                            model: [60, 80, 100]

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 32
                                color: Theme.buttonBackground
                                border.color: BatteryController.chargeLimit === modelData ? Theme.accent : Theme.border
                                border.width: BatteryController.chargeLimit === modelData ? 2 : 1
                                radius: 4

                                Label {
                                    anchors.centerIn: parent
                                    text: modelData + "%"
                                    color: BatteryController.chargeLimit === modelData ? Theme.accent : Theme.textPrimary
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: BatteryController.setChargeLimit(modelData)
                                }
                            }
                        }
                    }

                    Label {
                        text: "Limiting charge extends battery lifespan. Recommended: 80% for daily use."
                        font.pixelSize: 11
                        color: Theme.textSecondary
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
            }
        }
    }

    // Dialogs
    AboutDialog {
        id: aboutDialog
        anchors.centerIn: parent
    }

    // Fan Curve Window (separate window)
    Loader {
        id: fanCurveLoader
        active: false
        sourceComponent: FanCurveDialog {}
    }

    function openFanCurveWindow() {
        if (!fanCurveLoader.active) {
            fanCurveLoader.active = true
        }
        if (fanCurveLoader.item) {
            fanCurveLoader.item.open(window.x, window.y)
        }
    }

    // Color picker dialog (simplified)
    Dialog {
        id: colorDialog
        title: "Select Color"
        anchors.centerIn: parent
        modal: true

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: 8
        }

        GridLayout {
            columns: 6
            rowSpacing: 8
            columnSpacing: 8

            Repeater {
                model: [
                    "#ff0000", "#ff8000", "#ffff00", "#80ff00", "#00ff00", "#00ff80",
                    "#00ffff", "#0080ff", "#0000ff", "#8000ff", "#ff00ff", "#ff0080",
                    "#ffffff", "#c0c0c0", "#808080", "#404040", "#000000", "#804000"
                ]

                delegate: Rectangle {
                    width: 32
                    height: 32
                    color: modelData
                    border.color: Theme.border
                    radius: 4

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            AuraController.setColor1(modelData)
                            colorDialog.close()
                        }
                    }
                }
            }
        }
    }

    // Settings popup
    Popup {
        id: settingsPopup
        x: parent.width - width - 16
        y: 60
        width: 250
        padding: 16

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: 8
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            Label {
                text: "Settings"
                font.pixelSize: 16
                font.bold: true
                color: Theme.textPrimary
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            CheckBox {
                text: "Start minimized"
                checked: Settings.startMinimized
                onCheckedChanged: Settings.startMinimized = checked
                contentItem: Label {
                    text: parent.text
                    color: Theme.textPrimary
                    leftPadding: parent.indicator.width + 8
                }
            }

            CheckBox {
                text: "Start with system"
                checked: Settings.autoStart
                onCheckedChanged: Settings.autoStart = checked
                contentItem: Label {
                    text: parent.text
                    color: Theme.textPrimary
                    leftPadding: parent.indicator.width + 8
                }
            }

            CheckBox {
                text: "Minimize to tray"
                checked: Settings.minimizeToTray
                onCheckedChanged: Settings.minimizeToTray = checked
                contentItem: Label {
                    text: parent.text
                    color: Theme.textPrimary
                    leftPadding: parent.indicator.width + 8
                }
            }
        }
    }

    Popup {
        id: keyboardExtraPopup
        anchors.centerIn: parent
        width: 200
        padding: 16

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: 8
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            Label {
                text: "Keyboard Settings"
                font.bold: true
                color: Theme.textPrimary
            }

            CheckBox {
                text: "Disable on battery"
                contentItem: Label {
                    text: parent.text
                    color: Theme.textPrimary
                    leftPadding: parent.indicator.width + 8
                }
            }

            CheckBox {
                text: "Disable on lid close"
                contentItem: Label {
                    text: parent.text
                    color: Theme.textPrimary
                    leftPadding: parent.indicator.width + 8
                }
            }
        }
    }
}

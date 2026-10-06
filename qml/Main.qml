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

    // Shown from C++ (unless started minimized), which also positions it
    visible: false
    title: deviceName !== "" ? "G-Helper — " + deviceName : "G-Helper"
    color: Theme.background

    // The window is exactly as tall as its content, like G-Helper on
    // Windows. Only on very small screens does the content scroll.
    readonly property int fittedHeight: Math.min(content.implicitHeight, Screen.desktopAvailableHeight - 40)
    width: 470
    height: fittedHeight
    minimumWidth: 470
    maximumWidth: 470
    minimumHeight: fittedHeight
    maximumHeight: fittedHeight

    onClosing: function(close) {
        if (Settings.minimizeToTray && TrayManager.visible) {
            close.accepted = false
            window.hide()
        } else {
            Qt.quit()
        }
    }

    function showWindow() {
        window.show()
        window.raise()
        window.requestActivate()
    }

    // Asks for confirmation if the switch needs a reboot
    function requestGpuMode(mode) {
        // Clicking the current mode while a switch is pending cancels it
        if (mode === GpuController.currentMode && !GpuController.switchPending)
            return
        var warning = GpuController.confirmationText(mode)
        if (warning !== "") {
            gpuConfirmDialog.targetMode = mode
            gpuConfirmDialog.message = warning
            showWindow()
            gpuConfirmDialog.open()
        } else {
            GpuController.setMode(mode)
        }
    }

    function openFanCurveWindow() {
        if (!fanCurveLoader.active)
            fanCurveLoader.active = true
        if (fanCurveLoader.item)
            fanCurveLoader.item.open(window.x, window.y)
    }

    function profileColor(profile) {
        switch (profile) {
            case 0: return Theme.colorEco
            case 1: return Theme.colorStandard
            case 2: return Theme.colorTurbo
            default: return Theme.colorCustom
        }
    }

    function gpuColor(mode) {
        switch (mode) {
            case 0: return Theme.colorEco
            case 1: return Theme.colorStandard
            case 2: return Theme.colorTurbo
            default: return Theme.colorEco
        }
    }

    // GPU mode as G-Helper describes it in the section header
    function gpuModeText() {
        if (!GpuController.available)
            return qsTr("unavailable")
        switch (GpuController.currentMode) {
            case 0: return qsTr("iGPU only")
            case 1: return qsTr("iGPU + dGPU")
            case 2: return qsTr("dGPU exclusive")
            case 3: return GpuController.currentModeName
            default: return GpuController.currentModeName
        }
    }

    Connections {
        target: TrayManager
        function onShowWindowRequested() { showWindow() }
        function onQuitRequested() { Qt.quit() }
        function onGpuModeRequested(mode) { requestGpuMode(mode) }
    }

    Connections {
        target: GpuController
        // Switches triggered from the tray while the window is hidden
        function onSwitchPendingChanged(pending) {
            if (pending && !window.visible)
                TrayManager.showMessage(qsTr("GPU mode"), GpuController.pendingText)
        }
        function onUserActionRequired(message) {
            gpuActionDialog.message = message
            if (window.visible)
                gpuActionDialog.open()
            else
                TrayManager.showMessage(qsTr("GPU mode"), message)
        }
    }

    Connections {
        target: Notifications
        function onError(message) { errorToast.show(message) }
    }

    // ------------------------------------------------------------------
    // Reusable pieces
    // ------------------------------------------------------------------

    // Section title with icon on the left and live values on the right
    component SectionHeader: RowLayout {
        property url iconSource
        property string title
        property string info
        Layout.fillWidth: true
        spacing: 7

        Image {
            source: parent.iconSource
            sourceSize: Qt.size(20, 20)
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20
        }
        Label {
            text: parent.title
            font.pixelSize: 15
            font.bold: true
            color: Theme.textPrimary
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
        Label {
            text: parent.info
            font.pixelSize: 13
            color: Theme.textPrimary
            visible: text !== ""
        }
    }

    // Big G-Helper style button: icon above label, coloured border when selected
    component ModeTile: Rectangle {
        id: tile
        property string label
        property url iconSource
        property bool selected: false
        property bool pending: false      // queued for the next restart
        property color selectedColor: Theme.accent
        property bool tileEnabled: true
        property string tooltip: ""
        signal clicked()

        Layout.fillWidth: true
        Layout.preferredWidth: 1
        Layout.preferredHeight: 64
        radius: 5
        color: tileMouse.containsMouse && tileEnabled ? Theme.buttonHover : Theme.buttonBackground
        border.width: selected || pending ? 2 : 0
        border.color: selected ? selectedColor : Theme.colorCustom
        opacity: tileEnabled ? 1.0 : 0.4

        Column {
            anchors.centerIn: parent
            spacing: 5

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: tile.iconSource
                sourceSize: Qt.size(22, 22)
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: tile.label
                font.pixelSize: 13
                color: Theme.textPrimary
            }
        }

        MouseArea {
            id: tileMouse
            anchors.fill: parent
            hoverEnabled: true
            enabled: tile.tileEnabled
            cursorShape: Qt.PointingHandCursor
            onClicked: tile.clicked()
            ToolTip.visible: containsMouse && tile.tooltip !== ""
            ToolTip.delay: 700
            ToolTip.text: tile.tooltip
        }
    }

    // Flat dark button like G-Helper's "Color" / "Extra" / "Quit"
    component FlatButton: Rectangle {
        id: flat
        property string text
        property url iconSource: ""
        property color swatch: "transparent"
        property bool showSwatch: false
        signal clicked()

        Layout.preferredHeight: 32
        radius: 3
        color: flatMouse.containsMouse && enabled ? Theme.buttonHover : Theme.buttonBackground
        opacity: enabled ? 1.0 : 0.4

        RowLayout {
            anchors.centerIn: parent
            spacing: 8
            Image {
                visible: flat.iconSource.toString() !== ""
                source: flat.iconSource
                sourceSize: Qt.size(16, 16)
            }
            Label {
                text: flat.text
                font.pixelSize: 13
                color: Theme.textPrimary
            }
            Rectangle {
                visible: flat.showSwatch
                width: 16
                height: 16
                color: flat.swatch
                border.color: "#808080"
            }
        }

        MouseArea {
            id: flatMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: flat.clicked()
        }
    }

    // G-Helper style dropdown
    component DarkCombo: ComboBox {
        id: combo
        Layout.preferredHeight: 32
        font.pixelSize: 13

        background: Rectangle {
            radius: 3
            color: combo.hovered ? Theme.buttonHover : Theme.controlBackground
        }
        contentItem: Label {
            text: combo.displayText
            color: Theme.textPrimary
            font: combo.font
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8
            elide: Text.ElideRight
        }
        indicator: Label {
            x: combo.width - width - 8
            anchors.verticalCenter: parent.verticalCenter
            text: "▾"
            color: Theme.textSecondary
            font.pixelSize: 12
        }
    }

    // Slider with G-Helper's blue track
    component BlueSlider: Slider {
        id: slider
        Layout.fillWidth: true
        Layout.preferredHeight: 24

        background: Rectangle {
            x: slider.leftPadding
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: slider.availableWidth
            height: 4
            radius: 2
            color: "#5a5a5a"

            Rectangle {
                width: slider.visualPosition * parent.width
                height: parent.height
                radius: 2
                color: Theme.accent
            }
        }
        handle: Rectangle {
            x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
            y: slider.topPadding + slider.availableHeight / 2 - height / 2
            width: 18
            height: 18
            radius: 9
            color: slider.pressed ? Theme.accentLight : Theme.accent
            border.color: Theme.background
            border.width: 2
        }
    }

    // Windows 11 style toggle switch
    component DarkSwitch: Switch {
        id: sw
        padding: 0
        spacing: 0
        implicitWidth: 40
        Layout.preferredWidth: 40
        Layout.preferredHeight: 26
        indicator: Rectangle {
            implicitWidth: 40
            implicitHeight: 20
            x: sw.leftPadding
            y: parent.height / 2 - height / 2
            radius: 10
            color: sw.checked ? Theme.accent : "transparent"
            border.color: sw.checked ? Theme.accent : Theme.textSecondary
            border.width: 1

            Rectangle {
                width: sw.checked ? 14 : 12
                height: width
                radius: width / 2
                x: sw.checked ? parent.width - width - 3 : 4
                anchors.verticalCenter: parent.verticalCenter
                color: sw.checked ? "#000000" : Theme.textSecondary
                Behavior on x { NumberAnimation { duration: 120 } }
            }
        }
        contentItem: Item {}
    }

    // Windows style checkbox
    component DarkCheckBox: CheckBox {
        id: cb
        font.pixelSize: 13
        indicator: Rectangle {
            implicitWidth: 18
            implicitHeight: 18
            x: cb.leftPadding
            y: parent.height / 2 - height / 2
            radius: 3
            color: cb.checked ? Theme.accent : "transparent"
            border.color: cb.checked ? Theme.accent : Theme.textSecondary
            border.width: 1

            Label {
                anchors.centerIn: parent
                visible: cb.checked
                text: "✓"
                font.pixelSize: 13
                font.bold: true
                color: "#000000"
            }
        }
        contentItem: Label {
            text: cb.text
            font: cb.font
            color: Theme.textPrimary
            leftPadding: cb.indicator.width + 8
            verticalAlignment: Text.AlignVCenter
        }
    }

    // ------------------------------------------------------------------
    // Content
    // ------------------------------------------------------------------

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: content.implicitHeight
        interactive: contentHeight > height
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: content
            width: parent.width
            spacing: 0

            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 18
                Layout.bottomMargin: 10
                spacing: 9

                // === Performance mode ===
                SectionHeader {
                    iconSource: "qrc:/icons/section-mode.svg"
                    title: qsTr("Mode: %1").arg(PerformanceController.currentProfileName)
                    info: qsTr("CPU: %1°C Fan: %2RPM").arg(SystemMonitor.cpuTemp).arg(SystemMonitor.cpuFanRpm)
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 9

                    ModeTile {
                        label: qsTr("Silent")
                        iconSource: "qrc:/icons/mode-silent.svg"
                        selected: PerformanceController.currentProfile === 0
                        selectedColor: Theme.colorEco
                        tileEnabled: PerformanceController.available
                        tooltip: PerformanceController.profileDescription(0)
                        onClicked: PerformanceController.setProfile(0)
                    }
                    ModeTile {
                        label: qsTr("Balanced")
                        iconSource: "qrc:/icons/mode-balanced.svg"
                        selected: PerformanceController.currentProfile === 1
                        selectedColor: Theme.colorStandard
                        tileEnabled: PerformanceController.available
                        tooltip: PerformanceController.profileDescription(1)
                        onClicked: PerformanceController.setProfile(1)
                    }
                    ModeTile {
                        label: qsTr("Turbo")
                        iconSource: "qrc:/icons/mode-turbo.svg"
                        selected: PerformanceController.currentProfile === 2
                        selectedColor: Theme.colorTurbo
                        tileEnabled: PerformanceController.available
                        tooltip: PerformanceController.profileDescription(2)
                        onClicked: PerformanceController.setProfile(2)
                    }
                    ModeTile {
                        label: qsTr("Fans + Power")
                        iconSource: "qrc:/icons/mode-fans.svg"
                        // Orange like G-Helper's custom mode while custom curves are active
                        selected: FanController.curvesEnabled
                        selectedColor: Theme.colorCustom
                        tooltip: FanController.curvesEnabled ? qsTr("Custom fan curves active for this mode")
                                                             : qsTr("Edit fan curves")
                        onClicked: openFanCurveWindow()
                    }
                }

                Item { Layout.preferredHeight: 6 }

                // === GPU mode ===
                SectionHeader {
                    iconSource: "qrc:/icons/gpu.svg"
                    title: qsTr("GPU Mode: %1").arg(gpuModeText())
                    // dGPU temperature while it is awake, otherwise the iGPU
                    info: qsTr("GPU: %1°C Fan: %2RPM")
                          .arg(SystemMonitor.dgpuTemp > 0 ? SystemMonitor.dgpuTemp : SystemMonitor.gpuTemp)
                          .arg(SystemMonitor.gpuFanRpm)
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 9

                    Repeater {
                        model: [
                            { name: qsTr("Eco"), icon: "qrc:/icons/gpu-eco.svg", mode: 0 },
                            { name: qsTr("Standard"), icon: "qrc:/icons/gpu-standard.svg", mode: 1 },
                            { name: qsTr("Ultimate"), icon: "qrc:/icons/gpu-ultimate.svg", mode: 2 },
                            { name: qsTr("Optimized"), icon: "qrc:/icons/gpu-optimized.svg", mode: 3 }
                        ]

                        delegate: ModeTile {
                            required property var modelData
                            label: modelData.name
                            iconSource: modelData.icon
                            selected: GpuController.currentMode === modelData.mode
                            pending: GpuController.switchPending && GpuController.pendingMode === modelData.mode
                            selectedColor: gpuColor(modelData.mode)
                            tileEnabled: GpuController.available
                                         && GpuController.supportedModes.indexOf(modelData.mode) >= 0
                            tooltip: GpuController.modeDescription(modelData.mode)
                            onClicked: requestGpuMode(modelData.mode)
                        }
                    }
                }

                // One status line: pending switch, missing backend or dGPU state
                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        color: GpuController.switchPending ? Theme.colorCustom : Theme.textSecondary
                        text: {
                            if (GpuController.switchPending)
                                return qsTr("Pending: %1").arg(GpuController.pendingText)
                            if (!GpuController.available)
                                return qsTr("GPU switching needs asusd (asus-armoury) or supergfxd")
                            var state = GpuController.gpuPower
                            if (state === "") {
                                // asusd backend: use the kernel's runtime PM state
                                state = SystemMonitor.dgpuState === "active" ? "Active"
                                      : SystemMonitor.dgpuState === "suspended" ? "Suspended"
                                      : SystemMonitor.dgpuState !== "" ? SystemMonitor.dgpuState
                                      : "Off"
                            }
                            var line = "dGPU: " + state
                            if (SystemMonitor.dgpuUsage > 0)
                                line += " · " + Math.round(SystemMonitor.dgpuUsage) + "%"
                            return line
                        }
                    }

                    FlatButton {
                        visible: GpuController.rebootRequired
                        Layout.preferredHeight: 24
                        Layout.preferredWidth: 130
                        text: qsTr("Restart now")
                        onClicked: GpuController.rebootNow()
                    }
                }

                Item { Layout.preferredHeight: 2 }

                // === Keyboard ===
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 7

                    SectionHeader {
                        iconSource: "qrc:/icons/keyboard.svg"
                        title: qsTr("Laptop Keyboard")
                    }
                    DarkSwitch {
                        enabled: AuraController.available
                        checked: AuraController.lightOn
                        onToggled: AuraController.setLightOn(checked)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 9
                    enabled: AuraController.available && AuraController.lightOn
                    opacity: enabled ? 1.0 : 0.45

                    DarkCombo {
                        id: auraModeCombo
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        model: AuraController.availableModes
                        textRole: "name"
                        currentIndex: {
                            var modes = AuraController.availableModes
                            for (var i = 0; i < modes.length; i++) {
                                if (modes[i].mode === AuraController.currentMode)
                                    return i
                            }
                            return -1
                        }
                        // onActivated only fires on user interaction
                        onActivated: function(index) {
                            AuraController.setMode(AuraController.availableModes[index].mode)
                        }
                    }

                    FlatButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        text: qsTr("Color")
                        showSwatch: true
                        swatch: AuraController.color1
                        enabled: AuraController.modeUsesColor(AuraController.currentMode)
                        onClicked: {
                            colorDialog.target = 0
                            colorDialog.open()
                        }
                    }

                    FlatButton {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        text: qsTr("Extra")
                        iconSource: "qrc:/icons/settings.svg"
                        onClicked: keyboardExtraPopup.open()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    enabled: AuraController.available && AuraController.lightOn
                    opacity: enabled ? 1.0 : 0.45

                    Label {
                        text: qsTr("Brightness")
                        font.pixelSize: 13
                        color: Theme.textSecondary
                    }
                    // Low..High; "off" is the switch in the header
                    BlueSlider {
                        from: 1
                        to: 3
                        stepSize: 1
                        snapMode: Slider.SnapAlways
                        value: Math.max(1, AuraController.brightness)
                        // onMoved only fires on user interaction, so the
                        // hardware isn't written when the value is loaded
                        onMoved: AuraController.setBrightness(Math.round(value))
                    }
                    Label {
                        Layout.preferredWidth: 52
                        horizontalAlignment: Text.AlignRight
                        font.pixelSize: 13
                        color: Theme.textPrimary
                        text: [qsTr("Off"), qsTr("Low"), qsTr("Medium"), qsTr("High")][AuraController.brightness] || ""
                    }
                }

                // === Slash lightbar (only on models that have one) ===
                Item {
                    visible: SlashController.available
                    Layout.preferredHeight: 2
                }

                RowLayout {
                    visible: SlashController.available
                    Layout.fillWidth: true
                    spacing: 7

                    SectionHeader {
                        iconSource: "qrc:/icons/section-slash.svg"
                        title: qsTr("Slash Lightbar")
                    }
                    DarkSwitch {
                        checked: SlashController.enabled
                        onToggled: SlashController.setEnabled(checked)
                    }
                }

                RowLayout {
                    visible: SlashController.available
                    Layout.fillWidth: true
                    spacing: 10
                    enabled: SlashController.enabled
                    opacity: enabled ? 1.0 : 0.45

                    DarkCombo {
                        Layout.preferredWidth: 140
                        model: SlashController.availableModes
                        currentIndex: SlashController.availableModes.indexOf(SlashController.currentMode)
                        onActivated: SlashController.setMode(currentText)
                    }
                    BlueSlider {
                        from: 0
                        to: 255
                        stepSize: 1
                        value: SlashController.brightness
                        onPressedChanged: {
                            if (!pressed)
                                SlashController.setBrightness(Math.round(value))
                        }
                    }
                }

                Item { Layout.preferredHeight: 2 }

                // === Battery ===
                SectionHeader {
                    iconSource: BatteryController.isCharging ? "qrc:/icons/charging.svg" : "qrc:/icons/battery.svg"
                    title: qsTr("Battery Charge Limit: %1%").arg(chargeSlider.pressed ? Math.round(chargeSlider.value)
                                                                                       : BatteryController.chargeLimit)
                    info: {
                        var text = BatteryController.currentCharge + "%"
                        if (BatteryController.isCharging)
                            text += " · " + qsTr("Charging: %1W").arg(BatteryController.powerDraw.toFixed(1))
                        else if (SystemMonitor.onBattery)
                            text += " · " + qsTr("Discharging: %1W").arg(SystemMonitor.batteryPower.toFixed(1))
                        else
                            text += " · " + qsTr("Plugged in")
                        return text
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    enabled: BatteryController.available

                    BlueSlider {
                        id: chargeSlider
                        from: 20
                        to: 100
                        stepSize: 5
                        snapMode: Slider.SnapAlways
                        value: BatteryController.chargeLimit
                        onPressedChanged: {
                            if (!pressed)
                                BatteryController.setChargeLimit(Math.round(value))
                        }
                    }
                    Rectangle {
                        Layout.preferredWidth: 52
                        Layout.preferredHeight: 26
                        radius: 3
                        color: Theme.controlBackground
                        Label {
                            anchors.centerIn: parent
                            text: Math.round(chargeSlider.value) + "%"
                            font.pixelSize: 13
                            font.bold: true
                            color: Theme.textSecondary
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: BatteryController.timeRemaining !== ""
                    text: BatteryController.timeRemaining
                          + (SystemMonitor.systemPower > 0 && !SystemMonitor.onBattery
                             ? " · " + qsTr("System ~%1W").arg(SystemMonitor.systemPower.toFixed(0)) : "")
                    font.pixelSize: 12
                    color: Theme.textSecondary
                }
            }

            // === Footer like G-Helper: startup checkbox, version, settings, quit ===
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                color: "#1a1a1a"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 18
                    anchors.rightMargin: 18
                    spacing: 9

                    DarkCheckBox {
                        text: qsTr("Run on Startup")
                        checked: Settings.autoStart
                        onToggled: Settings.autoStart = checked
                    }

                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: "v" + appVersion
                        font.pixelSize: 12
                        color: versionMouse.containsMouse ? Theme.accentLight : Theme.textSecondary
                        font.underline: versionMouse.containsMouse
                        MouseArea {
                            id: versionMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: aboutDialog.open()
                        }
                    }

                    // Connection indicator: only asusd matters for most features
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: DBusWatcher.asusdConnected ? Theme.colorEco : Theme.error
                        ToolTip.visible: dotMouse.containsMouse
                        ToolTip.text: DBusWatcher.asusdConnected ? qsTr("asusd connected")
                                                                 : qsTr("asusd is not running")
                        MouseArea {
                            id: dotMouse
                            anchors.fill: parent
                            anchors.margins: -6
                            hoverEnabled: true
                        }
                    }

                    FlatButton {
                        id: settingsButton
                        Layout.preferredWidth: 36
                        iconSource: "qrc:/icons/settings.svg"
                        onClicked: settingsPopup.open()
                    }

                    FlatButton {
                        Layout.preferredWidth: 70
                        text: qsTr("Quit")
                        onClicked: Qt.quit()
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------
    // Dialogs & popups
    // ------------------------------------------------------------------

    AboutDialog {
        id: aboutDialog
        anchors.centerIn: parent
    }

    // Fan curve editor (separate window)
    Loader {
        id: fanCurveLoader
        active: false
        sourceComponent: FanCurveDialog {}
    }

    // Colour picker
    Dialog {
        id: colorDialog
        property int target: 0   // 0 = colour 1, 1 = colour 2
        title: target === 0 ? qsTr("Select Color") : qsTr("Select Second Color")
        anchors.centerIn: parent
        modal: true

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: 6
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
                    required property string modelData
                    width: 32
                    height: 32
                    color: modelData
                    border.color: Theme.border
                    radius: 4

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (colorDialog.target === 0)
                                AuraController.setColor1(parent.modelData)
                            else
                                AuraController.setColor2(parent.modelData)
                            colorDialog.close()
                        }
                    }
                }
            }
        }
    }

    // Keyboard extras: speed and second colour
    Popup {
        id: keyboardExtraPopup
        anchors.centerIn: parent
        width: 260
        padding: 16
        modal: true

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: 6
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            Label {
                text: qsTr("Keyboard Settings")
                font.bold: true
                font.pixelSize: 14
                color: Theme.textPrimary
            }

            Label {
                text: qsTr("Effect speed")
                color: Theme.textSecondary
                font.pixelSize: 12
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                enabled: AuraController.available && AuraController.modeUsesSpeed(AuraController.currentMode)
                opacity: enabled ? 1.0 : 0.4

                Repeater {
                    model: [qsTr("Low"), qsTr("Medium"), qsTr("High")]
                    delegate: ModeTile {
                        required property string modelData
                        required property int index
                        Layout.preferredHeight: 30
                        label: modelData
                        selected: AuraController.speed === index
                        onClicked: AuraController.setSpeed(index)
                    }
                }
            }

            FlatButton {
                Layout.fillWidth: true
                visible: AuraController.modeUsesTwoColors(AuraController.currentMode)
                text: qsTr("Second color")
                showSwatch: true
                swatch: AuraController.color2
                onClicked: {
                    keyboardExtraPopup.close()
                    colorDialog.target = 1
                    colorDialog.open()
                }
            }
        }
    }

    // App settings
    Popup {
        id: settingsPopup
        x: parent.width - width - 18
        y: parent.height - height - 60
        width: 250
        padding: 16

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: 6
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 6

            Label {
                text: qsTr("Settings")
                font.pixelSize: 15
                font.bold: true
                color: Theme.textPrimary
            }

            Repeater {
                model: [
                    { text: qsTr("Start minimized"), key: "startMinimized" },
                    { text: qsTr("Minimize to tray"), key: "minimizeToTray" },
                    { text: qsTr("Show tray icon"), key: "showTrayIcon" }
                ]
                delegate: DarkCheckBox {
                    required property var modelData
                    text: modelData.text
                    checked: Settings[modelData.key]
                    onToggled: Settings[modelData.key] = checked
                }
            }
        }
    }

    // Confirmation before switches that need a reboot (GPU MUX)
    Dialog {
        id: gpuConfirmDialog
        property int targetMode: -1
        property string message: ""
        title: qsTr("Switch GPU mode?")
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: GpuController.setMode(targetMode)

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: 6
        }

        Label {
            width: 300
            text: gpuConfirmDialog.message
            wrapMode: Text.WordWrap
            color: Theme.textPrimary
        }
    }

    // Shown when a GPU switch needs a logout/reboot to finish
    Dialog {
        id: gpuActionDialog
        property string message: ""
        title: qsTr("GPU mode")
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok

        background: Rectangle {
            color: Theme.surface
            border.color: Theme.border
            radius: 6
        }

        Label {
            width: 300
            text: gpuActionDialog.message
            wrapMode: Text.WordWrap
            color: Theme.textPrimary
        }
    }

    // Error messages from all controllers
    Rectangle {
        id: errorToast
        property string message: ""
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: 62
        width: Math.min(parent.width - 32, toastLabel.implicitWidth + 32)
        height: toastLabel.implicitHeight + 20
        radius: 6
        color: Theme.error
        visible: opacity > 0
        opacity: 0
        z: 100

        function show(text) {
            message = text
            opacity = 1
            toastTimer.restart()
        }

        Behavior on opacity { NumberAnimation { duration: 200 } }

        Label {
            id: toastLabel
            anchors.centerIn: parent
            width: Math.min(implicitWidth, window.width - 64)
            text: errorToast.message
            wrapMode: Text.WordWrap
            color: "white"
        }

        Timer {
            id: toastTimer
            interval: 5000
            onTriggered: errorToast.opacity = 0
        }

        MouseArea {
            anchors.fill: parent
            onClicked: errorToast.opacity = 0
        }
    }
}

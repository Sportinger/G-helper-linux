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
            title: qsTr("Battery")
            subtitle: BatteryController.isPluggedIn ?
                      (BatteryController.isCharging ? qsTr("Charging") : qsTr("Plugged in")) :
                      qsTr("On battery")
            icon: "qrc:/icons/battery.svg"
        }

        // Battery status row
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLarge

            // Battery level indicator
            Rectangle {
                width: 60
                height: 28
                radius: 4
                color: "transparent"
                border.color: Theme.textSecondary
                border.width: 2

                // Battery tip
                Rectangle {
                    anchors.left: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 4
                    height: 12
                    color: Theme.textSecondary
                }

                // Battery fill
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.margins: 3
                    width: (parent.width - 6) * (BatteryController.currentCharge / 100)
                    radius: 2
                    color: BatteryController.currentCharge > 20 ? Theme.success :
                           BatteryController.currentCharge > 10 ? Theme.warning : Theme.error

                    Behavior on width {
                        NumberAnimation { duration: Theme.animationMedium }
                    }
                }

                // Charging indicator
                Image {
                    anchors.centerIn: parent
                    width: 16
                    height: 16
                    source: "qrc:/icons/charging.svg"
                    visible: BatteryController.isCharging
                }
            }

            ColumnLayout {
                spacing: 2

                Text {
                    text: BatteryController.currentCharge + "%"
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    color: Theme.textPrimary
                }

                Text {
                    text: BatteryController.timeRemaining
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                    visible: BatteryController.timeRemaining !== ""
                }
            }

            Item { Layout.fillWidth: true }

            // Power draw
            ColumnLayout {
                spacing: 2
                visible: BatteryController.powerDraw > 0

                Text {
                    text: qsTr("Power")
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                }

                Text {
                    text: BatteryController.powerDraw.toFixed(1) + " W"
                    font.pixelSize: Theme.fontSizeMedium
                    font.bold: true
                    color: Theme.textPrimary
                }
            }
        }

        // Charge limit slider
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: qsTr("Charge Limit")
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.textPrimary
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: chargeLimitSlider.value + "%"
                    font.pixelSize: Theme.fontSizeMedium
                    font.bold: true
                    color: Theme.accent
                }
            }

            Slider {
                id: chargeLimitSlider
                Layout.fillWidth: true
                from: 20
                to: 100
                stepSize: 5
                value: BatteryController.chargeLimit
                enabled: BatteryController.available

                onPressedChanged: {
                    if (!pressed) {
                        BatteryController.setChargeLimit(value)
                    }
                }

                background: Rectangle {
                    x: chargeLimitSlider.leftPadding
                    y: chargeLimitSlider.topPadding + chargeLimitSlider.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 6
                    width: chargeLimitSlider.availableWidth
                    height: implicitHeight
                    radius: 3
                    color: Theme.surface

                    // Filled portion
                    Rectangle {
                        width: chargeLimitSlider.visualPosition * parent.width
                        height: parent.height
                        radius: 3
                        color: Theme.accent
                    }

                    // Tick marks
                    Row {
                        anchors.fill: parent
                        spacing: (parent.width - 5) / 4

                        Repeater {
                            model: 5

                            Rectangle {
                                width: 1
                                height: parent.height + 4
                                y: -2
                                color: Theme.border
                            }
                        }
                    }
                }

                handle: Rectangle {
                    x: chargeLimitSlider.leftPadding + chargeLimitSlider.visualPosition *
                       (chargeLimitSlider.availableWidth - width)
                    y: chargeLimitSlider.topPadding + chargeLimitSlider.availableHeight / 2 - height / 2
                    implicitWidth: 18
                    implicitHeight: 18
                    radius: 9
                    color: chargeLimitSlider.pressed ? Theme.accentLight : Theme.accent
                    border.color: Theme.accentDark
                    border.width: 2

                    Behavior on color {
                        ColorAnimation { duration: Theme.animationFast }
                    }
                }
            }

            // Preset buttons
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSmall

                Repeater {
                    model: [
                        { value: 60, label: "60%" },
                        { value: 80, label: "80%" },
                        { value: 100, label: "100%" }
                    ]

                    Button {
                        Layout.fillWidth: true
                        text: modelData.label
                        highlighted: BatteryController.chargeLimit === modelData.value
                        enabled: BatteryController.available

                        onClicked: BatteryController.setChargeLimit(modelData.value)

                        background: Rectangle {
                            radius: Theme.radiusSmall
                            color: parent.highlighted ? Theme.colorWithAlpha(Theme.accent, 0.3) :
                                   parent.hovered ? Theme.surfaceLight : "transparent"
                            border.color: parent.highlighted ? Theme.accent : Theme.border
                        }

                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: Theme.fontSizeSmall
                            color: parent.highlighted ? Theme.accent : Theme.textSecondary
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Limiting charge extends battery lifespan. Recommended: 80% for daily use.")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
                wrapMode: Text.WordWrap
            }
        }

        // Not available message
        Text {
            Layout.fillWidth: true
            visible: !BatteryController.available
            text: qsTr("Battery control not available. Is asusd running?")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.warning
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }
}

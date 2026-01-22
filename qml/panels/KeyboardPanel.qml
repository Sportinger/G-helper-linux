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
            title: qsTr("Keyboard Backlight")
            subtitle: AuraController.modeName(AuraController.currentMode)
            icon: "qrc:/icons/keyboard.svg"
        }

        // Brightness slider
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMedium

            Text {
                text: qsTr("Brightness")
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textPrimary
            }

            Slider {
                id: brightnessSlider
                Layout.fillWidth: true
                from: 0
                to: 3
                stepSize: 1
                value: AuraController.brightness
                enabled: AuraController.available

                onMoved: AuraController.setBrightness(value)

                background: Rectangle {
                    x: brightnessSlider.leftPadding
                    y: brightnessSlider.topPadding + brightnessSlider.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 6
                    width: brightnessSlider.availableWidth
                    height: implicitHeight
                    radius: 3
                    color: Theme.surfaceLight

                    Rectangle {
                        width: brightnessSlider.visualPosition * parent.width
                        height: parent.height
                        radius: 3
                        color: Theme.accent
                    }
                }

                handle: Rectangle {
                    x: brightnessSlider.leftPadding + brightnessSlider.visualPosition *
                       (brightnessSlider.availableWidth - width)
                    y: brightnessSlider.topPadding + brightnessSlider.availableHeight / 2 - height / 2
                    implicitWidth: 18
                    implicitHeight: 18
                    radius: 9
                    color: brightnessSlider.pressed ? Theme.accentLight : Theme.accent
                    border.color: Theme.accentDark
                    border.width: 2
                }
            }

            Text {
                text: ["Off", "Low", "Med", "High"][brightnessSlider.value]
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.accent
                Layout.preferredWidth: 35
            }
        }

        // Effect mode selector
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall

            Text {
                text: qsTr("Effect")
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textPrimary
            }

            ComboBox {
                id: modeCombo
                Layout.fillWidth: true
                model: AuraController.availableModes
                textRole: "name"
                currentIndex: {
                    for (var i = 0; i < model.length; i++) {
                        if (model[i].mode === AuraController.currentMode)
                            return i
                    }
                    return 0
                }
                enabled: AuraController.available

                onActivated: {
                    AuraController.setMode(model[currentIndex].mode)
                }

                background: Rectangle {
                    color: modeCombo.pressed ? Theme.surfaceLight : Theme.surface
                    border.color: modeCombo.hovered ? Theme.borderHover : Theme.border
                    radius: Theme.radiusSmall
                }

                contentItem: Text {
                    leftPadding: Theme.spacingSmall
                    rightPadding: modeCombo.indicator.width + Theme.spacingSmall
                    text: modeCombo.displayText
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.textPrimary
                    verticalAlignment: Text.AlignVCenter
                }

                popup: Popup {
                    y: modeCombo.height
                    width: modeCombo.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 1

                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: modeCombo.delegateModel
                        currentIndex: modeCombo.highlightedIndex
                    }

                    background: Rectangle {
                        color: Theme.surface
                        border.color: Theme.border
                        radius: Theme.radiusSmall
                    }
                }

                delegate: ItemDelegate {
                    width: modeCombo.width
                    height: Theme.buttonHeight

                    contentItem: Text {
                        text: modelData.name
                        font.pixelSize: Theme.fontSizeMedium
                        color: highlighted ? Theme.accent : Theme.textPrimary
                        verticalAlignment: Text.AlignVCenter
                    }

                    highlighted: modeCombo.highlightedIndex === index

                    background: Rectangle {
                        color: highlighted ? Theme.colorWithAlpha(Theme.accent, 0.2) : "transparent"
                    }
                }
            }
        }

        // Color picker (visible only for modes that use color)
        ColumnLayout {
            Layout.fillWidth: true
            visible: AuraController.modeUsesColor(AuraController.currentMode)
            spacing: Theme.spacingSmall

            ColorPicker {
                label: qsTr("Color 1")
                selectedColor: AuraController.color1
                onColorSelected: function(color) {
                    AuraController.setColor1(color)
                }
            }

            ColorPicker {
                visible: AuraController.modeUsesTwoColors(AuraController.currentMode)
                label: qsTr("Color 2")
                selectedColor: AuraController.color2
                onColorSelected: function(color) {
                    AuraController.setColor2(color)
                }
            }
        }

        // Speed control (for animated effects)
        RowLayout {
            Layout.fillWidth: true
            visible: AuraController.modeUsesSpeed(AuraController.currentMode)
            spacing: Theme.spacingMedium

            Text {
                text: qsTr("Speed")
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textPrimary
            }

            Slider {
                id: speedSlider
                Layout.fillWidth: true
                from: 0
                to: 2
                stepSize: 1
                value: AuraController.speed
                enabled: AuraController.available

                onMoved: AuraController.setSpeed(value)

                background: Rectangle {
                    x: speedSlider.leftPadding
                    y: speedSlider.topPadding + speedSlider.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 6
                    width: speedSlider.availableWidth
                    height: implicitHeight
                    radius: 3
                    color: Theme.surfaceLight

                    Rectangle {
                        width: speedSlider.visualPosition * parent.width
                        height: parent.height
                        radius: 3
                        color: Theme.accent
                    }
                }

                handle: Rectangle {
                    x: speedSlider.leftPadding + speedSlider.visualPosition *
                       (speedSlider.availableWidth - width)
                    y: speedSlider.topPadding + speedSlider.availableHeight / 2 - height / 2
                    implicitWidth: 18
                    implicitHeight: 18
                    radius: 9
                    color: speedSlider.pressed ? Theme.accentLight : Theme.accent
                    border.color: Theme.accentDark
                    border.width: 2
                }
            }

            Text {
                text: ["Slow", "Med", "Fast"][speedSlider.value]
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                color: Theme.accent
                Layout.preferredWidth: 35
            }
        }

        // Apply button
        Button {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Apply")
            enabled: AuraController.available

            onClicked: AuraController.applyEffect()

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
                font.bold: true
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        // Not available message
        Text {
            Layout.fillWidth: true
            visible: !AuraController.available
            text: qsTr("Keyboard backlight control not available.")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.warning
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }
}

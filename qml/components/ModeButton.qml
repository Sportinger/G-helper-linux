import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Control {
    id: root

    property string text: ""
    property string icon: ""
    property bool selected: false
    property color accentColor: Theme.accent
    property alias containsMouse: mouseArea.containsMouse

    signal clicked()

    implicitWidth: Theme.modeButtonSize
    implicitHeight: Theme.modeButtonSize + 24

    background: Rectangle {
        width: Theme.modeButtonSize
        height: Theme.modeButtonSize
        radius: Theme.radiusMedium
        color: root.selected ? Theme.colorWithAlpha(root.accentColor, 0.2) :
               mouseArea.containsMouse ? Theme.surfaceLight : Theme.surface
        border.width: root.selected ? 2 : 1
        border.color: root.selected ? root.accentColor :
                      mouseArea.containsMouse ? Theme.borderHover : Theme.border

        Behavior on color {
            ColorAnimation { duration: Theme.animationFast }
        }
        Behavior on border.color {
            ColorAnimation { duration: Theme.animationFast }
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingSmall

        Item {
            Layout.preferredWidth: Theme.modeButtonSize
            Layout.preferredHeight: Theme.modeButtonSize

            Image {
                anchors.centerIn: parent
                width: Theme.iconSizeLarge
                height: Theme.iconSizeLarge
                source: root.icon
                sourceSize: Qt.size(width, height)
                visible: root.icon !== ""

                layer.enabled: true
                layer.effect: ShaderEffect {
                    property color tintColor: root.selected ? root.accentColor : Theme.textSecondary
                    fragmentShader: "
                        varying highp vec2 qt_TexCoord0;
                        uniform sampler2D source;
                        uniform highp vec4 tintColor;
                        void main() {
                            highp vec4 tex = texture2D(source, qt_TexCoord0);
                            gl_FragColor = vec4(tintColor.rgb, tex.a);
                        }
                    "
                }
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.text
            font.pixelSize: Theme.fontSizeSmall
            font.bold: root.selected
            color: root.selected ? root.accentColor : Theme.textSecondary

            Behavior on color {
                ColorAnimation { duration: Theme.animationFast }
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}

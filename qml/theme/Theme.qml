pragma Singleton
import QtQuick

QtObject {
    // Colors - G-Helper (Windows) dark theme
    readonly property color background: "#202020"
    readonly property color surface: "#2b2b2b"
    readonly property color surfaceLight: "#383838"
    readonly property color buttonBackground: "#373737"
    readonly property color buttonHover: "#434343"
    readonly property color controlBackground: "#333333"
    readonly property color border: "#3d3d3d"
    readonly property color borderSelected: "#3aaeef"
    readonly property color borderHover: "#606060"

    // Text colors
    readonly property color textPrimary: "#ffffff"
    readonly property color textSecondary: "#b8b8b8"
    readonly property color textDisabled: "#6a6a6a"

    // Accent colors (G-Helper blue)
    readonly property color accent: "#3aaeef"
    readonly property color accentLight: "#6cc4f4"
    readonly property color accentDark: "#1f8acb"

    // G-Helper mode colours (selected tile borders)
    readonly property color colorEco: "#06b48a"
    readonly property color colorStandard: "#3aaeef"
    readonly property color colorTurbo: "#ff2020"
    readonly property color colorCustom: "#ff8000"

    // Status colors
    readonly property color success: "#4caf50"
    readonly property color warning: "#ff9800"
    readonly property color error: "#f44336"

    // Profile colors
    readonly property color quietColor: colorEco
    readonly property color balancedColor: colorStandard
    readonly property color performanceColor: colorTurbo

    // GPU mode colors
    readonly property color ecoColor: colorEco
    readonly property color standardColor: colorStandard
    readonly property color ultimateColor: colorTurbo
    readonly property color optimizedColor: colorEco

    // Spacing
    readonly property int spacingTiny: 4
    readonly property int spacingSmall: 8
    readonly property int spacingMedium: 12
    readonly property int spacingLarge: 16
    readonly property int spacingXLarge: 24

    // Border radius
    readonly property int radiusSmall: 4
    readonly property int radiusMedium: 8
    readonly property int radiusLarge: 12

    // Font sizes
    readonly property int fontSizeSmall: 11
    readonly property int fontSizeMedium: 13
    readonly property int fontSizeLarge: 15
    readonly property int fontSizeXLarge: 18
    readonly property int fontSizeTitle: 22

    // Component sizes
    readonly property int buttonHeight: 36
    readonly property int modeButtonWidth: 90
    readonly property int modeButtonHeight: 56
    readonly property int iconSize: 24
    readonly property int iconSizeLarge: 28

    // Animations
    readonly property int animationFast: 100
    readonly property int animationMedium: 200
    readonly property int animationSlow: 300

    // Helper functions
    function colorWithAlpha(color, alpha) {
        return Qt.rgba(color.r, color.g, color.b, alpha)
    }

    function profileColor(profile) {
        switch (profile) {
            case 0: return quietColor
            case 1: return balancedColor
            case 2: return performanceColor
            default: return accent
        }
    }

    function gpuModeColor(mode) {
        switch (mode) {
            case 0: return ecoColor
            case 1: return standardColor
            case 2: return ultimateColor
            case 3: return optimizedColor
            default: return accent
        }
    }
}

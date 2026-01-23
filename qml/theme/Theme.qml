pragma Singleton
import QtQuick

QtObject {
    // Colors - G-Helper Dark Theme
    readonly property color background: "#1e1e1e"
    readonly property color surface: "#2d2d2d"
    readonly property color surfaceLight: "#383838"
    readonly property color buttonBackground: "#363636"
    readonly property color border: "#404040"
    readonly property color borderSelected: "#00a0e0"
    readonly property color borderHover: "#606060"

    // Text colors
    readonly property color textPrimary: "#ffffff"
    readonly property color textSecondary: "#b0b0b0"
    readonly property color textDisabled: "#606060"

    // Accent colors
    readonly property color accent: "#00a0e0"
    readonly property color accentLight: "#40c0ff"
    readonly property color accentDark: "#007ab0"

    // Status colors
    readonly property color success: "#4caf50"
    readonly property color warning: "#ff9800"
    readonly property color error: "#f44336"

    // Profile colors
    readonly property color quietColor: "#4fc3f7"
    readonly property color balancedColor: "#81c784"
    readonly property color performanceColor: "#ff8a65"

    // GPU mode colors
    readonly property color ecoColor: "#4caf50"
    readonly property color standardColor: "#2196f3"
    readonly property color ultimateColor: "#f44336"
    readonly property color optimizedColor: "#9c27b0"

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

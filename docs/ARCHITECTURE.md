# G-Helper Linux - Architecture & Implementation Plan

## Overview

A lightweight Qt6/QML control tool for ASUS ROG laptops on Linux, inspired by [G-Helper](https://github.com/seerge/g-helper) for Windows.

The application interfaces with existing Linux tools:
- **asusctl** (`asusd` daemon) - Performance profiles, fan curves, battery, RGB
- **supergfxctl** (`supergfxd` daemon) - GPU mode switching

---

## D-Bus Interface Research

### asusctl (asusd)

| Property | Value |
|----------|-------|
| **Service Name** | `xyz.ljones.Asusd` |
| **Object Path** | `/xyz/ljones` |
| **Written in** | Rust (zbus library) |

#### Platform Interface (`xyz.ljones.Platform`)
- `PlatformProfile` property - Current performance profile (0=Quiet, 1=Balanced, 2=Performance)
- `AvailableProfiles` property - List of available profiles
- `ChargingLimit` property - Battery charge limit (20-100%)

#### Fan Curves Interface (`xyz.ljones.FanCurves`)
- `SetFanCurvesEnabled(profile: u32, enabled: bool)` - Enable/disable custom curves
- Format: `"30c:0%,40c:5%,50c:10%,..."` - 8 temperature/percentage pairs
- Supports CPU, GPU, and Mid (system) fans

#### Aura Interface (`xyz.ljones.Aura`)
- Path prefix: `/xyz/ljones/aura/*` (device-specific paths)
- `LedBrightness` property - Brightness level (0=Off, 1=Low, 2=Med, 3=High)
- `LedMode` property - Effect mode (Static, Breathe, Rainbow, etc.)
- `LedModeData` property - Mode-specific parameters (colors, speed)

### supergfxctl (supergfxd)

| Property | Value |
|----------|-------|
| **Service Name** | `org.supergfxctl.Daemon` |
| **Object Path** | `/org/supergfxctl/Gfx` |
| **Config File** | `/etc/supergfxd.conf` |

#### Available Methods
- `Mode` property - Current GPU mode
- `PendingMode` property - Mode pending after logout
- `Supported` property - List of supported modes
- `Vendor` property - dGPU vendor name
- `PendingUserAction` property - Required user action (logout, etc.)

#### GPU Modes
| Mode | Description | UI Name |
|------|-------------|---------|
| Integrated | iGPU only, dGPU powered off | Eco |
| Hybrid | iGPU + dGPU on demand | Standard |
| AsusMuxDgpu | dGPU only via MUX switch | Ultimate |
| Vfio | GPU passthrough for VMs | Optimized |

---

## Hardware Monitoring

### sysfs Paths (`/sys/class/hwmon/`)

| Sensor | Hwmon Name | Values |
|--------|------------|--------|
| CPU Temperature | `k10temp` | `temp1_input` (millidegrees) |
| GPU Temperature | `amdgpu` | `temp1_input` |
| Fan Speeds | `asus-nb-wmi` | `fan1_input`, `fan2_input`, `fan3_input` (RPM) |

### Battery (`/sys/class/power_supply/BAT0/`)
- `capacity` - Current charge percentage
- `status` - Charging/Discharging/Full
- `power_now` - Power draw in microwatts
- `charge_control_end_threshold` - Charge limit (set by asus-nb-wmi)

---

## Project Structure

```
g-helper-linux/
├── CMakeLists.txt                    # Root CMake configuration
├── src/
│   ├── main.cpp                      # Application entry point
│   │
│   ├── core/                         # Core application classes
│   │   ├── Application.cpp/.h        # QGuiApplication subclass
│   │   └── Settings.cpp/.h           # QSettings wrapper
│   │
│   ├── dbus/                         # D-Bus abstraction layer
│   │   ├── DBusTypes.h               # Custom D-Bus type definitions
│   │   ├── AsusdClient.cpp/.h        # asusctl D-Bus client
│   │   ├── SuperGfxClient.cpp/.h     # supergfxctl D-Bus client
│   │   └── DBusWatcher.cpp/.h        # Connection monitoring
│   │
│   ├── controllers/                  # QML-exposed backend controllers
│   │   ├── PerformanceController.cpp/.h
│   │   ├── GpuController.cpp/.h
│   │   ├── FanController.cpp/.h
│   │   ├── BatteryController.cpp/.h
│   │   ├── AuraController.cpp/.h
│   │   └── SystemMonitor.cpp/.h
│   │
│   ├── models/                       # Data models
│   │   ├── FanCurveModel.cpp/.h
│   │   └── AuraModeModel.cpp/.h
│   │
│   └── tray/                         # System tray
│       └── TrayManager.cpp/.h
│
├── qml/                              # QML UI files
│   ├── Main.qml                      # Root window
│   ├── theme/
│   │   └── Theme.qml                 # Colors, fonts, spacing
│   ├── components/
│   │   ├── ModeButton.qml            # Selectable mode button
│   │   ├── ModeButtonGroup.qml       # Row of mode buttons
│   │   ├── SectionHeader.qml         # Section title with icon
│   │   ├── StatusDisplay.qml         # Temp/fan display
│   │   ├── PercentSlider.qml         # Styled slider
│   │   ├── ColorPicker.qml           # RGB color picker
│   │   ├── DropdownSelect.qml        # Styled combobox
│   │   └── IconButton.qml            # Small icon button
│   ├── panels/
│   │   ├── PerformancePanel.qml
│   │   ├── GpuPanel.qml
│   │   ├── KeyboardPanel.qml
│   │   ├── BatteryPanel.qml
│   │   └── FanCurvePanel.qml
│   └── dialogs/
│       ├── FanCurveDialog.qml
│       └── AboutDialog.qml
│
├── resources/
│   ├── resources.qrc
│   └── icons/
│
└── desktop/
    ├── g-helper-linux.desktop
    └── g-helper-linux.svg
```

---

## Key C++ Classes

### AsusdClient

```cpp
class AsusdClient : public QObject {
    Q_OBJECT
public:
    bool isConnected() const;

    // Platform profiles
    quint32 platformProfile() const;
    QVector<quint32> availableProfiles() const;
    void setPlatformProfile(quint32 profile);

    // Battery
    quint8 chargeLimit() const;
    void setChargeLimit(quint8 limit);

    // Fan curves
    QVector<FanCurveData> getFanCurves(quint32 profile);
    void setFanCurve(quint32 profile, const FanCurveData& curve);
    void setFanCurvesEnabled(quint32 profile, bool enabled);

    // Aura
    quint32 ledBrightness() const;
    void setLedBrightness(quint32 level);
    quint32 ledMode() const;
    void setLedMode(quint32 mode);

signals:
    void connectionChanged(bool connected);
    void platformProfileChanged(quint32 profile);
    void chargeLimitChanged(quint8 limit);
    void ledBrightnessChanged(quint32 level);
    void ledModeChanged(quint32 mode);
};
```

### SuperGfxClient

```cpp
class SuperGfxClient : public QObject {
    Q_OBJECT
public:
    enum GpuMode {
        Integrated = 0,  // Eco
        Hybrid = 1,      // Standard
        AsusMuxDgpu = 2, // Ultimate
        Vfio = 3         // Optimized
    };
    Q_ENUM(GpuMode)

    bool isConnected() const;
    GpuMode currentMode() const;
    GpuMode pendingMode() const;
    QVector<GpuMode> supportedModes() const;
    quint32 pendingUserAction() const;

    void setMode(GpuMode mode);

signals:
    void connectionChanged(bool connected);
    void modeChanged(GpuMode mode);
    void pendingModeChanged(GpuMode mode);
    void logoutRequired();
};
```

### Controllers (QML Singletons)

```cpp
// PerformanceController
Q_PROPERTY(int currentProfile READ currentProfile NOTIFY currentProfileChanged)
Q_INVOKABLE void setProfile(int profile);

// GpuController
Q_PROPERTY(int currentMode READ currentMode NOTIFY currentModeChanged)
Q_PROPERTY(bool actionRequired READ actionRequired NOTIFY actionRequiredChanged)
Q_INVOKABLE void setMode(int mode);

// BatteryController
Q_PROPERTY(int chargeLimit READ chargeLimit WRITE setChargeLimit NOTIFY chargeLimitChanged)
Q_PROPERTY(int currentCharge READ currentCharge NOTIFY currentChargeChanged)
Q_PROPERTY(bool isCharging READ isCharging NOTIFY chargingStateChanged)
Q_PROPERTY(double powerDraw READ powerDraw NOTIFY powerDrawChanged)

// SystemMonitor
Q_PROPERTY(int cpuTemp READ cpuTemp NOTIFY cpuTempChanged)
Q_PROPERTY(int gpuTemp READ gpuTemp NOTIFY gpuTempChanged)
Q_PROPERTY(int cpuFanRpm READ cpuFanRpm NOTIFY cpuFanRpmChanged)
Q_PROPERTY(int gpuFanRpm READ gpuFanRpm NOTIFY gpuFanRpmChanged)
```

---

## Theme (G-Helper Dark)

```qml
pragma Singleton
import QtQuick

QtObject {
    // Colors
    readonly property color background: "#1e1e1e"
    readonly property color surface: "#2d2d2d"
    readonly property color surfaceHover: "#3d3d3d"
    readonly property color border: "#404040"
    readonly property color borderSelected: "#00a0e0"  // Cyan accent

    readonly property color textPrimary: "#ffffff"
    readonly property color textSecondary: "#b0b0b0"
    readonly property color textMuted: "#707070"

    readonly property color accentBlue: "#00a0e0"
    readonly property color accentGreen: "#00c853"
    readonly property color accentRed: "#ff5252"

    // Spacing
    readonly property int spacingSmall: 4
    readonly property int spacingMedium: 8
    readonly property int spacingLarge: 16

    // Sizes
    readonly property int buttonHeight: 48
    readonly property int iconSize: 20
    readonly property int borderRadius: 6
}
```

---

## Implementation Phases

### Phase 1: Foundation
- [ ] CMake build system with Qt6 QML module
- [ ] Application entry point (`main.cpp`)
- [ ] Settings class with QSettings
- [ ] DBusWatcher for connection monitoring
- [ ] Basic Main.qml with dark theme

### Phase 2: D-Bus Layer
- [ ] AsusdClient implementation
- [ ] SuperGfxClient implementation
- [ ] Custom D-Bus type definitions
- [ ] PropertiesChanged signal handling

### Phase 3: Core Controllers
- [ ] PerformanceController
- [ ] GpuController with logout notification
- [ ] BatteryController with power monitoring
- [ ] SystemMonitor via hwmon sysfs

### Phase 4: QML UI
- [ ] Theme.qml singleton
- [ ] ModeButton and ModeButtonGroup
- [ ] PerformancePanel and GpuPanel
- [ ] BatteryPanel with slider
- [ ] StatusDisplay components

### Phase 5: Advanced Features
- [ ] AuraController and KeyboardPanel
- [ ] FanController with FanCurveModel
- [ ] FanCurveDialog with chart
- [ ] TrayManager with menu

### Phase 6: Polish
- [ ] Error handling and notifications
- [ ] Settings persistence
- [ ] Desktop file and autostart
- [ ] Testing

---

## Build & Run

```bash
# Install dependencies (Arch Linux)
sudo pacman -S qt6-base qt6-declarative qt6-quickcontrols2 cmake

# Ensure asusctl and supergfxctl are installed
sudo pacman -S asusctl supergfxctl

# Build
mkdir build && cd build
cmake ..
make

# Run
./g-helper-linux
```

---

## References

- [asusctl GitLab](https://gitlab.com/asus-linux/asusctl)
- [supergfxctl GitLab](https://gitlab.com/asus-linux/supergfxctl)
- [asusctl Manual](https://asus-linux.org/manual/asusctl-manual/)
- [supergfxctl Manual](https://asus-linux.org/manual/supergfxctl-manual/)
- [Original G-Helper](https://github.com/seerge/g-helper)
- [Qt D-Bus Documentation](https://doc.qt.io/qt-6/qtdbus-module.html)

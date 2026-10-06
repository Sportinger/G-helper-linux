#ifndef DBUSTYPES_H
#define DBUSTYPES_H

#include <QtGlobal>

// Numeric values as they are sent over D-Bus by the daemons.
// Verified against asusctl 6.3.x (rog-platform, rog-aura) and supergfxctl 5.2.x.
// Do NOT confuse these with the UI-side enums of the controllers.

// asusd: xyz.ljones.Platform.PlatformProfile (rog_platform::platform::PlatformProfile)
namespace AsusdProfile {
enum : quint32 {
    Balanced = 0,
    Performance = 1,
    Quiet = 2,
    LowPower = 3,
    Custom = 4
};
}

// asusd: xyz.ljones.Aura.LedModeData mode field (rog_aura::AuraModeNum)
namespace AsusdAuraMode {
enum : quint32 {
    Static = 0,
    Breathe = 1,
    RainbowCycle = 2,
    RainbowWave = 3,
    Star = 4,
    Rain = 5,
    Highlight = 6,
    Laser = 7,
    Ripple = 8,
    Pulse = 10,
    Comet = 11,
    Flash = 12
};
}

// supergfxd: org.supergfxctl.Daemon Mode/SetMode/Supported (GfxMode)
namespace GfxMode {
enum : quint32 {
    Hybrid = 0,
    Integrated = 1,
    NvidiaNoModeset = 2,
    Vfio = 3,
    AsusEgpu = 4,
    AsusMuxDgpu = 5,
    None = 6
};
}

// supergfxd: org.supergfxctl.Daemon Power (GfxPower)
namespace GfxPower {
enum : quint32 {
    Active = 0,
    Suspended = 1,
    Off = 2,
    AsusDisabled = 3,
    AsusMuxDiscreet = 4,
    Unknown = 5
};
}

// supergfxd: return value of SetMode / PendingUserAction (UserActionRequired)
namespace GfxAction {
enum : quint32 {
    Logout = 0,
    Reboot = 1,
    SwitchToIntegrated = 2,
    AsusEgpuDisable = 3,
    Nothing = 4
};
}

#endif // DBUSTYPES_H

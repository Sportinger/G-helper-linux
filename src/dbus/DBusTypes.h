#ifndef DBUSTYPES_H
#define DBUSTYPES_H

#include <QDBusArgument>
#include <QDBusMetaType>
#include <QVector>
#include <QPair>

// Performance profiles from asusd
enum class PerformanceProfile : quint32 {
    Quiet = 0,
    Balanced = 1,
    Performance = 2
};

// GPU modes from supergfxctl
enum class GpuMode : quint32 {
    Integrated = 0,
    Hybrid = 1,
    AsusMuxDgpu = 2,
    Vfio = 3,
    Egpu = 4,
    None = 5
};

// Fan curve point (temperature, fan percentage)
struct FanCurvePoint {
    quint8 temperature;
    quint8 fanPercent;
};

// Fan curve data
struct FanCurveData {
    quint32 profile;
    quint32 fanType; // 0 = CPU, 1 = GPU
    bool enabled;
    QVector<FanCurvePoint> points;
};

// Aura modes
enum class AuraMode : quint32 {
    Static = 0,
    Breathe = 1,
    Strobe = 2,
    Rainbow = 3,
    Star = 4,
    Rain = 5,
    Highlight = 6,
    Laser = 7,
    Ripple = 8,
    Pulse = 9,
    Comet = 10,
    Flash = 11
};

// RGB color
struct AuraColor {
    quint8 red;
    quint8 green;
    quint8 blue;
};

// Aura effect settings
struct AuraEffect {
    AuraMode mode;
    AuraColor color1;
    AuraColor color2;
    quint8 speed;
    quint8 direction;
};

// D-Bus argument operators for FanCurvePoint
inline QDBusArgument &operator<<(QDBusArgument &arg, const FanCurvePoint &point)
{
    arg.beginStructure();
    arg << point.temperature << point.fanPercent;
    arg.endStructure();
    return arg;
}

inline const QDBusArgument &operator>>(const QDBusArgument &arg, FanCurvePoint &point)
{
    arg.beginStructure();
    arg >> point.temperature >> point.fanPercent;
    arg.endStructure();
    return arg;
}

// D-Bus argument operators for AuraColor
inline QDBusArgument &operator<<(QDBusArgument &arg, const AuraColor &color)
{
    arg.beginStructure();
    arg << color.red << color.green << color.blue;
    arg.endStructure();
    return arg;
}

inline const QDBusArgument &operator>>(const QDBusArgument &arg, AuraColor &color)
{
    arg.beginStructure();
    arg >> color.red >> color.green >> color.blue;
    arg.endStructure();
    return arg;
}

// Register metatypes
inline void registerDBusTypes()
{
    qRegisterMetaType<FanCurvePoint>("FanCurvePoint");
    qRegisterMetaType<FanCurveData>("FanCurveData");
    qRegisterMetaType<AuraColor>("AuraColor");
    qRegisterMetaType<AuraEffect>("AuraEffect");
    qDBusRegisterMetaType<FanCurvePoint>();
    qDBusRegisterMetaType<AuraColor>();
    qDBusRegisterMetaType<QVector<FanCurvePoint>>();
}

#endif // DBUSTYPES_H

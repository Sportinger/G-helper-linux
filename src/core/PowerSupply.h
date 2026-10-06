#ifndef POWERSUPPLY_H
#define POWERSUPPLY_H

#include <QString>

// Locates the laptop battery and AC adapter in /sys/class/power_supply.
// Names differ between models (BAT0/BAT1, AC0/ACAD/ADP1), so they are
// detected by their "type" attribute instead of being hardcoded.
namespace PowerSupply {

// Path of the system battery, e.g. "/sys/class/power_supply/BAT1" (empty if none)
QString batteryPath();

// Path of the AC adapter, e.g. "/sys/class/power_supply/ACAD" (empty if none)
QString acPath();

// Reads a sysfs attribute and returns its trimmed content (empty on failure)
QString readAttribute(const QString &devicePath, const QString &attribute);

// Convenience: reads a numeric attribute, returns `fallback` if missing/invalid
double readNumber(const QString &devicePath, const QString &attribute, double fallback = -1.0);

// true if AC power is connected (falls back to battery status if no AC node exists)
bool isOnAc();

}

#endif // POWERSUPPLY_H

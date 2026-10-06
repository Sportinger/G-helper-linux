#include "PowerSupply.h"
#include <QDir>
#include <QFile>

namespace {

const QString POWER_SUPPLY_DIR = QStringLiteral("/sys/class/power_supply");

QString findSupply(const QString &wantedType)
{
    QDir dir(POWER_SUPPLY_DIR);
    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System, QDir::Name);
    for (const QString &entry : entries) {
        const QString path = POWER_SUPPLY_DIR + "/" + entry;
        if (PowerSupply::readAttribute(path, "type") != wantedType)
            continue;
        // Skip batteries of peripherals (mice, headsets, ...)
        if (PowerSupply::readAttribute(path, "scope") == "Device")
            continue;
        return path;
    }
    return QString();
}

}

namespace PowerSupply {

QString batteryPath()
{
    static const QString path = findSupply("Battery");
    return path;
}

QString acPath()
{
    static const QString path = findSupply("Mains");
    return path;
}

QString readAttribute(const QString &devicePath, const QString &attribute)
{
    if (devicePath.isEmpty())
        return QString();

    QFile file(devicePath + "/" + attribute);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readLine()).trimmed();
}

double readNumber(const QString &devicePath, const QString &attribute, double fallback)
{
    bool ok = false;
    const double value = readAttribute(devicePath, attribute).toDouble(&ok);
    return ok ? value : fallback;
}

bool isOnAc()
{
    const QString ac = acPath();
    if (!ac.isEmpty())
        return readAttribute(ac, "online") == "1";

    const QString status = readAttribute(batteryPath(), "status");
    return status == "Charging" || status == "Full" || status == "Not charging";
}

}

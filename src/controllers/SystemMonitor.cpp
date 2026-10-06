#include "SystemMonitor.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QProcess>
#include "PowerSupply.h"
#include <QDateTime>
#include <QCoreApplication>
#include <QFileInfo>
#include <QRegularExpression>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

SystemMonitor::SystemMonitor(QObject *parent)
    : QObject(parent)
    , m_updateTimer(new QTimer(this))
{
    m_updateTimer->setInterval(1000); // Update every second
    connect(m_updateTimer, &QTimer::timeout, this, &SystemMonitor::update);

    findHwmonPaths();
}

SystemMonitor::~SystemMonitor()
{
    stop();
}

void SystemMonitor::start()
{
    if (!m_updateTimer->isActive()) {
        m_updateTimer->start();
        update(); // Initial update
    }
}

void SystemMonitor::stop()
{
    m_updateTimer->stop();
}

void SystemMonitor::setUpdateInterval(int msec)
{
    m_updateTimer->setInterval(qMax(100, msec));
}

void SystemMonitor::findHwmonPaths()
{
    // Prefer the first sensor (temp1_input / fan1_input). Note that a plain
    // alphabetical sort would put temp10_input before temp1_input.
    auto firstSensor = [](const QDir &dir, const QString &prefix) -> QString {
        if (dir.exists(prefix + "1_input"))
            return dir.filePath(prefix + "1_input");
        const QStringList files = dir.entryList(QStringList() << prefix + "*_input", QDir::Files);
        return files.isEmpty() ? QString() : dir.filePath(files.first());
    };

    QDir hwmonDir("/sys/class/hwmon");
    const QStringList hwmonDevices = hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System, QDir::Name);

    for (const QString &device : hwmonDevices) {
        const QString basePath = "/sys/class/hwmon/" + device;
        const QDir deviceDir(basePath);
        const QString name = PowerSupply::readAttribute(basePath, "name");

        // CPU temperature (k10temp for AMD, coretemp for Intel)
        if ((name == "k10temp" || name == "coretemp") && m_cpuTempPath.isEmpty()) {
            m_cpuTempPath = firstSensor(deviceDir, "temp");
            qDebug() << "Found CPU temp at:" << m_cpuTempPath;
        }

        // iGPU temperature and APU power (amdgpu)
        if (name == "amdgpu" && m_gpuTempPath.isEmpty()) {
            m_gpuTempPath = firstSensor(deviceDir, "temp");
            qDebug() << "Found GPU temp at:" << m_gpuTempPath;

            if (deviceDir.exists("power1_input"))
                m_apuPowerPath = basePath + "/power1_input";
            else if (deviceDir.exists("power1_average"))
                m_apuPowerPath = basePath + "/power1_average";
            if (!m_apuPowerPath.isEmpty())
                qDebug() << "Found APU power at:" << m_apuPowerPath;
        }

        // ASUS WMI fan speeds: fan1 = CPU, fan2 = GPU
        if (name == "asus" || name == "asus-nb-wmi" || name == "asus_fan") {
            if (m_cpuFanPath.isEmpty() && deviceDir.exists("fan1_input")) {
                m_cpuFanPath = basePath + "/fan1_input";
                qDebug() << "Found CPU fan at:" << m_cpuFanPath;
            }
            if (m_gpuFanPath.isEmpty() && deviceDir.exists("fan2_input")) {
                m_gpuFanPath = basePath + "/fan2_input";
                qDebug() << "Found GPU fan at:" << m_gpuFanPath;
            }
        }
    }

    m_available = !m_cpuTempPath.isEmpty() || !m_gpuTempPath.isEmpty();
    emit availableChanged(m_available);

    // iGPU load: only amdgpu exposes gpu_busy_percent. Card numbering
    // differs between kernels, so search instead of assuming card0.
    QDir drmDir("/sys/class/drm");
    const QStringList cards = drmDir.entryList(QStringList() << "card*", QDir::Dirs | QDir::System, QDir::Name);
    for (const QString &card : cards) {
        if (card.contains('-'))
            continue;   // connectors like card1-eDP-1
        const QString busyPath = "/sys/class/drm/" + card + "/device/gpu_busy_percent";
        if (QFile::exists(busyPath)) {
            m_gpuBusyPath = busyPath;
            qDebug() << "Found GPU load at:" << m_gpuBusyPath;
            break;
        }
    }

    // NVIDIA dGPU on the PCI bus (vendor 0x10de, display class 0x03xxxx)
    QDir pciDir("/sys/bus/pci/devices");
    const QStringList pciDevices = pciDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System, QDir::Name);
    for (const QString &device : pciDevices) {
        const QString path = "/sys/bus/pci/devices/" + device;
        if (PowerSupply::readAttribute(path, "vendor") == "0x10de"
            && PowerSupply::readAttribute(path, "class").startsWith("0x03")) {
            m_dgpuPciPath = path;
            const QStringList drmNodes = QDir(path + "/drm").entryList(
                QStringList() << "card*" << "renderD*", QDir::Dirs | QDir::System);
            for (const QString &node : drmNodes)
                m_dgpuDevNodes << "/dev/dri/" + node;
            qDebug() << "Found NVIDIA dGPU at:" << m_dgpuPciPath << m_dgpuDevNodes;
            break;
        }
    }

    // Find backlight device, preferring the iGPU one
    QDir backlightDir("/sys/class/backlight");
    const QStringList backlightDevices = backlightDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System, QDir::Name);
    for (const QString &device : backlightDevices) {
        if (!device.startsWith("amdgpu") && !device.startsWith("intel") && !m_backlightPath.isEmpty())
            continue;
        const QString basePath = "/sys/class/backlight/" + device;
        const int maxBrightness = static_cast<int>(PowerSupply::readNumber(basePath, "max_brightness", 0));
        if (maxBrightness > 0) {
            m_backlightPath = basePath;
            m_maxBrightness = maxBrightness;
            qDebug() << "Found backlight at:" << m_backlightPath << "max:" << m_maxBrightness;
        }
    }
}

void SystemMonitor::update()
{
    // Read temperatures
    if (!m_cpuTempPath.isEmpty()) {
        int temp = readTemperature(m_cpuTempPath);
        if (temp != m_cpuTemp) {
            m_cpuTemp = temp;
            emit cpuTempChanged(temp);
        }
    }

    if (!m_gpuTempPath.isEmpty()) {
        int temp = readTemperature(m_gpuTempPath);
        if (temp != m_gpuTemp) {
            m_gpuTemp = temp;
            emit gpuTempChanged(temp);
        }
    }

    // Read fan speeds
    if (!m_cpuFanPath.isEmpty()) {
        int rpm = readFanSpeed(m_cpuFanPath);
        if (rpm != m_cpuFanRpm) {
            m_cpuFanRpm = rpm;
            m_cpuFanPercent = qMin(100, (rpm * 100) / MAX_FAN_RPM);
            emit cpuFanRpmChanged(rpm);
            emit cpuFanPercentChanged(m_cpuFanPercent);
        }
    }

    if (!m_gpuFanPath.isEmpty()) {
        int rpm = readFanSpeed(m_gpuFanPath);
        if (rpm != m_gpuFanRpm) {
            m_gpuFanRpm = rpm;
            m_gpuFanPercent = qMin(100, (rpm * 100) / MAX_FAN_RPM);
            emit gpuFanRpmChanged(rpm);
            emit gpuFanPercentChanged(m_gpuFanPercent);
        }
    }

    // Read CPU/GPU usage and power
    readCpuUsage();
    readGpuUsage();
    readDgpuInfo();
    readMemoryInfo();
    readApuPower();
    readDisplayBrightness();
    readBatteryPower();
    calculateSystemPower();
}

int SystemMonitor::readTemperature(const QString &path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        int temp = in.readLine().toInt() / 1000; // Convert millidegrees to degrees
        file.close();
        return temp;
    }
    return 0;
}

int SystemMonitor::readFanSpeed(const QString &path)
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        int rpm = in.readLine().toInt();
        file.close();
        return rpm;
    }
    return 0;
}

void SystemMonitor::readCpuUsage()
{
    QFile file("/proc/stat");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString line = in.readLine();
        file.close();

        if (line.startsWith("cpu ")) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 5) {
                qint64 user = parts[1].toLongLong();
                qint64 nice = parts[2].toLongLong();
                qint64 system = parts[3].toLongLong();
                qint64 idle = parts[4].toLongLong();
                qint64 iowait = parts.size() > 5 ? parts[5].toLongLong() : 0;
                qint64 irq = parts.size() > 6 ? parts[6].toLongLong() : 0;
                qint64 softirq = parts.size() > 7 ? parts[7].toLongLong() : 0;
                qint64 steal = parts.size() > 8 ? parts[8].toLongLong() : 0;

                qint64 totalTime = user + nice + system + idle + iowait + irq + softirq + steal;
                qint64 idleTime = idle + iowait;

                if (m_prevTotalTime > 0) {
                    qint64 totalDiff = totalTime - m_prevTotalTime;
                    qint64 idleDiff = idleTime - m_prevIdleTime;

                    if (totalDiff > 0) {
                        double usage = 100.0 * (1.0 - static_cast<double>(idleDiff) / totalDiff);
                        if (qAbs(m_cpuUsage - usage) > 0.5) {
                            m_cpuUsage = usage;
                            emit cpuUsageChanged(usage);
                        }
                    }
                }

                m_prevTotalTime = totalTime;
                m_prevIdleTime = idleTime;
            }
        }
    }
}

void SystemMonitor::readGpuUsage()
{
    if (m_gpuBusyPath.isEmpty())
        return;

    QFile file(m_gpuBusyPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const double usage = QString::fromUtf8(file.readLine()).trimmed().toDouble();
        if (qAbs(m_gpuUsage - usage) > 0.5) {
            m_gpuUsage = usage;
            emit gpuUsageChanged(usage);
        }
    }
}

void SystemMonitor::resetDgpuStats()
{
    if (m_dgpuUsage != 0.0) {
        m_dgpuUsage = 0.0;
        emit dgpuUsageChanged(0.0);
    }
    if (m_dgpuTemp != 0) {
        m_dgpuTemp = 0;
        emit dgpuTempChanged(0);
    }
}

namespace {

// Display name for a process: the script/module for interpreters
// ("python -m vidscout" -> "vidscout"), otherwise the command name.
QString processName(const QString &pid)
{
    QString comm = PowerSupply::readAttribute("/proc/" + pid, "comm");

    static const QRegularExpression interpreter("^(python[0-9.]*|node|java|ruby|perl|bash|sh)$");
    if (interpreter.match(comm).hasMatch()) {
        QFile cmdlineFile("/proc/" + pid + "/cmdline");
        if (cmdlineFile.open(QIODevice::ReadOnly)) {
            const QList<QByteArray> args = cmdlineFile.readAll().split('\0');
            for (int i = 1; i < args.size(); ++i) {
                const QString arg = QString::fromLocal8Bit(args[i]);
                if (arg == "-m" && i + 1 < args.size())
                    return QString::fromLocal8Bit(args[i + 1]).section('.', 0, 0);
                if (!arg.isEmpty() && !arg.startsWith('-'))
                    return QFileInfo(arg).completeBaseName();
            }
        }
    }

    if (!comm.isEmpty())
        comm[0] = comm[0].toUpper();
    return comm;
}

}

void SystemMonitor::scanDgpuUsers()
{
    const uid_t uid = getuid();
    const qint64 ownPid = QCoreApplication::applicationPid();
    QVariantList users;
    QStringList appNames;

    DIR *proc = opendir("/proc");
    if (!proc)
        return;

    while (dirent *entry = readdir(proc)) {
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9')
            continue;

        const QByteArray pidDir = QByteArray("/proc/") + entry->d_name;
        struct stat st;
        // Other users' file descriptors aren't readable anyway
        if (stat(pidDir.constData(), &st) != 0 || st.st_uid != uid)
            continue;
        if (QByteArray(entry->d_name).toLongLong() == ownPid)
            continue;

        DIR *fdDir = opendir((pidDir + "/fd").constData());
        if (!fdDir)
            continue;

        bool usesDgpu = false;
        char target[256];
        while (dirent *fd = readdir(fdDir)) {
            if (fd->d_name[0] == '.')
                continue;
            const QByteArray link = pidDir + "/fd/" + fd->d_name;
            const ssize_t len = readlink(link.constData(), target, sizeof(target) - 1);
            if (len <= 0)
                continue;
            target[len] = '\0';
            const QLatin1String path(target, len);
            // /dev/nvidia0..N and nvidia-uvm (CUDA) or the dGPU's DRM nodes
            if ((path.startsWith(QLatin1String("/dev/nvidia")) && path != QLatin1String("/dev/nvidiactl")
                 && path != QLatin1String("/dev/nvidia-modeset"))
                || m_dgpuDevNodes.contains(QString(path))) {
                usesDgpu = true;
                break;
            }
        }
        closedir(fdDir);

        if (!usesDgpu)
            continue;

        const QString pid = QString::fromLatin1(entry->d_name);
        const QString name = processName(pid);
        // The compositor always keeps the device open; that alone doesn't
        // prevent the dGPU from sleeping
        static const QStringList desktopProcesses = {
            "Gnome-shell", "Xwayland", "Xorg", "Kwin_wayland", "Kwin_x11", "Mutter", "Gnome-shell-cal"
        };
        const bool desktop = desktopProcesses.contains(name);
        users << QVariantMap{{"pid", pid.toInt()}, {"name", name}, {"desktop", desktop}};
        if (!desktop && !appNames.contains(name))
            appNames << name;
    }
    closedir(proc);

    setDgpuUsers(users);
    if (m_dgpuUserNames != appNames.join(", ")) {
        m_dgpuUserNames = appNames.join(", ");
        emit dgpuUsersChanged();
    }
}

void SystemMonitor::setDgpuUsers(const QVariantList &users)
{
    if (m_dgpuUsers == users)
        return;
    m_dgpuUsers = users;
    if (users.isEmpty())
        m_dgpuUserNames.clear();
    emit dgpuUsersChanged();
}

void SystemMonitor::readDgpuInfo()
{
    if (m_dgpuPciPath.isEmpty())
        return;

    // Runtime PM state straight from sysfs - reading it never wakes the GPU
    const QString state = PowerSupply::readAttribute(m_dgpuPciPath + "/power", "runtime_status");
    if (m_dgpuState != state) {
        m_dgpuState = state;
        emit dgpuStateChanged(state);
    }

    // Only ask nvidia-smi while the dGPU is awake *and* in use. Polling an
    // idle dGPU (usage count 0) would keep it from suspending.
    const double usageCount = PowerSupply::readNumber(m_dgpuPciPath + "/power", "runtime_usage", 1);
    // Who keeps the dGPU awake? Only interesting while it is awake.
    if (state == "active") {
        if (m_dgpuScanTick++ % 3 == 0)
            scanDgpuUsers();
    } else {
        m_dgpuScanTick = 0;
        setDgpuUsers({});
    }

    const bool queryable = (state == "active") && usageCount > 0;
    if (!queryable) {
        resetDgpuStats();
        return;
    }

    if (m_nvidiaSmiMissing || m_nvidiaSmi)
        return;
    if (QDateTime::currentMSecsSinceEpoch() < m_nvidiaSmiBackoffUntil)
        return;
    // nvidia-smi is comparatively expensive, every 3rd tick is enough
    if (m_dgpuTick++ % 3 != 0)
        return;

    m_nvidiaSmi = new QProcess(this);
    QProcess *process = m_nvidiaSmi;

    connect(process, &QProcess::finished, this, [this, process](int exitCode, QProcess::ExitStatus status) {
        if (m_nvidiaSmi == process)
            m_nvidiaSmi = nullptr;
        process->deleteLater();

        if (status != QProcess::NormalExit || exitCode != 0)
            return;

        const QString output = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
        const QStringList values = output.split(',');
        if (values.size() < 2)
            return;

        const double usage = values[0].trimmed().toDouble();
        const int temp = values[1].trimmed().toInt();

        if (qAbs(m_dgpuUsage - usage) > 0.5) {
            m_dgpuUsage = usage;
            emit dgpuUsageChanged(usage);
        }
        if (m_dgpuTemp != temp) {
            m_dgpuTemp = temp;
            emit dgpuTempChanged(temp);
        }
    });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            // nvidia-smi is not installed; don't keep trying
            m_nvidiaSmiMissing = true;
            if (m_nvidiaSmi == process)
                m_nvidiaSmi = nullptr;
            process->deleteLater();
        }
    });

    // A hanging nvidia-smi (driver busy) must not pile up: kill it and back off
    QTimer::singleShot(3000, process, [this, process]() {
        if (process->state() != QProcess::NotRunning) {
            qWarning() << "SystemMonitor: nvidia-smi timed out, backing off";
            m_nvidiaSmiBackoffUntil = QDateTime::currentMSecsSinceEpoch() + 30000;
            process->kill();
        }
    });

    process->start("nvidia-smi", QStringList() << "--query-gpu=utilization.gpu,temperature.gpu"
                                               << "--format=csv,noheader,nounits");
}

void SystemMonitor::readMemoryInfo()
{
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        qint64 memTotal = 0, memAvailable = 0;

        while (!in.atEnd()) {
            QString line = in.readLine();
            const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() < 2)
                continue;
            if (parts[0] == "MemTotal:") {
                memTotal = parts[1].toLongLong();
            } else if (parts[0] == "MemAvailable:") {
                memAvailable = parts[1].toLongLong();
            }
        }
        file.close();

        int total = static_cast<int>(memTotal / 1024); // MB
        int used = static_cast<int>((memTotal - memAvailable) / 1024); // MB

        if (m_memoryTotal != total || m_memoryUsed != used) {
            m_memoryTotal = total;
            m_memoryUsed = used;
            emit memoryChanged();
        }
    }
}

void SystemMonitor::readApuPower()
{
    if (m_apuPowerPath.isEmpty()) return;

    QFile file(m_apuPowerPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        double power = in.readLine().toDouble() / 1000000.0; // microwatts to watts
        file.close();

        if (qAbs(m_apuPower - power) > 0.1) {
            m_apuPower = power;
            emit apuPowerChanged(power);
        }
    }
}

void SystemMonitor::readDisplayBrightness()
{
    if (m_backlightPath.isEmpty() || m_maxBrightness <= 0) return;

    QFile file(m_backlightPath + "/brightness");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        int brightness = in.readLine().toInt();
        file.close();

        // Calculate brightness percentage
        int brightnessPercent = (brightness * 100) / m_maxBrightness;
        if (m_displayBrightness != brightnessPercent) {
            m_displayBrightness = brightnessPercent;
            emit displayBrightnessChanged(brightnessPercent);
        }

        // Estimate display power based on brightness (linear interpolation)
        double displayPower = MIN_DISPLAY_POWER +
            (MAX_DISPLAY_POWER - MIN_DISPLAY_POWER) * (brightnessPercent / 100.0);
        if (qAbs(m_displayPower - displayPower) > 0.1) {
            m_displayPower = displayPower;
            emit displayPowerChanged(displayPower);
        }
    }
}

void SystemMonitor::readBatteryPower()
{
    const bool onBattery = !PowerSupply::isOnAc();
    if (m_onBattery != onBattery) {
        m_onBattery = onBattery;
        emit onBatteryChanged(onBattery);
    }

    double power = 0.0;
    if (m_onBattery) {
        const QString bat = PowerSupply::batteryPath();
        const double powerNow = PowerSupply::readNumber(bat, "power_now");
        if (powerNow >= 0) {
            power = powerNow / 1e6;  // uW -> W
        } else {
            const double current = PowerSupply::readNumber(bat, "current_now");
            const double voltage = PowerSupply::readNumber(bat, "voltage_now");
            if (current >= 0 && voltage >= 0)
                power = (current / 1e6) * (voltage / 1e6);  // uA * uV -> W
        }
        power = qAbs(power);
    }

    if (qAbs(m_batteryPower - power) > 0.1) {
        m_batteryPower = power;
        emit batteryPowerChanged(power);
    }
}

void SystemMonitor::calculateSystemPower()
{
    double systemPower = 0.0;

    if (m_onBattery && m_batteryPower > 0.1) {
        // On battery the discharge rate is the real total system draw
        // (display included).
        systemPower = m_batteryPower;
    } else {
        // On AC there is no measurement for the whole system, so estimate:
        // APU (CPU + iGPU) + display + misc (SSD, WiFi, RAM, fans, ...)
        systemPower = m_apuPower + m_displayPower + MISC_POWER_ESTIMATE;
    }

    if (qAbs(m_systemPower - systemPower) > 0.1) {
        m_systemPower = systemPower;
        emit systemPowerChanged(systemPower);
    }
}

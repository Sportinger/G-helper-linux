#include "SystemMonitor.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>

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
    QDir hwmonDir("/sys/class/hwmon");
    QStringList hwmonDevices = hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &device : hwmonDevices) {
        QString basePath = "/sys/class/hwmon/" + device;
        QString namePath = basePath + "/name";

        QFile nameFile(namePath);
        if (nameFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&nameFile);
            QString name = in.readLine().trimmed();
            nameFile.close();

            // CPU temperature (k10temp for AMD, coretemp for Intel)
            if (name == "k10temp" || name == "coretemp") {
                // Find temp input file
                QDir deviceDir(basePath);
                QStringList tempFiles = deviceDir.entryList(QStringList() << "temp*_input", QDir::Files);
                if (!tempFiles.isEmpty()) {
                    m_cpuTempPath = basePath + "/" + tempFiles.first();
                    qDebug() << "Found CPU temp at:" << m_cpuTempPath;
                }
            }

            // GPU temperature and power (amdgpu for AMD, nvidia for NVIDIA)
            if (name == "amdgpu" || name == "nvidia") {
                QDir deviceDir(basePath);
                QStringList tempFiles = deviceDir.entryList(QStringList() << "temp*_input", QDir::Files);
                if (!tempFiles.isEmpty()) {
                    m_gpuTempPath = basePath + "/" + tempFiles.first();
                    qDebug() << "Found GPU temp at:" << m_gpuTempPath;
                }
                // APU/GPU power
                if (deviceDir.exists("power1_input")) {
                    m_apuPowerPath = basePath + "/power1_input";
                    qDebug() << "Found APU power at:" << m_apuPowerPath;
                } else if (deviceDir.exists("power1_average")) {
                    m_apuPowerPath = basePath + "/power1_average";
                    qDebug() << "Found APU power (average) at:" << m_apuPowerPath;
                }
            }

            // ASUS WMI for fan speeds
            if (name == "asus-nb-wmi" || name == "asus_fan" || name == "asus") {
                QDir deviceDir(basePath);

                // Look for fan inputs
                QStringList fanFiles = deviceDir.entryList(QStringList() << "fan*_input", QDir::Files);
                for (const QString &fanFile : fanFiles) {
                    QString fanPath = basePath + "/" + fanFile;
                    if (fanFile.contains("1")) {
                        m_cpuFanPath = fanPath;
                        qDebug() << "Found CPU fan at:" << m_cpuFanPath;
                    } else if (fanFile.contains("2")) {
                        m_gpuFanPath = fanPath;
                        qDebug() << "Found GPU fan at:" << m_gpuFanPath;
                    }
                }

                // Alternative naming
                if (m_cpuFanPath.isEmpty() && deviceDir.exists("pwm1")) {
                    m_cpuFanPath = basePath + "/pwm1";
                }
            }
        }
    }

    m_available = !m_cpuTempPath.isEmpty() || !m_gpuTempPath.isEmpty();
    emit availableChanged(m_available);

    // Find backlight device
    QDir backlightDir("/sys/class/backlight");
    QStringList backlightDevices = backlightDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &device : backlightDevices) {
        QString basePath = "/sys/class/backlight/" + device;
        // Prefer amdgpu backlight over nvidia
        if (device.startsWith("amdgpu") || m_backlightPath.isEmpty()) {
            m_backlightPath = basePath;
            QFile maxFile(basePath + "/max_brightness");
            if (maxFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&maxFile);
                m_maxBrightness = in.readLine().toInt();
                maxFile.close();
                qDebug() << "Found backlight at:" << m_backlightPath << "max:" << m_maxBrightness;
            }
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

                qint64 totalTime = user + nice + system + idle + iowait;
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
    // Try AMD GPU usage
    QFile amdFile("/sys/class/drm/card0/device/gpu_busy_percent");
    if (amdFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&amdFile);
        double usage = in.readLine().toDouble();
        if (qAbs(m_gpuUsage - usage) > 0.5) {
            m_gpuUsage = usage;
            emit gpuUsageChanged(usage);
        }
        amdFile.close();
        return;
    }

    // Try NVIDIA GPU usage via nvidia-smi (would need parsing)
    // For now, leave as 0 if not AMD
}

void SystemMonitor::readMemoryInfo()
{
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        qint64 memTotal = 0, memAvailable = 0;

        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith("MemTotal:")) {
                memTotal = line.split(' ', Qt::SkipEmptyParts)[1].toLongLong();
            } else if (line.startsWith("MemAvailable:")) {
                memAvailable = line.split(' ', Qt::SkipEmptyParts)[1].toLongLong();
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
    // Check if on battery
    QFile acFile(QString("%1/online").arg(AC_PATH));
    if (acFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&acFile);
        bool pluggedIn = (in.readLine().trimmed() == "1");
        acFile.close();

        bool onBattery = !pluggedIn;
        if (m_onBattery != onBattery) {
            m_onBattery = onBattery;
            emit onBatteryChanged(onBattery);
        }
    }

    // Read battery discharge power
    if (m_onBattery) {
        // Try power_now first
        QFile powerFile(QString("%1/power_now").arg(BATTERY_PATH));
        if (powerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&powerFile);
            double power = in.readLine().toDouble() / 1000000.0; // µW to W
            powerFile.close();

            if (qAbs(m_batteryPower - power) > 0.1) {
                m_batteryPower = power;
                emit batteryPowerChanged(power);
            }
            return;
        }

        // Fallback to current_now * voltage_now
        QFile currentFile(QString("%1/current_now").arg(BATTERY_PATH));
        QFile voltageFile(QString("%1/voltage_now").arg(BATTERY_PATH));
        if (currentFile.open(QIODevice::ReadOnly | QIODevice::Text) &&
            voltageFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream currentIn(&currentFile);
            QTextStream voltageIn(&voltageFile);
            double current = currentIn.readLine().toDouble() / 1000000.0; // µA to A
            double voltage = voltageIn.readLine().toDouble() / 1000000.0; // µV to V
            double power = current * voltage;
            currentFile.close();
            voltageFile.close();

            if (qAbs(m_batteryPower - power) > 0.1) {
                m_batteryPower = power;
                emit batteryPowerChanged(power);
            }
        }
    } else {
        // On AC, battery power reading is not useful
        if (m_batteryPower != 0.0) {
            m_batteryPower = 0.0;
            emit batteryPowerChanged(0.0);
        }
    }
}

void SystemMonitor::calculateSystemPower()
{
    double systemPower = 0.0;

    if (m_onBattery && m_batteryPower > 0.1) {
        // On battery: use battery discharge as base (includes everything except display)
        // Add display power estimate
        systemPower = m_batteryPower + m_displayPower;
    } else {
        // On AC: estimate from components
        // APU power (CPU + iGPU) + Display + Misc (SSD, WiFi, RAM, fans, etc.)
        systemPower = m_apuPower + m_displayPower + MISC_POWER_ESTIMATE;
    }

    if (qAbs(m_systemPower - systemPower) > 0.1) {
        m_systemPower = systemPower;
        emit systemPowerChanged(systemPower);
    }
}

#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <QObject>
#include <QTimer>
#include <QMap>

class SystemMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int cpuTemp READ cpuTemp NOTIFY cpuTempChanged)
    Q_PROPERTY(int gpuTemp READ gpuTemp NOTIFY gpuTempChanged)
    Q_PROPERTY(int cpuFanRpm READ cpuFanRpm NOTIFY cpuFanRpmChanged)
    Q_PROPERTY(int gpuFanRpm READ gpuFanRpm NOTIFY gpuFanRpmChanged)
    Q_PROPERTY(int cpuFanPercent READ cpuFanPercent NOTIFY cpuFanPercentChanged)
    Q_PROPERTY(int gpuFanPercent READ gpuFanPercent NOTIFY gpuFanPercentChanged)
    Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY cpuUsageChanged)
    Q_PROPERTY(double gpuUsage READ gpuUsage NOTIFY gpuUsageChanged)
    Q_PROPERTY(double dgpuUsage READ dgpuUsage NOTIFY dgpuUsageChanged)
    Q_PROPERTY(int dgpuTemp READ dgpuTemp NOTIFY dgpuTempChanged)
    Q_PROPERTY(int memoryUsed READ memoryUsed NOTIFY memoryChanged)
    Q_PROPERTY(int memoryTotal READ memoryTotal NOTIFY memoryChanged)
    Q_PROPERTY(double apuPower READ apuPower NOTIFY apuPowerChanged)
    Q_PROPERTY(double systemPower READ systemPower NOTIFY systemPowerChanged)
    Q_PROPERTY(double displayPower READ displayPower NOTIFY displayPowerChanged)
    Q_PROPERTY(double batteryPower READ batteryPower NOTIFY batteryPowerChanged)
    Q_PROPERTY(int displayBrightness READ displayBrightness NOTIFY displayBrightnessChanged)
    Q_PROPERTY(bool onBattery READ isOnBattery NOTIFY onBatteryChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    explicit SystemMonitor(QObject *parent = nullptr);
    ~SystemMonitor() override;

    int cpuTemp() const { return m_cpuTemp; }
    int gpuTemp() const { return m_gpuTemp; }
    int cpuFanRpm() const { return m_cpuFanRpm; }
    int gpuFanRpm() const { return m_gpuFanRpm; }
    int cpuFanPercent() const { return m_cpuFanPercent; }
    int gpuFanPercent() const { return m_gpuFanPercent; }
    double cpuUsage() const { return m_cpuUsage; }
    double gpuUsage() const { return m_gpuUsage; }
    double dgpuUsage() const { return m_dgpuUsage; }
    int dgpuTemp() const { return m_dgpuTemp; }
    int memoryUsed() const { return m_memoryUsed; }
    int memoryTotal() const { return m_memoryTotal; }
    double apuPower() const { return m_apuPower; }
    double systemPower() const { return m_systemPower; }
    double displayPower() const { return m_displayPower; }
    double batteryPower() const { return m_batteryPower; }
    int displayBrightness() const { return m_displayBrightness; }
    bool isOnBattery() const { return m_onBattery; }
    bool isAvailable() const { return m_available; }

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void setUpdateInterval(int msec);

signals:
    void cpuTempChanged(int temp);
    void gpuTempChanged(int temp);
    void cpuFanRpmChanged(int rpm);
    void gpuFanRpmChanged(int rpm);
    void cpuFanPercentChanged(int percent);
    void gpuFanPercentChanged(int percent);
    void cpuUsageChanged(double usage);
    void gpuUsageChanged(double usage);
    void dgpuUsageChanged(double usage);
    void dgpuTempChanged(int temp);
    void memoryChanged();
    void apuPowerChanged(double power);
    void systemPowerChanged(double power);
    void displayPowerChanged(double power);
    void batteryPowerChanged(double power);
    void displayBrightnessChanged(int brightness);
    void onBatteryChanged(bool onBattery);
    void availableChanged(bool available);

private slots:
    void update();

private:
    void findHwmonPaths();
    int readTemperature(const QString &path);
    int readFanSpeed(const QString &path);
    void readCpuUsage();
    void readGpuUsage();
    void readDgpuInfo();
    void readMemoryInfo();
    void readApuPower();
    void readDisplayBrightness();
    void readBatteryPower();
    void calculateSystemPower();

    QTimer *m_updateTimer;
    bool m_available = false;

    // Hwmon paths
    QString m_cpuTempPath;
    QString m_gpuTempPath;
    QString m_cpuFanPath;
    QString m_gpuFanPath;
    QString m_apuPowerPath;
    QString m_backlightPath;
    int m_maxBrightness = 0;

    // Cached values
    int m_cpuTemp = 0;
    int m_gpuTemp = 0;
    int m_cpuFanRpm = 0;
    int m_gpuFanRpm = 0;
    int m_cpuFanPercent = 0;
    int m_gpuFanPercent = 0;
    double m_cpuUsage = 0.0;
    double m_gpuUsage = 0.0;
    double m_dgpuUsage = 0.0;
    int m_dgpuTemp = 0;
    int m_memoryUsed = 0;
    int m_memoryTotal = 0;
    double m_apuPower = 0.0;
    double m_systemPower = 0.0;
    double m_displayPower = 0.0;
    double m_batteryPower = 0.0;
    int m_displayBrightness = 0;
    bool m_onBattery = false;

    // For CPU usage calculation
    qint64 m_prevIdleTime = 0;
    qint64 m_prevTotalTime = 0;

    static constexpr int MAX_FAN_RPM = 6000; // Approximate max RPM for percentage calculation
    static constexpr double MAX_DISPLAY_POWER = 15.0; // Max display power in watts at 100% brightness
    static constexpr double MIN_DISPLAY_POWER = 2.0;  // Min display power in watts at 0% brightness
    static constexpr double MISC_POWER_ESTIMATE = 5.0; // Estimated power for SSD, WiFi, RAM, etc.
    static constexpr const char* BATTERY_PATH = "/sys/class/power_supply/BAT1";
    static constexpr const char* AC_PATH = "/sys/class/power_supply/ACAD";
};

#endif // SYSTEMMONITOR_H

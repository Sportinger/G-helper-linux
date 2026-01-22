#ifndef BATTERYCONTROLLER_H
#define BATTERYCONTROLLER_H

#include <QObject>
#include <QTimer>

class AsusdClient;

class BatteryController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int chargeLimit READ chargeLimit NOTIFY chargeLimitChanged)
    Q_PROPERTY(int currentCharge READ currentCharge NOTIFY currentChargeChanged)
    Q_PROPERTY(bool isCharging READ isCharging NOTIFY isChargingChanged)
    Q_PROPERTY(bool isPluggedIn READ isPluggedIn NOTIFY isPluggedInChanged)
    Q_PROPERTY(QString timeRemaining READ timeRemaining NOTIFY timeRemainingChanged)
    Q_PROPERTY(double powerDraw READ powerDraw NOTIFY powerDrawChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    explicit BatteryController(AsusdClient *client, QObject *parent = nullptr);
    ~BatteryController() override;

    int chargeLimit() const { return m_chargeLimit; }
    int currentCharge() const { return m_currentCharge; }
    bool isCharging() const { return m_isCharging; }
    bool isPluggedIn() const { return m_isPluggedIn; }
    QString timeRemaining() const { return m_timeRemaining; }
    double powerDraw() const { return m_powerDraw; }
    bool isAvailable() const { return m_available; }

    Q_INVOKABLE void setChargeLimit(int limit);
    Q_INVOKABLE void refresh();

signals:
    void chargeLimitChanged(int limit);
    void currentChargeChanged(int charge);
    void isChargingChanged(bool charging);
    void isPluggedInChanged(bool pluggedIn);
    void timeRemainingChanged(const QString &time);
    void powerDrawChanged(double power);
    void availableChanged(bool available);
    void errorOccurred(const QString &error);

private slots:
    void onChargeLimitChanged(quint8 limit);
    void onClientConnected(bool connected);
    void updateBatteryStatus();

private:
    void readBatteryInfo();

    AsusdClient *m_client;
    QTimer *m_updateTimer;

    int m_chargeLimit = 100;
    int m_currentCharge = 0;
    bool m_isCharging = false;
    bool m_isPluggedIn = false;
    QString m_timeRemaining;
    double m_powerDraw = 0.0;
    bool m_available = false;

    static constexpr const char* BATTERY_PATH = "/sys/class/power_supply/BAT0";
    static constexpr const char* AC_PATH = "/sys/class/power_supply/AC0";
};

#endif // BATTERYCONTROLLER_H

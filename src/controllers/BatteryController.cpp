#include "BatteryController.h"
#include "AsusdClient.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>

BatteryController::BatteryController(AsusdClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_updateTimer(new QTimer(this))
{
    connect(m_client, &AsusdClient::chargeLimitChanged,
            this, &BatteryController::onChargeLimitChanged);
    connect(m_client, &AsusdClient::connectedChanged,
            this, &BatteryController::onClientConnected);
    connect(m_client, &AsusdClient::errorOccurred,
            this, &BatteryController::errorOccurred);

    m_updateTimer->setInterval(5000); // Update every 5 seconds
    connect(m_updateTimer, &QTimer::timeout, this, &BatteryController::updateBatteryStatus);

    m_available = m_client->isConnected();
    if (m_available) {
        m_chargeLimit = m_client->chargeLimit();
    }

    // Check if battery exists
    if (QDir(BATTERY_PATH).exists()) {
        m_updateTimer->start();
        updateBatteryStatus();
    }
}

BatteryController::~BatteryController() = default;

void BatteryController::setChargeLimit(int limit)
{
    if (!m_available) {
        emit errorOccurred(tr("Battery control is not available"));
        return;
    }

    if (limit < 20 || limit > 100) {
        emit errorOccurred(tr("Charge limit must be between 20 and 100"));
        return;
    }

    m_client->setChargeLimit(static_cast<quint8>(limit));
}

void BatteryController::refresh()
{
    if (m_available) {
        m_client->refresh();
    }
    updateBatteryStatus();
}

void BatteryController::onChargeLimitChanged(quint8 limit)
{
    int newLimit = static_cast<int>(limit);
    if (m_chargeLimit != newLimit) {
        m_chargeLimit = newLimit;
        emit chargeLimitChanged(newLimit);
    }
}

void BatteryController::onClientConnected(bool connected)
{
    if (m_available != connected) {
        m_available = connected;
        emit availableChanged(connected);

        if (connected) {
            m_chargeLimit = m_client->chargeLimit();
            emit chargeLimitChanged(m_chargeLimit);
        }
    }
}

void BatteryController::updateBatteryStatus()
{
    readBatteryInfo();
}

void BatteryController::readBatteryInfo()
{
    // Read current charge
    QFile capacityFile(QString("%1/capacity").arg(BATTERY_PATH));
    if (capacityFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&capacityFile);
        int charge = in.readLine().toInt();
        if (m_currentCharge != charge) {
            m_currentCharge = charge;
            emit currentChargeChanged(charge);
        }
        capacityFile.close();
    }

    // Read charging status
    QFile statusFile(QString("%1/status").arg(BATTERY_PATH));
    if (statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&statusFile);
        QString status = in.readLine().trimmed();
        bool charging = (status == "Charging");
        bool pluggedIn = (status == "Charging" || status == "Not charging" || status == "Full");

        if (m_isCharging != charging) {
            m_isCharging = charging;
            emit isChargingChanged(charging);
        }
        if (m_isPluggedIn != pluggedIn) {
            m_isPluggedIn = pluggedIn;
            emit isPluggedInChanged(pluggedIn);
        }
        statusFile.close();
    }

    // Try to read AC status directly
    QFile acOnlineFile(QString("%1/online").arg(AC_PATH));
    if (acOnlineFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&acOnlineFile);
        bool online = (in.readLine().trimmed() == "1");
        if (m_isPluggedIn != online) {
            m_isPluggedIn = online;
            emit isPluggedInChanged(online);
        }
        acOnlineFile.close();
    }

    // Read power draw (current_now * voltage_now / 1000000 for watts)
    QFile currentFile(QString("%1/current_now").arg(BATTERY_PATH));
    QFile voltageFile(QString("%1/voltage_now").arg(BATTERY_PATH));
    if (currentFile.open(QIODevice::ReadOnly | QIODevice::Text) &&
        voltageFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream currentIn(&currentFile);
        QTextStream voltageIn(&voltageFile);
        double current = currentIn.readLine().toDouble() / 1000000.0; // uA to A
        double voltage = voltageIn.readLine().toDouble() / 1000000.0; // uV to V
        double power = current * voltage;
        if (qAbs(m_powerDraw - power) > 0.1) {
            m_powerDraw = power;
            emit powerDrawChanged(power);
        }
        currentFile.close();
        voltageFile.close();
    }

    // Calculate time remaining
    QFile energyNowFile(QString("%1/energy_now").arg(BATTERY_PATH));
    QFile energyFullFile(QString("%1/energy_full").arg(BATTERY_PATH));
    QFile powerNowFile(QString("%1/power_now").arg(BATTERY_PATH));

    if (powerNowFile.open(QIODevice::ReadOnly | QIODevice::Text) &&
        energyNowFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream energyIn(&energyNowFile);
        QTextStream powerIn(&powerNowFile);
        double energyNow = energyIn.readLine().toDouble() / 1000000.0; // uWh to Wh
        double powerNow = powerIn.readLine().toDouble() / 1000000.0;   // uW to W

        QString timeStr;
        if (powerNow > 0.1) {
            double hours = energyNow / powerNow;
            int h = static_cast<int>(hours);
            int m = static_cast<int>((hours - h) * 60);
            if (m_isCharging) {
                timeStr = tr("%1h %2m until full").arg(h).arg(m);
            } else {
                timeStr = tr("%1h %2m remaining").arg(h).arg(m);
            }
            m_powerDraw = powerNow;
            emit powerDrawChanged(powerNow);
        } else {
            timeStr = m_isPluggedIn ? tr("Fully charged") : tr("Calculating...");
        }

        if (m_timeRemaining != timeStr) {
            m_timeRemaining = timeStr;
            emit timeRemainingChanged(timeStr);
        }

        energyNowFile.close();
        powerNowFile.close();
    }
}

#include "BatteryController.h"
#include "AsusdClient.h"
#include "PowerSupply.h"
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

    m_batteryPath = PowerSupply::batteryPath();
    if (!m_batteryPath.isEmpty()) {
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
    const QString bat = m_batteryPath;

    const double capacity = PowerSupply::readNumber(bat, "capacity");
    if (capacity >= 0) {
        const int charge = static_cast<int>(capacity);
        if (m_currentCharge != charge) {
            m_currentCharge = charge;
            emit currentChargeChanged(charge);
        }
    }

    const QString status = PowerSupply::readAttribute(bat, "status");
    const bool charging = (status == "Charging");
    if (m_isCharging != charging) {
        m_isCharging = charging;
        emit isChargingChanged(charging);
    }

    const bool pluggedIn = PowerSupply::isOnAc();
    if (m_isPluggedIn != pluggedIn) {
        m_isPluggedIn = pluggedIn;
        emit isPluggedInChanged(pluggedIn);
    }

    // Batteries report either energy (uWh/uW) or charge (uAh/uA) values.
    // Ratios are the same in both cases, so the time calculation works for both.
    double now = PowerSupply::readNumber(bat, "energy_now");
    double full = PowerSupply::readNumber(bat, "energy_full");
    double rate = PowerSupply::readNumber(bat, "power_now");
    double powerWatts = rate / 1e6;
    if (now < 0 || full < 0 || rate < 0) {
        now = PowerSupply::readNumber(bat, "charge_now");
        full = PowerSupply::readNumber(bat, "charge_full");
        rate = PowerSupply::readNumber(bat, "current_now");
        const double voltage = PowerSupply::readNumber(bat, "voltage_now");
        powerWatts = (rate >= 0 && voltage >= 0) ? (rate / 1e6) * (voltage / 1e6) : 0.0;
    }
    rate = qAbs(rate);
    powerWatts = qAbs(powerWatts);

    if (qAbs(m_powerDraw - powerWatts) > 0.1) {
        m_powerDraw = powerWatts;
        emit powerDrawChanged(powerWatts);
    }

    QString timeStr;
    if (now >= 0 && full > 0 && rate > 0 && powerWatts > 0.1) {
        double hours = -1;
        if (charging) {
            // Charging stops at the charge limit, not at 100 %
            const double target = full * qBound(20, m_chargeLimit, 100) / 100.0;
            if (target > now)
                hours = (target - now) / rate;
        } else if (status == "Discharging") {
            hours = now / rate;
        }

        if (hours >= 0) {
            const int totalMinutes = static_cast<int>(hours * 60);
            const int h = totalMinutes / 60;
            const int m = totalMinutes % 60;
            timeStr = charging ? tr("%1h %2m until full").arg(h).arg(m)
                               : tr("%1h %2m remaining").arg(h).arg(m);
        }
    }
    if (timeStr.isEmpty()) {
        if (status == "Full" || status == "Not charging")
            timeStr = tr("Fully charged");
        else if (!pluggedIn)
            timeStr = tr("Calculating...");
    }

    if (m_timeRemaining != timeStr) {
        m_timeRemaining = timeStr;
        emit timeRemainingChanged(timeStr);
    }
}

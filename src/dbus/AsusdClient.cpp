#include "AsusdClient.h"
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QDebug>
#include <QRegularExpression>

AsusdClient::AsusdClient(QObject *parent)
    : QObject(parent)
{
    registerDBusTypes();
    setupConnections();
}

AsusdClient::~AsusdClient() = default;

void AsusdClient::setupConnections()
{
    QDBusConnection bus = QDBusConnection::systemBus();

    m_platformInterface = new QDBusInterface(
        SERVICE, PATH_PLATFORM, "org.freedesktop.DBus.Properties", bus, this);

    m_connected = m_platformInterface->isValid();

    if (m_connected) {
        // Connect to PropertiesChanged signal
        bus.connect(SERVICE, PATH_PLATFORM, "org.freedesktop.DBus.Properties",
                    "PropertiesChanged", this, SLOT(onPropertiesChanged(QString, QVariantMap, QStringList)));

        refresh();
    }
}

void AsusdClient::refresh()
{
    if (!m_connected) return;

    fetchPlatformProfile();
    fetchChargeLimit();
    fetchLedBrightness();
}

void AsusdClient::fetchPlatformProfile()
{
    QDBusPendingCall call = m_platformInterface->asyncCall("Get", INTERFACE_PLATFORM, "PlatformProfile");
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &AsusdClient::onPlatformProfileResult);
}

void AsusdClient::onPlatformProfileResult(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "Failed to get platform profile:" << reply.error().message();
    } else {
        quint32 profile = reply.value().variant().toUInt();
        if (m_platformProfile != profile) {
            m_platformProfile = profile;
            emit platformProfileChanged(profile);
        }
    }
    watcher->deleteLater();
}

void AsusdClient::setPlatformProfile(quint32 profile)
{
    if (!m_connected) {
        qWarning() << "AsusdClient: Not connected, cannot set profile";
        return;
    }

    qDebug() << "AsusdClient: Setting platform profile to" << profile;

    // Use QDBusInterface for cleaner property setting
    QDBusInterface iface(SERVICE, PATH_PLATFORM, INTERFACE_PLATFORM, QDBusConnection::systemBus());
    if (!iface.isValid()) {
        qWarning() << "AsusdClient: Interface not valid:" << iface.lastError().message();
        return;
    }

    // Set property directly
    bool success = iface.setProperty("PlatformProfile", QVariant::fromValue(profile));
    if (!success) {
        qWarning() << "AsusdClient: Failed to set PlatformProfile property";
        emit errorOccurred(tr("Failed to set performance profile"));
    } else {
        qDebug() << "AsusdClient: Profile set successfully";
        m_platformProfile = profile;
        emit platformProfileChanged(profile);
    }
}

void AsusdClient::onPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated)
{
    Q_UNUSED(invalidated)

    if (interface == INTERFACE_PLATFORM) {
        if (changed.contains("PlatformProfile")) {
            quint32 profile = changed["PlatformProfile"].toUInt();
            if (m_platformProfile != profile) {
                m_platformProfile = profile;
                emit platformProfileChanged(profile);
            }
        }
        if (changed.contains("ChargeControlEndThreshold")) {
            quint8 limit = static_cast<quint8>(changed["ChargeControlEndThreshold"].toUInt());
            if (m_chargeLimit != limit) {
                m_chargeLimit = limit;
                emit chargeLimitChanged(limit);
            }
        }
    } else if (interface == INTERFACE_AURA) {
        if (changed.contains("Brightness")) {
            quint32 brightness = changed["Brightness"].toUInt();
            if (m_ledBrightness != brightness) {
                m_ledBrightness = brightness;
                emit ledBrightnessChanged(brightness);
            }
        }
    }
}

void AsusdClient::fetchChargeLimit()
{
    if (!m_connected) return;

    QDBusPendingCall call = m_platformInterface->asyncCall("Get", INTERFACE_PLATFORM, "ChargeControlEndThreshold");
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &AsusdClient::onChargeLimitResult);
}

void AsusdClient::onChargeLimitResult(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "Failed to get charge limit:" << reply.error().message();
    } else {
        quint8 limit = static_cast<quint8>(reply.value().variant().toUInt());
        if (m_chargeLimit != limit) {
            m_chargeLimit = limit;
            emit chargeLimitChanged(limit);
        }
    }
    watcher->deleteLater();
}

void AsusdClient::setChargeLimit(quint8 limit)
{
    if (!m_connected) return;

    QDBusMessage msg = QDBusMessage::createMethodCall(
        SERVICE, PATH_PLATFORM, "org.freedesktop.DBus.Properties", "Set");
    msg << INTERFACE_PLATFORM << "ChargeControlEndThreshold" << QVariant::fromValue(QDBusVariant(static_cast<uchar>(limit)));

    QDBusPendingCall call = QDBusConnection::systemBus().asyncCall(msg);
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, limit](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<> reply = *w;
        if (reply.isError()) {
            qWarning() << "Failed to set charge limit:" << reply.error().message();
            emit errorOccurred(tr("Failed to set charge limit: %1").arg(reply.error().message()));
        } else {
            m_chargeLimit = limit;
            emit chargeLimitChanged(limit);
        }
        w->deleteLater();
    });
}

void AsusdClient::findAuraDevice()
{
    // Find the aura device path dynamically
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "ListNames");

    QDBusConnection bus = QDBusConnection::systemBus();

    // Try to introspect to find aura path
    QDBusMessage introMsg = QDBusMessage::createMethodCall(
        SERVICE, "/xyz/ljones/aura", "org.freedesktop.DBus.Introspectable", "Introspect");

    QDBusReply<QString> introReply = bus.call(introMsg);
    if (introReply.isValid()) {
        QString xml = introReply.value();
        // Parse to find child nodes like "19b6_3_4"
        QRegularExpression re("node name=\"([^\"]+)\"");
        QRegularExpressionMatch match = re.match(xml);
        if (match.hasMatch()) {
            m_auraPath = "/xyz/ljones/aura/" + match.captured(1);
            m_auraInterface = new QDBusInterface(
                SERVICE, m_auraPath, "org.freedesktop.DBus.Properties", bus, this);
        }
    }
}

void AsusdClient::fetchLedBrightness()
{
    if (m_auraPath.isEmpty()) {
        findAuraDevice();
    }
    if (!m_auraInterface || !m_auraInterface->isValid()) return;

    QDBusPendingCall call = m_auraInterface->asyncCall("Get", INTERFACE_AURA, "Brightness");
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &AsusdClient::onLedBrightnessResult);
}

void AsusdClient::onLedBrightnessResult(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QDBusVariant> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "Failed to get LED brightness:" << reply.error().message();
    } else {
        quint32 brightness = reply.value().variant().toUInt();
        if (m_ledBrightness != brightness) {
            m_ledBrightness = brightness;
            emit ledBrightnessChanged(brightness);
        }
    }
    watcher->deleteLater();
}

void AsusdClient::setLedBrightness(quint32 level)
{
    if (!m_connected || m_auraPath.isEmpty()) return;

    QDBusMessage msg = QDBusMessage::createMethodCall(
        SERVICE, m_auraPath, "org.freedesktop.DBus.Properties", "Set");
    msg << INTERFACE_AURA << "Brightness" << QVariant::fromValue(QDBusVariant(level));

    QDBusPendingCall call = QDBusConnection::systemBus().asyncCall(msg);
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, level](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<> reply = *w;
        if (reply.isError()) {
            qWarning() << "Failed to set LED brightness:" << reply.error().message();
            emit errorOccurred(tr("Failed to set LED brightness: %1").arg(reply.error().message()));
        } else {
            m_ledBrightness = level;
            emit ledBrightnessChanged(level);
        }
        w->deleteLater();
    });
}

void AsusdClient::setLedMode(quint32 mode, const QColor &color1, const QColor &color2, quint8 speed)
{
    if (!m_connected || !m_auraInterface) return;

    // This would need to be adapted to the actual asusd API
    QDBusMessage msg = QDBusMessage::createMethodCall(
        SERVICE, "/xyz/ljones/aura", INTERFACE_AURA, "SetLedMode");

    QVariantMap modeData;
    modeData["mode"] = mode;
    modeData["color1"] = QVariant::fromValue(QList<quint8>{
        static_cast<quint8>(color1.red()),
        static_cast<quint8>(color1.green()),
        static_cast<quint8>(color1.blue())
    });
    if (color2.isValid()) {
        modeData["color2"] = QVariant::fromValue(QList<quint8>{
            static_cast<quint8>(color2.red()),
            static_cast<quint8>(color2.green()),
            static_cast<quint8>(color2.blue())
        });
    }
    modeData["speed"] = speed;

    msg << modeData;

    QDBusPendingCall call = QDBusConnection::systemBus().asyncCall(msg);
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<> reply = *w;
        if (reply.isError()) {
            qWarning() << "Failed to set LED mode:" << reply.error().message();
            emit errorOccurred(tr("Failed to set LED mode: %1").arg(reply.error().message()));
        }
        w->deleteLater();
    });
}

QVariantList AsusdClient::getFanCurves(quint32 profile)
{
    QVariantList result;
    // Fan curve retrieval would be implemented based on actual asusd fan curve API
    // For now, return default curves

    // CPU fan default curve
    QVariantMap cpuCurve;
    cpuCurve["fanType"] = 0;
    cpuCurve["enabled"] = true;
    QVariantList cpuPoints;
    cpuPoints << QVariantMap{{"temp", 30}, {"fan", 0}};
    cpuPoints << QVariantMap{{"temp", 40}, {"fan", 20}};
    cpuPoints << QVariantMap{{"temp", 50}, {"fan", 35}};
    cpuPoints << QVariantMap{{"temp", 60}, {"fan", 50}};
    cpuPoints << QVariantMap{{"temp", 70}, {"fan", 70}};
    cpuPoints << QVariantMap{{"temp", 80}, {"fan", 85}};
    cpuPoints << QVariantMap{{"temp", 90}, {"fan", 95}};
    cpuPoints << QVariantMap{{"temp", 100}, {"fan", 100}};
    cpuCurve["points"] = cpuPoints;
    result << cpuCurve;

    // GPU fan default curve
    QVariantMap gpuCurve;
    gpuCurve["fanType"] = 1;
    gpuCurve["enabled"] = true;
    gpuCurve["points"] = cpuPoints; // Same as CPU for default
    result << gpuCurve;

    return result;
}

void AsusdClient::setFanCurve(quint32 profile, quint32 fanType, const QVariantList &points, bool enabled)
{
    if (!m_connected) return;

    // This would send the fan curve to asusd
    // Implementation depends on actual asusd API

    qDebug() << "Setting fan curve for profile" << profile
             << "fan type" << fanType
             << "enabled" << enabled
             << "points" << points.size();

    emit fanCurvesChanged();
}

void AsusdClient::resetFanCurves(quint32 profile)
{
    if (!m_connected) return;

    qDebug() << "Resetting fan curves for profile" << profile;
    emit fanCurvesChanged();
}

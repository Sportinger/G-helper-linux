#include "AsusdClient.h"
#include <QDBusArgument>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDebug>
#include <QRegularExpression>
#include <QXmlStreamReader>

namespace {

// Returns the names of the direct child nodes of an introspected object
QStringList childNodes(const QString &xml)
{
    QStringList nodes;
    QXmlStreamReader reader(xml);
    int depth = 0;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            depth++;
            if (depth == 2 && reader.name() == QLatin1String("node")) {
                const QString name = reader.attributes().value("name").toString();
                if (!name.isEmpty())
                    nodes << name;
            }
        } else if (reader.isEndElement()) {
            depth--;
        }
    }
    return nodes;
}

QString introspect(const QString &service, const QString &path)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        service, path, "org.freedesktop.DBus.Introspectable", "Introspect");
    QDBusReply<QString> reply = QDBusConnection::systemBus().call(msg, QDBus::Block, 2000);
    return reply.isValid() ? reply.value() : QString();
}

int speedFromString(const QString &speed)
{
    if (speed.compare("Low", Qt::CaseInsensitive) == 0) return 0;
    if (speed.compare("High", Qt::CaseInsensitive) == 0) return 2;
    return 1;
}

QString speedToString(int speed)
{
    switch (speed) {
        case 0: return QStringLiteral("Low");
        case 2: return QStringLiteral("High");
        default: return QStringLiteral("Med");
    }
}

QString dbusErrorText(const QString &text, const QDBusError &error)
{
    return error.message().isEmpty() ? text : QStringLiteral("%1: %2").arg(text, error.message());
}

}

AsusdClient::AsusdClient(QObject *parent)
    : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::systemBus();

    m_serviceWatcher = new QDBusServiceWatcher(
        SERVICE, bus,
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
        this);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered,
            this, &AsusdClient::onServiceRegistered);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &AsusdClient::onServiceUnregistered);

    if (bus.interface() && bus.interface()->isServiceRegistered(SERVICE)) {
        connectToService();
    } else {
        qWarning() << "AsusdClient: asusd is not running, waiting for it";
    }
}

AsusdClient::~AsusdClient() = default;

void AsusdClient::onServiceRegistered()
{
    qDebug() << "AsusdClient: asusd appeared on the bus";
    connectToService();
}

void AsusdClient::onServiceUnregistered()
{
    qWarning() << "AsusdClient: asusd disappeared from the bus";
    disconnectFromService();
}

void AsusdClient::connectToService()
{
    if (m_connected)
        return;

    QDBusConnection bus = QDBusConnection::systemBus();
    bus.connect(SERVICE, PATH_PLATFORM, INTERFACE_PROPERTIES, "PropertiesChanged",
                this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));

    findDevices();
    if (!m_auraPath.isEmpty()) {
        bus.connect(SERVICE, m_auraPath, INTERFACE_PROPERTIES, "PropertiesChanged",
                    this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));
    }

    m_connected = true;
    emit connectedChanged(true);

    refresh();
}

void AsusdClient::disconnectFromService()
{
    if (!m_connected)
        return;

    QDBusConnection bus = QDBusConnection::systemBus();
    bus.disconnect(SERVICE, PATH_PLATFORM, INTERFACE_PROPERTIES, "PropertiesChanged",
                   this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));
    if (!m_auraPath.isEmpty()) {
        bus.disconnect(SERVICE, m_auraPath, INTERFACE_PROPERTIES, "PropertiesChanged",
                       this, SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));
    }

    m_auraPath.clear();
    m_slashPath.clear();
    m_connected = false;
    emit connectedChanged(false);
}

void AsusdClient::findDevices()
{
    m_auraPath.clear();
    m_slashPath.clear();
    m_slashModeIsByte = false;

    const QString auraRoot = QStringLiteral("/xyz/ljones/aura");
    const QStringList nodes = childNodes(introspect(SERVICE, auraRoot));

    for (const QString &node : nodes) {
        const QString path = auraRoot + "/" + node;
        const QString xml = introspect(SERVICE, path);

        // The Slash lightbar is either its own "slash" object or an extra
        // interface on the keyboard object (e.g. Zephyrus G14 GA403)
        if (m_slashPath.isEmpty() && xml.contains(QLatin1String("\"xyz.ljones.Slash\""))) {
            m_slashPath = path;
            m_slashModeIsByte = QRegularExpression(
                "<property\\s+name=\"Mode\"\\s+type=\"y\"").match(xml).hasMatch();
        }

        // AniMe matrix and external SCSI devices are not the keyboard
        if (node == QLatin1String("slash") || node == QLatin1String("anime") || node.endsWith(QLatin1String("_scsi")))
            continue;

        if (m_auraPath.isEmpty() && xml.contains(QLatin1String("\"xyz.ljones.Aura\"")))
            m_auraPath = path;
    }

    qDebug() << "AsusdClient: keyboard aura device:" << (m_auraPath.isEmpty() ? "none" : m_auraPath)
             << "slash:" << (m_slashPath.isEmpty() ? "none" : m_slashPath);
}

void AsusdClient::refresh()
{
    if (!m_connected)
        return;

    fetchPlatformProfileChoices();
    fetchPlatformProfile();
    fetchChargeLimit();
    fetchLedState();
}

// --- Generic property helpers --------------------------------------------

void AsusdClient::getProperty(const QString &path, const QString &interface, const QString &name,
                              std::function<void(const QVariant &)> onSuccess)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(SERVICE, path, INTERFACE_PROPERTIES, "Get");
    msg << interface << name;

    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [name, onSuccess](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        const QDBusMessage reply = w->reply();
        if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty()) {
            qWarning() << "AsusdClient: failed to read" << name << ":" << reply.errorMessage();
            return;
        }
        onSuccess(reply.arguments().constFirst().value<QDBusVariant>().variant());
    });
}

void AsusdClient::setProperty(const QString &path, const QString &interface, const QString &name,
                              const QVariant &value, std::function<void()> onSuccess,
                              const QString &errorText, std::function<void()> onError)
{
    if (!m_connected) {
        emit errorOccurred(tr("asusd is not running"));
        if (onError) onError();
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(SERVICE, path, INTERFACE_PROPERTIES, "Set");
    msg << interface << name << QVariant::fromValue(QDBusVariant(value));

    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, name, onSuccess, onError, errorText](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        QDBusPendingReply<> reply = *w;
        if (reply.isError()) {
            qWarning() << "AsusdClient: failed to set" << name << ":" << reply.error().message();
            emit errorOccurred(dbusErrorText(errorText, reply.error()));
            if (onError) onError();
            return;
        }
        if (onSuccess) onSuccess();
    });
}

// --- Platform profile ------------------------------------------------------

int AsusdClient::profileFromDbus(quint32 dbusProfile)
{
    switch (dbusProfile) {
        case AsusdProfile::Balanced: return ProfileBalanced;
        case AsusdProfile::Performance: return ProfilePerformance;
        case AsusdProfile::Quiet: return ProfileQuiet;
        case AsusdProfile::LowPower: return ProfileQuiet;
        default:
            qWarning() << "AsusdClient: unexpected platform profile" << dbusProfile << "- treating as Balanced";
            return ProfileBalanced;
    }
}

quint32 AsusdClient::profileToDbus(int profile) const
{
    switch (profile) {
        case ProfileQuiet:
            // Some models only offer "low-power" instead of "quiet"
            if (!m_profileChoices.isEmpty() && !m_profileChoices.contains(AsusdProfile::Quiet)
                && m_profileChoices.contains(AsusdProfile::LowPower)) {
                return AsusdProfile::LowPower;
            }
            return AsusdProfile::Quiet;
        case ProfilePerformance:
            return AsusdProfile::Performance;
        default:
            return AsusdProfile::Balanced;
    }
}

void AsusdClient::fetchPlatformProfile()
{
    getProperty(PATH_PLATFORM, INTERFACE_PLATFORM, "PlatformProfile", [this](const QVariant &value) {
        const int profile = profileFromDbus(value.toUInt());
        if (m_platformProfile != profile) {
            m_platformProfile = profile;
            emit platformProfileChanged(profile);
        }
    });
}

void AsusdClient::fetchPlatformProfileChoices()
{
    getProperty(PATH_PLATFORM, INTERFACE_PLATFORM, "PlatformProfileChoices", [this](const QVariant &value) {
        m_profileChoices.clear();
        if (value.canConvert<QDBusArgument>()) {
            const QDBusArgument arg = value.value<QDBusArgument>();
            arg.beginArray();
            while (!arg.atEnd()) {
                quint32 choice = 0;
                arg >> choice;
                m_profileChoices << choice;
            }
            arg.endArray();
        } else {
            for (const QVariant &v : value.toList())
                m_profileChoices << v.toUInt();
        }
    });
}

void AsusdClient::setPlatformProfile(int profile)
{
    if (profile < ProfileQuiet || profile > ProfilePerformance) {
        qWarning() << "AsusdClient: invalid profile" << profile;
        return;
    }

    setProperty(PATH_PLATFORM, INTERFACE_PLATFORM, "PlatformProfile",
                QVariant::fromValue(profileToDbus(profile)),
                [this, profile]() {
                    if (m_platformProfile != profile) {
                        m_platformProfile = profile;
                        emit platformProfileChanged(profile);
                    }
                },
                tr("Failed to set performance profile"),
                [this]() {
                    // Re-sync the UI with the real hardware state
                    fetchPlatformProfile();
                });
}

// --- Battery ---------------------------------------------------------------

void AsusdClient::fetchChargeLimit()
{
    getProperty(PATH_PLATFORM, INTERFACE_PLATFORM, "ChargeControlEndThreshold", [this](const QVariant &value) {
        const quint8 limit = static_cast<quint8>(value.toUInt());
        if (m_chargeLimit != limit) {
            m_chargeLimit = limit;
            emit chargeLimitChanged(limit);
        }
    });
}

void AsusdClient::setChargeLimit(quint8 limit)
{
    setProperty(PATH_PLATFORM, INTERFACE_PLATFORM, "ChargeControlEndThreshold",
                QVariant::fromValue(static_cast<uchar>(limit)),
                [this, limit]() {
                    if (m_chargeLimit != limit) {
                        m_chargeLimit = limit;
                        emit chargeLimitChanged(limit);
                    }
                },
                tr("Failed to set charge limit"),
                [this]() { fetchChargeLimit(); });
}

// --- Keyboard LED ----------------------------------------------------------

void AsusdClient::fetchLedState()
{
    if (m_auraPath.isEmpty())
        return;

    getProperty(m_auraPath, INTERFACE_AURA, "Brightness", [this](const QVariant &value) {
        const quint32 brightness = value.toUInt();
        if (m_ledBrightness != brightness) {
            m_ledBrightness = brightness;
            emit ledBrightnessChanged(brightness);
        }
    });

    getProperty(m_auraPath, INTERFACE_AURA, "SupportedBasicModes", [this](const QVariant &value) {
        applySupportedAuraModes(value);
    });

    getProperty(m_auraPath, INTERFACE_AURA, "LedModeData", [this](const QVariant &value) {
        if (applyAuraEffect(value))
            emit auraModeDataChanged();
    });
}

void AsusdClient::applySupportedAuraModes(const QVariant &value)
{
    QList<quint32> modes;
    if (value.canConvert<QDBusArgument>()) {
        const QDBusArgument arg = value.value<QDBusArgument>();
        arg.beginArray();
        while (!arg.atEnd()) {
            quint32 mode = 0;
            arg >> mode;
            modes << mode;
        }
        arg.endArray();
    } else {
        for (const QVariant &v : value.toList())
            modes << v.toUInt();
    }
    std::sort(modes.begin(), modes.end());

    if (m_supportedAuraModes != modes) {
        m_supportedAuraModes = modes;
        emit supportedAuraModesChanged();
    }
}

bool AsusdClient::applyAuraEffect(const QVariant &value)
{
    if (!value.canConvert<QDBusArgument>())
        return false;

    const QDBusArgument arg = value.value<QDBusArgument>();
    if (arg.currentSignature() != QLatin1String("(uu(yyy)(yyy)ss)")) {
        qWarning() << "AsusdClient: unexpected LedModeData signature" << arg.currentSignature();
        return false;
    }

    quint32 mode = 0, zone = 0;
    uchar r1 = 0, g1 = 0, b1 = 0, r2 = 0, g2 = 0, b2 = 0;
    QString speed, direction;

    arg.beginStructure();
    arg >> mode >> zone;
    arg.beginStructure();
    arg >> r1 >> g1 >> b1;
    arg.endStructure();
    arg.beginStructure();
    arg >> r2 >> g2 >> b2;
    arg.endStructure();
    arg >> speed >> direction;
    arg.endStructure();

    m_auraMode = mode;
    m_auraColor1 = QColor(r1, g1, b1);
    m_auraColor2 = QColor(r2, g2, b2);
    m_auraSpeed = speedFromString(speed);
    return true;
}

void AsusdClient::setLedBrightness(quint32 level)
{
    if (m_auraPath.isEmpty()) {
        emit errorOccurred(tr("No keyboard backlight found"));
        return;
    }

    setProperty(m_auraPath, INTERFACE_AURA, "Brightness", QVariant::fromValue(level),
                [this, level]() {
                    if (m_ledBrightness != level) {
                        m_ledBrightness = level;
                        emit ledBrightnessChanged(level);
                    }
                },
                tr("Failed to set keyboard brightness"),
                [this]() { fetchLedState(); });
}

void AsusdClient::setLedMode(quint32 mode, const QColor &color1, const QColor &color2, int speed)
{
    if (m_auraPath.isEmpty()) {
        emit errorOccurred(tr("No keyboard backlight found"));
        return;
    }

    const QColor c2 = color2.isValid() ? color2 : QColor(0, 0, 0);

    // LedModeData has the signature (uu(yyy)(yyy)ss):
    // mode, zone, colour1, colour2, speed, direction
    QDBusArgument effect;
    effect.beginStructure();
    effect << mode << quint32(0);
    effect.beginStructure();
    effect << uchar(color1.red()) << uchar(color1.green()) << uchar(color1.blue());
    effect.endStructure();
    effect.beginStructure();
    effect << uchar(c2.red()) << uchar(c2.green()) << uchar(c2.blue());
    effect.endStructure();
    effect << speedToString(speed) << QStringLiteral("Right");
    effect.endStructure();

    setProperty(m_auraPath, INTERFACE_AURA, "LedModeData", QVariant::fromValue(effect),
                [this, mode, color1, c2, speed]() {
                    m_auraMode = mode;
                    m_auraColor1 = color1;
                    m_auraColor2 = c2;
                    m_auraSpeed = speed;
                    emit auraModeDataChanged();
                },
                tr("Failed to set keyboard effect"),
                [this]() { fetchLedState(); });
}

// --- Change notifications --------------------------------------------------

void AsusdClient::onPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated)
{
    if (interface == QLatin1String(INTERFACE_PLATFORM)) {
        if (changed.contains("PlatformProfile")) {
            const int profile = profileFromDbus(changed.value("PlatformProfile").toUInt());
            if (m_platformProfile != profile) {
                m_platformProfile = profile;
                emit platformProfileChanged(profile);
            }
        } else if (invalidated.contains("PlatformProfile")) {
            fetchPlatformProfile();
        }

        if (changed.contains("ChargeControlEndThreshold")) {
            const quint8 limit = static_cast<quint8>(changed.value("ChargeControlEndThreshold").toUInt());
            if (m_chargeLimit != limit) {
                m_chargeLimit = limit;
                emit chargeLimitChanged(limit);
            }
        } else if (invalidated.contains("ChargeControlEndThreshold")) {
            fetchChargeLimit();
        }
    } else if (interface == QLatin1String(INTERFACE_AURA)) {
        if (changed.contains("Brightness")) {
            const quint32 brightness = changed.value("Brightness").toUInt();
            if (m_ledBrightness != brightness) {
                m_ledBrightness = brightness;
                emit ledBrightnessChanged(brightness);
            }
        }
        if (changed.contains("LedModeData")) {
            if (applyAuraEffect(changed.value("LedModeData")))
                emit auraModeDataChanged();
        }
        if (invalidated.contains("Brightness") || invalidated.contains("LedModeData"))
            fetchLedState();
    }
}

// --- Fan curves ------------------------------------------------------------

void AsusdClient::callFanCurves(const QString &method, const QVariantList &args,
                                std::function<void(const QDBusMessage &)> onSuccess,
                                const QString &errorText)
{
    if (!m_connected) {
        emit errorOccurred(tr("asusd is not running"));
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(SERVICE, PATH_PLATFORM, INTERFACE_FAN_CURVES, method);
    msg.setArguments(args);

    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, method, onSuccess, errorText](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        const QDBusMessage reply = w->reply();
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << "AsusdClient:" << method << "failed:" << reply.errorMessage();
            if (!errorText.isEmpty())
                emit errorOccurred(QStringLiteral("%1: %2").arg(errorText, reply.errorMessage()));
            else
                emit fanCurvesUnavailable();
            return;
        }
        if (onSuccess) onSuccess(reply);
    });
}

namespace {

// asusd sends the 8 curve values either as a fixed struct (yyyyyyyy) or,
// in other zvariant versions, as a byte array (ay). Accept both.
QByteArray readCurveBytes(const QDBusArgument &arg)
{
    QByteArray values;
    if (arg.currentType() == QDBusArgument::StructureType) {
        arg.beginStructure();
        while (!arg.atEnd()) {
            uchar v = 0;
            arg >> v;
            values.append(static_cast<char>(v));
        }
        arg.endStructure();
    } else {
        arg >> values;
    }
    return values;
}

void writeCurveBytes(QDBusArgument &arg, const QByteArray &values, bool asStruct)
{
    if (asStruct) {
        arg.beginStructure();
        for (char v : values)
            arg << static_cast<uchar>(v);
        arg.endStructure();
    } else {
        arg << values;
    }
}

}

void AsusdClient::fetchFanCurves(int profile)
{
    callFanCurves("FanCurveData", {QVariant::fromValue(profileToDbus(profile))},
                  [this, profile](const QDBusMessage &reply) {
        if (reply.arguments().isEmpty() || !reply.arguments().constFirst().canConvert<QDBusArgument>()) {
            emit fanCurvesUnavailable();
            return;
        }

        // a(s(yyyyyyyy)(yyyyyyyy)b): fan, pwm[8], temp[8], enabled
        const QDBusArgument arg = reply.arguments().constFirst().value<QDBusArgument>();
        m_fanCurveStructs = arg.currentSignature().contains(QLatin1String("(yyyyyyyy)"));
        QVariantList curves;

        arg.beginArray();
        while (!arg.atEnd()) {
            QString fanName;
            bool enabled = false;

            arg.beginStructure();
            arg >> fanName;
            const QByteArray pwm = readCurveBytes(arg);
            const QByteArray temp = readCurveBytes(arg);
            arg >> enabled;
            arg.endStructure();

            int fan = FanCpu;
            if (fanName.compare("GPU", Qt::CaseInsensitive) == 0) fan = FanGpu;
            else if (fanName.compare("MID", Qt::CaseInsensitive) == 0) fan = FanMid;
            m_fanNames[fan] = fanName;

            QVariantList points;
            const int count = qMin(pwm.size(), temp.size());
            for (int i = 0; i < count; ++i) {
                // Keep the fan value unrounded so pwm -> % -> pwm is lossless
                points << QVariantMap{
                    {"temp", static_cast<int>(static_cast<uchar>(temp.at(i)))},
                    {"fan", static_cast<uchar>(pwm.at(i)) * 100.0 / 255.0}
                };
            }

            curves << QVariantMap{{"fan", fan}, {"enabled", enabled}, {"points", points}};
        }
        arg.endArray();

        emit fanCurvesReceived(profile, curves);
    }, QString());
}

void AsusdClient::setFanCurve(int profile, int fan, const QVariantList &points, bool enabled)
{
    if (!m_connected) {
        emit errorOccurred(tr("asusd is not running"));
        return;
    }

    if (points.size() != 8) {
        emit errorOccurred(tr("A fan curve needs exactly 8 points"));
        return;
    }

    // asusd rejects curves where temperature or fan speed decreases
    QByteArray pwm, temps;
    int prevTemp = -1, prevPwm = -1;
    for (const QVariant &point : points) {
        const QVariantMap p = point.toMap();
        const int temp = qBound(0, p.value("temp").toInt(), 255);
        const int pwmValue = qBound(0, static_cast<int>(qRound(p.value("fan").toDouble() * 2.55)), 255);
        if (temp < prevTemp || pwmValue < prevPwm) {
            emit errorOccurred(tr("Fan curve must not decrease"));
            return;
        }
        prevTemp = temp;
        prevPwm = pwmValue;
        temps.append(static_cast<char>(temp));
        pwm.append(static_cast<char>(pwmValue));
    }

    // CurveData (s(yyyyyyyy)(yyyyyyyy)b): fan, pwm, temp, enabled
    QDBusArgument curve;
    curve.beginStructure();
    curve << m_fanNames[qBound(0, fan, 2)];
    writeCurveBytes(curve, pwm, m_fanCurveStructs);
    writeCurveBytes(curve, temps, m_fanCurveStructs);
    curve << enabled;
    curve.endStructure();

    callFanCurves("SetFanCurve", {QVariant::fromValue(profileToDbus(profile)), QVariant::fromValue(curve)},
                  [this, profile, enabled](const QDBusMessage &) {
                      // Custom curves always apply to all fans of a profile
                      setFanCurvesEnabled(profile, enabled);
                  },
                  tr("Failed to set fan curve"));
}

void AsusdClient::setFanCurvesEnabled(int profile, bool enabled)
{
    callFanCurves("SetFanCurvesEnabled",
                  {QVariant::fromValue(profileToDbus(profile)), QVariant::fromValue(enabled)},
                  [this, profile](const QDBusMessage &) { fetchFanCurves(profile); },
                  tr("Failed to enable fan curves"));
}

void AsusdClient::resetFanCurves(int profile)
{
    // Restore the factory curves and hand fan control back to the firmware
    callFanCurves("SetCurvesToDefaults", {QVariant::fromValue(profileToDbus(profile))},
                  [this, profile](const QDBusMessage &) { setFanCurvesEnabled(profile, false); },
                  tr("Failed to reset fan curves"));
}

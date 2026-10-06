#include "SuperGfxClient.h"
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QDebug>

SuperGfxClient::SuperGfxClient(QObject *parent)
    : QObject(parent)
    , m_powerPollTimer(new QTimer(this))
{
    // supergfxd only sends NotifyGfxStatus on some transitions, so the power
    // state is polled as well. Reading it does not wake the dGPU.
    m_powerPollTimer->setInterval(2000);
    connect(m_powerPollTimer, &QTimer::timeout, this, &SuperGfxClient::fetchGpuPower);

    QDBusConnection bus = QDBusConnection::systemBus();
    m_serviceWatcher = new QDBusServiceWatcher(
        SERVICE, bus,
        QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
        this);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered,
            this, &SuperGfxClient::onServiceRegistered);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &SuperGfxClient::onServiceUnregistered);

    if (bus.interface() && bus.interface()->isServiceRegistered(SERVICE)) {
        connectToService();
    } else {
        qWarning() << "SuperGfxClient: supergfxd is not running, waiting for it";
    }
}

SuperGfxClient::~SuperGfxClient() = default;

void SuperGfxClient::onServiceRegistered()
{
    qDebug() << "SuperGfxClient: supergfxd appeared on the bus";
    connectToService();
}

void SuperGfxClient::onServiceUnregistered()
{
    qWarning() << "SuperGfxClient: supergfxd disappeared from the bus";
    disconnectFromService();
}

void SuperGfxClient::connectToService()
{
    if (m_connected)
        return;

    QDBusConnection bus = QDBusConnection::systemBus();
    bus.connect(SERVICE, PATH, INTERFACE, "NotifyGfxStatus", this, SLOT(onNotifyGfxStatus(quint32)));
    bus.connect(SERVICE, PATH, INTERFACE, "NotifyGfx", this, SLOT(onNotifyGfx(quint32)));

    m_connected = true;
    emit connectedChanged(true);

    refresh();
    m_powerPollTimer->start();
}

void SuperGfxClient::disconnectFromService()
{
    if (!m_connected)
        return;

    QDBusConnection bus = QDBusConnection::systemBus();
    bus.disconnect(SERVICE, PATH, INTERFACE, "NotifyGfxStatus", this, SLOT(onNotifyGfxStatus(quint32)));
    bus.disconnect(SERVICE, PATH, INTERFACE, "NotifyGfx", this, SLOT(onNotifyGfx(quint32)));

    m_powerPollTimer->stop();
    m_modeKnown = false;
    m_connected = false;
    emit connectedChanged(false);
}

QDBusPendingCallWatcher *SuperGfxClient::call(const QString &method, const QVariantList &args)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(SERVICE, PATH, INTERFACE, method);
    msg.setArguments(args);
    return new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
}

void SuperGfxClient::refresh()
{
    if (!m_connected)
        return;

    fetchCurrentMode();
    fetchSupportedModes();
    fetchGpuPower();
    fetchPendingState();
}

void SuperGfxClient::fetchCurrentMode()
{
    connect(call("Mode"), &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        QDBusPendingReply<quint32> reply = *w;
        if (reply.isError()) {
            qWarning() << "SuperGfxClient: failed to get GPU mode:" << reply.error().message();
            return;
        }
        const int mode = static_cast<int>(reply.value());
        // Always announce the first value so listeners know the real mode
        if (m_currentMode != mode || !m_modeKnown) {
            m_modeKnown = true;
            m_currentMode = mode;
            emit currentModeChanged(mode);
        }
        if (m_switchPending && m_pendingMode == mode)
            clearPending();
    });
}

void SuperGfxClient::fetchSupportedModes()
{
    connect(call("Supported"), &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        QDBusPendingReply<QList<quint32>> reply = *w;
        QList<int> modes;
        if (reply.isError()) {
            qWarning() << "SuperGfxClient: failed to get supported GPU modes:" << reply.error().message();
            modes = {Hybrid, Integrated};
        } else {
            for (quint32 mode : reply.value())
                modes.append(static_cast<int>(mode));
        }
        if (m_supportedModes != modes) {
            m_supportedModes = modes;
            emit supportedModesChanged();
        }
    });
}

void SuperGfxClient::fetchGpuPower()
{
    connect(call("Power"), &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        QDBusPendingReply<quint32> reply = *w;
        if (reply.isError()) {
            qWarning() << "SuperGfxClient: failed to get GPU power status:" << reply.error().message();
            return;
        }
        setGpuPower(reply.value());
    });
}

void SuperGfxClient::fetchPendingState()
{
    connect(call("PendingMode"), &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        QDBusPendingReply<quint32> modeReply = *w;
        if (modeReply.isError())
            return;
        const int mode = static_cast<int>(modeReply.value());
        if (mode == None) {
            clearPending();
            return;
        }

        connect(call("PendingUserAction"), &QDBusPendingCallWatcher::finished, this,
                [this, mode](QDBusPendingCallWatcher *w2) {
            w2->deleteLater();
            QDBusPendingReply<quint32> actionReply = *w2;
            const int action = actionReply.isError() ? int(ActionLogout) : static_cast<int>(actionReply.value());
            if (action == ActionNothing)
                clearPending();
            else
                setPending(mode, action);
        });
    });
}

void SuperGfxClient::setGpuPower(quint32 power)
{
    QString powerStr;
    switch (power) {
        case GfxPower::Active: powerStr = "Active"; break;
        case GfxPower::Suspended: powerStr = "Suspended"; break;
        case GfxPower::Off: powerStr = "Off"; break;
        case GfxPower::AsusDisabled: powerStr = "AsusDisabled"; break;
        case GfxPower::AsusMuxDiscreet: powerStr = "AsusMuxDiscreet"; break;
        default: powerStr = "Unknown"; break;
    }
    if (m_gpuPower != powerStr) {
        m_gpuPower = powerStr;
        emit gpuPowerChanged(powerStr);
    }
}

void SuperGfxClient::setPending(int mode, int action)
{
    const bool modeChanged = (m_pendingMode != mode || m_pendingAction != action);
    m_pendingMode = mode;
    m_pendingAction = action;
    if (modeChanged)
        emit pendingModeChanged(mode);
    if (!m_switchPending) {
        m_switchPending = true;
        emit switchPendingChanged(true);
    }
}

void SuperGfxClient::clearPending()
{
    if (!m_switchPending && m_pendingMode == -1)
        return;
    m_switchPending = false;
    m_pendingMode = -1;
    m_pendingAction = ActionNothing;
    emit pendingModeChanged(-1);
    emit switchPendingChanged(false);
}

void SuperGfxClient::setMode(int mode)
{
    if (!m_connected) {
        emit errorOccurred(tr("supergfxd is not running"));
        return;
    }

    qDebug() << "SuperGfxClient: switching to" << modeName(mode);

    connect(call("SetMode", {QVariant::fromValue(static_cast<quint32>(mode))}),
            &QDBusPendingCallWatcher::finished, this, [this, mode](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        QDBusPendingReply<quint32> reply = *w;
        if (reply.isError()) {
            qWarning() << "SuperGfxClient: failed to set GPU mode:" << reply.error().message();
            emit errorOccurred(tr("Failed to set GPU mode: %1").arg(reply.error().message()));
            fetchCurrentMode();
            return;
        }

        const int action = static_cast<int>(reply.value());
        if (action == ActionNothing) {
            clearPending();
            fetchCurrentMode();
        } else {
            setPending(mode, action);
            emit userActionRequired(mode, action);
        }
        fetchGpuPower();
    });
}

void SuperGfxClient::onNotifyGfxStatus(quint32 status)
{
    setGpuPower(status);
}

void SuperGfxClient::onNotifyGfx(quint32 mode)
{
    Q_UNUSED(mode)
    fetchCurrentMode();
    fetchPendingState();
}

QString SuperGfxClient::modeName(int mode)
{
    switch (mode) {
        case Hybrid: return tr("Hybrid");
        case Integrated: return tr("Integrated");
        case NvidiaNoModeset: return tr("Nvidia (no modeset)");
        case Vfio: return tr("VFIO");
        case AsusEgpu: return tr("eGPU");
        case AsusMuxDgpu: return tr("Dedicated (MUX)");
        default: return tr("Unknown");
    }
}

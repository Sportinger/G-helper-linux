#include "SuperGfxClient.h"
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QDebug>

SuperGfxClient::SuperGfxClient(QObject *parent)
    : QObject(parent)
{
    setupConnections();
}

SuperGfxClient::~SuperGfxClient() = default;

void SuperGfxClient::setupConnections()
{
    QDBusConnection bus = QDBusConnection::systemBus();

    m_interface = new QDBusInterface(SERVICE, PATH, INTERFACE, bus, this);
    m_connected = m_interface->isValid();

    if (m_connected) {
        // Connect to signals
        bus.connect(SERVICE, PATH, INTERFACE,
                    "NotifyGfxStatus", this, SLOT(onNotifyGfxStatus(quint32)));

        refresh();
    }
}

void SuperGfxClient::refresh()
{
    if (!m_connected) return;

    fetchCurrentMode();
    fetchSupportedModes();
    fetchGpuPower();
}

void SuperGfxClient::fetchCurrentMode()
{
    QDBusPendingCall call = m_interface->asyncCall("Mode");
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &SuperGfxClient::onModeResult);
}

void SuperGfxClient::onModeResult(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<quint32> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "Failed to get GPU mode:" << reply.error().message();
    } else {
        int mode = static_cast<int>(reply.value());
        if (m_currentMode != mode) {
            m_currentMode = mode;
            emit currentModeChanged(mode);
        }
        if (m_switchPending && m_pendingMode == mode) {
            m_switchPending = false;
            m_pendingMode = -1;
            emit switchPendingChanged(false);
            emit pendingModeChanged(-1);
        }
    }
    watcher->deleteLater();
}

void SuperGfxClient::fetchSupportedModes()
{
    QDBusPendingCall call = m_interface->asyncCall("Supported");
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &SuperGfxClient::onSupportedModesResult);
}

void SuperGfxClient::onSupportedModesResult(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QList<quint32>> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "Failed to get supported GPU modes:" << reply.error().message();
        // Default to common modes
        m_supportedModes = {Integrated, Hybrid};
    } else {
        m_supportedModes.clear();
        for (quint32 mode : reply.value()) {
            m_supportedModes.append(static_cast<int>(mode));
        }
    }
    emit supportedModesChanged();
    watcher->deleteLater();
}

void SuperGfxClient::fetchGpuPower()
{
    QDBusPendingCall call = m_interface->asyncCall("Power");
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &SuperGfxClient::onPowerResult);
}

void SuperGfxClient::onPowerResult(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<quint32> reply = *watcher;
    if (reply.isError()) {
        qWarning() << "Failed to get GPU power status:" << reply.error().message();
    } else {
        quint32 power = reply.value();
        QString powerStr;
        switch (power) {
            case 0: powerStr = "Active"; break;
            case 1: powerStr = "Suspended"; break;
            case 2: powerStr = "Off"; break;
            case 3: powerStr = "AsusDisabled"; break;
            case 4: powerStr = "AsusMuxDiscreet"; break;
            default: powerStr = "Unknown"; break;
        }
        if (m_gpuPower != powerStr) {
            m_gpuPower = powerStr;
            emit gpuPowerChanged(powerStr);
        }
    }
    watcher->deleteLater();
}

void SuperGfxClient::setMode(int mode)
{
    if (!m_connected) return;

    // Check if logout is required
    bool needsLogout = requiresLogout(m_currentMode, mode);

    QDBusMessage msg = QDBusMessage::createMethodCall(SERVICE, PATH, INTERFACE, "SetMode");
    msg << static_cast<quint32>(mode);

    QDBusPendingCall call = QDBusConnection::systemBus().asyncCall(msg);
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, mode, needsLogout](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<> reply = *w;
        if (reply.isError()) {
            qWarning() << "Failed to set GPU mode:" << reply.error().message();
            emit errorOccurred(tr("Failed to set GPU mode: %1").arg(reply.error().message()));
        } else {
            if (needsLogout) {
                m_pendingMode = mode;
                m_switchPending = true;
                emit pendingModeChanged(mode);
                emit switchPendingChanged(true);
                emit logoutRequired(mode);
            } else {
                m_currentMode = mode;
                emit currentModeChanged(mode);
            }
        }
        w->deleteLater();
    });
}

void SuperGfxClient::onNotifyGfxStatus(quint32 status)
{
    qDebug() << "GPU status notification:" << status;
    fetchCurrentMode();
    fetchGpuPower();
}

QString SuperGfxClient::modeName(int mode) const
{
    switch (mode) {
        case Integrated: return tr("Integrated");
        case Hybrid: return tr("Hybrid");
        case AsusMuxDgpu: return tr("Dedicated");
        case Vfio: return tr("VFIO");
        case Egpu: return tr("eGPU");
        default: return tr("Unknown");
    }
}

QString SuperGfxClient::modeDescription(int mode) const
{
    switch (mode) {
        case Integrated:
            return tr("Uses only the integrated GPU. Best battery life, lowest power consumption.");
        case Hybrid:
            return tr("Automatic GPU switching. dGPU activates when needed.");
        case AsusMuxDgpu:
            return tr("Uses only the discrete GPU. Best performance, highest power consumption.");
        case Vfio:
            return tr("Passes the discrete GPU to a virtual machine.");
        case Egpu:
            return tr("Uses an external GPU connected via Thunderbolt.");
        default:
            return tr("Unknown GPU mode.");
    }
}

bool SuperGfxClient::requiresLogout(int fromMode, int toMode) const
{
    // Switching to/from dedicated (MUX) mode requires logout
    if (fromMode == AsusMuxDgpu || toMode == AsusMuxDgpu) {
        return true;
    }
    // VFIO mode changes typically require logout
    if (fromMode == Vfio || toMode == Vfio) {
        return true;
    }
    return false;
}

#include "GpuController.h"
#include "SuperGfxClient.h"

GpuController::GpuController(SuperGfxClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    connect(m_client, &SuperGfxClient::currentModeChanged,
            this, &GpuController::onModeChanged);
    connect(m_client, &SuperGfxClient::pendingModeChanged,
            this, &GpuController::onPendingModeChanged);
    connect(m_client, &SuperGfxClient::switchPendingChanged,
            this, &GpuController::onSwitchPendingChanged);
    connect(m_client, &SuperGfxClient::gpuPowerChanged,
            this, &GpuController::onGpuPowerChanged);
    connect(m_client, &SuperGfxClient::connectedChanged,
            this, &GpuController::onClientConnected);
    connect(m_client, &SuperGfxClient::supportedModesChanged,
            this, &GpuController::updateSupportedModes);
    connect(m_client, &SuperGfxClient::logoutRequired,
            this, &GpuController::onLogoutRequired);
    connect(m_client, &SuperGfxClient::errorOccurred,
            this, &GpuController::errorOccurred);

    m_available = m_client->isConnected();
    if (m_available) {
        m_currentMode = m_client->currentMode();
        m_gpuPower = m_client->gpuPower();
        updateSupportedModes();
    }
}

GpuController::~GpuController() = default;

QString GpuController::currentModeName() const
{
    return modeName(m_currentMode);
}

void GpuController::setMode(int mode)
{
    if (!m_available) {
        emit errorOccurred(tr("GPU control is not available"));
        return;
    }

    m_client->setMode(mode);
}

QString GpuController::modeName(int mode) const
{
    switch (mode) {
        case Eco: return tr("Eco");
        case Standard: return tr("Standard");
        case Ultimate: return tr("Ultimate");
        case Optimized: return tr("Optimized");
        default: return tr("Unknown");
    }
}

QString GpuController::modeDescription(int mode) const
{
    switch (mode) {
        case Eco:
            return tr("iGPU only. Best battery life, dGPU is powered off.");
        case Standard:
            return tr("Automatic switching between iGPU and dGPU based on demand.");
        case Ultimate:
            return tr("dGPU only via MUX switch. Best gaming performance.");
        case Optimized:
            return tr("dGPU passed to virtual machine for GPU passthrough.");
        default:
            return tr("Unknown GPU mode.");
    }
}

QString GpuController::modeIcon(int mode) const
{
    switch (mode) {
        case Eco: return "qrc:/icons/eco.svg";
        case Standard: return "qrc:/icons/standard.svg";
        case Ultimate: return "qrc:/icons/ultimate.svg";
        case Optimized: return "qrc:/icons/optimized.svg";
        default: return "";
    }
}

bool GpuController::requiresLogout(int mode) const
{
    return m_client->requiresLogout(m_currentMode, mode);
}

void GpuController::refresh()
{
    if (m_available) {
        m_client->refresh();
    }
}

void GpuController::onModeChanged(int mode)
{
    if (m_currentMode != mode) {
        m_currentMode = mode;
        emit currentModeChanged(mode);
    }
}

void GpuController::onPendingModeChanged(int mode)
{
    if (m_pendingMode != mode) {
        m_pendingMode = mode;
        emit pendingModeChanged(mode);
    }
}

void GpuController::onSwitchPendingChanged(bool pending)
{
    if (m_switchPending != pending) {
        m_switchPending = pending;
        emit switchPendingChanged(pending);
    }
}

void GpuController::onGpuPowerChanged(const QString &power)
{
    if (m_gpuPower != power) {
        m_gpuPower = power;
        emit gpuPowerChanged(power);
    }
}

void GpuController::onClientConnected(bool connected)
{
    if (m_available != connected) {
        m_available = connected;
        emit availableChanged(connected);

        if (connected) {
            m_currentMode = m_client->currentMode();
            m_gpuPower = m_client->gpuPower();
            updateSupportedModes();
            emit currentModeChanged(m_currentMode);
            emit gpuPowerChanged(m_gpuPower);
        }
    }
}

void GpuController::onLogoutRequired(int mode)
{
    emit logoutRequired(modeName(mode));
}

void GpuController::updateSupportedModes()
{
    m_supportedModes.clear();
    for (int mode : m_client->supportedModes()) {
        QVariantMap modeInfo;
        modeInfo["mode"] = mode;
        modeInfo["name"] = modeName(mode);
        modeInfo["description"] = modeDescription(mode);
        modeInfo["icon"] = modeIcon(mode);
        modeInfo["requiresLogout"] = requiresLogout(mode);
        m_supportedModes.append(modeInfo);
    }
    emit supportedModesChanged();
}

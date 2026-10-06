#include "GpuController.h"
#include "SuperGfxClient.h"
#include "PowerSupply.h"
#include <QDebug>
#include <QSettings>

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
    connect(m_client, &SuperGfxClient::userActionRequired,
            this, &GpuController::onUserActionRequired);
    connect(m_client, &SuperGfxClient::errorOccurred,
            this, &GpuController::errorOccurred);

    QSettings settings("g-helper-linux", "g-helper-linux");
    m_optimized = settings.value("Gpu/optimized", false).toBool();
    m_onBattery = !PowerSupply::isOnAc();

    m_available = m_client->isConnected();
    if (m_available) {
        m_gpuPower = m_client->gpuPower();
        updateSupportedModes();
    }
}

GpuController::~GpuController() = default;

int GpuController::gfxToUi(int gfxMode)
{
    switch (gfxMode) {
        case SuperGfxClient::Integrated: return Eco;
        case SuperGfxClient::Hybrid: return Standard;
        case SuperGfxClient::AsusMuxDgpu: return Ultimate;
        default: return Unknown;
    }
}

int GpuController::uiToGfx(int uiMode)
{
    switch (uiMode) {
        case Eco: return SuperGfxClient::Integrated;
        case Standard: return SuperGfxClient::Hybrid;
        case Ultimate: return SuperGfxClient::AsusMuxDgpu;
        default: return -1;
    }
}

int GpuController::currentMode() const
{
    if (m_optimized && (m_gfxMode == SuperGfxClient::Integrated || m_gfxMode == SuperGfxClient::Hybrid))
        return Optimized;
    return gfxToUi(m_gfxMode);
}

QString GpuController::currentModeName() const
{
    if (!m_modeKnown)
        return tr("Unknown");

    const int ui = gfxToUi(m_gfxMode);
    if (currentMode() == Optimized)
        return tr("Optimized (%1)").arg(modeName(ui));
    if (ui == Unknown)
        return SuperGfxClient::modeName(m_gfxMode);
    return modeName(ui);
}

int GpuController::pendingMode() const
{
    return gfxToUi(m_client->pendingMode());
}

QString GpuController::pendingText() const
{
    if (!m_switchPending)
        return QString();

    const int pending = m_client->pendingMode();
    const int ui = gfxToUi(pending);
    const QString name = (ui == Unknown) ? SuperGfxClient::modeName(pending) : modeName(ui);
    switch (m_client->pendingAction()) {
        case SuperGfxClient::ActionReboot: return tr("%1 after reboot").arg(name);
        case SuperGfxClient::ActionLogout: return tr("%1 after logout").arg(name);
        default: return tr("%1 pending").arg(name);
    }
}

bool GpuController::isModeSupported(int mode) const
{
    const QList<int> modes = m_client->supportedModes();
    if (mode == Optimized)
        return modes.contains(SuperGfxClient::Integrated) && modes.contains(SuperGfxClient::Hybrid);
    const int gfx = uiToGfx(mode);
    return gfx >= 0 && modes.contains(gfx);
}

void GpuController::setMode(int mode)
{
    if (!m_available) {
        emit errorOccurred(tr("GPU control is not available (supergfxd not running)"));
        return;
    }
    if (!isModeSupported(mode)) {
        emit errorOccurred(tr("%1 mode is not supported on this device").arg(modeName(mode)));
        return;
    }

    if (mode == Optimized) {
        setOptimized(true);
        applyOptimized();
        return;
    }

    setOptimized(false);
    m_client->setMode(uiToGfx(mode));
}

void GpuController::setOptimized(bool optimized)
{
    if (m_optimized == optimized)
        return;
    m_optimized = optimized;
    QSettings settings("g-helper-linux", "g-helper-linux");
    settings.setValue("Gpu/optimized", optimized);
    emit currentModeChanged(currentMode());
}

void GpuController::applyOptimized()
{
    if (!m_optimized || !m_available || !m_modeKnown)
        return;
    // Leaving the MUX mode needs a reboot - never do that automatically
    // and don't stack a second switch on top of a pending one.
    if (m_switchPending)
        return;

    const int target = m_onBattery ? SuperGfxClient::Integrated : SuperGfxClient::Hybrid;
    if (m_gfxMode == target || !m_client->supportedModes().contains(target))
        return;

    qDebug() << "GpuController: Optimized mode ->" << SuperGfxClient::modeName(target)
             << (m_onBattery ? "(on battery)" : "(on AC)");
    m_client->setMode(target);
}

void GpuController::setOnBattery(bool onBattery)
{
    if (m_onBattery == onBattery)
        return;
    m_onBattery = onBattery;
    applyOptimized();
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
            return tr("dGPU only via MUX switch. Best gaming performance. Requires a reboot.");
        case Optimized:
            return tr("Eco on battery, Standard when plugged in.");
        default:
            return tr("Unknown GPU mode.");
    }
}

QString GpuController::confirmationText(int mode) const
{
    const bool toMux = (mode == Ultimate);
    const bool fromMux = (m_gfxMode == SuperGfxClient::AsusMuxDgpu);
    if (toMux && !fromMux)
        return tr("Switching to Ultimate changes the GPU MUX. A reboot is required to apply it.");
    if (fromMux && mode != Ultimate)
        return tr("Leaving Ultimate changes the GPU MUX. A reboot is required to apply it.");
    return QString();
}

QString GpuController::actionText(int gfxMode, int action) const
{
    const int ui = gfxToUi(gfxMode);
    const QString name = (ui == Unknown) ? SuperGfxClient::modeName(gfxMode) : modeName(ui);
    switch (action) {
        case SuperGfxClient::ActionLogout:
            return tr("Log out and back in to finish switching to %1.").arg(name);
        case SuperGfxClient::ActionReboot:
            return tr("Reboot to finish switching to %1.").arg(name);
        case SuperGfxClient::ActionSwitchToIntegrated:
            return tr("Switch to Eco first, then try %1 again.").arg(name);
        case SuperGfxClient::ActionAsusEgpuDisable:
            return tr("Disable the eGPU first, then try %1 again.").arg(name);
        default:
            return QString();
    }
}

void GpuController::refresh()
{
    if (m_available)
        m_client->refresh();
}

void GpuController::onModeChanged(int gfxMode)
{
    m_gfxMode = gfxMode;
    emit currentModeChanged(currentMode());

    if (!m_modeKnown) {
        m_modeKnown = true;
        applyOptimized();
    }
}

void GpuController::onPendingModeChanged(int gfxMode)
{
    emit pendingModeChanged(gfxToUi(gfxMode));
}

void GpuController::onSwitchPendingChanged(bool pending)
{
    if (m_switchPending != pending) {
        m_switchPending = pending;
        emit switchPendingChanged(pending);
        emit pendingModeChanged(pendingMode());
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
    if (!connected) {
        m_modeKnown = false;
        m_gfxMode = -1;
        emit currentModeChanged(currentMode());
    }

    if (m_available != connected) {
        m_available = connected;
        emit availableChanged(connected);
    }

    if (connected) {
        m_gpuPower = m_client->gpuPower();
        updateSupportedModes();
        emit gpuPowerChanged(m_gpuPower);
    }
}

void GpuController::onUserActionRequired(int gfxMode, int action)
{
    const QString text = actionText(gfxMode, action);
    if (!text.isEmpty())
        emit userActionRequired(text);
}

void GpuController::updateSupportedModes()
{
    QVariantList modes;
    for (int mode : {Eco, Standard, Ultimate, Optimized}) {
        if (isModeSupported(mode))
            modes << mode;
    }
    if (m_supportedModes != modes) {
        m_supportedModes = modes;
        emit supportedModesChanged();
    }
}

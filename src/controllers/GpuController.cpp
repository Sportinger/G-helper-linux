#include "GpuController.h"
#include "SuperGfxClient.h"
#include "AsusdClient.h"
#include "PowerSupply.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>
#include <QSettings>

GpuController::GpuController(SuperGfxClient *superGfx, AsusdClient *asusd, QObject *parent)
    : QObject(parent)
    , m_superGfx(superGfx)
    , m_asusd(asusd)
{
    // asus-armoury via asusd (preferred)
    connect(m_asusd, &AsusdClient::connectedChanged, this, &GpuController::updateBackend);
    connect(m_asusd, &AsusdClient::armouryGpuChanged, this, &GpuController::onArmouryChanged);

    // supergfxd (fallback)
    connect(m_superGfx, &SuperGfxClient::connectedChanged, this, &GpuController::updateBackend);
    connect(m_superGfx, &SuperGfxClient::currentModeChanged, this, &GpuController::onSuperGfxModeChanged);
    connect(m_superGfx, &SuperGfxClient::pendingModeChanged, this, &GpuController::onSuperGfxPendingChanged);
    connect(m_superGfx, &SuperGfxClient::switchPendingChanged, this, &GpuController::onSuperGfxPendingChanged);
    connect(m_superGfx, &SuperGfxClient::supportedModesChanged, this, &GpuController::updateSupportedModes);
    connect(m_superGfx, &SuperGfxClient::gpuPowerChanged, this, &GpuController::onGpuPowerChanged);
    connect(m_superGfx, &SuperGfxClient::userActionRequired, this, &GpuController::onUserActionRequired);

    QSettings settings("g-helper-linux", "g-helper-linux");
    m_optimized = settings.value("Gpu/optimized", false).toBool();
    m_onBattery = !PowerSupply::isOnAc();

    updateBackend();
}

GpuController::~GpuController() = default;

// --- Backend selection -------------------------------------------------------

void GpuController::updateBackend()
{
    Backend backend = NoBackend;
    if (m_asusd->isConnected() && m_asusd->hasArmouryGpu())
        backend = ArmouryBackend;
    else if (m_superGfx->isConnected())
        backend = SuperGfxBackend;

    if (backend != m_backend) {
        qDebug() << "GpuController: backend" << (backend == ArmouryBackend ? "asusd (asus-armoury)"
                                                 : backend == SuperGfxBackend ? "supergfxd" : "none");
        m_backend = backend;
        m_modeKnown = (backend == ArmouryBackend && m_asusd->dgpuDisable() >= 0)
                      || (backend == SuperGfxBackend && m_gfxMode >= 0);
        emit availableChanged(isAvailable());
    }

    if (m_backend == SuperGfxBackend)
        onGpuPowerChanged(m_superGfx->gpuPower());
    else
        onGpuPowerChanged(QString());

    updateSupportedModes();
    updatePending();
    emitModeIfChanged();
}

QString GpuController::backendName() const
{
    switch (m_backend) {
        case ArmouryBackend: return QStringLiteral("asusd");
        case SuperGfxBackend: return QStringLiteral("supergfxd");
        default: return QString();
    }
}

// --- Mode mapping --------------------------------------------------------------

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

int GpuController::armouryMode(int dgpuDisable, int mux) const
{
    if (dgpuDisable < 0)
        return Unknown;
    if (m_asusd->hasGpuMux() && mux == 0)
        return Ultimate;
    return dgpuDisable == 1 ? Eco : Standard;
}

int GpuController::hardwareMode() const
{
    switch (m_backend) {
        case ArmouryBackend:
            return armouryMode(m_asusd->dgpuDisable(), m_asusd->gpuMux());
        case SuperGfxBackend:
            return m_modeKnown ? gfxToUi(m_gfxMode) : Unknown;
        default:
            return Unknown;
    }
}

int GpuController::currentMode() const
{
    const int hw = hardwareMode();
    if (m_optimized && (hw == Eco || hw == Standard))
        return Optimized;
    return hw;
}

QString GpuController::currentModeName() const
{
    if (!isAvailable() || !m_modeKnown)
        return tr("Unknown");

    const int hw = hardwareMode();
    if (currentMode() == Optimized)
        return tr("Optimized (%1)").arg(modeName(hw));
    if (hw == Unknown && m_backend == SuperGfxBackend)
        return SuperGfxClient::modeName(m_gfxMode);
    return modeName(hw);
}

int GpuController::pendingMode() const
{
    return m_switchPending ? m_pendingMode : Unknown;
}

QString GpuController::pendingText() const
{
    if (!m_switchPending)
        return QString();

    if (m_backend == ArmouryBackend)
        return tr("%1 after restart").arg(modeName(m_pendingMode));

    const int pending = m_superGfx->pendingMode();
    const int ui = gfxToUi(pending);
    const QString name = (ui == Unknown) ? SuperGfxClient::modeName(pending) : modeName(ui);
    switch (m_superGfx->pendingAction()) {
        case SuperGfxClient::ActionReboot: return tr("%1 after restart").arg(name);
        case SuperGfxClient::ActionLogout: return tr("%1 after logout").arg(name);
        default: return tr("%1 pending").arg(name);
    }
}

bool GpuController::rebootRequired() const
{
    if (!m_switchPending)
        return false;
    if (m_backend == ArmouryBackend)
        return true;
    return m_backend == SuperGfxBackend && m_superGfx->pendingAction() == SuperGfxClient::ActionReboot;
}

bool GpuController::isModeSupported(int mode) const
{
    switch (m_backend) {
        case ArmouryBackend:
            if (mode == Ultimate)
                return m_asusd->hasGpuMux();
            return mode == Eco || mode == Standard || mode == Optimized;
        case SuperGfxBackend: {
            const QList<int> modes = m_superGfx->supportedModes();
            if (mode == Optimized)
                return modes.contains(SuperGfxClient::Integrated) && modes.contains(SuperGfxClient::Hybrid);
            const int gfx = uiToGfx(mode);
            return gfx >= 0 && modes.contains(gfx);
        }
        default:
            return false;
    }
}

// --- Switching -------------------------------------------------------------------

void GpuController::setMode(int mode)
{
    if (!isAvailable()) {
        emit errorOccurred(tr("GPU switching is not available (needs asusd with asus-armoury or supergfxd)"));
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
    setHardwareMode(mode);
}

void GpuController::setHardwareMode(int mode)
{
    if (m_backend == ArmouryBackend) {
        const bool mux = m_asusd->hasGpuMux();
        qDebug() << "GpuController: queueing" << modeName(mode) << "for next restart";
        switch (mode) {
            case Eco:      m_asusd->setGpuAttributes(1, mux ? 1 : -1); break;
            case Standard: m_asusd->setGpuAttributes(0, mux ? 1 : -1); break;
            case Ultimate: m_asusd->setGpuAttributes(0, 0); break;
            default: break;
        }
    } else if (m_backend == SuperGfxBackend) {
        m_superGfx->setMode(uiToGfx(mode));
    }
}

void GpuController::setOptimized(bool optimized)
{
    if (m_optimized == optimized)
        return;
    m_optimized = optimized;
    QSettings settings("g-helper-linux", "g-helper-linux");
    settings.setValue("Gpu/optimized", optimized);
    m_lastEmittedMode = -2;   // force a refresh of the UI
    emitModeIfChanged();
}

void GpuController::applyOptimized()
{
    if (!m_optimized || !isAvailable() || !m_modeKnown)
        return;

    const int target = m_onBattery ? Eco : Standard;

    if (m_backend == ArmouryBackend) {
        // Everything is applied at the next restart, so just make sure the
        // queued (or current) state matches the power source.
        const int effective = m_switchPending ? m_pendingMode : hardwareMode();
        if (effective == target)
            return;
        qDebug() << "GpuController: Optimized ->" << modeName(target) << "on next restart"
                 << (m_onBattery ? "(on battery)" : "(on AC)");
        setHardwareMode(target);
        return;
    }

    // supergfxd switches live; never leave MUX mode automatically and don't
    // stack a second switch on top of a pending one
    if (m_switchPending || hardwareMode() == Ultimate)
        return;
    if (hardwareMode() == target || !isModeSupported(target))
        return;

    qDebug() << "GpuController: Optimized ->" << modeName(target) << (m_onBattery ? "(on battery)" : "(on AC)");
    m_superGfx->setMode(uiToGfx(target));
}

void GpuController::setOnBattery(bool onBattery)
{
    if (m_onBattery == onBattery)
        return;
    m_onBattery = onBattery;
    applyOptimized();
}

void GpuController::rebootNow()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "Reboot");
    msg << true;  // interactive: allow polkit to ask for authentication
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        QDBusPendingReply<> reply = *w;
        if (reply.isError())
            emit errorOccurred(tr("Restart failed: %1").arg(reply.error().message()));
    });
}

// --- Texts ---------------------------------------------------------------------------

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
    QString text;
    switch (mode) {
        case Eco: text = tr("iGPU only. Best battery life, dGPU is powered off."); break;
        case Standard: text = tr("Automatic switching between iGPU and dGPU based on demand."); break;
        case Ultimate: text = tr("dGPU only via MUX switch. Best gaming performance."); break;
        case Optimized: text = tr("Eco on battery, Standard when plugged in."); break;
        default: return tr("Unknown GPU mode.");
    }
    if (m_backend == ArmouryBackend)
        text += " " + tr("Takes effect after a restart.");
    else if (mode == Ultimate)
        text += " " + tr("Requires a restart.");
    return text;
}

QString GpuController::confirmationText(int mode) const
{
    // With asusd nothing happens before the next restart and the change can
    // be undone by clicking the current mode again - no confirmation needed.
    if (m_backend != SuperGfxBackend)
        return QString();

    const bool toMux = (mode == Ultimate);
    const bool fromMux = (m_gfxMode == SuperGfxClient::AsusMuxDgpu);
    if (toMux && !fromMux)
        return tr("Switching to Ultimate changes the GPU MUX. A restart is required to apply it.");
    if (fromMux && mode != Ultimate)
        return tr("Leaving Ultimate changes the GPU MUX. A restart is required to apply it.");
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
            return tr("Restart to finish switching to %1.").arg(name);
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
    if (m_backend == ArmouryBackend)
        m_asusd->fetchArmouryGpu();
    else if (m_backend == SuperGfxBackend)
        m_superGfx->refresh();
}

// --- State updates --------------------------------------------------------------------

void GpuController::emitModeIfChanged()
{
    const int mode = currentMode();
    if (mode != m_lastEmittedMode) {
        m_lastEmittedMode = mode;
        emit currentModeChanged(mode);
    }
}

void GpuController::updatePending()
{
    bool pending = false;
    int pendingMode = Unknown;

    if (m_backend == ArmouryBackend) {
        const int d = m_asusd->dgpuDisableQueued() >= 0 ? m_asusd->dgpuDisableQueued() : m_asusd->dgpuDisable();
        const int m = m_asusd->gpuMuxQueued() >= 0 ? m_asusd->gpuMuxQueued() : m_asusd->gpuMux();
        pendingMode = armouryMode(d, m);
        pending = pendingMode != Unknown && pendingMode != hardwareMode();
    } else if (m_backend == SuperGfxBackend) {
        pending = m_superGfx->switchPending();
        pendingMode = gfxToUi(m_superGfx->pendingMode());
    }

    const bool changed = (pending != m_switchPending) || (pendingMode != m_pendingMode);
    m_switchPending = pending;
    m_pendingMode = pendingMode;
    if (changed) {
        emit switchPendingChanged(pending);
        emit pendingModeChanged(this->pendingMode());
    }
}

void GpuController::onArmouryChanged()
{
    if (m_backend != ArmouryBackend) {
        updateBackend();
        return;
    }

    const bool firstKnown = !m_modeKnown && m_asusd->dgpuDisable() >= 0;
    if (firstKnown)
        m_modeKnown = true;

    updatePending();
    emitModeIfChanged();

    if (firstKnown)
        applyOptimized();
}

void GpuController::onSuperGfxModeChanged(int gfxMode)
{
    m_gfxMode = gfxMode;
    if (m_backend != SuperGfxBackend)
        return;

    const bool firstKnown = !m_modeKnown;
    m_modeKnown = true;
    updatePending();
    emitModeIfChanged();

    if (firstKnown)
        applyOptimized();
}

void GpuController::onSuperGfxPendingChanged()
{
    if (m_backend == SuperGfxBackend)
        updatePending();
}

void GpuController::onGpuPowerChanged(const QString &power)
{
    if (m_gpuPower != power) {
        m_gpuPower = power;
        emit gpuPowerChanged(power);
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

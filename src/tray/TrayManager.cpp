#include "TrayManager.h"
#include "PerformanceController.h"
#include "GpuController.h"
#include <QGuiApplication>
#include <QIcon>

TrayManager::TrayManager(PerformanceController *perfController,
                        GpuController *gpuController,
                        QObject *parent)
    : QObject(parent)
    , m_trayIcon(new QSystemTrayIcon(this))
    , m_menu(new QMenu())
    , m_perfController(perfController)
    , m_gpuController(gpuController)
{
    createMenu();
    createTrayIcon();

    connect(m_perfController, &PerformanceController::currentProfileChanged,
            this, &TrayManager::onPerformanceProfileChanged);
    connect(m_gpuController, &GpuController::currentModeChanged,
            this, &TrayManager::onGpuModeChanged);
    connect(m_gpuController, &GpuController::supportedModesChanged,
            this, &TrayManager::updateGpuActions);
    connect(m_gpuController, &GpuController::availableChanged,
            this, &TrayManager::updateGpuActions);

    updateIcon();
    updateTooltip();
}

TrayManager::~TrayManager()
{
    delete m_menu;
}

bool TrayManager::isVisible() const
{
    // Without a system tray (e.g. GNOME without AppIndicator extension) the
    // icon is never shown, so the window must not hide into it.
    return m_trayIcon->isVisible() && QSystemTrayIcon::isSystemTrayAvailable();
}

void TrayManager::setVisible(bool visible)
{
    if (m_trayIcon->isVisible() != visible) {
        m_trayIcon->setVisible(visible);
        emit visibleChanged(visible);
    }
}

void TrayManager::showMessage(const QString &title, const QString &message, int icon, int msecs)
{
    QSystemTrayIcon::MessageIcon msgIcon = static_cast<QSystemTrayIcon::MessageIcon>(icon);
    m_trayIcon->showMessage(title, message, msgIcon, msecs);
}

void TrayManager::createTrayIcon()
{
    m_trayIcon->setContextMenu(m_menu);
    m_trayIcon->setIcon(QIcon(":/icons/g-helper.svg"));

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &TrayManager::onActivated);

    m_trayIcon->show();
}

void TrayManager::createMenu()
{
    // Performance submenu
    m_perfMenu = m_menu->addMenu(tr("Performance"));

    m_quietAction = m_perfMenu->addAction(tr("Silent"));
    m_quietAction->setCheckable(true);
    connect(m_quietAction, &QAction::triggered, this, &TrayManager::setQuietProfile);

    m_balancedAction = m_perfMenu->addAction(tr("Balanced"));
    m_balancedAction->setCheckable(true);
    connect(m_balancedAction, &QAction::triggered, this, &TrayManager::setBalancedProfile);

    m_performanceAction = m_perfMenu->addAction(tr("Turbo"));
    m_performanceAction->setCheckable(true);
    connect(m_performanceAction, &QAction::triggered, this, &TrayManager::setPerformanceProfile);

    // GPU submenu
    m_gpuMenu = m_menu->addMenu(tr("GPU Mode"));

    m_ecoAction = m_gpuMenu->addAction(tr("Eco (Integrated)"));
    m_ecoAction->setCheckable(true);
    connect(m_ecoAction, &QAction::triggered, this, &TrayManager::setEcoMode);

    m_standardAction = m_gpuMenu->addAction(tr("Standard (Hybrid)"));
    m_standardAction->setCheckable(true);
    connect(m_standardAction, &QAction::triggered, this, &TrayManager::setStandardMode);

    m_ultimateAction = m_gpuMenu->addAction(tr("Ultimate (Dedicated)"));
    m_ultimateAction->setCheckable(true);
    connect(m_ultimateAction, &QAction::triggered, this, &TrayManager::setUltimateMode);

    m_optimizedAction = m_gpuMenu->addAction(tr("Optimized (Auto)"));
    m_optimizedAction->setCheckable(true);
    connect(m_optimizedAction, &QAction::triggered, this, &TrayManager::setOptimizedMode);

    m_menu->addSeparator();

    // Show window action
    m_showAction = m_menu->addAction(tr("Show G-Helper"));
    connect(m_showAction, &QAction::triggered, this, &TrayManager::showWindowRequested);

    // Quit action
    m_quitAction = m_menu->addAction(tr("Quit"));
    connect(m_quitAction, &QAction::triggered, this, &TrayManager::quitRequested);

    // Set initial states
    onPerformanceProfileChanged(m_perfController->currentProfile());
    onGpuModeChanged(m_gpuController->currentMode());
    updateGpuActions();
}

void TrayManager::updateIcon()
{
    // Could change icon based on current profile
    m_trayIcon->setIcon(QIcon(":/icons/g-helper.svg"));
}

void TrayManager::updateTooltip()
{
    QString tooltip = QString("G-Helper Linux\n%1 | %2")
        .arg(m_perfController->currentProfileName())
        .arg(m_gpuController->currentModeName());
    m_trayIcon->setToolTip(tooltip);
}

void TrayManager::onActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick ||
        reason == QSystemTrayIcon::Trigger) {
        emit showWindowRequested();
    }
}

void TrayManager::onPerformanceProfileChanged(int profile)
{
    m_quietAction->setChecked(profile == 0);
    m_balancedAction->setChecked(profile == 1);
    m_performanceAction->setChecked(profile == 2);
    updateTooltip();
}

void TrayManager::onGpuModeChanged(int mode)
{
    m_ecoAction->setChecked(mode == GpuController::Eco);
    m_standardAction->setChecked(mode == GpuController::Standard);
    m_ultimateAction->setChecked(mode == GpuController::Ultimate);
    m_optimizedAction->setChecked(mode == GpuController::Optimized);
    updateTooltip();
}

void TrayManager::updateGpuActions()
{
    const bool available = m_gpuController->isAvailable();
    m_ecoAction->setVisible(m_gpuController->isModeSupported(GpuController::Eco));
    m_standardAction->setVisible(m_gpuController->isModeSupported(GpuController::Standard));
    m_ultimateAction->setVisible(m_gpuController->isModeSupported(GpuController::Ultimate));
    m_optimizedAction->setVisible(m_gpuController->isModeSupported(GpuController::Optimized));
    m_gpuMenu->setEnabled(available);
    // QAction::triggered toggles the check mark; restore the real state
    onGpuModeChanged(m_gpuController->currentMode());
}

void TrayManager::setQuietProfile()
{
    m_perfController->setProfile(0);
    onPerformanceProfileChanged(m_perfController->currentProfile());
}

void TrayManager::setBalancedProfile()
{
    m_perfController->setProfile(1);
    onPerformanceProfileChanged(m_perfController->currentProfile());
}

void TrayManager::setPerformanceProfile()
{
    m_perfController->setProfile(2);
    onPerformanceProfileChanged(m_perfController->currentProfile());
}

void TrayManager::setEcoMode()
{
    emit gpuModeRequested(GpuController::Eco);
    onGpuModeChanged(m_gpuController->currentMode());
}

void TrayManager::setStandardMode()
{
    emit gpuModeRequested(GpuController::Standard);
    onGpuModeChanged(m_gpuController->currentMode());
}

void TrayManager::setUltimateMode()
{
    emit gpuModeRequested(GpuController::Ultimate);
    onGpuModeChanged(m_gpuController->currentMode());
}

void TrayManager::setOptimizedMode()
{
    emit gpuModeRequested(GpuController::Optimized);
    onGpuModeChanged(m_gpuController->currentMode());
}

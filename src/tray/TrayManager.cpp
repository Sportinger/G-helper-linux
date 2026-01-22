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

    updateIcon();
    updateTooltip();
}

TrayManager::~TrayManager()
{
    delete m_menu;
}

bool TrayManager::isVisible() const
{
    return m_trayIcon->isVisible();
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
    m_ecoAction->setChecked(mode == 0);
    m_standardAction->setChecked(mode == 1);
    m_ultimateAction->setChecked(mode == 2);
    updateTooltip();
}

void TrayManager::setQuietProfile()
{
    m_perfController->setProfile(0);
}

void TrayManager::setBalancedProfile()
{
    m_perfController->setProfile(1);
}

void TrayManager::setPerformanceProfile()
{
    m_perfController->setProfile(2);
}

void TrayManager::setEcoMode()
{
    m_gpuController->setMode(0);
}

void TrayManager::setStandardMode()
{
    m_gpuController->setMode(1);
}

void TrayManager::setUltimateMode()
{
    m_gpuController->setMode(2);
}

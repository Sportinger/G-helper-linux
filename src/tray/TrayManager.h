#ifndef TRAYMANAGER_H
#define TRAYMANAGER_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

class PerformanceController;
class GpuController;

class TrayManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)

public:
    explicit TrayManager(PerformanceController *perfController,
                        GpuController *gpuController,
                        QObject *parent = nullptr);
    ~TrayManager() override;

    bool isVisible() const;
    void setVisible(bool visible);

    Q_INVOKABLE void showMessage(const QString &title, const QString &message,
                                 int icon = 0, int msecs = 5000);

signals:
    void visibleChanged(bool visible);
    void showWindowRequested();
    void quitRequested();

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);
    void onPerformanceProfileChanged(int profile);
    void onGpuModeChanged(int mode);

    // Performance profile actions
    void setQuietProfile();
    void setBalancedProfile();
    void setPerformanceProfile();

    // GPU mode actions
    void setEcoMode();
    void setStandardMode();
    void setUltimateMode();

private:
    void createTrayIcon();
    void createMenu();
    void updateIcon();
    void updateTooltip();

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_menu;

    // Performance submenu
    QMenu *m_perfMenu;
    QAction *m_quietAction;
    QAction *m_balancedAction;
    QAction *m_performanceAction;

    // GPU submenu
    QMenu *m_gpuMenu;
    QAction *m_ecoAction;
    QAction *m_standardAction;
    QAction *m_ultimateAction;

    // Other actions
    QAction *m_showAction;
    QAction *m_quitAction;

    PerformanceController *m_perfController;
    GpuController *m_gpuController;
};

#endif // TRAYMANAGER_H

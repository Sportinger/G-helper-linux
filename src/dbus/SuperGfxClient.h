#ifndef SUPERGFXCLIENT_H
#define SUPERGFXCLIENT_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>
#include <QTimer>
#include "DBusTypes.h"

// Talks to supergfxd over the system bus. Mode numbers in this class are the
// raw supergfxd values (see GfxMode in DBusTypes.h).
class SuperGfxClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(int currentMode READ currentMode NOTIFY currentModeChanged)
    Q_PROPERTY(int pendingMode READ pendingMode NOTIFY pendingModeChanged)
    Q_PROPERTY(int pendingAction READ pendingAction NOTIFY pendingModeChanged)
    Q_PROPERTY(QList<int> supportedModes READ supportedModes NOTIFY supportedModesChanged)
    Q_PROPERTY(bool switchPending READ switchPending NOTIFY switchPendingChanged)
    Q_PROPERTY(QString gpuPower READ gpuPower NOTIFY gpuPowerChanged)

public:
    enum Mode {
        Hybrid = GfxMode::Hybrid,
        Integrated = GfxMode::Integrated,
        NvidiaNoModeset = GfxMode::NvidiaNoModeset,
        Vfio = GfxMode::Vfio,
        AsusEgpu = GfxMode::AsusEgpu,
        AsusMuxDgpu = GfxMode::AsusMuxDgpu,
        None = GfxMode::None
    };
    Q_ENUM(Mode)

    enum Action {
        ActionLogout = GfxAction::Logout,
        ActionReboot = GfxAction::Reboot,
        ActionSwitchToIntegrated = GfxAction::SwitchToIntegrated,
        ActionAsusEgpuDisable = GfxAction::AsusEgpuDisable,
        ActionNothing = GfxAction::Nothing
    };
    Q_ENUM(Action)

    explicit SuperGfxClient(QObject *parent = nullptr);
    ~SuperGfxClient() override;

    bool isConnected() const { return m_connected; }
    int currentMode() const { return m_currentMode; }
    int pendingMode() const { return m_pendingMode; }
    int pendingAction() const { return m_pendingAction; }
    QList<int> supportedModes() const { return m_supportedModes; }
    bool switchPending() const { return m_switchPending; }
    QString gpuPower() const { return m_gpuPower; }

    void setMode(int mode);
    void refresh();
    static QString modeName(int mode);

signals:
    void connectedChanged(bool connected);
    void currentModeChanged(int mode);
    void pendingModeChanged(int mode);
    void supportedModesChanged();
    void switchPendingChanged(bool pending);
    void gpuPowerChanged(const QString &power);
    void errorOccurred(const QString &error);
    // Emitted after a mode change that only takes effect after a user action
    void userActionRequired(int mode, int action);

private slots:
    void onServiceRegistered();
    void onServiceUnregistered();
    void onNotifyGfxStatus(quint32 status);
    void onNotifyGfx(quint32 mode);

private:
    void connectToService();
    void disconnectFromService();
    void fetchCurrentMode();
    void fetchSupportedModes();
    void fetchGpuPower();
    void fetchPendingState();
    void setGpuPower(quint32 power);
    void setPending(int mode, int action);
    void clearPending();
    QDBusPendingCallWatcher *call(const QString &method, const QVariantList &args = {});

    static constexpr const char* SERVICE = "org.supergfxctl.Daemon";
    static constexpr const char* PATH = "/org/supergfxctl/Gfx";
    static constexpr const char* INTERFACE = "org.supergfxctl.Daemon";

    QDBusServiceWatcher *m_serviceWatcher = nullptr;
    QTimer *m_powerPollTimer = nullptr;
    bool m_connected = false;
    int m_currentMode = Hybrid;
    bool m_modeKnown = false;
    int m_pendingMode = -1;
    int m_pendingAction = ActionNothing;
    QList<int> m_supportedModes;
    bool m_switchPending = false;
    QString m_gpuPower;
};

#endif // SUPERGFXCLIENT_H

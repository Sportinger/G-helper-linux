#ifndef SUPERGFXCLIENT_H
#define SUPERGFXCLIENT_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include "DBusTypes.h"

class SuperGfxClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(int currentMode READ currentMode NOTIFY currentModeChanged)
    Q_PROPERTY(int pendingMode READ pendingMode NOTIFY pendingModeChanged)
    Q_PROPERTY(QList<int> supportedModes READ supportedModes NOTIFY supportedModesChanged)
    Q_PROPERTY(bool switchPending READ switchPending NOTIFY switchPendingChanged)
    Q_PROPERTY(QString gpuPower READ gpuPower NOTIFY gpuPowerChanged)

public:
    enum Mode {
        Integrated = 0,
        Hybrid = 1,
        AsusMuxDgpu = 2,
        Vfio = 3,
        Egpu = 4,
        None = 5
    };
    Q_ENUM(Mode)

    explicit SuperGfxClient(QObject *parent = nullptr);
    ~SuperGfxClient() override;

    bool isConnected() const { return m_connected; }
    int currentMode() const { return m_currentMode; }
    int pendingMode() const { return m_pendingMode; }
    QList<int> supportedModes() const { return m_supportedModes; }
    bool switchPending() const { return m_switchPending; }
    QString gpuPower() const { return m_gpuPower; }

    Q_INVOKABLE void setMode(int mode);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString modeName(int mode) const;
    Q_INVOKABLE QString modeDescription(int mode) const;
    Q_INVOKABLE bool requiresLogout(int fromMode, int toMode) const;

signals:
    void connectedChanged(bool connected);
    void currentModeChanged(int mode);
    void pendingModeChanged(int mode);
    void supportedModesChanged();
    void switchPendingChanged(bool pending);
    void gpuPowerChanged(const QString &power);
    void errorOccurred(const QString &error);
    void logoutRequired(int newMode);

private slots:
    void onModeResult(QDBusPendingCallWatcher *watcher);
    void onSupportedModesResult(QDBusPendingCallWatcher *watcher);
    void onPowerResult(QDBusPendingCallWatcher *watcher);
    void onNotifyGfxStatus(quint32 status);

private:
    void setupConnections();
    void fetchCurrentMode();
    void fetchSupportedModes();
    void fetchGpuPower();

    static constexpr const char* SERVICE = "org.supergfxctl.Daemon";
    static constexpr const char* PATH = "/org/supergfxctl/Gfx";
    static constexpr const char* INTERFACE = "org.supergfxctl.Daemon";

    QDBusInterface *m_interface = nullptr;
    bool m_connected = false;
    int m_currentMode = Hybrid;
    int m_pendingMode = -1;
    QList<int> m_supportedModes;
    bool m_switchPending = false;
    QString m_gpuPower;
};

#endif // SUPERGFXCLIENT_H

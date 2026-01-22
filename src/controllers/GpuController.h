#ifndef GPUCONTROLLER_H
#define GPUCONTROLLER_H

#include <QObject>
#include <QVariantList>

class SuperGfxClient;

class GpuController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentMode READ currentMode NOTIFY currentModeChanged)
    Q_PROPERTY(int pendingMode READ pendingMode NOTIFY pendingModeChanged)
    Q_PROPERTY(QString currentModeName READ currentModeName NOTIFY currentModeChanged)
    Q_PROPERTY(QVariantList supportedModes READ supportedModes NOTIFY supportedModesChanged)
    Q_PROPERTY(bool switchPending READ switchPending NOTIFY switchPendingChanged)
    Q_PROPERTY(QString gpuPower READ gpuPower NOTIFY gpuPowerChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    enum Mode {
        Eco = 0,        // Integrated
        Standard = 1,   // Hybrid
        Ultimate = 2,   // Dedicated (MUX)
        Optimized = 3   // VFIO
    };
    Q_ENUM(Mode)

    explicit GpuController(SuperGfxClient *client, QObject *parent = nullptr);
    ~GpuController() override;

    int currentMode() const { return m_currentMode; }
    int pendingMode() const { return m_pendingMode; }
    QString currentModeName() const;
    QVariantList supportedModes() const { return m_supportedModes; }
    bool switchPending() const { return m_switchPending; }
    QString gpuPower() const { return m_gpuPower; }
    bool isAvailable() const { return m_available; }

    Q_INVOKABLE void setMode(int mode);
    Q_INVOKABLE QString modeName(int mode) const;
    Q_INVOKABLE QString modeDescription(int mode) const;
    Q_INVOKABLE QString modeIcon(int mode) const;
    Q_INVOKABLE bool requiresLogout(int mode) const;
    Q_INVOKABLE void refresh();

signals:
    void currentModeChanged(int mode);
    void pendingModeChanged(int mode);
    void supportedModesChanged();
    void switchPendingChanged(bool pending);
    void gpuPowerChanged(const QString &power);
    void availableChanged(bool available);
    void errorOccurred(const QString &error);
    void logoutRequired(const QString &modeName);

private slots:
    void onModeChanged(int mode);
    void onPendingModeChanged(int mode);
    void onSwitchPendingChanged(bool pending);
    void onGpuPowerChanged(const QString &power);
    void onClientConnected(bool connected);
    void onLogoutRequired(int mode);

private:
    void updateSupportedModes();

    SuperGfxClient *m_client;
    int m_currentMode = Standard;
    int m_pendingMode = -1;
    QVariantList m_supportedModes;
    bool m_switchPending = false;
    QString m_gpuPower;
    bool m_available = false;
};

#endif // GPUCONTROLLER_H

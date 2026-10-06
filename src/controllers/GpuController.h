#ifndef GPUCONTROLLER_H
#define GPUCONTROLLER_H

#include <QObject>
#include <QVariantList>

class SuperGfxClient;

// Maps the G-Helper GPU modes shown in the UI onto supergfxd modes.
//   Eco       -> Integrated
//   Standard  -> Hybrid
//   Ultimate  -> AsusMuxDgpu (MUX switch, needs reboot)
//   Optimized -> Eco on battery, Standard on AC (switched automatically)
class GpuController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentMode READ currentMode NOTIFY currentModeChanged)
    Q_PROPERTY(QString currentModeName READ currentModeName NOTIFY currentModeChanged)
    Q_PROPERTY(int pendingMode READ pendingMode NOTIFY pendingModeChanged)
    Q_PROPERTY(QString pendingText READ pendingText NOTIFY pendingModeChanged)
    Q_PROPERTY(bool switchPending READ switchPending NOTIFY switchPendingChanged)
    Q_PROPERTY(QVariantList supportedModes READ supportedModes NOTIFY supportedModesChanged)
    Q_PROPERTY(QString gpuPower READ gpuPower NOTIFY gpuPowerChanged)
    Q_PROPERTY(bool optimized READ isOptimized NOTIFY currentModeChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    enum Mode {
        Unknown = -1,
        Eco = 0,
        Standard = 1,
        Ultimate = 2,
        Optimized = 3
    };
    Q_ENUM(Mode)

    explicit GpuController(SuperGfxClient *client, QObject *parent = nullptr);
    ~GpuController() override;

    int currentMode() const;
    QString currentModeName() const;
    int pendingMode() const;
    QString pendingText() const;
    bool switchPending() const { return m_switchPending; }
    QVariantList supportedModes() const { return m_supportedModes; }
    QString gpuPower() const { return m_gpuPower; }
    bool isOptimized() const { return m_optimized; }
    bool isAvailable() const { return m_available; }

    Q_INVOKABLE void setMode(int mode);
    Q_INVOKABLE QString modeName(int mode) const;
    Q_INVOKABLE QString modeDescription(int mode) const;
    Q_INVOKABLE bool isModeSupported(int mode) const;
    // Returns a warning text if switching to `mode` needs a reboot/logout, otherwise ""
    Q_INVOKABLE QString confirmationText(int mode) const;
    Q_INVOKABLE void refresh();

public slots:
    void setOnBattery(bool onBattery);

signals:
    void currentModeChanged(int mode);
    void pendingModeChanged(int mode);
    void supportedModesChanged();
    void switchPendingChanged(bool pending);
    void gpuPowerChanged(const QString &power);
    void availableChanged(bool available);
    void errorOccurred(const QString &error);
    void userActionRequired(const QString &message);

private slots:
    void onModeChanged(int gfxMode);
    void onPendingModeChanged(int gfxMode);
    void onSwitchPendingChanged(bool pending);
    void onGpuPowerChanged(const QString &power);
    void onClientConnected(bool connected);
    void onUserActionRequired(int gfxMode, int action);

private:
    void updateSupportedModes();
    void applyOptimized();
    void setOptimized(bool optimized);
    QString actionText(int gfxMode, int action) const;
    static int gfxToUi(int gfxMode);
    static int uiToGfx(int uiMode);

    SuperGfxClient *m_client;
    int m_gfxMode = -1;
    bool m_modeKnown = false;
    QVariantList m_supportedModes;
    bool m_switchPending = false;
    QString m_gpuPower;
    bool m_available = false;
    bool m_optimized = false;
    bool m_onBattery = false;
};

#endif // GPUCONTROLLER_H

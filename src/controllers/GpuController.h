#ifndef GPUCONTROLLER_H
#define GPUCONTROLLER_H

#include <QObject>
#include <QVariantList>

class SuperGfxClient;
class AsusdClient;

// Maps the G-Helper GPU modes shown in the UI onto the available backend.
//
// Preferred backend: asusd + kernel asus-armoury driver
//   Eco      -> dgpu_disable=1, gpu_mux_mode=1
//   Standard -> dgpu_disable=0, gpu_mux_mode=1
//   Ultimate -> dgpu_disable=0, gpu_mux_mode=0
//   asusd queues these writes and applies them at shutdown, so every
//   switch becomes active after the next reboot.
//
// Fallback backend: supergfxd (deprecated upstream)
//   Eco -> Integrated, Standard -> Hybrid, Ultimate -> AsusMuxDgpu
//
// Optimized = Eco on battery, Standard on AC.
class GpuController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentMode READ currentMode NOTIFY currentModeChanged)
    Q_PROPERTY(QString currentModeName READ currentModeName NOTIFY currentModeChanged)
    Q_PROPERTY(int pendingMode READ pendingMode NOTIFY pendingModeChanged)
    Q_PROPERTY(QString pendingText READ pendingText NOTIFY pendingModeChanged)
    Q_PROPERTY(bool switchPending READ switchPending NOTIFY switchPendingChanged)
    Q_PROPERTY(bool rebootRequired READ rebootRequired NOTIFY switchPendingChanged)
    Q_PROPERTY(QVariantList supportedModes READ supportedModes NOTIFY supportedModesChanged)
    Q_PROPERTY(QString gpuPower READ gpuPower NOTIFY gpuPowerChanged)
    Q_PROPERTY(bool optimized READ isOptimized NOTIFY currentModeChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)
    Q_PROPERTY(QString backendName READ backendName NOTIFY availableChanged)

public:
    enum Mode {
        Unknown = -1,
        Eco = 0,
        Standard = 1,
        Ultimate = 2,
        Optimized = 3
    };
    Q_ENUM(Mode)

    GpuController(SuperGfxClient *superGfx, AsusdClient *asusd, QObject *parent = nullptr);
    ~GpuController() override;

    int currentMode() const;
    QString currentModeName() const;
    int pendingMode() const;
    QString pendingText() const;
    bool switchPending() const { return m_switchPending; }
    bool rebootRequired() const;
    QVariantList supportedModes() const { return m_supportedModes; }
    QString gpuPower() const { return m_gpuPower; }
    bool isOptimized() const { return m_optimized; }
    bool isAvailable() const { return m_backend != NoBackend; }
    QString backendName() const;

    Q_INVOKABLE void setMode(int mode);
    Q_INVOKABLE QString modeName(int mode) const;
    Q_INVOKABLE QString modeDescription(int mode) const;
    Q_INVOKABLE bool isModeSupported(int mode) const;
    // Returns a warning text if switching to `mode` needs confirmation, otherwise ""
    Q_INVOKABLE QString confirmationText(int mode) const;
    // Asks logind to reboot (polkit may ask for a password)
    Q_INVOKABLE void rebootNow();
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
    void updateBackend();
    void onArmouryChanged();
    void onSuperGfxModeChanged(int gfxMode);
    void onSuperGfxPendingChanged();
    void onGpuPowerChanged(const QString &power);
    void onUserActionRequired(int gfxMode, int action);

private:
    enum Backend { NoBackend, ArmouryBackend, SuperGfxBackend };

    int hardwareMode() const;          // Eco/Standard/Ultimate/Unknown, what runs now
    int armouryMode(int dgpuDisable, int mux) const;
    void setHardwareMode(int mode);    // Eco/Standard/Ultimate
    void updateSupportedModes();
    void updatePending();
    void applyOptimized();
    void setOptimized(bool optimized);
    void emitModeIfChanged();
    QString actionText(int gfxMode, int action) const;
    static int gfxToUi(int gfxMode);
    static int uiToGfx(int uiMode);

    SuperGfxClient *m_superGfx;
    AsusdClient *m_asusd;
    Backend m_backend = NoBackend;

    int m_gfxMode = -1;               // supergfxd mode
    bool m_modeKnown = false;
    int m_lastEmittedMode = -2;
    QVariantList m_supportedModes;
    bool m_switchPending = false;
    int m_pendingMode = Unknown;
    QString m_gpuPower;
    bool m_optimized = false;
    bool m_onBattery = false;
};

#endif // GPUCONTROLLER_H

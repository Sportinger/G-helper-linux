#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H

#include <QObject>
#include <QVariantList>

class AsusdClient;

// Fan curves are stored by asusd per platform profile; asusd applies them
// itself whenever the profile changes. This controller only reads them,
// edits them on request and never pushes anything on its own.
class FanController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList cpuCurve READ cpuCurve NOTIFY fanCurvesChanged)
    Q_PROPERTY(QVariantList gpuCurve READ gpuCurve NOTIFY fanCurvesChanged)
    Q_PROPERTY(bool cpuCurveEnabled READ cpuCurveEnabled NOTIFY fanCurvesChanged)
    Q_PROPERTY(bool gpuCurveEnabled READ gpuCurveEnabled NOTIFY fanCurvesChanged)
    Q_PROPERTY(bool curvesEnabled READ curvesEnabled NOTIFY fanCurvesChanged)
    Q_PROPERTY(int currentProfile READ currentProfile WRITE setCurrentProfile NOTIFY currentProfileChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    enum FanType {
        CpuFan = 0,
        GpuFan = 1
    };
    Q_ENUM(FanType)

    enum Profile {
        Silent = 0,
        Balanced = 1,
        Turbo = 2
    };
    Q_ENUM(Profile)

    explicit FanController(AsusdClient *client, QObject *parent = nullptr);
    ~FanController() override;

    QVariantList cpuCurve() const { return m_curves[m_currentProfile][CpuFan]; }
    QVariantList gpuCurve() const { return m_curves[m_currentProfile][GpuFan]; }
    bool cpuCurveEnabled() const { return m_enabled[m_currentProfile][CpuFan]; }
    bool gpuCurveEnabled() const { return m_enabled[m_currentProfile][GpuFan]; }
    bool curvesEnabled() const { return cpuCurveEnabled() || gpuCurveEnabled(); }
    int currentProfile() const { return m_currentProfile; }
    bool isAvailable() const { return m_available; }

    // Editing a curve stores it in asusd and enables custom curves for the
    // profile that is currently being edited.
    Q_INVOKABLE void setCpuCurve(const QVariantList &points);
    Q_INVOKABLE void setGpuCurve(const QVariantList &points);
    Q_INVOKABLE void setCurvesEnabled(bool enabled);
    Q_INVOKABLE void setCurrentProfile(int profile);
    Q_INVOKABLE void resetCurrentProfileToDefaults();
    Q_INVOKABLE void refresh();

signals:
    void fanCurvesChanged();
    void currentProfileChanged(int profile);
    void availableChanged(bool available);
    void errorOccurred(const QString &error);

private slots:
    void onFanCurvesReceived(int profile, const QVariantList &curves);
    void onFanCurvesUnavailable();
    void onProfileChanged(int profile);
    void onClientConnected(bool connected);

private:
    void setCurve(int fan, const QVariantList &points);
    void setAvailable(bool available);
    static QVariantList normalizedCurve(const QVariantList &points);

    AsusdClient *m_client;

    // [profile][fan]
    QVariantList m_curves[3][2];
    bool m_enabled[3][2] = {};

    int m_currentProfile = Balanced;
    bool m_available = false;
};

#endif // FANCONTROLLER_H

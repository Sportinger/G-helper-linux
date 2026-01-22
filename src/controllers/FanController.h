#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H

#include <QObject>
#include <QVariantList>
#include <QSettings>

class AsusdClient;

class FanController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList cpuCurve READ cpuCurve NOTIFY fanCurvesChanged)
    Q_PROPERTY(QVariantList gpuCurve READ gpuCurve NOTIFY fanCurvesChanged)
    Q_PROPERTY(bool cpuCurveEnabled READ cpuCurveEnabled NOTIFY fanCurvesChanged)
    Q_PROPERTY(bool gpuCurveEnabled READ gpuCurveEnabled NOTIFY fanCurvesChanged)
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

    QVariantList cpuCurve() const { return m_cpuCurves[m_currentProfile]; }
    QVariantList gpuCurve() const { return m_gpuCurves[m_currentProfile]; }
    bool cpuCurveEnabled() const { return m_cpuCurveEnabled; }
    bool gpuCurveEnabled() const { return m_gpuCurveEnabled; }
    int currentProfile() const { return m_currentProfile; }
    bool isAvailable() const { return m_available; }

    Q_INVOKABLE void setCpuCurve(const QVariantList &points, bool enabled);
    Q_INVOKABLE void setGpuCurve(const QVariantList &points, bool enabled);
    Q_INVOKABLE void setCurrentProfile(int profile);
    Q_INVOKABLE void resetToDefaults();
    Q_INVOKABLE void resetCurrentProfileToDefaults();
    Q_INVOKABLE void refresh();

    Q_INVOKABLE static QVariantList defaultCurve(int profile);

signals:
    void fanCurvesChanged();
    void currentProfileChanged(int profile);
    void availableChanged(bool available);
    void errorOccurred(const QString &error);

private slots:
    void onFanCurvesChanged();
    void onProfileChanged(quint32 profile);
    void onClientConnected(bool connected);

private:
    void loadFanCurves();
    void loadFromSettings();
    void saveToSettings();
    void initializeDefaultCurves();

    AsusdClient *m_client;
    QSettings m_settings;

    // 3 profiles, each with CPU and GPU curves
    QVariantList m_cpuCurves[3];
    QVariantList m_gpuCurves[3];

    bool m_cpuCurveEnabled = true;
    bool m_gpuCurveEnabled = true;
    int m_currentProfile = 1; // Balanced
    bool m_available = false;
};

#endif // FANCONTROLLER_H

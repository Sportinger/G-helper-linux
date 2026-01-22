#ifndef FANCONTROLLER_H
#define FANCONTROLLER_H

#include <QObject>
#include <QVariantList>

class AsusdClient;

class FanController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList cpuCurve READ cpuCurve NOTIFY fanCurvesChanged)
    Q_PROPERTY(QVariantList gpuCurve READ gpuCurve NOTIFY fanCurvesChanged)
    Q_PROPERTY(bool cpuCurveEnabled READ cpuCurveEnabled NOTIFY fanCurvesChanged)
    Q_PROPERTY(bool gpuCurveEnabled READ gpuCurveEnabled NOTIFY fanCurvesChanged)
    Q_PROPERTY(int currentProfile READ currentProfile NOTIFY currentProfileChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    enum FanType {
        CpuFan = 0,
        GpuFan = 1
    };
    Q_ENUM(FanType)

    explicit FanController(AsusdClient *client, QObject *parent = nullptr);
    ~FanController() override;

    QVariantList cpuCurve() const { return m_cpuCurve; }
    QVariantList gpuCurve() const { return m_gpuCurve; }
    bool cpuCurveEnabled() const { return m_cpuCurveEnabled; }
    bool gpuCurveEnabled() const { return m_gpuCurveEnabled; }
    int currentProfile() const { return m_currentProfile; }
    bool isAvailable() const { return m_available; }

    Q_INVOKABLE void setCpuCurve(const QVariantList &points, bool enabled);
    Q_INVOKABLE void setGpuCurve(const QVariantList &points, bool enabled);
    Q_INVOKABLE void resetToDefaults();
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

    AsusdClient *m_client;

    QVariantList m_cpuCurve;
    QVariantList m_gpuCurve;
    bool m_cpuCurveEnabled = true;
    bool m_gpuCurveEnabled = true;
    int m_currentProfile = 1; // Balanced
    bool m_available = false;
};

#endif // FANCONTROLLER_H

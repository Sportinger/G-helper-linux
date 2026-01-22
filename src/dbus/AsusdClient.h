#ifndef ASUSDCLIENT_H
#define ASUSDCLIENT_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QColor>
#include "DBusTypes.h"

class AsusdClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(quint32 platformProfile READ platformProfile NOTIFY platformProfileChanged)
    Q_PROPERTY(quint8 chargeLimit READ chargeLimit NOTIFY chargeLimitChanged)
    Q_PROPERTY(quint32 ledBrightness READ ledBrightness NOTIFY ledBrightnessChanged)

public:
    explicit AsusdClient(QObject *parent = nullptr);
    ~AsusdClient() override;

    bool isConnected() const { return m_connected; }

    // Platform profile
    quint32 platformProfile() const { return m_platformProfile; }
    Q_INVOKABLE void setPlatformProfile(quint32 profile);

    // Battery
    quint8 chargeLimit() const { return m_chargeLimit; }
    Q_INVOKABLE void setChargeLimit(quint8 limit);

    // LED/Aura
    quint32 ledBrightness() const { return m_ledBrightness; }
    Q_INVOKABLE void setLedBrightness(quint32 level);
    Q_INVOKABLE void setLedMode(quint32 mode, const QColor &color1, const QColor &color2 = QColor(), quint8 speed = 1);

    // Fan curves
    Q_INVOKABLE QVariantList getFanCurves(quint32 profile);
    Q_INVOKABLE void setFanCurve(quint32 profile, quint32 fanType, const QVariantList &points, bool enabled);
    Q_INVOKABLE void resetFanCurves(quint32 profile);

    Q_INVOKABLE void refresh();

signals:
    void connectedChanged(bool connected);
    void platformProfileChanged(quint32 profile);
    void chargeLimitChanged(quint8 limit);
    void ledBrightnessChanged(quint32 brightness);
    void fanCurvesChanged();
    void errorOccurred(const QString &error);

private slots:
    void onPlatformProfileResult(QDBusPendingCallWatcher *watcher);
    void onChargeLimitResult(QDBusPendingCallWatcher *watcher);
    void onLedBrightnessResult(QDBusPendingCallWatcher *watcher);
    void onPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated);

private:
    void setupConnections();
    void fetchPlatformProfile();
    void fetchChargeLimit();
    void fetchLedBrightness();
    void findAuraDevice();

    static constexpr const char* SERVICE = "xyz.ljones.Asusd";
    static constexpr const char* PATH_PLATFORM = "/xyz/ljones";
    static constexpr const char* INTERFACE_PLATFORM = "xyz.ljones.Platform";
    static constexpr const char* INTERFACE_AURA = "xyz.ljones.Aura";

    QDBusInterface *m_platformInterface = nullptr;
    QDBusInterface *m_auraInterface = nullptr;
    QString m_auraPath;

    bool m_connected = false;
    quint32 m_platformProfile = 1; // Balanced
    quint8 m_chargeLimit = 100;
    quint32 m_ledBrightness = 2; // Medium
};

#endif // ASUSDCLIENT_H

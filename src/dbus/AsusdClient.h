#ifndef ASUSDCLIENT_H
#define ASUSDCLIENT_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>
#include <QColor>
#include <QVariantList>
#include <functional>
#include "DBusTypes.h"

// Talks to asusd (asusctl daemon) over the system bus.
//
// All profile numbers in this class' public API use the APP numbering
// (Quiet = 0, Balanced = 1, Performance = 2). Conversion to the daemon's
// numbering (Balanced = 0, Performance = 1, Quiet = 2) happens internally.
class AsusdClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(int platformProfile READ platformProfile NOTIFY platformProfileChanged)
    Q_PROPERTY(quint8 chargeLimit READ chargeLimit NOTIFY chargeLimitChanged)
    Q_PROPERTY(quint32 ledBrightness READ ledBrightness NOTIFY ledBrightnessChanged)

public:
    // App-side profile numbering
    enum Profile {
        ProfileQuiet = 0,
        ProfileBalanced = 1,
        ProfilePerformance = 2
    };

    // App-side fan numbering
    enum Fan {
        FanCpu = 0,
        FanGpu = 1,
        FanMid = 2
    };

    explicit AsusdClient(QObject *parent = nullptr);
    ~AsusdClient() override;

    bool isConnected() const { return m_connected; }

    // Platform profile (app numbering)
    int platformProfile() const { return m_platformProfile; }
    void setPlatformProfile(int profile);

    // Battery
    quint8 chargeLimit() const { return m_chargeLimit; }
    void setChargeLimit(quint8 limit);

    // Keyboard LED (Aura)
    bool hasAura() const { return !m_auraPath.isEmpty(); }
    quint32 ledBrightness() const { return m_ledBrightness; }
    void setLedBrightness(quint32 level);
    QList<quint32> supportedAuraModes() const { return m_supportedAuraModes; }
    quint32 auraMode() const { return m_auraMode; }
    QColor auraColor1() const { return m_auraColor1; }
    QColor auraColor2() const { return m_auraColor2; }
    int auraSpeed() const { return m_auraSpeed; }   // 0 = low, 1 = medium, 2 = high
    void setLedMode(quint32 mode, const QColor &color1, const QColor &color2, int speed);

    // Slash lightbar
    bool hasSlash() const { return m_hasSlash; }
    QString slashPath() const { return QStringLiteral("/xyz/ljones/aura/slash"); }

    // Fan curves (asusd stores them per profile and applies them itself
    // whenever the platform profile changes)
    void fetchFanCurves(int profile);
    void setFanCurve(int profile, int fan, const QVariantList &points, bool enabled);
    void setFanCurvesEnabled(int profile, bool enabled);
    void resetFanCurves(int profile);

    void refresh();

    static int profileFromDbus(quint32 dbusProfile);
    quint32 profileToDbus(int profile) const;

    // Generic async property helpers (also used by SlashController)
    void getProperty(const QString &path, const QString &interface, const QString &name,
                     std::function<void(const QVariant &)> onSuccess);
    void setProperty(const QString &path, const QString &interface, const QString &name,
                     const QVariant &value, std::function<void()> onSuccess,
                     const QString &errorText, std::function<void()> onError = nullptr);

    static constexpr const char* SERVICE = "xyz.ljones.Asusd";

signals:
    void connectedChanged(bool connected);
    void platformProfileChanged(int profile);
    void chargeLimitChanged(quint8 limit);
    void ledBrightnessChanged(quint32 brightness);
    void auraModeDataChanged();
    void supportedAuraModesChanged();
    void fanCurvesReceived(int profile, const QVariantList &curves);
    void fanCurvesUnavailable();
    void errorOccurred(const QString &error);

private slots:
    void onServiceRegistered();
    void onServiceUnregistered();
    void onPropertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated);

private:
    void connectToService();
    void disconnectFromService();
    void findDevices();
    void fetchPlatformProfile();
    void fetchPlatformProfileChoices();
    void fetchChargeLimit();
    void fetchLedState();
    bool applyAuraEffect(const QVariant &value);
    void applySupportedAuraModes(const QVariant &value);
    void callFanCurves(const QString &method, const QVariantList &args,
                       std::function<void(const QDBusMessage &)> onSuccess,
                       const QString &errorText);
    static QString dbusProfileName(quint32 dbusProfile);

    static constexpr const char* PATH_PLATFORM = "/xyz/ljones";
    static constexpr const char* INTERFACE_PLATFORM = "xyz.ljones.Platform";
    static constexpr const char* INTERFACE_AURA = "xyz.ljones.Aura";
    static constexpr const char* INTERFACE_FAN_CURVES = "xyz.ljones.FanCurves";
    static constexpr const char* INTERFACE_PROPERTIES = "org.freedesktop.DBus.Properties";

    QDBusServiceWatcher *m_serviceWatcher = nullptr;
    QString m_auraPath;
    bool m_hasSlash = false;

    bool m_connected = false;
    int m_platformProfile = ProfileBalanced;
    QList<quint32> m_profileChoices;  // daemon numbering
    quint8 m_chargeLimit = 100;

    quint32 m_ledBrightness = 2; // Medium
    QList<quint32> m_supportedAuraModes;
    quint32 m_auraMode = AsusdAuraMode::Static;
    QColor m_auraColor1 = QColor(255, 0, 0);
    QColor m_auraColor2 = QColor(0, 0, 0);
    int m_auraSpeed = 1;
};

#endif // ASUSDCLIENT_H

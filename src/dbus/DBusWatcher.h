#ifndef DBUSWATCHER_H
#define DBUSWATCHER_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QTimer>

class DBusWatcher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool asusdConnected READ asusdConnected NOTIFY asusdConnectedChanged)
    Q_PROPERTY(bool supergfxConnected READ supergfxConnected NOTIFY supergfxConnectedChanged)
    Q_PROPERTY(bool allConnected READ allConnected NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)

public:
    explicit DBusWatcher(QObject *parent = nullptr);
    ~DBusWatcher() override;

    bool asusdConnected() const { return m_asusdConnected; }
    bool supergfxConnected() const { return m_supergfxConnected; }
    bool allConnected() const { return m_asusdConnected && m_supergfxConnected; }
    QString connectionStatus() const;

    Q_INVOKABLE void checkConnections();

signals:
    void asusdConnectedChanged(bool connected);
    void supergfxConnectedChanged(bool connected);
    void connectionStatusChanged();

private slots:
    void onServiceRegistered(const QString &service);
    void onServiceUnregistered(const QString &service);
    void onRetryConnection();

private:
    void checkAsusd();
    void checkSupergfx();
    void setAsusdConnected(bool connected);
    void setSupergfxConnected(bool connected);

    static constexpr const char* ASUSD_SERVICE = "xyz.ljones.Asusd";
    static constexpr const char* SUPERGFX_SERVICE = "org.supergfxctl.Daemon";

    QDBusServiceWatcher *m_watcher;
    QTimer *m_retryTimer;
    bool m_asusdConnected = false;
    bool m_supergfxConnected = false;
};

#endif // DBUSWATCHER_H

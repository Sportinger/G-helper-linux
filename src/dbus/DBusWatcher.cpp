#include "DBusWatcher.h"
#include <QDBusConnectionInterface>
#include <QDebug>

DBusWatcher::DBusWatcher(QObject *parent)
    : QObject(parent)
    , m_watcher(new QDBusServiceWatcher(this))
    , m_retryTimer(new QTimer(this))
{
    m_watcher->setConnection(QDBusConnection::systemBus());
    m_watcher->addWatchedService(ASUSD_SERVICE);
    m_watcher->addWatchedService(SUPERGFX_SERVICE);

    connect(m_watcher, &QDBusServiceWatcher::serviceRegistered,
            this, &DBusWatcher::onServiceRegistered);
    connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &DBusWatcher::onServiceUnregistered);

    m_retryTimer->setInterval(5000); // Retry every 5 seconds
    connect(m_retryTimer, &QTimer::timeout, this, &DBusWatcher::onRetryConnection);

    // Initial check
    checkConnections();
}

DBusWatcher::~DBusWatcher() = default;

QString DBusWatcher::connectionStatus() const
{
    if (m_asusdConnected && m_supergfxConnected) {
        return tr("Connected");
    }

    QStringList missing;
    if (!m_asusdConnected) missing << "asusd";
    if (!m_supergfxConnected) missing << "supergfxd";

    return tr("Disconnected: %1").arg(missing.join(", "));
}

void DBusWatcher::checkConnections()
{
    checkAsusd();
    checkSupergfx();

    // Start retry timer if not all connected
    if (!allConnected() && !m_retryTimer->isActive()) {
        m_retryTimer->start();
    } else if (allConnected() && m_retryTimer->isActive()) {
        m_retryTimer->stop();
    }
}

void DBusWatcher::checkAsusd()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    bool connected = bus.interface()->isServiceRegistered(ASUSD_SERVICE);
    setAsusdConnected(connected);
}

void DBusWatcher::checkSupergfx()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    bool connected = bus.interface()->isServiceRegistered(SUPERGFX_SERVICE);
    setSupergfxConnected(connected);
}

void DBusWatcher::setAsusdConnected(bool connected)
{
    if (m_asusdConnected != connected) {
        m_asusdConnected = connected;
        qDebug() << "asusd connection:" << (connected ? "connected" : "disconnected");
        emit asusdConnectedChanged(connected);
        emit connectionStatusChanged();
    }
}

void DBusWatcher::setSupergfxConnected(bool connected)
{
    if (m_supergfxConnected != connected) {
        m_supergfxConnected = connected;
        qDebug() << "supergfxd connection:" << (connected ? "connected" : "disconnected");
        emit supergfxConnectedChanged(connected);
        emit connectionStatusChanged();
    }
}

void DBusWatcher::onServiceRegistered(const QString &service)
{
    qDebug() << "D-Bus service registered:" << service;

    if (service == ASUSD_SERVICE) {
        setAsusdConnected(true);
    } else if (service == SUPERGFX_SERVICE) {
        setSupergfxConnected(true);
    }

    if (allConnected() && m_retryTimer->isActive()) {
        m_retryTimer->stop();
    }
}

void DBusWatcher::onServiceUnregistered(const QString &service)
{
    qDebug() << "D-Bus service unregistered:" << service;

    if (service == ASUSD_SERVICE) {
        setAsusdConnected(false);
    } else if (service == SUPERGFX_SERVICE) {
        setSupergfxConnected(false);
    }

    if (!m_retryTimer->isActive()) {
        m_retryTimer->start();
    }
}

void DBusWatcher::onRetryConnection()
{
    qDebug() << "Retrying D-Bus connections...";
    checkConnections();
}

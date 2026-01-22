#include "PerformanceController.h"
#include "AsusdClient.h"

PerformanceController::PerformanceController(AsusdClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    connect(m_client, &AsusdClient::platformProfileChanged,
            this, &PerformanceController::onProfileChanged);
    connect(m_client, &AsusdClient::connectedChanged,
            this, &PerformanceController::onClientConnected);
    connect(m_client, &AsusdClient::errorOccurred,
            this, &PerformanceController::errorOccurred);

    m_available = m_client->isConnected();
    if (m_available) {
        m_currentProfile = static_cast<int>(m_client->platformProfile());
    }
}

PerformanceController::~PerformanceController() = default;

QString PerformanceController::currentProfileName() const
{
    return profileName(m_currentProfile);
}

void PerformanceController::setProfile(int profile)
{
    if (!m_available) {
        emit errorOccurred(tr("Performance control is not available"));
        return;
    }

    if (profile < Quiet || profile > Performance) {
        emit errorOccurred(tr("Invalid profile: %1").arg(profile));
        return;
    }

    m_client->setPlatformProfile(static_cast<quint32>(profile));
}

QString PerformanceController::profileName(int profile) const
{
    switch (profile) {
        case Quiet: return tr("Silent");
        case Balanced: return tr("Balanced");
        case Performance: return tr("Turbo");
        default: return tr("Unknown");
    }
}

QString PerformanceController::profileDescription(int profile) const
{
    switch (profile) {
        case Quiet:
            return tr("Minimal fan noise, reduced performance. Best for quiet environments.");
        case Balanced:
            return tr("Balanced performance and cooling. Recommended for daily use.");
        case Performance:
            return tr("Maximum performance with aggressive cooling. Best for demanding tasks.");
        default:
            return tr("Unknown profile.");
    }
}

QString PerformanceController::profileIcon(int profile) const
{
    switch (profile) {
        case Quiet: return "qrc:/icons/quiet.svg";
        case Balanced: return "qrc:/icons/balanced.svg";
        case Performance: return "qrc:/icons/performance.svg";
        default: return "";
    }
}

void PerformanceController::refresh()
{
    if (m_available) {
        m_client->refresh();
    }
}

void PerformanceController::onProfileChanged(quint32 profile)
{
    int newProfile = static_cast<int>(profile);
    if (m_currentProfile != newProfile) {
        m_currentProfile = newProfile;
        emit currentProfileChanged(newProfile);
    }
}

void PerformanceController::onClientConnected(bool connected)
{
    if (m_available != connected) {
        m_available = connected;
        emit availableChanged(connected);

        if (connected) {
            m_currentProfile = static_cast<int>(m_client->platformProfile());
            emit currentProfileChanged(m_currentProfile);
        }
    }
}

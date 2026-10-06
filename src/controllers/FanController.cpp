#include "FanController.h"
#include "AsusdClient.h"
#include <QDebug>
#include <algorithm>

FanController::FanController(AsusdClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    connect(m_client, &AsusdClient::fanCurvesReceived,
            this, &FanController::onFanCurvesReceived);
    connect(m_client, &AsusdClient::fanCurvesUnavailable,
            this, &FanController::onFanCurvesUnavailable);
    connect(m_client, &AsusdClient::platformProfileChanged,
            this, &FanController::onProfileChanged);
    connect(m_client, &AsusdClient::connectedChanged,
            this, &FanController::onClientConnected);

    if (m_client->isConnected())
        onClientConnected(true);
}

FanController::~FanController() = default;

void FanController::refresh()
{
    if (!m_client->isConnected())
        return;
    for (int profile = Silent; profile <= Turbo; ++profile)
        m_client->fetchFanCurves(profile);
}

void FanController::onClientConnected(bool connected)
{
    if (connected) {
        m_currentProfile = m_client->platformProfile();
        emit currentProfileChanged(m_currentProfile);
        refresh();
    } else {
        setAvailable(false);
    }
}

void FanController::setAvailable(bool available)
{
    if (m_available != available) {
        m_available = available;
        emit availableChanged(available);
    }
}

void FanController::onFanCurvesReceived(int profile, const QVariantList &curves)
{
    if (profile < Silent || profile > Turbo)
        return;

    bool hasCurve = false;
    for (int fan = CpuFan; fan <= GpuFan; ++fan) {
        m_curves[profile][fan].clear();
        m_enabled[profile][fan] = false;
    }

    for (const QVariant &c : curves) {
        const QVariantMap curve = c.toMap();
        const int fan = curve.value("fan").toInt();
        if (fan != CpuFan && fan != GpuFan)
            continue;   // mid fan is not shown in the UI
        m_curves[profile][fan] = curve.value("points").toList();
        m_enabled[profile][fan] = curve.value("enabled").toBool();
        hasCurve = hasCurve || !m_curves[profile][fan].isEmpty();
    }

    if (hasCurve)
        setAvailable(true);

    if (profile == m_currentProfile)
        emit fanCurvesChanged();
}

void FanController::onFanCurvesUnavailable()
{
    qWarning() << "FanController: asusd reports no fan curve support";
    setAvailable(false);
}

void FanController::onProfileChanged(int profile)
{
    // Only follow the active profile in the UI. asusd applies the stored
    // curves of the new profile itself.
    if (m_currentProfile != profile) {
        m_currentProfile = profile;
        emit currentProfileChanged(profile);
        emit fanCurvesChanged();
    }
    m_client->fetchFanCurves(profile);
}

void FanController::setCurrentProfile(int profile)
{
    if (profile < Silent || profile > Turbo)
        return;
    if (m_currentProfile != profile) {
        m_currentProfile = profile;
        emit currentProfileChanged(profile);
        emit fanCurvesChanged();
    }
    m_client->fetchFanCurves(profile);
}

QVariantList FanController::normalizedCurve(const QVariantList &points)
{
    // Sort by temperature and make fan speeds non-decreasing, which is what
    // asusd requires.
    QList<QPair<int, int>> pts;
    for (const QVariant &p : points) {
        const QVariantMap map = p.toMap();
        pts.append({qBound(0, map.value("temp").toInt(), 120), qBound(0, map.value("fan").toInt(), 100)});
    }
    std::sort(pts.begin(), pts.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

    QVariantList result;
    int prevFan = 0;
    for (const auto &pt : pts) {
        prevFan = qMax(prevFan, pt.second);
        result << QVariantMap{{"temp", pt.first}, {"fan", prevFan}};
    }
    return result;
}

void FanController::setCurve(int fan, const QVariantList &points)
{
    if (!m_available) {
        emit errorOccurred(tr("Fan curve control is not available"));
        return;
    }

    const QVariantList curve = normalizedCurve(points);
    m_curves[m_currentProfile][fan] = curve;
    emit fanCurvesChanged();

    m_client->setFanCurve(m_currentProfile, fan, curve, true);
}

void FanController::setCpuCurve(const QVariantList &points)
{
    setCurve(CpuFan, points);
}

void FanController::setGpuCurve(const QVariantList &points)
{
    setCurve(GpuFan, points);
}

void FanController::setCurvesEnabled(bool enabled)
{
    if (!m_available) {
        emit errorOccurred(tr("Fan curve control is not available"));
        return;
    }
    m_client->setFanCurvesEnabled(m_currentProfile, enabled);
}

void FanController::resetCurrentProfileToDefaults()
{
    if (!m_available) {
        emit errorOccurred(tr("Fan curve control is not available"));
        return;
    }
    m_client->resetFanCurves(m_currentProfile);
}

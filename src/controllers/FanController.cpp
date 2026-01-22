#include "FanController.h"
#include "AsusdClient.h"

FanController::FanController(AsusdClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    connect(m_client, &AsusdClient::fanCurvesChanged,
            this, &FanController::onFanCurvesChanged);
    connect(m_client, &AsusdClient::platformProfileChanged,
            this, &FanController::onProfileChanged);
    connect(m_client, &AsusdClient::connectedChanged,
            this, &FanController::onClientConnected);
    connect(m_client, &AsusdClient::errorOccurred,
            this, &FanController::errorOccurred);

    m_available = m_client->isConnected();
    if (m_available) {
        m_currentProfile = static_cast<int>(m_client->platformProfile());
        loadFanCurves();
    }
}

FanController::~FanController() = default;

void FanController::setCpuCurve(const QVariantList &points, bool enabled)
{
    if (!m_available) {
        emit errorOccurred(tr("Fan control is not available"));
        return;
    }

    m_cpuCurve = points;
    m_cpuCurveEnabled = enabled;
    m_client->setFanCurve(static_cast<quint32>(m_currentProfile), CpuFan, points, enabled);
    emit fanCurvesChanged();
}

void FanController::setGpuCurve(const QVariantList &points, bool enabled)
{
    if (!m_available) {
        emit errorOccurred(tr("Fan control is not available"));
        return;
    }

    m_gpuCurve = points;
    m_gpuCurveEnabled = enabled;
    m_client->setFanCurve(static_cast<quint32>(m_currentProfile), GpuFan, points, enabled);
    emit fanCurvesChanged();
}

void FanController::resetToDefaults()
{
    if (!m_available) {
        emit errorOccurred(tr("Fan control is not available"));
        return;
    }

    m_client->resetFanCurves(static_cast<quint32>(m_currentProfile));
    loadFanCurves();
}

void FanController::refresh()
{
    if (m_available) {
        loadFanCurves();
    }
}

QVariantList FanController::defaultCurve(int profile)
{
    QVariantList curve;

    // Different default curves based on profile
    switch (profile) {
        case 0: // Quiet
            curve << QVariantMap{{"temp", 30}, {"fan", 0}};
            curve << QVariantMap{{"temp", 45}, {"fan", 10}};
            curve << QVariantMap{{"temp", 55}, {"fan", 25}};
            curve << QVariantMap{{"temp", 65}, {"fan", 40}};
            curve << QVariantMap{{"temp", 75}, {"fan", 55}};
            curve << QVariantMap{{"temp", 85}, {"fan", 70}};
            curve << QVariantMap{{"temp", 95}, {"fan", 85}};
            curve << QVariantMap{{"temp", 100}, {"fan", 100}};
            break;

        case 1: // Balanced
            curve << QVariantMap{{"temp", 30}, {"fan", 0}};
            curve << QVariantMap{{"temp", 40}, {"fan", 15}};
            curve << QVariantMap{{"temp", 50}, {"fan", 30}};
            curve << QVariantMap{{"temp", 60}, {"fan", 45}};
            curve << QVariantMap{{"temp", 70}, {"fan", 60}};
            curve << QVariantMap{{"temp", 80}, {"fan", 75}};
            curve << QVariantMap{{"temp", 90}, {"fan", 90}};
            curve << QVariantMap{{"temp", 100}, {"fan", 100}};
            break;

        case 2: // Performance
            curve << QVariantMap{{"temp", 30}, {"fan", 10}};
            curve << QVariantMap{{"temp", 40}, {"fan", 25}};
            curve << QVariantMap{{"temp", 50}, {"fan", 40}};
            curve << QVariantMap{{"temp", 60}, {"fan", 55}};
            curve << QVariantMap{{"temp", 70}, {"fan", 70}};
            curve << QVariantMap{{"temp", 80}, {"fan", 85}};
            curve << QVariantMap{{"temp", 90}, {"fan", 95}};
            curve << QVariantMap{{"temp", 100}, {"fan", 100}};
            break;

        default:
            curve = defaultCurve(1); // Default to balanced
            break;
    }

    return curve;
}

void FanController::onFanCurvesChanged()
{
    loadFanCurves();
}

void FanController::onProfileChanged(quint32 profile)
{
    int newProfile = static_cast<int>(profile);
    if (m_currentProfile != newProfile) {
        m_currentProfile = newProfile;
        emit currentProfileChanged(newProfile);
        loadFanCurves();
    }
}

void FanController::onClientConnected(bool connected)
{
    if (m_available != connected) {
        m_available = connected;
        emit availableChanged(connected);

        if (connected) {
            m_currentProfile = static_cast<int>(m_client->platformProfile());
            loadFanCurves();
        }
    }
}

void FanController::loadFanCurves()
{
    QVariantList curves = m_client->getFanCurves(static_cast<quint32>(m_currentProfile));

    for (const QVariant &curveVar : curves) {
        QVariantMap curve = curveVar.toMap();
        int fanType = curve["fanType"].toInt();
        bool enabled = curve["enabled"].toBool();
        QVariantList points = curve["points"].toList();

        if (fanType == CpuFan) {
            m_cpuCurve = points;
            m_cpuCurveEnabled = enabled;
        } else if (fanType == GpuFan) {
            m_gpuCurve = points;
            m_gpuCurveEnabled = enabled;
        }
    }

    emit fanCurvesChanged();
}

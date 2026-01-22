#include "FanController.h"
#include "AsusdClient.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

FanController::FanController(AsusdClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
    , m_settings("g-helper-linux", "fan-curves")
{
    connect(m_client, &AsusdClient::fanCurvesChanged,
            this, &FanController::onFanCurvesChanged);
    connect(m_client, &AsusdClient::platformProfileChanged,
            this, &FanController::onProfileChanged);
    connect(m_client, &AsusdClient::connectedChanged,
            this, &FanController::onClientConnected);
    connect(m_client, &AsusdClient::errorOccurred,
            this, &FanController::errorOccurred);

    // Initialize with defaults first
    initializeDefaultCurves();

    // Try to load from settings
    loadFromSettings();

    m_available = m_client->isConnected();
    if (m_available) {
        m_currentProfile = static_cast<int>(m_client->platformProfile());
    }
}

FanController::~FanController() = default;

void FanController::initializeDefaultCurves()
{
    for (int profile = 0; profile < 3; profile++) {
        m_cpuCurves[profile] = defaultCurve(profile);
        m_gpuCurves[profile] = defaultCurve(profile);
    }
}

void FanController::loadFromSettings()
{
    for (int profile = 0; profile < 3; profile++) {
        QString cpuKey = QString("profile%1/cpu").arg(profile);
        QString gpuKey = QString("profile%1/gpu").arg(profile);

        if (m_settings.contains(cpuKey)) {
            QByteArray cpuData = m_settings.value(cpuKey).toByteArray();
            QJsonDocument doc = QJsonDocument::fromJson(cpuData);
            if (doc.isArray()) {
                QVariantList curve;
                QJsonArray arr = doc.array();
                for (const auto &item : arr) {
                    QJsonObject obj = item.toObject();
                    curve.append(QVariantMap{
                        {"temp", obj["temp"].toInt()},
                        {"fan", obj["fan"].toInt()}
                    });
                }
                if (!curve.isEmpty()) {
                    m_cpuCurves[profile] = curve;
                }
            }
        }

        if (m_settings.contains(gpuKey)) {
            QByteArray gpuData = m_settings.value(gpuKey).toByteArray();
            QJsonDocument doc = QJsonDocument::fromJson(gpuData);
            if (doc.isArray()) {
                QVariantList curve;
                QJsonArray arr = doc.array();
                for (const auto &item : arr) {
                    QJsonObject obj = item.toObject();
                    curve.append(QVariantMap{
                        {"temp", obj["temp"].toInt()},
                        {"fan", obj["fan"].toInt()}
                    });
                }
                if (!curve.isEmpty()) {
                    m_gpuCurves[profile] = curve;
                }
            }
        }
    }
    qDebug() << "FanController: Loaded curves from settings";
}

void FanController::saveToSettings()
{
    for (int profile = 0; profile < 3; profile++) {
        QString cpuKey = QString("profile%1/cpu").arg(profile);
        QString gpuKey = QString("profile%1/gpu").arg(profile);

        // Save CPU curve
        QJsonArray cpuArr;
        for (const auto &point : m_cpuCurves[profile]) {
            QVariantMap map = point.toMap();
            cpuArr.append(QJsonObject{
                {"temp", map["temp"].toInt()},
                {"fan", map["fan"].toInt()}
            });
        }
        m_settings.setValue(cpuKey, QJsonDocument(cpuArr).toJson(QJsonDocument::Compact));

        // Save GPU curve
        QJsonArray gpuArr;
        for (const auto &point : m_gpuCurves[profile]) {
            QVariantMap map = point.toMap();
            gpuArr.append(QJsonObject{
                {"temp", map["temp"].toInt()},
                {"fan", map["fan"].toInt()}
            });
        }
        m_settings.setValue(gpuKey, QJsonDocument(gpuArr).toJson(QJsonDocument::Compact));
    }
    m_settings.sync();
    qDebug() << "FanController: Saved curves to settings";
}

void FanController::setCpuCurve(const QVariantList &points, bool enabled)
{
    qDebug() << "FanController::setCpuCurve for profile" << m_currentProfile << "with" << points.size() << "points";

    m_cpuCurves[m_currentProfile] = points;
    m_cpuCurveEnabled = enabled;

    // Save to settings
    saveToSettings();

    emit fanCurvesChanged();

    // Send to daemon if available
    if (m_available) {
        m_client->setFanCurve(static_cast<quint32>(m_currentProfile), CpuFan, points, enabled);
    }
}

void FanController::setGpuCurve(const QVariantList &points, bool enabled)
{
    qDebug() << "FanController::setGpuCurve for profile" << m_currentProfile << "with" << points.size() << "points";

    m_gpuCurves[m_currentProfile] = points;
    m_gpuCurveEnabled = enabled;

    // Save to settings
    saveToSettings();

    emit fanCurvesChanged();

    // Send to daemon if available
    if (m_available) {
        m_client->setFanCurve(static_cast<quint32>(m_currentProfile), GpuFan, points, enabled);
    }
}

void FanController::setCurrentProfile(int profile)
{
    if (profile < 0 || profile > 2) return;
    if (m_currentProfile == profile) return;

    qDebug() << "FanController::setCurrentProfile from" << m_currentProfile << "to" << profile;

    m_currentProfile = profile;
    emit currentProfileChanged(profile);
    emit fanCurvesChanged();  // Curves changed because we switched profile
}

void FanController::resetToDefaults()
{
    qDebug() << "FanController::resetToDefaults - resetting all profiles";

    initializeDefaultCurves();
    saveToSettings();
    emit fanCurvesChanged();

    if (m_available) {
        m_client->resetFanCurves(static_cast<quint32>(m_currentProfile));
    }
}

void FanController::resetCurrentProfileToDefaults()
{
    qDebug() << "FanController::resetCurrentProfileToDefaults for profile" << m_currentProfile;

    m_cpuCurves[m_currentProfile] = defaultCurve(m_currentProfile);
    m_gpuCurves[m_currentProfile] = defaultCurve(m_currentProfile);
    saveToSettings();
    emit fanCurvesChanged();
}

void FanController::refresh()
{
    loadFromSettings();
    emit fanCurvesChanged();
}

QVariantList FanController::defaultCurve(int profile)
{
    QVariantList curve;

    switch (profile) {
        case 0: // Silent
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

        case 2: // Turbo
            curve << QVariantMap{{"temp", 30}, {"fan", 15}};
            curve << QVariantMap{{"temp", 40}, {"fan", 30}};
            curve << QVariantMap{{"temp", 50}, {"fan", 45}};
            curve << QVariantMap{{"temp", 60}, {"fan", 60}};
            curve << QVariantMap{{"temp", 70}, {"fan", 75}};
            curve << QVariantMap{{"temp", 80}, {"fan", 90}};
            curve << QVariantMap{{"temp", 90}, {"fan", 100}};
            curve << QVariantMap{{"temp", 100}, {"fan", 100}};
            break;

        default:
            curve = defaultCurve(1);
            break;
    }

    return curve;
}

void FanController::onFanCurvesChanged()
{
    // External change from daemon - reload
    loadFanCurves();
}

void FanController::onProfileChanged(quint32 profile)
{
    int newProfile = static_cast<int>(profile);
    if (m_currentProfile != newProfile) {
        m_currentProfile = newProfile;
        emit currentProfileChanged(newProfile);
        emit fanCurvesChanged();

        // Apply this profile's fan curves to hardware
        applyCurrentCurvesToHardware();
    }
}

void FanController::applyCurrentCurvesToHardware()
{
    if (!m_available) return;

    qDebug() << "FanController: Applying curves for profile" << m_currentProfile << "to hardware";

    // Send CPU curve
    m_client->setFanCurve(static_cast<quint32>(m_currentProfile), CpuFan,
                          m_cpuCurves[m_currentProfile], m_cpuCurveEnabled);

    // Send GPU curve
    m_client->setFanCurve(static_cast<quint32>(m_currentProfile), GpuFan,
                          m_gpuCurves[m_currentProfile], m_gpuCurveEnabled);
}

void FanController::onClientConnected(bool connected)
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

void FanController::loadFanCurves()
{
    // This is called when daemon reports curve changes
    // For now, we keep our local curves and don't overwrite from daemon
    qDebug() << "FanController::loadFanCurves - keeping local curves";
    emit fanCurvesChanged();
}

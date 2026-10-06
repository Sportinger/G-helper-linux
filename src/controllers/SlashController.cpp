#include "SlashController.h"
#include "AsusdClient.h"
#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>

SlashController::SlashController(AsusdClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    m_availableModes = QStringList{
        "Static", "Bounce", "Slash", "Loading", "BitStream",
        "Transmission", "Flow", "Flux", "Phantom", "Spectrum",
        "Hazard", "Interfacing", "Ramp", "GameOver", "Start", "Buzzer"
    };

    connect(m_client, &AsusdClient::connectedChanged,
            this, &SlashController::onClientConnected);

    onClientConnected(m_client->isConnected());
}

SlashController::~SlashController() = default;

void SlashController::onClientConnected(bool connected)
{
    const bool available = connected && m_client->hasSlash();
    if (m_available != available) {
        m_available = available;
        emit availableChanged(available);
    }
    if (m_available)
        refresh();
}

void SlashController::refresh()
{
    if (!m_available)
        return;

    m_client->getProperty(m_client->slashPath(), INTERFACE_SLASH, "Enabled", [this](const QVariant &value) {
        const bool enabled = value.toBool();
        if (m_enabled != enabled) {
            m_enabled = enabled;
            emit enabledChanged(enabled);
        }
    });

    m_client->getProperty(m_client->slashPath(), INTERFACE_SLASH, "Brightness", [this](const QVariant &value) {
        const int brightness = static_cast<int>(value.toUInt());
        if (m_brightness != brightness) {
            m_brightness = brightness;
            emit brightnessChanged(brightness);
        }
    });

    // asusd's "Mode" D-Bus getter returns the animation interval instead of
    // the mode, so the mode is read from the daemon's config file.
    readModeFromConfig();
}

void SlashController::readModeFromConfig()
{
    QFile configFile("/etc/asusd/slash.ron");
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    const QString content = QString::fromUtf8(configFile.readAll());
    const QRegularExpressionMatch match = QRegularExpression("display_mode:\\s*(\\w+)").match(content);
    if (match.hasMatch() && m_availableModes.contains(match.captured(1))) {
        const QString mode = match.captured(1);
        if (m_currentMode != mode) {
            m_currentMode = mode;
            emit modeChanged(mode);
        }
    }
}

void SlashController::setEnabled(bool enabled)
{
    if (!m_available || m_enabled == enabled)
        return;

    m_enabled = enabled;
    emit enabledChanged(enabled);

    m_client->setProperty(m_client->slashPath(), INTERFACE_SLASH, "Enabled", QVariant::fromValue(enabled),
                          nullptr, tr("Failed to switch the Slash lightbar"),
                          [this]() { refresh(); });
}

void SlashController::setBrightness(int brightness)
{
    if (!m_available)
        return;

    brightness = qBound(0, brightness, 255);
    if (m_brightness != brightness) {
        m_brightness = brightness;
        emit brightnessChanged(brightness);
    }

    m_client->setProperty(m_client->slashPath(), INTERFACE_SLASH, "Brightness",
                          QVariant::fromValue(static_cast<uchar>(brightness)),
                          nullptr, tr("Failed to set Slash brightness"),
                          [this]() { refresh(); });
}

void SlashController::setMode(const QString &mode)
{
    if (!m_available)
        return;

    if (!m_availableModes.contains(mode)) {
        emit errorOccurred(tr("Unknown slash mode: %1").arg(mode));
        return;
    }

    if (m_currentMode != mode) {
        m_currentMode = mode;
        emit modeChanged(mode);
    }

    // The SlashMode D-Bus type is not a plain integer, so use asusctl here
    auto *process = new QProcess(this);
    connect(process, &QProcess::finished, this,
            [this, process](int exitCode, QProcess::ExitStatus status) {
        process->deleteLater();
        if (status != QProcess::NormalExit || exitCode != 0) {
            const QString error = QString::fromUtf8(process->readAllStandardError()).trimmed();
            qWarning() << "SlashController: asusctl failed:" << error;
            emit errorOccurred(tr("Failed to set Slash mode: %1").arg(error));
            readModeFromConfig();
        }
    });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            emit errorOccurred(tr("asusctl not found"));
            process->deleteLater();
        }
    });
    process->start("asusctl", {"slash", "--mode", mode});
}

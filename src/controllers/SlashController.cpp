#include "SlashController.h"
#include <QDebug>
#include <QFile>
#include <QRegularExpression>

SlashController::SlashController(QObject *parent)
    : QObject(parent)
{
    m_availableModes = QStringList{
        "Static", "Bounce", "Slash", "Loading", "BitStream",
        "Transmission", "Flow", "Flux", "Phantom", "Spectrum",
        "Hazard", "Interfacing", "Ramp", "GameOver", "Start", "Buzzer"
    };

    checkAvailability();
}

SlashController::~SlashController() = default;

void SlashController::checkAvailability()
{
    QProcess process;
    process.start("asusctl", QStringList{"slash", "--help"});
    process.waitForFinished(3000);
    m_available = (process.exitCode() == 0);
    emit availableChanged(m_available);

    if (m_available) {
        fetchCurrentState();
    }
}

void SlashController::fetchCurrentState()
{
    // Read current slash state from /etc/asusd/slash.ron
    QFile configFile("/etc/asusd/slash.ron");
    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "SlashController: Could not read /etc/asusd/slash.ron";
        return;
    }

    QString content = QString::fromUtf8(configFile.readAll());
    configFile.close();

    qDebug() << "SlashController: Reading config from slash.ron";

    // Parse RON format
    QRegularExpression enabledRe("enabled:\\s*(true|false)");
    QRegularExpression brightnessRe("brightness:\\s*(\\d+)");
    QRegularExpression modeRe("display_mode:\\s*(\\w+)");

    QRegularExpressionMatch match;

    match = enabledRe.match(content);
    if (match.hasMatch()) {
        m_enabled = (match.captured(1) == "true");
        emit enabledChanged(m_enabled);
        qDebug() << "SlashController: enabled =" << m_enabled;
    }

    match = brightnessRe.match(content);
    if (match.hasMatch()) {
        m_brightness = match.captured(1).toInt();
        emit brightnessChanged(m_brightness);
        qDebug() << "SlashController: brightness =" << m_brightness;
    }

    match = modeRe.match(content);
    if (match.hasMatch()) {
        m_currentMode = match.captured(1);
        emit modeChanged(m_currentMode);
        qDebug() << "SlashController: mode =" << m_currentMode;
    }
}

void SlashController::setEnabled(bool enabled)
{
    if (!m_available) return;

    QStringList args{"slash"};
    args << (enabled ? "--enable" : "--disable");
    runAsusctl(args);

    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit enabledChanged(enabled);
    }
}

void SlashController::setBrightness(int brightness)
{
    if (!m_available) return;

    brightness = qBound(0, brightness, 255);
    runAsusctl(QStringList{"slash", "-l", QString::number(brightness)});

    if (m_brightness != brightness) {
        m_brightness = brightness;
        emit brightnessChanged(brightness);
    }
}

void SlashController::setMode(const QString &mode)
{
    if (!m_available) return;

    if (!m_availableModes.contains(mode)) {
        emit errorOccurred(tr("Unknown slash mode: %1").arg(mode));
        return;
    }

    runAsusctl(QStringList{"slash", "--mode", mode});

    if (m_currentMode != mode) {
        m_currentMode = mode;
        emit modeChanged(mode);
    }
}

void SlashController::runAsusctl(const QStringList &args)
{
    QProcess process;
    process.start("asusctl", args);
    process.waitForFinished(3000);

    if (process.exitCode() != 0) {
        QString error = QString::fromUtf8(process.readAllStandardError());
        qWarning() << "asusctl command failed:" << args << error;
        emit errorOccurred(error);
    }
}

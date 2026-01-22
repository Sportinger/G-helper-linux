#include "SlashController.h"
#include <QDebug>

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

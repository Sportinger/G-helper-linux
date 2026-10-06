#include "AuraController.h"
#include "AsusdClient.h"
#include <QDebug>
#include <QSettings>

AuraController::AuraController(AsusdClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    connect(m_client, &AsusdClient::ledBrightnessChanged,
            this, &AuraController::onBrightnessChanged);
    connect(m_client, &AsusdClient::connectedChanged,
            this, &AuraController::onClientConnected);
    connect(m_client, &AsusdClient::auraModeDataChanged,
            this, &AuraController::onModeDataChanged);
    connect(m_client, &AsusdClient::supportedAuraModesChanged,
            this, &AuraController::updateAvailableModes);

    QSettings settings("g-helper-linux", "g-helper-linux");
    m_lastBrightness = qBound(int(BrightnessLow), settings.value("Keyboard/lastBrightness", int(BrightnessMedium)).toInt(),
                              int(BrightnessHigh));

    updateAvailableModes();
    updateAvailability();
    if (m_available) {
        m_brightness = static_cast<int>(m_client->ledBrightness());
        onModeDataChanged();
    }
}

AuraController::~AuraController() = default;

void AuraController::updateAvailability()
{
    const bool available = m_client->isConnected() && m_client->hasAura();
    if (m_available != available) {
        m_available = available;
        emit availableChanged(available);
    }
}

void AuraController::updateAvailableModes()
{
    QList<quint32> modes = m_client->supportedAuraModes();
    if (modes.isEmpty()) {
        // Every Aura keyboard supports at least these
        modes = {Static, Breathe, RainbowCycle, RainbowWave, Pulse};
    }

    m_availableModes.clear();
    for (quint32 mode : modes) {
        QVariantMap modeInfo;
        modeInfo["mode"] = static_cast<int>(mode);
        modeInfo["name"] = modeName(static_cast<int>(mode));
        modeInfo["usesColor"] = modeUsesColor(static_cast<int>(mode));
        modeInfo["usesTwoColors"] = modeUsesTwoColors(static_cast<int>(mode));
        modeInfo["usesSpeed"] = modeUsesSpeed(static_cast<int>(mode));
        m_availableModes.append(modeInfo);
    }
    emit availableModesChanged();
}

void AuraController::setBrightness(int level)
{
    if (!m_available) {
        emit errorOccurred(tr("Keyboard backlight control is not available"));
        return;
    }

    if (level < BrightnessOff || level > BrightnessHigh) {
        emit errorOccurred(tr("Invalid brightness level"));
        return;
    }

    if (level == m_brightness)
        return;

    m_client->setLedBrightness(static_cast<quint32>(level));
}

void AuraController::setLightOn(bool on)
{
    setBrightness(on ? m_lastBrightness : BrightnessOff);
}

void AuraController::setMode(int mode)
{
    if (m_currentMode != mode) {
        m_currentMode = mode;
        emit currentModeChanged(mode);
    }
    applyEffect();
}

void AuraController::setColor1(const QColor &color)
{
    if (m_color1 != color) {
        m_color1 = color;
        emit colorsChanged();
    }
    applyEffect();
}

void AuraController::setColor2(const QColor &color)
{
    if (m_color2 != color) {
        m_color2 = color;
        emit colorsChanged();
    }
    applyEffect();
}

void AuraController::setSpeed(int speed)
{
    speed = qBound(0, speed, 2);
    if (m_speed != speed) {
        m_speed = speed;
        emit speedChanged(speed);
    }
    applyEffect();
}

void AuraController::applyEffect()
{
    if (!m_available) {
        emit errorOccurred(tr("Keyboard backlight control is not available"));
        return;
    }

    m_client->setLedMode(static_cast<quint32>(m_currentMode), m_color1, m_color2, m_speed);
}

void AuraController::refresh()
{
    if (m_available)
        m_client->refresh();
}

QString AuraController::modeName(int mode) const
{
    switch (mode) {
        case Static: return tr("Static");
        case Breathe: return tr("Breathe");
        case RainbowCycle: return tr("Rainbow Cycle");
        case RainbowWave: return tr("Rainbow Wave");
        case Star: return tr("Star");
        case Rain: return tr("Rain");
        case Highlight: return tr("Highlight");
        case Laser: return tr("Laser");
        case Ripple: return tr("Ripple");
        case Pulse: return tr("Pulse");
        case Comet: return tr("Comet");
        case Flash: return tr("Flash");
        default: return tr("Mode %1").arg(mode);
    }
}

bool AuraController::modeUsesColor(int mode) const
{
    // The rainbow effects cycle through all colours themselves
    return mode != RainbowCycle && mode != RainbowWave && mode != Rain;
}

bool AuraController::modeUsesTwoColors(int mode) const
{
    return mode == Breathe || mode == Star;
}

bool AuraController::modeUsesSpeed(int mode) const
{
    return mode != Static;
}

void AuraController::onBrightnessChanged(quint32 brightness)
{
    const int newBrightness = static_cast<int>(brightness);
    if (newBrightness > BrightnessOff && newBrightness != m_lastBrightness) {
        m_lastBrightness = qMin(newBrightness, int(BrightnessHigh));
        QSettings settings("g-helper-linux", "g-helper-linux");
        settings.setValue("Keyboard/lastBrightness", m_lastBrightness);
    }
    if (m_brightness != newBrightness) {
        m_brightness = newBrightness;
        emit brightnessChanged(newBrightness);
    }
}

void AuraController::onModeDataChanged()
{
    const int mode = static_cast<int>(m_client->auraMode());
    if (m_currentMode != mode) {
        m_currentMode = mode;
        emit currentModeChanged(mode);
    }
    if (m_color1 != m_client->auraColor1() || m_color2 != m_client->auraColor2()) {
        m_color1 = m_client->auraColor1();
        m_color2 = m_client->auraColor2();
        emit colorsChanged();
    }
    if (m_speed != m_client->auraSpeed()) {
        m_speed = m_client->auraSpeed();
        emit speedChanged(m_speed);
    }
}

void AuraController::onClientConnected(bool connected)
{
    updateAvailability();
    updateAvailableModes();

    if (connected && m_available) {
        m_brightness = static_cast<int>(m_client->ledBrightness());
        emit brightnessChanged(m_brightness);
    }
}

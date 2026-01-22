#include "AuraController.h"
#include "AsusdClient.h"
#include <QProcess>
#include <QDebug>
#include <QRegularExpression>

AuraController::AuraController(AsusdClient *client, QObject *parent)
    : QObject(parent)
    , m_client(client)
{
    connect(m_client, &AsusdClient::ledBrightnessChanged,
            this, &AuraController::onBrightnessChanged);
    connect(m_client, &AsusdClient::connectedChanged,
            this, &AuraController::onClientConnected);
    connect(m_client, &AsusdClient::errorOccurred,
            this, &AuraController::errorOccurred);

    initAvailableModes();

    m_available = m_client->isConnected();
    if (m_available) {
        m_brightness = static_cast<int>(m_client->ledBrightness());
        fetchCurrentState();
    }
}

void AuraController::fetchCurrentState()
{
    // Read current LED state from asusctl
    QProcess process;
    process.start("asusctl", QStringList{"led-mode", "-c"});
    process.waitForFinished(3000);

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    qDebug() << "AuraController: Current LED state:" << output;

    // Parse output to get current mode and colors
    // Example: "Current mode: Static, Colour 1: #00a0e0"
    for (const QString &line : output.split('\n')) {
        QString trimmed = line.trimmed().toLower();
        if (trimmed.contains("mode")) {
            if (line.contains("Static", Qt::CaseInsensitive)) m_currentMode = Static;
            else if (line.contains("Breathe", Qt::CaseInsensitive)) m_currentMode = Breathe;
            else if (line.contains("Strobe", Qt::CaseInsensitive)) m_currentMode = Strobe;
            else if (line.contains("Rainbow", Qt::CaseInsensitive)) m_currentMode = Rainbow;
            else if (line.contains("Pulse", Qt::CaseInsensitive)) m_currentMode = Pulse;
            else if (line.contains("Comet", Qt::CaseInsensitive)) m_currentMode = Comet;
            else if (line.contains("Off", Qt::CaseInsensitive)) m_currentMode = Off;
            emit currentModeChanged(m_currentMode);
        }
        if (trimmed.contains("colour") || trimmed.contains("color")) {
            // Try to extract hex color
            QRegularExpression re("#([0-9A-Fa-f]{6})");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                QString hex = "#" + match.captured(1);
                if (trimmed.contains("1") || !m_color1.isValid()) {
                    m_color1 = QColor(hex);
                } else {
                    m_color2 = QColor(hex);
                }
                emit colorsChanged();
            }
        }
    }
}

AuraController::~AuraController() = default;

void AuraController::initAvailableModes()
{
    m_availableModes.clear();

    auto addMode = [this](int mode, const QString &name, const QString &icon) {
        QVariantMap modeInfo;
        modeInfo["mode"] = mode;
        modeInfo["name"] = name;
        modeInfo["icon"] = icon;
        modeInfo["usesColor"] = modeUsesColor(mode);
        modeInfo["usesTwoColors"] = modeUsesTwoColors(mode);
        modeInfo["usesSpeed"] = modeUsesSpeed(mode);
        m_availableModes.append(modeInfo);
    };

    addMode(Off, tr("Off"), "qrc:/icons/led-off.svg");
    addMode(Static, tr("Static"), "qrc:/icons/led-static.svg");
    addMode(Breathe, tr("Breathe"), "qrc:/icons/led-breathe.svg");
    addMode(Strobe, tr("Strobe"), "qrc:/icons/led-strobe.svg");
    addMode(Rainbow, tr("Rainbow"), "qrc:/icons/led-rainbow.svg");
    addMode(Pulse, tr("Pulse"), "qrc:/icons/led-pulse.svg");
    addMode(Comet, tr("Comet"), "qrc:/icons/led-comet.svg");
}

void AuraController::setBrightness(int level)
{
    if (!m_available) {
        emit errorOccurred(tr("Aura control is not available"));
        return;
    }

    if (level < BrightnessOff || level > BrightnessHigh) {
        emit errorOccurred(tr("Invalid brightness level"));
        return;
    }

    m_client->setLedBrightness(static_cast<quint32>(level));
}

void AuraController::setMode(int mode)
{
    if (m_currentMode != mode) {
        m_currentMode = mode;
        emit currentModeChanged(mode);
    }
}

void AuraController::setColor1(const QColor &color)
{
    if (m_color1 != color) {
        m_color1 = color;
        emit colorsChanged();
        // Automatically apply the effect when color changes
        applyEffect();
    }
}

void AuraController::setColor2(const QColor &color)
{
    if (m_color2 != color) {
        m_color2 = color;
        emit colorsChanged();
    }
}

void AuraController::setSpeed(int speed)
{
    if (speed < 0 || speed > 2) {
        speed = 1; // Default to medium
    }
    if (m_speed != speed) {
        m_speed = speed;
        emit speedChanged(speed);
    }
}

void AuraController::applyEffect()
{
    if (!m_available) {
        emit errorOccurred(tr("Aura control is not available"));
        return;
    }

    m_client->setLedMode(static_cast<quint32>(m_currentMode), m_color1, m_color2, static_cast<quint8>(m_speed));
}

void AuraController::refresh()
{
    if (m_available) {
        m_client->refresh();
    }
}

QString AuraController::modeName(int mode) const
{
    switch (mode) {
        case Off: return tr("Off");
        case Static: return tr("Static");
        case Breathe: return tr("Breathe");
        case Strobe: return tr("Strobe");
        case Rainbow: return tr("Rainbow");
        case Star: return tr("Star");
        case Rain: return tr("Rain");
        case Highlight: return tr("Highlight");
        case Laser: return tr("Laser");
        case Ripple: return tr("Ripple");
        case Pulse: return tr("Pulse");
        case Comet: return tr("Comet");
        case Flash: return tr("Flash");
        default: return tr("Unknown");
    }
}

bool AuraController::modeUsesColor(int mode) const
{
    // Rainbow doesn't use custom colors
    return mode != Rainbow && mode != Off;
}

bool AuraController::modeUsesTwoColors(int mode) const
{
    // These modes use two colors
    return mode == Breathe || mode == Strobe || mode == Comet || mode == Pulse;
}

bool AuraController::modeUsesSpeed(int mode) const
{
    // Static doesn't have speed
    return mode != Static && mode != Off;
}

void AuraController::onBrightnessChanged(quint32 brightness)
{
    int newBrightness = static_cast<int>(brightness);
    if (m_brightness != newBrightness) {
        m_brightness = newBrightness;
        emit brightnessChanged(newBrightness);
    }
}

void AuraController::onClientConnected(bool connected)
{
    if (m_available != connected) {
        m_available = connected;
        emit availableChanged(connected);

        if (connected) {
            m_brightness = static_cast<int>(m_client->ledBrightness());
            emit brightnessChanged(m_brightness);
        }
    }
}

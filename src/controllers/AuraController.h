#ifndef AURACONTROLLER_H
#define AURACONTROLLER_H

#include <QObject>
#include <QColor>
#include <QVariantList>
#include "DBusTypes.h"

class AsusdClient;

// Keyboard backlight (Aura). Mode numbers are the asusd AuraModeNum values.
class AuraController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int brightness READ brightness NOTIFY brightnessChanged)
    Q_PROPERTY(bool lightOn READ isLightOn NOTIFY brightnessChanged)
    Q_PROPERTY(int currentMode READ currentMode NOTIFY currentModeChanged)
    Q_PROPERTY(QColor color1 READ color1 NOTIFY colorsChanged)
    Q_PROPERTY(QColor color2 READ color2 NOTIFY colorsChanged)
    Q_PROPERTY(int speed READ speed NOTIFY speedChanged)
    Q_PROPERTY(QVariantList availableModes READ availableModes NOTIFY availableModesChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    enum Mode {
        Static = AsusdAuraMode::Static,
        Breathe = AsusdAuraMode::Breathe,
        RainbowCycle = AsusdAuraMode::RainbowCycle,
        RainbowWave = AsusdAuraMode::RainbowWave,
        Star = AsusdAuraMode::Star,
        Rain = AsusdAuraMode::Rain,
        Highlight = AsusdAuraMode::Highlight,
        Laser = AsusdAuraMode::Laser,
        Ripple = AsusdAuraMode::Ripple,
        Pulse = AsusdAuraMode::Pulse,
        Comet = AsusdAuraMode::Comet,
        Flash = AsusdAuraMode::Flash
    };
    Q_ENUM(Mode)

    enum Brightness {
        BrightnessOff = 0,
        BrightnessLow = 1,
        BrightnessMedium = 2,
        BrightnessHigh = 3
    };
    Q_ENUM(Brightness)

    explicit AuraController(AsusdClient *client, QObject *parent = nullptr);
    ~AuraController() override;

    int brightness() const { return m_brightness; }
    bool isLightOn() const { return m_brightness > BrightnessOff; }
    int currentMode() const { return m_currentMode; }
    QColor color1() const { return m_color1; }
    QColor color2() const { return m_color2; }
    int speed() const { return m_speed; }
    QVariantList availableModes() const { return m_availableModes; }
    bool isAvailable() const { return m_available; }

    // All setters apply the effect to the keyboard immediately
    Q_INVOKABLE void setBrightness(int level);
    // Off = brightness 0; on restores the last used brightness
    Q_INVOKABLE void setLightOn(bool on);
    Q_INVOKABLE void setMode(int mode);
    Q_INVOKABLE void setColor1(const QColor &color);
    Q_INVOKABLE void setColor2(const QColor &color);
    Q_INVOKABLE void setSpeed(int speed);
    Q_INVOKABLE void applyEffect();
    Q_INVOKABLE void refresh();

    Q_INVOKABLE QString modeName(int mode) const;
    Q_INVOKABLE bool modeUsesColor(int mode) const;
    Q_INVOKABLE bool modeUsesTwoColors(int mode) const;
    Q_INVOKABLE bool modeUsesSpeed(int mode) const;

signals:
    void brightnessChanged(int brightness);
    void currentModeChanged(int mode);
    void colorsChanged();
    void speedChanged(int speed);
    void availableModesChanged();
    void availableChanged(bool available);
    void errorOccurred(const QString &error);

private slots:
    void onBrightnessChanged(quint32 brightness);
    void onClientConnected(bool connected);
    void onModeDataChanged();
    void updateAvailableModes();

private:
    void updateAvailability();

    AsusdClient *m_client;

    int m_brightness = BrightnessMedium;
    int m_lastBrightness = BrightnessMedium;   // last level > 0, for switching back on
    int m_currentMode = Static;
    QColor m_color1 = QColor(255, 0, 0);
    QColor m_color2 = QColor(0, 0, 0);
    int m_speed = 1;
    QVariantList m_availableModes;
    bool m_available = false;
};

#endif // AURACONTROLLER_H

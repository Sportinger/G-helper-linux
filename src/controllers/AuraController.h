#ifndef AURACONTROLLER_H
#define AURACONTROLLER_H

#include <QObject>
#include <QColor>
#include <QVariantList>

class AsusdClient;

class AuraController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int brightness READ brightness NOTIFY brightnessChanged)
    Q_PROPERTY(int currentMode READ currentMode NOTIFY currentModeChanged)
    Q_PROPERTY(QColor color1 READ color1 NOTIFY colorsChanged)
    Q_PROPERTY(QColor color2 READ color2 NOTIFY colorsChanged)
    Q_PROPERTY(int speed READ speed NOTIFY speedChanged)
    Q_PROPERTY(QVariantList availableModes READ availableModes CONSTANT)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    enum Mode {
        Static = 0,
        Breathe = 1,
        Strobe = 2,
        Rainbow = 3,
        Star = 4,
        Rain = 5,
        Highlight = 6,
        Laser = 7,
        Ripple = 8,
        Pulse = 9,
        Comet = 10,
        Flash = 11,
        Off = 255
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
    int currentMode() const { return m_currentMode; }
    QColor color1() const { return m_color1; }
    QColor color2() const { return m_color2; }
    int speed() const { return m_speed; }
    QVariantList availableModes() const { return m_availableModes; }
    bool isAvailable() const { return m_available; }

    Q_INVOKABLE void setBrightness(int level);
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
    void availableChanged(bool available);
    void errorOccurred(const QString &error);

private slots:
    void onBrightnessChanged(quint32 brightness);
    void onClientConnected(bool connected);

private:
    void initAvailableModes();
    void fetchCurrentState();

    AsusdClient *m_client;

    int m_brightness = BrightnessMedium;
    int m_currentMode = Static;
    QColor m_color1 = QColor(0, 160, 224); // Cyan accent
    QColor m_color2 = QColor(255, 255, 255);
    int m_speed = 1;
    QVariantList m_availableModes;
    bool m_available = false;
};

#endif // AURACONTROLLER_H

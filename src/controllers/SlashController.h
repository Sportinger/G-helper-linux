#ifndef SLASHCONTROLLER_H
#define SLASHCONTROLLER_H

#include <QObject>
#include <QProcess>
#include <QStringList>

class SlashController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)
    Q_PROPERTY(QString currentMode READ currentMode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QStringList availableModes READ availableModes CONSTANT)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    explicit SlashController(QObject *parent = nullptr);
    ~SlashController() override;

    bool isEnabled() const { return m_enabled; }
    int brightness() const { return m_brightness; }
    QString currentMode() const { return m_currentMode; }
    QStringList availableModes() const { return m_availableModes; }
    bool isAvailable() const { return m_available; }

    Q_INVOKABLE void setEnabled(bool enabled);
    Q_INVOKABLE void setBrightness(int brightness);
    Q_INVOKABLE void setMode(const QString &mode);

signals:
    void enabledChanged(bool enabled);
    void brightnessChanged(int brightness);
    void modeChanged(const QString &mode);
    void availableChanged(bool available);
    void errorOccurred(const QString &error);

private:
    void runAsusctl(const QStringList &args);
    void checkAvailability();

    bool m_enabled = true;
    int m_brightness = 128;
    QString m_currentMode = "Static";
    QStringList m_availableModes;
    bool m_available = false;
};

#endif // SLASHCONTROLLER_H

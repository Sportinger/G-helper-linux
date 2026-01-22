#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>
#include <QColor>

class Settings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool startMinimized READ startMinimized WRITE setStartMinimized NOTIFY startMinimizedChanged)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
    Q_PROPERTY(bool showTrayIcon READ showTrayIcon WRITE setShowTrayIcon NOTIFY showTrayIconChanged)
    Q_PROPERTY(bool minimizeToTray READ minimizeToTray WRITE setMinimizeToTray NOTIFY minimizeToTrayChanged)
    Q_PROPERTY(int windowX READ windowX WRITE setWindowX NOTIFY windowXChanged)
    Q_PROPERTY(int windowY READ windowY WRITE setWindowY NOTIFY windowYChanged)
    Q_PROPERTY(int defaultPerformanceProfile READ defaultPerformanceProfile WRITE setDefaultPerformanceProfile NOTIFY defaultPerformanceProfileChanged)
    Q_PROPERTY(int defaultGpuMode READ defaultGpuMode WRITE setDefaultGpuMode NOTIFY defaultGpuModeChanged)

public:
    explicit Settings(QObject *parent = nullptr);
    ~Settings() override;

    // Window settings
    bool startMinimized() const;
    void setStartMinimized(bool value);

    bool autoStart() const;
    void setAutoStart(bool value);

    bool showTrayIcon() const;
    void setShowTrayIcon(bool value);

    bool minimizeToTray() const;
    void setMinimizeToTray(bool value);

    int windowX() const;
    void setWindowX(int value);

    int windowY() const;
    void setWindowY(int value);

    // Default settings
    int defaultPerformanceProfile() const;
    void setDefaultPerformanceProfile(int value);

    int defaultGpuMode() const;
    void setDefaultGpuMode(int value);

    Q_INVOKABLE void save();
    Q_INVOKABLE void load();
    Q_INVOKABLE void resetToDefaults();

signals:
    void startMinimizedChanged();
    void autoStartChanged();
    void showTrayIconChanged();
    void minimizeToTrayChanged();
    void windowXChanged();
    void windowYChanged();
    void defaultPerformanceProfileChanged();
    void defaultGpuModeChanged();

private:
    void setupAutostart(bool enable);

    QSettings m_settings;

    bool m_startMinimized = false;
    bool m_autoStart = false;
    bool m_showTrayIcon = true;
    bool m_minimizeToTray = true;
    int m_windowX = -1;
    int m_windowY = -1;
    int m_defaultPerformanceProfile = 1; // Balanced
    int m_defaultGpuMode = 1; // Hybrid
};

#endif // SETTINGS_H

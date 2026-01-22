#include "Settings.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_settings("g-helper-linux", "g-helper-linux")
{
    load();
}

Settings::~Settings()
{
    save();
}

bool Settings::startMinimized() const { return m_startMinimized; }
void Settings::setStartMinimized(bool value)
{
    if (m_startMinimized != value) {
        m_startMinimized = value;
        emit startMinimizedChanged();
        save();
    }
}

bool Settings::autoStart() const { return m_autoStart; }
void Settings::setAutoStart(bool value)
{
    if (m_autoStart != value) {
        m_autoStart = value;
        setupAutostart(value);
        emit autoStartChanged();
        save();
    }
}

bool Settings::showTrayIcon() const { return m_showTrayIcon; }
void Settings::setShowTrayIcon(bool value)
{
    if (m_showTrayIcon != value) {
        m_showTrayIcon = value;
        emit showTrayIconChanged();
        save();
    }
}

bool Settings::minimizeToTray() const { return m_minimizeToTray; }
void Settings::setMinimizeToTray(bool value)
{
    if (m_minimizeToTray != value) {
        m_minimizeToTray = value;
        emit minimizeToTrayChanged();
        save();
    }
}

int Settings::windowX() const { return m_windowX; }
void Settings::setWindowX(int value)
{
    if (m_windowX != value) {
        m_windowX = value;
        emit windowXChanged();
    }
}

int Settings::windowY() const { return m_windowY; }
void Settings::setWindowY(int value)
{
    if (m_windowY != value) {
        m_windowY = value;
        emit windowYChanged();
    }
}

int Settings::defaultPerformanceProfile() const { return m_defaultPerformanceProfile; }
void Settings::setDefaultPerformanceProfile(int value)
{
    if (m_defaultPerformanceProfile != value) {
        m_defaultPerformanceProfile = value;
        emit defaultPerformanceProfileChanged();
        save();
    }
}

int Settings::defaultGpuMode() const { return m_defaultGpuMode; }
void Settings::setDefaultGpuMode(int value)
{
    if (m_defaultGpuMode != value) {
        m_defaultGpuMode = value;
        emit defaultGpuModeChanged();
        save();
    }
}

void Settings::save()
{
    m_settings.beginGroup("Window");
    m_settings.setValue("startMinimized", m_startMinimized);
    m_settings.setValue("x", m_windowX);
    m_settings.setValue("y", m_windowY);
    m_settings.endGroup();

    m_settings.beginGroup("General");
    m_settings.setValue("autoStart", m_autoStart);
    m_settings.setValue("showTrayIcon", m_showTrayIcon);
    m_settings.setValue("minimizeToTray", m_minimizeToTray);
    m_settings.endGroup();

    m_settings.beginGroup("Defaults");
    m_settings.setValue("performanceProfile", m_defaultPerformanceProfile);
    m_settings.setValue("gpuMode", m_defaultGpuMode);
    m_settings.endGroup();

    m_settings.sync();
}

void Settings::load()
{
    m_settings.beginGroup("Window");
    m_startMinimized = m_settings.value("startMinimized", false).toBool();
    m_windowX = m_settings.value("x", -1).toInt();
    m_windowY = m_settings.value("y", -1).toInt();
    m_settings.endGroup();

    m_settings.beginGroup("General");
    m_autoStart = m_settings.value("autoStart", false).toBool();
    m_showTrayIcon = m_settings.value("showTrayIcon", true).toBool();
    m_minimizeToTray = m_settings.value("minimizeToTray", true).toBool();
    m_settings.endGroup();

    m_settings.beginGroup("Defaults");
    m_defaultPerformanceProfile = m_settings.value("performanceProfile", 1).toInt();
    m_defaultGpuMode = m_settings.value("gpuMode", 1).toInt();
    m_settings.endGroup();
}

void Settings::resetToDefaults()
{
    m_startMinimized = false;
    m_autoStart = false;
    m_showTrayIcon = true;
    m_minimizeToTray = true;
    m_windowX = -1;
    m_windowY = -1;
    m_defaultPerformanceProfile = 1;
    m_defaultGpuMode = 1;

    emit startMinimizedChanged();
    emit autoStartChanged();
    emit showTrayIconChanged();
    emit minimizeToTrayChanged();
    emit windowXChanged();
    emit windowYChanged();
    emit defaultPerformanceProfileChanged();
    emit defaultGpuModeChanged();

    save();
}

void Settings::setupAutostart(bool enable)
{
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QString desktopFile = autostartDir + "/g-helper-linux.desktop";

    QDir dir;
    if (!dir.exists(autostartDir)) {
        dir.mkpath(autostartDir);
    }

    if (enable) {
        QFile file(desktopFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Name=G-Helper Linux\n";
            out << "Exec=g-helper-linux --minimized\n";
            out << "Icon=g-helper-linux\n";
            out << "Comment=ASUS ROG Laptop Control\n";
            out << "Categories=System;Utility;\n";
            out << "X-GNOME-Autostart-enabled=true\n";
            file.close();
        }
    } else {
        QFile::remove(desktopFile);
    }
}

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QScreen>
#include <QWindow>

using namespace Qt::StringLiterals;

#include "core/Application.h"
#include "core/Settings.h"
#include "dbus/DBusWatcher.h"
#include "dbus/AsusdClient.h"
#include "dbus/SuperGfxClient.h"
#include "controllers/PerformanceController.h"
#include "controllers/GpuController.h"
#include "controllers/BatteryController.h"
#include "controllers/FanController.h"
#include "controllers/AuraController.h"
#include "controllers/SystemMonitor.h"
#include "controllers/SlashController.h"
#include "tray/TrayManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("G-Helper Linux");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("g-helper-linux");
    app.setOrganizationDomain("github.com/g-helper-linux");
    app.setWindowIcon(QIcon(":/icons/g-helper.svg"));

    QQuickStyle::setStyle("Basic");

    // Initialize core components
    Settings settings;
    DBusWatcher dbusWatcher;

    // Initialize D-Bus clients
    AsusdClient asusdClient;
    SuperGfxClient superGfxClient;

    // Initialize controllers
    PerformanceController performanceController(&asusdClient);
    GpuController gpuController(&superGfxClient);
    BatteryController batteryController(&asusdClient);
    FanController fanController(&asusdClient);
    AuraController auraController(&asusdClient);
    SystemMonitor systemMonitor;
    SlashController slashController;

    // Initialize tray manager
    TrayManager trayManager(&performanceController, &gpuController);

    // Setup QML engine
    QQmlApplicationEngine engine;
    engine.addImportPath("qrc:/");

    // Register singletons
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "Settings", &settings);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "DBusWatcher", &dbusWatcher);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "PerformanceController", &performanceController);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "GpuController", &gpuController);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "BatteryController", &batteryController);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "FanController", &fanController);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "AuraController", &auraController);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "SystemMonitor", &systemMonitor);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "SlashController", &slashController);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "TrayManager", &trayManager);

    // Load main QML
    const QUrl url(u"qrc:/GHelperLinux/qml/Main.qml"_s);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty())
        return -1;

    // Position window at bottom right
    QObject *rootObject = engine.rootObjects().first();
    QWindow *window = qobject_cast<QWindow*>(rootObject);

    auto positionWindow = [window]() {
        if (!window) return;
        QScreen *screen = QGuiApplication::primaryScreen();
        if (screen) {
            QRect availableGeometry = screen->availableGeometry();
            int x = availableGeometry.right() - window->width() - 12;
            int y = availableGeometry.bottom() - window->height() - 60;
            window->setPosition(x, y);
        }
    };

    // Position on startup
    positionWindow();

    // Reposition when shown from tray
    QObject::connect(&trayManager, &TrayManager::showWindowRequested, positionWindow);

    // Start monitoring
    systemMonitor.start();

    // Connect D-Bus watcher signals
    QObject::connect(&dbusWatcher, &DBusWatcher::asusdConnectedChanged, [&](bool connected) {
        if (connected) {
            performanceController.refresh();
            batteryController.refresh();
            fanController.refresh();
            auraController.refresh();
        }
    });

    QObject::connect(&dbusWatcher, &DBusWatcher::supergfxConnectedChanged, [&](bool connected) {
        if (connected) {
            superGfxClient.reconnect();
            gpuController.refresh();
        }
    });

    // Initial reconnect if services are already available but clients missed it
    if (dbusWatcher.supergfxConnected() && !superGfxClient.isConnected()) {
        superGfxClient.reconnect();
        gpuController.refresh();
    }

    return app.exec();
}

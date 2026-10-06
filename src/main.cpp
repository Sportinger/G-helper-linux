#include <QApplication>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QScreen>
#include <QWindow>
#include <QSystemTrayIcon>
#include <QQuickWindow>
#include <QTimer>

using namespace Qt::StringLiterals;

#include "core/Settings.h"
#include "core/Notifications.h"
#include "core/PowerSupply.h"
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
    // Lets the desktop match the window to g-helper-linux.desktop (dock icon)
    app.setDesktopFileName("g-helper-linux");

    QCommandLineParser parser;
    parser.setApplicationDescription("ASUS ROG laptop control");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption minimizedOption("minimized", "Start hidden in the system tray.");
    parser.addOption(minimizedOption);
    parser.process(app);

    QQuickStyle::setStyle("Basic");

    // Initialize core components
    Settings settings;
    settings.refreshAutostartEntry();
    Notifications notifications;
    DBusWatcher dbusWatcher;

    // Initialize D-Bus clients (they reconnect on their own when the
    // daemons are restarted)
    AsusdClient asusdClient;
    SuperGfxClient superGfxClient;

    // Initialize controllers
    PerformanceController performanceController(&asusdClient);
    GpuController gpuController(&superGfxClient, &asusdClient);
    BatteryController batteryController(&asusdClient);
    FanController fanController(&asusdClient);
    AuraController auraController(&asusdClient);
    SystemMonitor systemMonitor;
    SlashController slashController(&asusdClient);

    // Every error is shown exactly once in the UI
    QObject::connect(&asusdClient, &AsusdClient::errorOccurred, &notifications, &Notifications::reportError);
    QObject::connect(&superGfxClient, &SuperGfxClient::errorOccurred, &notifications, &Notifications::reportError);
    QObject::connect(&performanceController, &PerformanceController::errorOccurred, &notifications, &Notifications::reportError);
    QObject::connect(&gpuController, &GpuController::errorOccurred, &notifications, &Notifications::reportError);
    QObject::connect(&batteryController, &BatteryController::errorOccurred, &notifications, &Notifications::reportError);
    QObject::connect(&fanController, &FanController::errorOccurred, &notifications, &Notifications::reportError);
    QObject::connect(&auraController, &AuraController::errorOccurred, &notifications, &Notifications::reportError);
    QObject::connect(&slashController, &SlashController::errorOccurred, &notifications, &Notifications::reportError);

    // GPU "Optimized" mode follows the power source
    QObject::connect(&systemMonitor, &SystemMonitor::onBatteryChanged,
                     &gpuController, &GpuController::setOnBattery);

    // Initialize tray manager
    TrayManager trayManager(&performanceController, &gpuController);
    trayManager.setVisible(settings.showTrayIcon());
    QObject::connect(&settings, &Settings::showTrayIconChanged, &trayManager, [&]() {
        trayManager.setVisible(settings.showTrayIcon());
    });

    // Keep running in the tray when the window is closed/hidden
    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    app.setQuitOnLastWindowClosed(!trayAvailable);

    // Setup QML engine
    QQmlApplicationEngine engine;
    engine.addImportPath("qrc:/");

    // Register singletons
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "Settings", &settings);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "Notifications", &notifications);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "DBusWatcher", &dbusWatcher);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "PerformanceController", &performanceController);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "GpuController", &gpuController);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "BatteryController", &batteryController);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "FanController", &fanController);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "AuraController", &auraController);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "SystemMonitor", &systemMonitor);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "SlashController", &slashController);
    qmlRegisterSingletonInstance("GHelperLinux", 1, 0, "TrayManager", &trayManager);

    // Device name for the window title, e.g. "ROG Zephyrus G14 GA403WW"
    const QString deviceName = QStringList{
        PowerSupply::readAttribute("/sys/class/dmi/id", "product_family"),
        PowerSupply::readAttribute("/sys/class/dmi/id", "board_name")
    }.join(' ').trimmed();
    engine.rootContext()->setContextProperty("deviceName", deviceName);
    engine.rootContext()->setContextProperty("appVersion", app.applicationVersion());

    // Load main QML
    const QUrl url(u"qrc:/GHelperLinux/qml/Main.qml"_s);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty())
        return -1;

    QObject *rootObject = engine.rootObjects().first();
    QWindow *window = qobject_cast<QWindow*>(rootObject);

    // Position window at bottom right of the screen it is on
    auto positionWindow = [window]() {
        if (!window) return;
        QScreen *screen = window->screen() ? window->screen() : QGuiApplication::primaryScreen();
        if (screen) {
            const QRect availableGeometry = screen->availableGeometry();
            const int x = availableGeometry.right() - window->width() - 12;
            const int y = availableGeometry.bottom() - window->height() - 12;
            window->setPosition(x, y);
        }
    };

    positionWindow();

    // Reposition when shown from tray
    QObject::connect(&trayManager, &TrayManager::showWindowRequested, positionWindow);

    // Start hidden only if there is a tray icon to bring the window back
    const bool startHidden = (parser.isSet(minimizedOption) || settings.startMinimized())
                             && trayAvailable && settings.showTrayIcon();
    if (window && !startHidden)
        window->show();

    // Start monitoring
    systemMonitor.start();

    // Development aid: GHELPER_SCREENSHOT=/path/shot.png renders the main
    // window into a PNG after it has loaded its data, then exits.
    // Use with QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software.
    const QString screenshotPath = qEnvironmentVariable("GHELPER_SCREENSHOT");
    if (!screenshotPath.isEmpty()) {
        if (window)
            window->show();
        QTimer::singleShot(3000, &app, [window, screenshotPath]() {
            if (auto *quickWindow = qobject_cast<QQuickWindow *>(window))
                quickWindow->grabWindow().save(screenshotPath);
            QCoreApplication::quit();
        });
    }

    return app.exec();
}

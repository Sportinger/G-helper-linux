cmake_minimum_required(VERSION 3.16)

project(g-helper-linux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    DBus
    Widgets
)

qt_standard_project_setup()

# Source files
set(SOURCES
    src/main.cpp
    src/core/Application.cpp
    src/core/Settings.cpp
    src/dbus/DBusWatcher.cpp
    src/dbus/AsusdClient.cpp
    src/dbus/SuperGfxClient.cpp
    src/controllers/PerformanceController.cpp
    src/controllers/GpuController.cpp
    src/controllers/BatteryController.cpp
    src/controllers/FanController.cpp
    src/controllers/AuraController.cpp
    src/controllers/SystemMonitor.cpp
    src/models/FanCurveModel.cpp
    src/models/AuraModeModel.cpp
    src/tray/TrayManager.cpp
)

set(HEADERS
    src/core/Application.h
    src/core/Settings.h
    src/dbus/DBusTypes.h
    src/dbus/DBusWatcher.h
    src/dbus/AsusdClient.h
    src/dbus/SuperGfxClient.h
    src/controllers/PerformanceController.h
    src/controllers/GpuController.h
    src/controllers/BatteryController.h
    src/controllers/FanController.h
    src/controllers/AuraController.h
    src/controllers/SystemMonitor.h
    src/models/FanCurveModel.h
    src/models/AuraModeModel.h
    src/tray/TrayManager.h
)

# Resources
qt_add_resources(RESOURCES resources/resources.qrc)

# Executable
qt_add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# Mark Theme.qml as a singleton
set_source_files_properties(qml/theme/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# QML module
qt_add_qml_module(${PROJECT_NAME}
    URI GHelperLinux
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/theme/Theme.qml
        qml/components/ModeButton.qml
        qml/components/ModeButtonGroup.qml
        qml/components/SectionHeader.qml
        qml/components/StatusDisplay.qml
        qml/components/PercentSlider.qml
        qml/components/ColorPicker.qml
        qml/components/DropdownSelect.qml
        qml/components/IconButton.qml
        qml/panels/PerformancePanel.qml
        qml/panels/GpuPanel.qml
        qml/panels/KeyboardPanel.qml
        qml/panels/BatteryPanel.qml
        qml/panels/FanCurvePanel.qml
        qml/dialogs/FanCurveDialog.qml
        qml/dialogs/AboutDialog.qml
    SOURCES
        ${SOURCES}
        ${HEADERS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::DBus
    Qt6::Widgets
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dbus
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/tray
)

# Install
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES desktop/g-helper-linux.desktop
    DESTINATION share/applications
)

install(FILES desktop/g-helper-linux.svg
    DESTINATION share/icons/hicolor/scalable/apps
)

#include "Application.h"
#include <QGuiApplication>

Application::Application(QObject *parent)
    : QObject(parent)
{
}

Application::~Application() = default;

QString Application::version() const
{
    return VERSION;
}

QString Application::applicationName() const
{
    return APP_NAME;
}

void Application::quit()
{
    QGuiApplication::quit();
}

void Application::minimize()
{
    // Will be handled by QML window
}

void Application::showAbout()
{
    emit aboutRequested();
}

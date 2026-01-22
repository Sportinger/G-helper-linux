#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <QGuiApplication>

class Application : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString applicationName READ applicationName CONSTANT)

public:
    explicit Application(QObject *parent = nullptr);
    ~Application() override;

    QString version() const;
    QString applicationName() const;

    Q_INVOKABLE void quit();
    Q_INVOKABLE void minimize();
    Q_INVOKABLE void showAbout();

signals:
    void aboutRequested();

private:
    static constexpr const char* VERSION = "0.1.0";
    static constexpr const char* APP_NAME = "G-Helper Linux";
};

#endif // APPLICATION_H

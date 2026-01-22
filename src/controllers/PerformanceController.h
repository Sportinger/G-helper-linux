#ifndef PERFORMANCECONTROLLER_H
#define PERFORMANCECONTROLLER_H

#include <QObject>

class AsusdClient;

class PerformanceController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentProfile READ currentProfile NOTIFY currentProfileChanged)
    Q_PROPERTY(QString currentProfileName READ currentProfileName NOTIFY currentProfileChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    enum Profile {
        Quiet = 0,      // Silent
        Balanced = 1,   // Balanced
        Performance = 2 // Turbo
    };
    Q_ENUM(Profile)

    explicit PerformanceController(AsusdClient *client, QObject *parent = nullptr);
    ~PerformanceController() override;

    int currentProfile() const { return m_currentProfile; }
    QString currentProfileName() const;
    bool isAvailable() const { return m_available; }

    Q_INVOKABLE void setProfile(int profile);
    Q_INVOKABLE QString profileName(int profile) const;
    Q_INVOKABLE QString profileDescription(int profile) const;
    Q_INVOKABLE QString profileIcon(int profile) const;
    Q_INVOKABLE void refresh();

signals:
    void currentProfileChanged(int profile);
    void availableChanged(bool available);
    void errorOccurred(const QString &error);

private slots:
    void onProfileChanged(quint32 profile);
    void onClientConnected(bool connected);

private:
    AsusdClient *m_client;
    int m_currentProfile = Balanced;
    bool m_available = false;
};

#endif // PERFORMANCECONTROLLER_H

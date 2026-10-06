#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H

#include <QObject>
#include <QString>

// Single place where all error messages of clients and controllers end up,
// so the UI shows each error exactly once.
class Notifications : public QObject
{
    Q_OBJECT

public:
    explicit Notifications(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void reportError(const QString &message) { emit error(message); }
    void reportInfo(const QString &message) { emit info(message); }

signals:
    void error(const QString &message);
    void info(const QString &message);
};

#endif // NOTIFICATIONS_H

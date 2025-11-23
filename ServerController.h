#ifndef SERVERCONTROLLER_H
#define SERVERCONTROLLER_H

#include <QThread>
#include <QObject>
#include <QMutex>
#include "Chat.h"

class ServerController : public QThread
{
    Q_OBJECT

public:
    explicit ServerController(QObject *parent = nullptr);
    ~ServerController();

    void stopServer();
    bool isServerRunning() const;

    // Методы для статистики
    int getOnlineUsersCount() const;
    int getTotalUsersCount() const;
    QStringList getOnlineUsersList() const;
    QStringList getAllUsersList() const;

    // методы для управления админами
    bool addAdmin(const QString& login, const QString& password);
    bool removeAdmin(const QString& login);
    bool changeAdminPassword(const QString& login, const QString& newPassword);
    QStringList getAdminList() const;
signals:
    void serverStarted();
    void serverStopped();
    void serverError(const QString& error);
    void logMessage(const QString& message);

protected:
    void run() override;

private:
    mutable QMutex mutex;
    std::unique_ptr<Chat> chat;
    bool running;
};

#endif

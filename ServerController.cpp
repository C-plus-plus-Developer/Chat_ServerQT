#include "ServerController.h"
#include <iostream>
#include <QDateTime>

ServerController::ServerController(QObject *parent)
    : QThread(parent)
    , running(false)
{
}

ServerController::~ServerController()
{
    stopServer();
    if (isRunning()) {
        wait(3000);
    }
}

void ServerController::stopServer()
{
    QMutexLocker locker(&mutex);
    if (chat && running) {
        running = false;
        chat->stop();
        emit logMessage("Server stop requested");
    }
}

bool ServerController::isServerRunning() const
{
    QMutexLocker locker(&mutex);
    return running;
}

void ServerController::run()
{
    try {
        {
            QMutexLocker locker(&mutex);
            chat = std::make_unique<Chat>();
            running = true;
        }

        emit serverStarted();
        emit logMessage("✅ Сервер успешно запущен!");
        emit logMessage("📍 Порт: 7777");
        emit logMessage("🗄️ База данных подключена");

        // Запускаем сервер
        chat->runNonBlocking();

        {
            QMutexLocker locker(&mutex);
            running = false;
            chat.reset(); // Освобождаем ресурсы
        }

        emit serverStopped();
        emit logMessage("❌ Сервер остановлен");

    } catch (const std::exception& e) {
        {
            QMutexLocker locker(&mutex);
            running = false;
            chat.reset();
        }
        emit serverError(QString("Server error: %1").arg(e.what()));
        emit logMessage(QString("❌ Ошибка сервера: %1").arg(e.what()));
    }
}

int ServerController::getOnlineUsersCount() const
{
    QMutexLocker locker(&mutex);
    if (chat) {
        return chat->getOnlineUsersCount();
    }
    return 0;
}

int ServerController::getTotalUsersCount() const
{
    QMutexLocker locker(&mutex);
    if (chat) {
        return chat->getTotalUsersCount();
    }
    return 0;
}

QStringList ServerController::getOnlineUsersList() const
{
    QMutexLocker locker(&mutex);
    QStringList result;
    if (chat) {
        auto users = chat->getOnlineUsersList();
        for (const auto& user : users) {
            result.append(QString::fromStdString(user));
        }
    }
    return result;
}

QStringList ServerController::getAllUsersList() const
{
    QMutexLocker locker(&mutex);
    QStringList result;
    if (chat) {
        auto users = chat->getAllUsersList();
        for (const auto& user : users) {
            result.append(QString::fromStdString(user));
        }
    }
    return result;
}

//Методы для управления админами:

bool ServerController::addAdmin(const QString& login, const QString& password) {
    QMutexLocker locker(&mutex);
    if (chat) {
        return chat->addAdmin(login.toStdString(), password.toStdString());
    }
    return false;
}

bool ServerController::removeAdmin(const QString& login) {
    QMutexLocker locker(&mutex);
    if (chat) {
        return chat->removeAdmin(login.toStdString());
    }
    return false;
}

bool ServerController::changeAdminPassword(const QString& login, const QString& newPassword) {
    QMutexLocker locker(&mutex);
    if (chat) {
        return chat->changeAdminPassword(login.toStdString(), newPassword.toStdString());
    }
    return false;
}

QStringList ServerController::getAdminList() const {
    QMutexLocker locker(&mutex);
    QStringList result;
    if (chat) {
        auto admins = chat->getAdminList();
        for (const auto& admin : admins) {
            result.append(QString::fromStdString(admin));
        }
    }
    return result;
}

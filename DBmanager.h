#pragma once

#include <memory>

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>

#include "sha1.h"
#include "User.h"

using namespace MySha;

class DatabaseManager {
public:
    sql::mysql::MySQL_Driver* driver;
    std::unique_ptr<sql::Connection> connection;
    
public:
    DatabaseManager(const std::string& host, const std::string& user, 
    const std::string& password, const std::string& database);

private:
    void createTables();
public:
    bool banUser(const std::string& login);
    bool unbanUser(const std::string& login);
    bool deleteUser(const std::string& login);
    std::vector<User> getBannedUsers();
    std::vector<User> getOnlineUsers();
    bool isUserBanned(const std::string& login);
    // Регистрация нового пользователя
    bool registerUser(const std::string& name, const std::string& login, const std::string& password);
    // Аутентификация пользователя
    std::unique_ptr<User> authenticateUser(const std::string& login, const std::string& password);
    // Сохранение приватного сообщения
    bool savePrivateMessage(int senderId, int receiverId, const std::string& messageText);
    // Загрузка приватных сообщений для пользователя
    std::vector<Message> loadPrivateMessages(int userId);
    // Поиск пользователя по имени
    int findUserIdByName(const std::string& name);
    // Получение имени пользователя по ID
    std::string getUserNameById(int userId);

private:
    void markMessagesAsRead(int userId);
};

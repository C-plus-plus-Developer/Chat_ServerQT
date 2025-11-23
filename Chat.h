#pragma once
#include "User.h"
#include "Messages.h"
#include "DBmanager.h"
#include <thread>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <vector>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fstream>
#include <sstream>

#define PORT 7777
#define MESSAGE_LENGTH 1024
#define MAX_THREADS 50

using namespace MySha;

class Chat {
public:
    std::unique_ptr<DatabaseManager> dbManager;

    mutable std::mutex KlientMutex;

    Chat() {
        try {
            dbManager = std::make_unique<DatabaseManager>("tcp://127.0.0.1:3306", "dbeaver", "dbeaver123", "bd");
            loadAdminsFromFile(); // Загружаем админов при создании
        } catch (sql::SQLException& e) {
            std::cerr << "Database connection failed: " << e.what() << std::endl;
        }
    }

    Chat(std::vector<User> _users) : users(_users) {
        try {
            dbManager = std::make_unique<DatabaseManager>("tcp://127.0.0.1:3306", "dbeaver", "dbeaver123", "bd");
            loadAdminsFromFile();
        } catch (sql::SQLException& e) {
            std::cerr << "Database connection failed: " << e.what() << std::endl;
        }
    }

    struct sockaddr_in serverAddress, client;
    socklen_t length;
    int serverSocket, clientSocket;
    User currentLogin;

    std::vector<User> users;
    std::vector<Message> publicMessages;
    std::vector<std::string> banUsers;

    void setup();
    void runNonBlocking();

    // методы для интерфейса
    int getOnlineUsersCount() const {
        std::lock_guard<std::mutex> lock(KlientMutex);
        int count = 0;
        for (const auto& user : users) {
            if (user.clientSocket != -1) count++;
        }
        return count;
    }

    int getTotalUsersCount() const {
        std::lock_guard<std::mutex> lock(KlientMutex);
        return users.size();
    }

    std::vector<std::string> getOnlineUsersList() const {
        std::lock_guard<std::mutex> lock(KlientMutex);
        std::vector<std::string> list;
        for (const auto& user : users) {
            if (user.clientSocket != -1) {
                list.push_back(user.name + " (" + user.login + ")");
            }
        }
        return list;
    }

    std::vector<std::string> getAllUsersList() const {
        std::lock_guard<std::mutex> lock(KlientMutex);
        std::vector<std::string> list;
        for (const auto& user : users) {
            list.push_back(user.name + " (" + user.login + ")");
        }
        return list;
    }

    // методы для управления админами
    bool addAdmin(const std::string& login, const std::string& password);
    bool removeAdmin(const std::string& login);
    bool changeAdminPassword(const std::string& login, const std::string& newPassword);
    std::vector<std::string> getAdminList() const;
    void saveAdminsToFile();
    void loadAdminsFromFile();

    void stop();

    bool isRunning() const {
        return !stopServer;
    }

private:
    //Для хранения админов
    std::unordered_map<std::string, std::string> adminAkk;

    std::atomic<int> activeThreads{0};
    std::atomic<bool> stopServer{false};
    std::vector<std::thread> threadPool;

    bool AdminLogin(std::string login, std::string password);
    void AdminPanel(int clientSocket);
    void banUser(const std::string& login, int adminSocket);
    void unbanUser(const std::string& login, int adminSocket);
    void deleteUser(const std::string& login, int adminSocket);
    void PrintOnlineUsers(int clientSocket);
    void PrintBannedUsers(int clientSocket);
    bool CheckPassword(const std::string& login, const std::string& password);
    bool Login(const std::string& login, const std::string& password, int ClientSocket);
    void AddNewUser(const std::string& login, const std::string& password, const std::string& name, int clientSocket);
    void handleClient(int clientSocket);
    void SendMessage(const std::string& recipient, const std::string& message, int senderSocket);
    void UserPanel(int clientSocket);
    void PrintPrivateMessage(int clientSocket);
    void PrintAllUsers(int clientSocket);
    void PrintPublicMessage(int clientSocket);
    void SendPublicMessage(std::string& txt, int senderSocket);
    void sendKlient(std::string responce, int clientSocket);
    std::vector<User> loadAllUsersFromDB();
    std::vector<Message> loadPublicMessagesFromDB();
    std::vector<Message> loadPrivateMessagesFromDB(const std::string& userLogin);
    void updateOnlineUsers();
    void safeCreateThread(int clientSocket);
    void cleanupUserSocket(int clientSocket);
};

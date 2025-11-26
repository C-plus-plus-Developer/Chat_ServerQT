#include "Chat.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>

void Chat::sendKlient(std::string responce, int clientSocket) {
    if (clientSocket != -1) {
        ::send(clientSocket, responce.c_str(), responce.size(), 0);
    }
}

void Chat::cleanupUserSocket(int clientSocket) {
    std::lock_guard<std::mutex> lock(KlientMutex);
    for (auto& user : users) {
        if (user.clientSocket == clientSocket) {
            user.clientSocket = -1;
            std::cout << "Cleaned up socket for user: " << user.name << std::endl;
            break;
        }
    }
}

void Chat::setup() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Socket creation failed!" << std::endl;
        exit(1);
    }

    // Для повторного использования адреса
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Setsockopt failed!" << std::endl;
        close(serverSocket);
        exit(1);
    }

    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_family = AF_INET;

    if (::bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Socket binding failed!" << std::endl;
        close(serverSocket);
        exit(1);
    }

    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Server is unable to listen for new connections!" << std::endl;
        close(serverSocket);
        exit(1);
    } else {
        std::cout << "Server is listening for new connections on port " << PORT << "...\n";
    }

    while (!stopServer) {
        length = sizeof(client);
        clientSocket = ::accept(serverSocket, (sockaddr*)&client, &length);
        if (clientSocket == -1) {
            if (stopServer) break;
            std::cerr << "Server is unable to accept the data from client!" << std::endl;
            continue;
        }

        std::cout << "New client connected: " << clientSocket << std::endl;
        std::thread clientThread([=]() { handleClient(clientSocket); });
        clientThread.detach();
    }

    close(serverSocket);
    std::cout << "Server shutdown complete" << std::endl;
}

void Chat::handleClient(int clientSocket) {
    std::cout << "Handling client: " << clientSocket << std::endl;
    bool connected = true;

    while (connected && !stopServer) {
        char buffer[MESSAGE_LENGTH] = {0};
        int bytes_read = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string command(buffer);
            std::cout << "Received command from client " << clientSocket << ": " << command << std::endl;

            std::istringstream iss(command);
            std::string action;
            iss >> action;

            if (action == "1") {
                std::string login, password, name;
                if (iss >> login >> password >> name) {
                    AddNewUser(login, password, name, clientSocket);
                } else {
                    sendKlient("Registration failed: invalid format", clientSocket);
                }
            } else if (action == "2") {
                std::string login, password;
                if (iss >> login >> password) {
                    if(Login(login, password, clientSocket)) {
                        UserPanel(clientSocket);
                    }
                } else {
                    sendKlient("Login failed: invalid format", clientSocket);
                }
            } else if (action == "3") {
                PrintAllUsers(clientSocket);
            } else if (action == "4") {
                connected = false;
                sendKlient("Goodbye!", clientSocket);
            } else if (action == "5") {
                std::string login, pass;
                if (iss >> login >> pass) {
                    if(AdminLogin(login, pass)) {
                        sendKlient("Admin login successful!", clientSocket);
                        AdminPanel(clientSocket);
                    } else {
                        sendKlient("Admin login failed!", clientSocket);
                    }
                } else {
                    sendKlient("Admin login: invalid format", clientSocket);
                }
            } else {
                sendKlient("Unknown command: " + action, clientSocket);
            }
        } else if (bytes_read == 0) {
            std::cout << "Client " << clientSocket << " disconnected normally" << std::endl;
            connected = false;
        } else {
            if (errno != EINTR) {
                std::cerr << "recv error from client " << clientSocket << ": " << strerror(errno) << std::endl;
                connected = false;
            }
        }
    }

    cleanupUserSocket(clientSocket);
    close(clientSocket);
    std::cout << "Client handler finished for: " << clientSocket << std::endl;
}

void Chat::UserPanel(int clientSocket) {
    std::cout << "User panel opened for client: " << clientSocket << std::endl;
    bool connected = true;

    while (connected && !stopServer) {
        char buffer[MESSAGE_LENGTH] = {0};
        int bytes_read = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if(bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string command(buffer);
            std::cout << "User command from " << clientSocket << ": " << command << std::endl;

            std::istringstream iss(command);
            std::string action;
            iss >> action;

            if (action == "1") {
                std::string recipient, message;
                if (iss >> recipient) {
                    std::getline(iss, message);
                    if (!message.empty() && message[0] == ' ') {
                        message = message.substr(1);
                    }
                    if (!recipient.empty() && !message.empty()) {
                        SendMessage(recipient, message, clientSocket);
                    } else {
                        sendKlient("Invalid message format", clientSocket);
                    }
                } else {
                    sendKlient("Please specify recipient", clientSocket);
                }
            } else if (action == "2") {
                std::string messageTxt;
                std::getline(iss, messageTxt);
                if (!messageTxt.empty() && messageTxt[0] == ' ') {
                    messageTxt = messageTxt.substr(1);
                }
                if (!messageTxt.empty()) {
                    SendPublicMessage(messageTxt, clientSocket);
                } else {
                    sendKlient("Message cannot be empty", clientSocket);
                }
            } else if (action == "3") {
                PrintPrivateMessage(clientSocket);
            } else if (action == "4") {
                PrintPublicMessage(clientSocket);
            } else if (action == "5") {
                std::string str = "You closed user session";
                sendKlient(str, clientSocket);
                connected = false;
            } else if (action == "6") {
                PrintAllUsers(clientSocket);
            } else {
                sendKlient("Unknown user command: " + action, clientSocket);
            }
        } else if (bytes_read == 0) {
            std::cout << "User panel: client " << clientSocket << " disconnected" << std::endl;
            connected = false;
        } else {
            if (errno != EINTR) {
                std::cerr << "User panel recv error: " << strerror(errno) << std::endl;
                connected = false;
            }
        }
    }

    cleanupUserSocket(clientSocket);
    std::cout << "User panel closed for: " << clientSocket << std::endl;
}

bool Chat::AdminLogin(std::string login, std::string pass){
    auto it = adminAkk.find(login);
    std::string HashPass = MySha::sha1(pass.c_str(), pass.size());
    return it != adminAkk.end() && it->second == HashPass;
}

void Chat::AdminPanel(int clientSocket){
    std::cout << "Admin panel opened for: " << clientSocket << std::endl;
    bool inAdminPanel = true;

    while(inAdminPanel && !stopServer){
        char buffer[MESSAGE_LENGTH] = {0};
        int bytes_read = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if(bytes_read > 0){
            buffer[bytes_read] = '\0';
            std::string command(buffer);
            std::cout << "Admin command from " << clientSocket << ": " << command << std::endl;

            std::istringstream iss(command);
            std::string action;
            iss >> action;

            if(action == "1" || action == "3"){
                PrintAllUsers(clientSocket);
            } else if(action == "4"){
                PrintPublicMessage(clientSocket);
            } else if(action == "admin_2"){
                PrintOnlineUsers(clientSocket);
            } else if(action == "admin_3"){
                std::string login;
                if (iss >> login && !login.empty()) {
                    banUser(login, clientSocket);
                } else {
                    sendKlient("Please provide a login to ban", clientSocket);
                }
            } else if(action == "admin_4"){
                std::string login;
                if (iss >> login && !login.empty()) {
                    unbanUser(login, clientSocket);
                } else {
                    sendKlient("Please provide a login to unban", clientSocket);
                }
            } else if(action == "admin_5"){
                std::string login;
                if (iss >> login && !login.empty()) {
                    deleteUser(login, clientSocket);
                } else {
                    sendKlient("Please provide a login to delete", clientSocket);
                }
            } else if(action == "admin_6"){
                PrintBannedUsers(clientSocket);
            } else if(action == "7" || action == "5"){
                std::string response = "Exiting admin panel";
                sendKlient(response, clientSocket);
                inAdminPanel = false;
            } else {
                std::string response = "Unknown admin command: " + action;
                sendKlient(response, clientSocket);
            }
        } else if (bytes_read == 0) {
            std::cout << "Admin panel: client " << clientSocket << " disconnected" << std::endl;
            inAdminPanel = false;
        } else {
            if (errno != EINTR) {
                std::cerr << "Admin panel recv error: " << strerror(errno) << std::endl;
                inAdminPanel = false;
            }
        }
    }

    std::cout << "Admin panel closed for: " << clientSocket << std::endl;
}

void Chat::PrintBannedUsers(int clientSocket) {
    try {
        std::vector<User> bannedUsers = dbManager->getBannedUsers();

        std::stringstream ss;
        if (bannedUsers.empty()) {
            ss << "No banned users!\n";
        } else {
            ss << "Banned users:\n";
            for (size_t i = 0; i < bannedUsers.size(); ++i) {
                ss << i + 1 << " - " << bannedUsers[i].name << " (login: " << bannedUsers[i].login << ")\n";
            }
        }

        std::string response = ss.str();
        sendKlient(response, clientSocket);

    } catch (sql::SQLException& e) {
        std::cerr << "Database error in PrintBannedUsers: " << e.what() << std::endl;
        sendKlient("Error loading banned users from database", clientSocket);
    }
}

void Chat::PrintOnlineUsers(int clientSocket) {
    std::lock_guard<std::mutex> lock(KlientMutex);
    std::stringstream ss;
    int onlineCount = 0;

    ss << "Online users:\n";
    for (const auto& user : users) {
        if (user.clientSocket != -1) {
            ss << " - " << user.name << " (" << user.login << ")\n";
            onlineCount++;
        }
    }

    if (onlineCount == 0) {
        ss << "No users online currently.\n";
    }

    std::string response = ss.str();
    sendKlient(response, clientSocket);
}

void Chat::banUser(const std::string& login, int adminSocket) {
    try {
        if (dbManager->banUser(login)) {
            // Отключаем пользователя если он онлайн
            {
                std::lock_guard<std::mutex> lock(KlientMutex);
                for (auto& user : users) {
                    if (user.login == login && user.clientSocket != -1) {
                        std::string banMessage = "You have been banned from the chat!";
                        sendKlient(banMessage, user.clientSocket);
                        close(user.clientSocket);
                        user.clientSocket = -1;
                        break;
                    }
                }
            }

            std::string response = "User " + login + " has been banned successfully!";
            sendKlient(response, adminSocket);
        } else {
            std::string response = "Failed to ban user " + login + " (user not found)";
            sendKlient(response, adminSocket);
        }

    } catch (sql::SQLException& e) {
        std::cerr << "Database error in banUser: " << e.what() << std::endl;
        std::string response = "Error banning user due to server error";
        sendKlient(response, adminSocket);
    }
}

void Chat::unbanUser(const std::string& login, int adminSocket) {
    try {
        if (dbManager->unbanUser(login)) {
            std::string response = "User " + login + " has been unbanned successfully!";
            sendKlient(response, adminSocket);
        } else {
            std::string response = "Failed to unban user " + login + " (user not found)";
            sendKlient(response, adminSocket);
        }

    } catch (sql::SQLException& e) {
        std::cerr << "Database error in unbanUser: " << e.what() << std::endl;
        std::string response = "Error unbanning user due to server error";
        sendKlient(response, adminSocket);
    }
}

void Chat::deleteUser(const std::string& login, int adminSocket) {
    try {
        // Отключаем пользователя если он онлайн
        {
            std::lock_guard<std::mutex> lock(KlientMutex);
            for (auto& user : users) {
                if (user.login == login && user.clientSocket != -1) {
                    std::string deleteMessage = "Your account has been deleted!";
                    sendKlient(deleteMessage, user.clientSocket);
                    close(user.clientSocket);
                    user.clientSocket = -1;
                    break;
                }
            }
        }

        if (dbManager->deleteUser(login)) {
            // Удаляем пользователя из локального списка
            {
                std::lock_guard<std::mutex> lock(KlientMutex);
                users.erase(
                    std::remove_if(users.begin(), users.end(),
                                   [&login](const User& user) { return user.login == login; }),
                    users.end()
                    );
            }

            std::string response = "User " + login + " has been deleted successfully!";
            sendKlient(response, adminSocket);
        } else {
            std::string response = "Failed to delete user " + login + " (user not found)";
            sendKlient(response, adminSocket);
        }

    } catch (sql::SQLException& e) {
        std::cerr << "Database error in deleteUser: " << e.what() << std::endl;
        std::string response = "Error deleting user due to server error";
        sendKlient(response, adminSocket);
    }
}

bool Chat::Login(const std::string& login, const std::string& password, int ClientSocket) {
    std::cout << "Login attempt for: " << login << std::endl;

    if (dbManager->isUserBanned(login)) {
        std::cout << "Banned user tried to login: " << login << std::endl;
        std::string response = "Login failed! Your account has been banned.";
        sendKlient(response, ClientSocket);
        return false;
    }

    if (CheckPassword(login, password)) {
        auto user = dbManager->authenticateUser(login, password);
        if (user) {
            std::lock_guard<std::mutex> lock(KlientMutex);
            currentLogin = *user;
            currentLogin.clientSocket = ClientSocket;

            // Обновляем или добавляем пользователя
            bool userExists = false;
            for (auto& u : users) {
                if (u.login == login) {
                    u.clientSocket = ClientSocket;
                    userExists = true;
                    currentLogin = u;
                    break;
                }
            }
            if (!userExists) {
                users.push_back(currentLogin);
            }

            std::cout << "Login successful for: " << login << std::endl;
            std::string response = "Login successful!";
            sendKlient(response, ClientSocket);
            return true;
        }
    }

    std::cout << "Login failed for: " << login << std::endl;
    std::string response = "Login failed! Invalid credentials.";
    sendKlient(response, ClientSocket);
    return false;
}

bool Chat::addAdmin(const std::string& login, const std::string& password) {
    std::lock_guard<std::mutex> lock(KlientMutex);

    // Проверка на существование логина
    if (adminAkk.find(login) != adminAkk.end()) {
        std::cout << "Admin with login '" << login << "' already exists!" << std::endl;
        return false;
    }

    // Добавляем нового админа
    adminAkk[login] = MySha::sha1(password.c_str(), password.size());
    std::cout << "Admin '" << login << "' added successfully!" << std::endl;

    saveAdminsToFile();
    return true;
}

bool Chat::removeAdmin(const std::string& login) {
    std::cout << "=== Chat::removeAdmin ===" << std::endl;
    std::cout << "Request to remove admin: '" << login << "'" << std::endl;

    std::lock_guard<std::mutex> lock(KlientMutex);

    // Вывод в консоль всех админов для отладки
    std::cout << "Current admins (" << adminAkk.size() << "): ";
    for (const auto& admin : adminAkk) {
        std::cout << "[" << admin.first << "] ";
    }
    std::cout << std::endl;

    // Проверка на минимальное количество админов
    if (adminAkk.size() <= 1) {
        std::cout << "ERROR: Cannot remove last admin. Total: " << adminAkk.size() << std::endl;
        return false;
    }

    // Поиск админа (прямое сравнение логинов)
    auto it = adminAkk.find(login);
    if (it == adminAkk.end()) {
        std::cout << "ERROR: Admin '" << login << "' not found!" << std::endl;
        std::cout << "Available admins: ";
        for (const auto& admin : adminAkk) {
            std::cout << admin.first << " ";
        }
        std::cout << std::endl;
        return false;
    }

    // Удаляем админа
    adminAkk.erase(it);
    std::cout << "Admin '" << login << "' removed successfully!" << std::endl;

    // Сохраняем изменения в файл
    saveAdminsToFile();
    std::cout << "Admins saved to file" << std::endl;

    // Дополнительная проверка после удаления
    std::cout << "Remaining admins (" << adminAkk.size() << "): ";
    for (const auto& admin : adminAkk) {
        std::cout << "[" << admin.first << "] ";
    }
    std::cout << std::endl;

    return true;
}

bool Chat::changeAdminPassword(const std::string& login, const std::string& newPassword) {
    std::lock_guard<std::mutex> lock(KlientMutex);

    // Проверка на существование админа
    auto it = adminAkk.find(login);
    if (it == adminAkk.end()) {
        std::cout << "Admin with login '" << login << "' not found!" << std::endl;
        return false;
    }

    std::string HashedPassword = MySha::sha1(newPassword.c_str(), newPassword.size());
    // Меняем пароль
    it->second = HashedPassword;
    std::cout << "Password for admin '" << login << "' changed successfully!" << std::endl;

    saveAdminsToFile();
    return true;
}

std::vector<std::string> Chat::getAdminList() const {
    std::lock_guard<std::mutex> lock(KlientMutex);
    std::vector<std::string> admins;

    for (const auto& admin : adminAkk) {
        admins.push_back(admin.first);
    }

    return admins;
}

void Chat::saveAdminsToFile() {
    const std::string ADMIN_FILE = "admins.txt";
    try {
        std::ofstream file(ADMIN_FILE);
        if (!file.is_open()) {
            std::cerr << "Failed to open admin file for writing: " << ADMIN_FILE << std::endl;
            return;
        }

        for (const auto& admin : adminAkk) {
            file << admin.first << " " << admin.second << std::endl;
        }

        file.close();
        std::cout << "Admins saved to file: " << ADMIN_FILE << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error saving admins to file: " << e.what() << std::endl;
    }
}

void Chat::loadAdminsFromFile() {
    const std::string ADMIN_FILE = "admins.txt";
    try {
        std::ifstream file(ADMIN_FILE);
        if (!file.is_open()) {
            std::cout << "Admin file not found, creating default admins..." << std::endl;

            // Создаем админов по умолчанию
            std::string adminPassOne = "admin123";
            std::string adminPassTwo = "moder123";
            adminAkk = {
                        {"admin", MySha::sha1(adminPassOne.c_str(), adminPassOne.size())},
                        {"moder", MySha::sha1(adminPassTwo.c_str(), adminPassTwo.size())}
            };
            saveAdminsToFile();
            return;
        }

        adminAkk.clear();
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string login, password;
            if (iss >> login >> password) {
                adminAkk[login] = password;
            }
        }

        file.close();
        std::cout << "Admins loaded from file: " << ADMIN_FILE << std::endl;
        std::cout << "Total admins: " << adminAkk.size() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error loading admins from file: " << e.what() << std::endl;

        // Создаем админов по умолчанию при ошибке
        adminAkk = {
            {"admin", "admin123"},
            {"moder", "moder123"}
        };
    }
}


void Chat::AddNewUser(const std::string& login, const std::string& password, const std::string& name, int clientSocket) {
    std::string response;
    std::lock_guard<std::mutex> lock(KlientMutex);
    if (dbManager->registerUser(name, login, password)) {
        response = "User added successfully!";
        updateOnlineUsers();
    } else {
        response = "Registration failed! Login might already exist.";
    }
    sendKlient(response, clientSocket);
}

void Chat::SendMessage(const std::string& recipient, const std::string& message, int senderSocket) {
    if (recipient.empty() || message.empty()) {
        sendKlient("Recipient or message cannot be empty", senderSocket);
        return;
    }

    try {
        int recipientId = dbManager->findUserIdByName(recipient);
        if (recipientId == -1) {
            std::string response = "Recipient not found!";
            sendKlient(response, senderSocket);
            return;
        }

        // Находим отправителя
        int senderId = -1;
        std::string senderName;
        {
            std::lock_guard<std::mutex> lock(KlientMutex);
            for (const auto& user : users) {
                if (user.clientSocket == senderSocket) {
                    senderId = dbManager->findUserIdByName(user.name);
                    senderName = user.name;
                    break;
                }
            }
        }

        if (senderId == -1) {
            sendKlient("Sender not found!", senderSocket);
            return;
        }

        if (dbManager->savePrivateMessage(senderId, recipientId, message)) {
            std::string response = "Message sent successfully to " + recipient + "!";
            sendKlient(response, senderSocket);

            // Уведомление получателю если онлайн
            std::string notification = "New private message from " + senderName + ": " + message;
            std::lock_guard<std::mutex> lock(KlientMutex);
            for (auto& user : users) {
                if (user.name == recipient && user.clientSocket != -1) {
                    sendKlient(notification, user.clientSocket);
                    break;
                }
            }
        } else {
            sendKlient("Message sending failed due to server error", senderSocket);
        }

    } catch (sql::SQLException& e) {
        std::cerr << "Database error in SendMessage: " << e.what() << std::endl;
        sendKlient("Message sending failed due to server error", senderSocket);
    }
}

void Chat::SendPublicMessage(std::string& message, int senderSocket) {
    if (message.empty()) {
        sendKlient("Message cannot be empty", senderSocket);
        return;
    }

    try {
        // Находим отправителя
        int senderId = -1;
        std::string senderName;
        {
            std::lock_guard<std::mutex> lock(KlientMutex);
            for (const auto& user : users) {
                if (user.clientSocket == senderSocket) {
                    senderId = dbManager->findUserIdByName(user.name);
                    senderName = user.name;
                    break;
                }
            }
        }

        if (senderId == -1) {
            sendKlient("Sender not found!", senderSocket);
            return;
        }

        // Сохраняем публичное сообщение в БД
        std::unique_ptr<sql::PreparedStatement> messageStmt(
            dbManager->connection->prepareStatement(
                "INSERT INTO public_messages (sender_id, sender_name, message_text) VALUES (?, ?, ?)"
                )
            );
        messageStmt->setInt(1, senderId);
        messageStmt->setString(2, senderName);
        messageStmt->setString(3, message);
        messageStmt->execute();

        // Отправка сообщения всем онлайн пользователям
        std::string response = "[" + senderName + "]: " + message;

        std::lock_guard<std::mutex> lock(KlientMutex);
        for (auto& user : users) {
            if (user.clientSocket != -1) {
                sendKlient(response, user.clientSocket);
            }
        }

    } catch (sql::SQLException& e) {
        std::cerr << "Database error in SendPublicMessage: " << e.what() << std::endl;
        sendKlient("Message sending failed due to server error", senderSocket);
    }
}

void Chat::PrintPrivateMessage(int clientSocket) {
    try {
        // Находим логин пользователя
        std::string userLogin;
        {
            std::lock_guard<std::mutex> lock(KlientMutex);
            for (const auto& user : users) {
                if (user.clientSocket == clientSocket) {
                    userLogin = user.login;
                    break;
                }
            }
        }

        if (userLogin.empty()) {
            sendKlient("User not found", clientSocket);
            return;
        }

        std::vector<Message> privateMessages = loadPrivateMessagesFromDB(userLogin);

        if (privateMessages.empty()) {
            std::string response = "No private messages available.";
            sendKlient(response, clientSocket);
            return;
        }

        std::stringstream ss;
        for (const auto& msg : privateMessages) {
            ss << "From: " << msg.from << "\n";
            ss << "To: " << msg.to << "\n";
            ss << "Message: " << msg.text << "\n";
            ss << "------------------------\n";
        }

        std::string response = ss.str();
        sendKlient(response, clientSocket);

    } catch (sql::SQLException& e) {
        std::cerr << "Database error in PrintPrivateMessage: " << e.what() << std::endl;
        std::string errorResponse = "Error loading messages from database";
        sendKlient(errorResponse, clientSocket);
    }
}

void Chat::PrintAllUsers(int clientSocket) {
    try {
        std::vector<User> allUsers = loadAllUsersFromDB();

        std::stringstream ss;
        if (allUsers.empty()) {
            ss << "No users registered!";
        } else {
            ss << "All registered users:\n";
            for (size_t i = 0; i < allUsers.size(); ++i) {
                ss << i + 1 << " - " << allUsers[i].name << " (login: " << allUsers[i].login << ")\n";
            }
        }

        std::string response = ss.str();
        sendKlient(response, clientSocket);

    } catch (sql::SQLException& e) {
        std::cerr << "Database error in PrintAllUsers: " << e.what() << std::endl;
        sendKlient("Error loading users from database", clientSocket);
    }
}

void Chat::PrintPublicMessage(int clientSocket) {
    try {
        std::vector<Message> publicMessages = loadPublicMessagesFromDB();

        std::stringstream ss;
        if (publicMessages.empty()) {
            ss << "No public messages available!";
        } else {

            for (size_t i = 0; i < publicMessages.size(); ++i) {
                ss << publicMessages[i].from << ": " << publicMessages[i].text << "\n";
            }
        }

        std::string response = ss.str();
        sendKlient(response, clientSocket);

    } catch (sql::SQLException& e) {
        std::cerr << "Database error in PrintPublicMessage: " << e.what() << std::endl;
        sendKlient("Error loading public messages from database", clientSocket);
    }
}

bool Chat::CheckPassword(const std::string& login, const std::string& password) {
    auto user = dbManager->authenticateUser(login, password);
    return user != nullptr;
}

// Методы для работы с БД
std::vector<User> Chat::loadAllUsersFromDB() {
    std::vector<User> users;

    try {
        std::unique_ptr<sql::Statement> stmt(dbManager->connection->createStatement());
        std::unique_ptr<sql::ResultSet> result(
            stmt->executeQuery("SELECT id, name, login FROM users ORDER BY name")
            );

        while (result->next()) {
            User user;
            user.id = result->getInt("id");
            user.name = result->getString("name");
            user.login = result->getString("login");
            users.push_back(user);
        }

    } catch (sql::SQLException& e) {
        std::cerr << "Error loading users from DB: " << e.what() << std::endl;
    }

    return users;
}

std::vector<Message> Chat::loadPublicMessagesFromDB() {
    std::vector<Message> messages;

    try {
        std::unique_ptr<sql::Statement> stmt(dbManager->connection->createStatement());
        std::unique_ptr<sql::ResultSet> result(
            stmt->executeQuery(
                "SELECT sender_name, message_text, timestamp "
                "FROM public_messages "
                "ORDER BY timestamp"
                )
            );

        while (result->next()) {
            Message msg;
            msg.from = result->getString("sender_name");
            msg.text = result->getString("message_text");
            messages.push_back(msg);
        }

    } catch (sql::SQLException& e) {
        std::cerr << "Error loading public messages from DB: " << e.what() << std::endl;
    }

    return messages;
}

std::vector<Message> Chat::loadPrivateMessagesFromDB(const std::string& userLogin) {
    std::vector<Message> messages;

    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            dbManager->connection->prepareStatement(
                "SELECT u_sender.name as sender_name, u_receiver.name as receiver_name, pm.message_text "
                "FROM private_messages pm "
                "JOIN users u_sender ON pm.sender_id = u_sender.id "
                "JOIN users u_receiver ON pm.receiver_id = u_receiver.id "
                "WHERE u_receiver.login = ? OR u_sender.login = ? "
                "ORDER BY pm.timestamp"
                )
            );
        pstmt->setString(1, userLogin);
        pstmt->setString(2, userLogin);

        std::unique_ptr<sql::ResultSet> result(pstmt->executeQuery());

        while (result->next()) {
            Message msg;
            msg.from = result->getString("sender_name");
            msg.to = result->getString("receiver_name");
            msg.text = result->getString("message_text");
            messages.push_back(msg);
        }

    } catch (sql::SQLException& e) {
        std::cerr << "Error loading private messages from DB: " << e.what() << std::endl;
    }

    return messages;
}

void Chat::updateOnlineUsers() {
    users = loadAllUsersFromDB();
    for (auto& user : users) {
        user.clientSocket = -1;
    }
}

void Chat::runNonBlocking() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Socket creation failed!" << std::endl;
        return;
    }

    // Добавляем опцию для повторного использования адреса
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Setsockopt failed!" << std::endl;
        close(serverSocket);
        return;
    }

    // Делаем сокет неблокирующим
    int flags = fcntl(serverSocket, F_GETFL, 0);
    fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK);

    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_family = AF_INET;

    if (::bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Socket binding failed!" << std::endl;
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Server is unable to listen for new connections!" << std::endl;
        close(serverSocket);
        return;
    }

    std::cout << "Server is listening for new connections on port " << PORT << " (non-blocking mode)..." << std::endl;

    std::vector<pollfd> fds;
    pollfd serverFd;
    serverFd.fd = serverSocket;
    serverFd.events = POLLIN;
    serverFd.revents = 0;
    fds.push_back(serverFd);

    while (!stopServer) {
        int pollCount = poll(fds.data(), fds.size(), 100); // Задержка на 100 мс

        if (pollCount == -1) {
            if (errno != EINTR) {
                std::cerr << "Poll error: " << strerror(errno) << std::endl;
            }
            continue;
        }

        if (pollCount == 0) {
            continue;
        }

        // Проверяем новые подключения
        if (fds[0].revents & POLLIN) {
            length = sizeof(client);
            clientSocket = ::accept(serverSocket, (sockaddr*)&client, &length);
            if (clientSocket != -1) {
                std::cout << "New client connected: " << clientSocket << std::endl;

                // Добавляем клиентский сокет в poll
                pollfd clientFd;
                clientFd.fd = clientSocket;
                clientFd.events = POLLIN;
                clientFd.revents = 0;
                fds.push_back(clientFd);

                // Запускаем обработчик клиента в отдельном потоке
                std::thread clientThread([=]() { handleClient(clientSocket); });
                clientThread.detach();
            }
        }
    }

    // Закрываем все сокеты
    for (auto& fd : fds) {
        if (fd.fd != -1) {
            close(fd.fd);
        }
    }

    std::cout << "Server shutdown complete" << std::endl;
}

void Chat::stop(){
    stopServer = true;
    if (serverSocket != -1) {
        close(serverSocket);
        serverSocket = -1;
    }
    // Закрываем все клиентские сокеты
    std::lock_guard<std::mutex> lock(KlientMutex);
    for (auto& user : users) {
        if (user.clientSocket != -1) {
            close(user.clientSocket);
            user.clientSocket = -1;
        }
    }
}

#pragma once
#include <vector>
#include <iostream>
#include <string>
#include "Messages.h"
#include <sstream>
#include <limits>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

class User {
public:
    int id;
    int clientSocket;
    std::string name, login, password, serverAddress;
    std::vector<Message> privateMessages;
    bool ban;

    User() : id(0), clientSocket(-1), ban(false) {}

    User(int userId, const std::string& userName, const std::string& userLogin) :
        id(userId),
        clientSocket(-1),
        name(userName),
        login(userLogin),
        ban(false)
    {}

    ~User() {
        if (clientSocket != -1) {
            close(clientSocket);
            std::cout << "Client socket closed for user " << name << std::endl;
        }
    }
};

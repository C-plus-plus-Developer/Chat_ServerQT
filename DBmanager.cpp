#include "DBmanager.h"

DatabaseManager::DatabaseManager(const std::string& host, const std::string& user, 
                   const std::string& password, const std::string& database) {
        try {
            driver = sql::mysql::get_mysql_driver_instance();
            connection.reset(driver->connect(host, user, password));
            connection->setSchema(database);
            std::cout << "Connected to database successfully" << std::endl;
            
            // Создаем таблицы, если они не существуют
            createTables();
        } catch (sql::SQLException& e) {
            std::cerr << "Database connection error: " << e.what() << std::endl;
        }
    }

    void DatabaseManager::createTables(){
        try {
            std::unique_ptr<sql::Statement> stmt(connection->createStatement());
            
            stmt->execute(
            "CREATE TABLE IF NOT EXISTS users ("
            "id INT AUTO_INCREMENT PRIMARY KEY, "
            "name VARCHAR(100) NOT NULL, "
            "login VARCHAR(100) UNIQUE NOT NULL, "
            "password_hash VARCHAR(255) NOT NULL, "
            "status VARCHAR(20) DEFAULT 'active', "
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"
            );
            
            stmt->execute(
                "CREATE TABLE IF NOT EXISTS private_messages ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "sender_id INT NOT NULL, "
                "receiver_id INT NOT NULL, "
                "message_text TEXT NOT NULL, "
                "timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                "is_read BOOLEAN DEFAULT FALSE, "
                "FOREIGN KEY (sender_id) REFERENCES users(id), "
                "FOREIGN KEY (receiver_id) REFERENCES users(id))"
            );
    
            stmt->execute(
                "CREATE TABLE IF NOT EXISTS public_messages ("
                "id INT AUTO_INCREMENT PRIMARY KEY, "
                "sender_id INT NOT NULL, "
                "sender_name VARCHAR(100) NOT NULL, "
                "message_text TEXT NOT NULL, "
                "timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP, "
                "FOREIGN KEY (sender_id) REFERENCES users(id))"
            );
            
        } catch (sql::SQLException& e) {
            std::cerr << "Table creation error: " << e.what() << std::endl;
        }
}

bool DatabaseManager::banUser(const std::string& login) {
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            connection->prepareStatement(
                "UPDATE users SET status = 'banned' WHERE login = ?"
            )
        );
        
        pstmt->setString(1, login);
        int affectedRows = pstmt->executeUpdate();
        return affectedRows > 0;
        
    } catch (sql::SQLException& e) {
        std::cerr << "Ban user error: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::unbanUser(const std::string& login) {
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            connection->prepareStatement(
                "UPDATE users SET status = 'active' WHERE login = ?"
            )
        );
        
        pstmt->setString(1, login);
        int affectedRows = pstmt->executeUpdate();
        return affectedRows > 0;
        
    } catch (sql::SQLException& e) {
        std::cerr << "Unban user error: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::deleteUser(const std::string& login) {
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt1(
            connection->prepareStatement(
                "DELETE FROM private_messages WHERE sender_id IN (SELECT id FROM users WHERE login = ?) OR receiver_id IN (SELECT id FROM users WHERE login = ?)"
            )
        );
        pstmt1->setString(1, login);
        pstmt1->setString(2, login);
        pstmt1->execute();
        
        std::unique_ptr<sql::PreparedStatement> pstmt2(
            connection->prepareStatement(
                "DELETE FROM public_messages WHERE sender_id IN (SELECT id FROM users WHERE login = ?)"
            )
        );
        pstmt2->setString(1, login);
        pstmt2->execute();
        
        std::unique_ptr<sql::PreparedStatement> pstmt3(
            connection->prepareStatement(
                "DELETE FROM users WHERE login = ?"
            )
        );
        pstmt3->setString(1, login);
        int affectedRows = pstmt3->executeUpdate();
        return affectedRows > 0;
        
    } catch (sql::SQLException& e) {
        std::cerr << "Delete user error: " << e.what() << std::endl;
        return false;
    }
}

std::vector<User> DatabaseManager::getBannedUsers() {
    std::vector<User> bannedUsers;
    
    try {
        std::unique_ptr<sql::Statement> stmt(connection->createStatement());
        std::unique_ptr<sql::ResultSet> res(
            stmt->executeQuery(
                "SELECT id, name, login FROM users WHERE status = 'banned' ORDER BY name"
            )
        );
        
        while (res->next()) {
            User user;
            user.id = res->getInt("id");
            user.name = res->getString("name");
            user.login = res->getString("login");
            bannedUsers.push_back(user);
        }
        
    } catch (sql::SQLException& e) {
        std::cerr << "Get banned users error: " << e.what() << std::endl;
    }
    
    return bannedUsers;
}

bool DatabaseManager::isUserBanned(const std::string& login) {
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            connection->prepareStatement(
                "SELECT status FROM users WHERE login = ?"
            )
        );
        
        pstmt->setString(1, login);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        
        if (res->next()) {
            std::string status = res->getString("status");
            return status == "banned";
        }
        return false;
        
    } catch (sql::SQLException& e) {
        std::cerr << "Check user ban status error: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::registerUser(const std::string& name, const std::string& login, 
                     const std::string& password) {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                connection->prepareStatement(
                    "INSERT INTO users (name, login, password_hash) VALUES (?, ?, ?)"
                )
            );
            
            pstmt->setString(1, name);
            pstmt->setString(2, login);
            
            // Хешируем пароль
            std::string passwordHash = sha1(password.c_str(), password.size());
            pstmt->setString(3, passwordHash);
            
            pstmt->execute();
            return true;
            
        } catch (sql::SQLException& e) {
            std::cerr << "Registration error: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Аутентификация пользователя
    std::unique_ptr<User> DatabaseManager::authenticateUser(const std::string& login, 
                                         const std::string& password) {
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            connection->prepareStatement(
                "SELECT id, name, password_hash FROM users WHERE login = ?"
            )
        );
        
        pstmt->setString(1, login);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        
        if (res->next()) {
            std::string storedHash = res->getString("password_hash");
            std::string inputHash = sha1(password.c_str(), password.size());
            
            if (storedHash == inputHash) {
                auto user = std::make_unique<User>();
                user->id = res->getInt("id");
                user->name = res->getString("name");
                user->login = login;
                return user;
            }
        }
        return nullptr;
        
    } catch (sql::SQLException& e) {
        std::cerr << "Authentication error: " << e.what() << std::endl;
        return nullptr;
    }
}
    // Сохранение приватного сообщения
    bool DatabaseManager::savePrivateMessage(int senderId, int receiverId, 
                           const std::string& messageText) {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                connection->prepareStatement(
                    "INSERT INTO private_messages (sender_id, receiver_id, message_text) "
                    "VALUES (?, ?, ?)"
                )
            );
            
            pstmt->setInt(1, senderId);
            pstmt->setInt(2, receiverId);
            pstmt->setString(3, messageText);
            
            pstmt->execute();
            return true;
            
        } catch (sql::SQLException& e) {
            std::cerr << "Save message error: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Загрузка приватных сообщений для пользователя
    std::vector<Message> DatabaseManager::loadPrivateMessages(int userId) {
        std::vector<Message> messages;
        
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                connection->prepareStatement(
                    "SELECT pm.id, u.name as sender_name, pm.message_text, pm.timestamp "
                    "FROM private_messages pm "
                    "JOIN users u ON pm.sender_id = u.id "
                    "WHERE pm.receiver_id = ? "
                    "ORDER BY pm.timestamp"
                )
            );
            
            pstmt->setInt(1, userId);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            
            while (res->next()) {
                Message msg;
                msg.from = res->getString("sender_name");
                msg.text = res->getString("message_text");
                // msg.timestamp = res->getString("timestamp");
                messages.push_back(msg);
            }
            
            // Помечаем сообщения как прочитанные
            markMessagesAsRead(userId);
            
        } catch (sql::SQLException& e) {
            std::cerr << "Load messages error: " << e.what() << std::endl;
        }
        
        return messages;
    }
    
    // Поиск пользователя по имени
    int DatabaseManager::findUserIdByName(const std::string& name) {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                connection->prepareStatement(
                    "SELECT id FROM users WHERE name = ?"
                )
            );
            
            pstmt->setString(1, name);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            
            if (res->next()) {
                return res->getInt("id");
            }
            return -1;
            
        } catch (sql::SQLException& e) {
            std::cerr << "Find user error: " << e.what() << std::endl;
            return -1;
        }
    }
    
    // Получение имени пользователя по ID
    std::string DatabaseManager::getUserNameById(int userId) {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                connection->prepareStatement(
                    "SELECT name FROM users WHERE id = ?"
                )
            );
            
            pstmt->setInt(1, userId);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            
            if (res->next()) {
                return res->getString("name");
            }
            return "";
            
        } catch (sql::SQLException& e) {
            std::cerr << "Get user name error: " << e.what() << std::endl;
            return "";
        }
    }

    void DatabaseManager::markMessagesAsRead(int userId) {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                connection->prepareStatement(
                    "UPDATE private_messages SET is_read = TRUE WHERE receiver_id = ?"
                )
            );
            
            pstmt->setInt(1, userId);
            pstmt->execute();
            
        } catch (sql::SQLException& e) {
            std::cerr << "Mark messages as read error: " << e.what() << std::endl;
        }
    }


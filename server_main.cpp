#include "Chat.h"
#include <iostream>
#include <atomic>
#include <csignal>
#include <thread>

std::atomic<bool> g_running{true};

void signalHandler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

extern "C" void startServer() {
    std::cout << "Starting chat server in thread..." << std::endl;

    Chat chat;

    // Запускаем сервер в отдельном потоке
    std::thread serverThread([&chat]() {
        std::cout << "Server thread started" << std::endl;
        chat.setup();
        std::cout << "Server thread finished" << std::endl;
    });

    // Устанавливаем обработчики сигналов
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;

    // Главный цикл ожидания
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "Stopping server..." << std::endl;
    chat.stop();

    if (serverThread.joinable()) {
        serverThread.join();
    }

    std::cout << "Server stopped successfully." << std::endl;
}

//Функция для тестирования
extern "C" void testServer() {
    std::cout << "Test function called - server library is working!" << std::endl;
}

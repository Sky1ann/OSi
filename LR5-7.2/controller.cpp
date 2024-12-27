#include <iostream>
#include <sstream>
#include <thread>
#include "controller.h"
#include <zmq.hpp>

// Конструктор
Controller::Controller() : running(false) {}

// Метод для создания узла
void Controller::createNode(int id, int parentId) {
    if (tree.createNode(id, parentId)) {
        // Успешное создание узла, сообщение уже выведено в Tree::createNode
    }
}

// Метод для выполнения команды на узле
void Controller::execNodeCommand(int id, const std::string& text, const std::string& pattern) {
    auto node = tree.findNode(id);
    if (!node) {
        std::cout << "Error:" << id << ": Not found" << std::endl;
        return;
    }

    if (!node->isAvailable) {
        std::cout << "Error:" << id << ": Node is unavailable" << std::endl;
        return;
    }

    // Получение REP-порта узла
    int repPort = node->repPort; // Исправлено

    try {
        zmq::socket_t socket(tree.getContext(), ZMQ_REQ); // Используем общий контекст из Tree
        std::string endpoint = "tcp://localhost:" + std::to_string(repPort);
        socket.connect(endpoint);

        // Отправка команды "exec" с мультипартовыми сообщениями
        zmq::message_t execCmd("exec", 4);
        socket.send(execCmd, zmq::send_flags::sndmore);

        zmq::message_t textMsg(text.c_str(), text.size());
        socket.send(textMsg, zmq::send_flags::sndmore);

        zmq::message_t patternMsg(pattern.c_str(), pattern.size());
        socket.send(patternMsg, zmq::send_flags::none);

        // Получение ответа
        zmq::message_t reply;
        auto received = socket.recv(reply, zmq::recv_flags::none);
        if (received) { // Используем auto и проверку
            std::string replyStr(static_cast<char*>(reply.data()), reply.size());
            std::cout << replyStr << std::endl;
        }
        else {
            std::cout << "Error:" << id << ": No response received" << std::endl;
        }
    }
    catch (const zmq::error_t& e) {
        std::cout << "Error:" << id << ": " << e.what() << std::endl;
    }
}

// Обработка команды create
void Controller::handleCreateCommand(const std::string& command) {
    std::stringstream ss(command);
    std::string token;
    int id, parentId = -1;

    ss >> token >> id;
    if (ss >> parentId) {
        createNode(id, parentId);
    } else {
        createNode(id, -1);
    }
}

// Обработка команды exec
void Controller::handleExecCommand(const std::string& command) {
    std::stringstream ss(command);
    std::string token;
    int id;

    ss >> token >> id;
    if (token != "exec") {
        std::cout << "Error:" << id << ": Invalid exec command format" << std::endl;
        return;
    }

    // Чтение text_string
    std::string text_string;
    std::getline(std::cin, text_string);
    if (text_string.empty()) {
        std::cout << "Error:" << id << ": Missing text string" << std::endl;
        return;
    }

    // Чтение pattern_string
    std::string pattern_string;
    std::getline(std::cin, pattern_string);
    if (pattern_string.empty()) {
        std::cout << "Error:" << id << ": Missing pattern string" << std::endl;
        return;
    }

    execNodeCommand(id, text_string, pattern_string);
}

// Обработка команды heartbeat
void Controller::handleHeartbeatCommand(const std::string& command) {
    std::stringstream ss(command);
    std::string token;
    int timeMs;

    ss >> token >> timeMs;
    if (token != "heartbeat") {
        std::cout << "Error: Invalid heartbeat command format" << std::endl;
        return;
    }
    tree.handleHeartbeatCommand(timeMs);
}

// Обработка команды ping
void Controller::handlePingCommand(const std::string& command) {
    std::stringstream ss(command);
    std::string token;
    int id;

    ss >> token >> id;
    if (token != "ping") {
        std::cout << "Error: Invalid ping command format" << std::endl;
        return;
    }
    tree.handlePingCommand(id);
}

// Запуск контроллера
void Controller::start() {
    running = true;
    std::string command;
    while (running) {
        std::cout << "> ";
        std::getline(std::cin, command);

        if (command.empty()) continue;

        if (command.find("create") == 0) {
            handleCreateCommand(command);
        } else if (command.find("exec") == 0) {
            // Обработка команды exec: чтение id, text_string и pattern_string
            handleExecCommand(command);
        } else if (command.find("heartbeat") == 0) {
            handleHeartbeatCommand(command);
        } else if (command.find("ping") == 0) {
            handlePingCommand(command);
        } else if (command == "exit") {  // Завершение по команде "exit"
            std::cout << "Exiting on command..." << std::endl;
            running = false;
        } else {
            std::cout << "Unknown command" << std::endl;
        }
    }
}

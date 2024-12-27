#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <string>
#include <memory>
#include "tree.h"

class Controller {
private:
    Tree tree; // Дерево для хранения узлов
    bool running; // Флаг для управления циклом

public:
    Controller();                                           // Конструктор

    void createNode(int id, int parentId);                  // Метод для создания узла
    void execNodeCommand(int id, const std::string& text, const std::string& pattern);  // Метод для выполнения команды на узле
    void handleCreateCommand(const std::string& command);   // Обработка команды для создания узла
    void handleExecCommand(const std::string& command);     // Обработка команды exec
    void handleHeartbeatCommand(const std::string& command); // Обработка команды heartbeat
    void handlePingCommand(const std::string& command);     // Обработка команды ping
    void start();                                           // Метод для старта работы контроллера
};

#endif // CONTROLLER_H

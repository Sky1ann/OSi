#ifndef TREE_H
#define TREE_H

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include <zmq.hpp>
#include "node.h"
#include <mutex>
#include <functional>
#include <vector>
#include <unordered_map>
#include <sys/types.h>

// Структура узла дерева
struct TreeNode {
    int id;
    std::shared_ptr<Node> node;
    TreeNode* parent;
    TreeNode* left;
    TreeNode* right;
    int height;

    TreeNode(int nodeId, std::shared_ptr<Node> nodePtr)
        : id(nodeId), node(nodePtr), parent(nullptr), left(nullptr), right(nullptr), height(1) {}
};

class Tree {
private:
    TreeNode* root; // Корень дерева
    zmq::context_t context;                    // Контекст ZeroMQ
    zmq::socket_t socket;                      // Сокет для отправки heartbeat-сообщений
    std::mutex treeMutex; // Мьютекс для защиты дерева

    // Вспомогательные функции для AVL-дерева
    int getHeight(TreeNode* N);
    int getBalance(TreeNode* N);
    TreeNode* rightRotate(TreeNode* y);
    TreeNode* leftRotate(TreeNode* x);
    TreeNode* insert(TreeNode* node, int id, std::shared_ptr<Node> newNode, bool& success, std::string& errorMsg);
    
    // Функция для рекурсивного удаления дерева
    void deleteTree(TreeNode* node);

    // Функция для отметки поддерева как недоступного
    void markSubtreeUnavailable(TreeNode* node);

    // Мониторинг heartbeat-сигнала
    void monitorHeartbeat(int id, int interval);

public:
    // Конструктор
    Tree();

    // Деструктор
    ~Tree();

    // Создание узла
    bool createNode(int id, int parentId);

    // Обработка команды heartbeat
    void handleHeartbeatCommand(int timeMs);

    // Обработка команды ping
    bool handlePingCommand(int id);

    // Поиск узла по id
    std::shared_ptr<Node> findNode(int id);

    // Поиск TreeNode по id
    TreeNode* findTreeNode(int id);

    // Удаление узла по id (не реализовано)
    bool removeNode(int id);

    // Getter для контекста
    zmq::context_t& getContext() { return context; }
};

#endif // TREE_H

#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <zmq.hpp>
#include <iostream> // Добавлено для использования std::cout и std::cerr

// Класс Node представляет вычислительный узел в системе
class Node {
public:
    int id; // Идентификатор узла
    bool isAvailable; // Флаг доступности узла
    int repPort; // Порт для REP-сокета
    zmq::socket_t socket; // REP-сокет для обработки запросов
    std::mutex nodeMutex; // Мьютекс для защиты данных узла
    std::atomic<bool> stopListening; // Флаг для остановки прослушивания
    std::chrono::steady_clock::time_point lastHeartbeatTime; // Время последнего heartbeat
    std::vector<std::shared_ptr<Node>> children; // Дочерние узлы

    // Конструктор
    Node(int nodeId, int port, zmq::context_t& context);

    // Деструктор
    ~Node();

    // Метод для запуска прослушивания команд
    void startListening();

    // Метод для остановки прослушивания
    void stop();

    // Метод для выполнения команды (например, поиск подстроки)
    std::string execute(const std::string& command, const std::string& text, const std::string& pattern);

    // Метод для проверки доступности узла
    bool checkAvailability(int interval);

    // Метод для обновления времени последнего heartbeat-сигнала
    void updateHeartbeatTime();

    // Метод для добавления дочернего узла
    void addChild(std::shared_ptr<Node> child);

private:
    // Метод для выполнения поиска подстроки с использованием алгоритма КМП
    std::string execSubstringSearch(const std::string& text, const std::string& pattern);
};

#endif // NODE_H

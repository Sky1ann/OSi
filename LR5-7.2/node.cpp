#include "node.h"
#include <chrono>
#include <sstream>
#include <algorithm>

// Конструктор
Node::Node(int nodeId, int port, zmq::context_t& context)
    : id(nodeId), isAvailable(true), repPort(port), socket(context, zmq::socket_type::rep),
      stopListening(false), lastHeartbeatTime(std::chrono::steady_clock::now()) {
    try {
        std::string endpoint = "tcp://*:" + std::to_string(repPort);
        socket.bind(endpoint);
        std::cout << "Node " << id << " bound to " << endpoint << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "Error binding Node " << id << " to port " << repPort << ": " << e.what() << std::endl;
        isAvailable = false;
    }
}

// Деструктор
Node::~Node() {
    stop(); // Остановить прослушивание при уничтожении узла
    socket.close();
}

// Метод для запуска прослушивания команд
void Node::startListening() {
    while (!stopListening.load()) {
        try {
            zmq::message_t request;
            // Установить таймаут для получения сообщения, чтобы периодически проверять флаг остановки
            socket.set(zmq::sockopt::rcvtimeo, 1000); // 1 секунда
            auto received = socket.recv(request, zmq::recv_flags::none);
            if (!received) {
                continue; // Таймаут, проверить флаг остановки
            }

            std::string message(static_cast<char*>(request.data()), request.size());
            std::cout << "Node " << id << " received: " << message << std::endl;

            std::stringstream ss(message);
            std::string cmd;
            ss >> cmd;

            if (cmd == "ping") {
                updateHeartbeatTime();
                zmq::message_t reply("Ok", 2);
                socket.send(reply, zmq::send_flags::none);
            }
            else if (cmd == "heartbeat") {
                updateHeartbeatTime();
                zmq::message_t reply("Ok", 2);
                socket.send(reply, zmq::send_flags::none);
            }
            else if (cmd == "exec") {
                // Получение двух дополнительных сообщений
                zmq::message_t text_msg;
                auto text_received = socket.recv(text_msg, zmq::recv_flags::none);
                if (!text_received) {
                    zmq::message_t error_reply("Error: Missing text string", 23);
                    socket.send(error_reply, zmq::send_flags::none);
                    continue;
                }
                std::string text(static_cast<char*>(text_msg.data()), text_msg.size());

                zmq::message_t pattern_msg;
                auto pattern_received = socket.recv(pattern_msg, zmq::recv_flags::none);
                if (!pattern_received) {
                    zmq::message_t error_reply("Error: Missing pattern string", 25);
                    socket.send(error_reply, zmq::send_flags::none);
                    continue;
                }
                std::string pattern(static_cast<char*>(pattern_msg.data()), pattern_msg.size());

                // Выполнение поиска подстроки
                std::string response = execute(cmd, text, pattern);
                zmq::message_t reply(response.c_str(), response.size());
                socket.send(reply, zmq::send_flags::none);
            }
            else {
                // Обработка неизвестной команды
                std::string errorMsg = "Error:" + std::to_string(id) + ": Unknown command";
                zmq::message_t reply(errorMsg.c_str(), errorMsg.size());
                socket.send(reply, zmq::send_flags::none);
            }
        }
        catch (const zmq::error_t& e) {
            if (e.num() == ETERM) {
                // Контекст закрыт, завершить прослушивание
                break;
            }
            std::cerr << "ZeroMQ Error in Node " << id << ": " << e.what() << std::endl;
            // Возможная логика повторного подключения или выхода
        }
        catch (const std::exception& e) {
            std::cerr << "Exception in Node " << id << ": " << e.what() << std::endl;
            // Обработка других исключений
        }
    }

    // Закрыть сокет при завершении
    socket.close();
}

// Метод для остановки прослушивания
void Node::stop() {
    stopListening.store(true);
}

// Метод для выполнения команды (поиск подстроки)
std::string Node::execute(const std::string& command, const std::string& text, const std::string& pattern) {
    if (command == "exec") {
        return execSubstringSearch(text, pattern);
    }
    return "Error:" + std::to_string(id) + ": Unknown command";
}

// Метод для проверки доступности узла
bool Node::checkAvailability(int interval) {
    std::lock_guard<std::mutex> lock(nodeMutex);
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeartbeatTime).count();
    return duration <= 4 * interval;  // Узел доступен, если прошло не более 4 * интервал
}

// Метод для обновления времени последнего heartbeat-сигнала
void Node::updateHeartbeatTime() {
    std::lock_guard<std::mutex> lock(nodeMutex);
    lastHeartbeatTime = std::chrono::steady_clock::now();  // Обновление на текущее время
}

// Метод для добавления дочернего узла
void Node::addChild(std::shared_ptr<Node> child) {
    std::lock_guard<std::mutex> lock(nodeMutex);
    children.push_back(child);
}

// Метод для выполнения поиска подстроки с использованием алгоритма КМП
std::string Node::execSubstringSearch(const std::string& text, const std::string& pattern) {
    std::vector<int> positions;
    if (pattern.empty()) {
        return "Error:" + std::to_string(id) + ": Empty pattern";
    }

    // Построение массива LPS для алгоритма КМП
    int m = pattern.length();
    int n = text.length();

    std::vector<int> lps(m, 0);
    int len = 0; // длина предыдущего наибольшего префикса суффикса

    for (int i = 1; i < m; ++i) {
        while (len > 0 && pattern[i] != pattern[len])
            len = lps[len - 1];

        if (pattern[i] == pattern[len])
            len++;
        lps[i] = len;
    }

    // Поиск подстроки
    int i = 0; // индекс для text
    int j = 0; // индекс для pattern
    while (i < n) {
        if (pattern[j] == text[i]) {
            j++;
            i++;
        }

        if (j == m) {
            positions.push_back(i - j);
            j = lps[j - 1];
        }
        else if (i < n && pattern[j] != text[i]) {
            if (j != 0)
                j = lps[j - 1];
            else
                i++;
        }
    }

    // Формирование результата
    if (positions.empty()) {
        return "Ok:" + std::to_string(id) + ":-1";
    }
    else {
        std::stringstream ss;
        ss << "Ok:" << id << ":";
        for (size_t k = 0; k < positions.size(); ++k) {
            ss << positions[k];
            if (k != positions.size() - 1)
                ss << ";";
        }
        return ss.str();
    }
}

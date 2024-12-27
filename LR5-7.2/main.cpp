#include <iostream>
#include "controller.h"
#include <csignal>
#include <atomic>
#include <thread>

// Глобальный флаг для управления завершением программы
std::atomic<bool> keepRunning(true);

// Обработчик сигнала для корректного завершения
void signalHandler(int signum) {
    keepRunning.store(false);
}

int main() {
    // Установка обработчика сигналов
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    Controller controller;

    std::cout << "Добро пожаловать в систему управления узлами.\n";
    std::cout << "Доступные команды:\n";
    std::cout << "  create <id> [parentId]       - Создать узел с optional родителем.\n";
    std::cout << "  exec <id>                    - Выполнить команду поиска подстроки на узле.\n";
    std::cout << "      После команды exec введите две строки:\n";
    std::cout << "          > text_string\n";
    std::cout << "          > pattern_string\n";
    std::cout << "  heartbeat <timeMs>           - Запустить мониторинг heartbeat с интервалом в миллисекундах.\n";
    std::cout << "  ping <id>                    - Проверить доступность узла.\n";
    std::cout << "  exit                         - Выход.\n";

    // Запуск контроллера в отдельном потоке
    std::thread controllerThread(&Controller::start, &controller);

    // Ожидание сигнала завершения
    while (keepRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nReceived termination signal. Exiting..." << std::endl;

    // Завершение работы контроллера
    // Предполагается, что контроллер реагирует на завершение, например, через команду "exit"
    // В текущей реализации пользователь должен ввести "exit"

    // Ожидание завершения контроллера
    if (controllerThread.joinable()) {
        controllerThread.join();
    }

    return 0;
}

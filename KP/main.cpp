#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdlib>
#include <signal.h>
#include "json.hpp"
#include <sys/wait.h>
#include <unistd.h>

// Для удобства
using json = nlohmann::json;

// Структура для представления задачи
struct Job {
    std::string id;
    std::string command;
    std::vector<std::string> dependents; // Задачи, зависящие от этой задачи
    int in_degree = 0; // Количество зависимостей
};

// Глобальные переменные для управления выполнением задач
std::unordered_map<std::string, Job> jobs_map;
std::mutex mtx;
std::condition_variable cv;
std::atomic<bool> error_flag(false);
std::vector<pid_t> running_pids;

// Функция для парсинга JSON-конфигурации
bool parse_json(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка при открытии JSON-файла: " << filename << std::endl;
        return false;
    }

    json config;
    try {
        file >> config;
    } catch (json::parse_error& e) {
        std::cerr << "Ошибка разбора JSON: " << e.what() << std::endl;
        return false;
    }

    // Парсинг задач
    if (!config.contains("jobs") || !config["jobs"].is_array()) {
        std::cerr << "JSON должен содержать массив задач." << std::endl;
        return false;
    }

    for (const auto& job_json : config["jobs"]) {
        if (!job_json.contains("id") || !job_json.contains("command")) {
            std::cerr << "Каждая задача должна иметь 'id' и 'command'." << std::endl;
            return false;
        }
        Job job;
        job.id = job_json["id"].get<std::string>();
        job.command = job_json["command"].get<std::string>();
        jobs_map[job.id] = job;
    }

    // Парсинг зависимостей
    if (!config.contains("dependencies") || !config["dependencies"].is_array()) {
        std::cerr << "JSON должен содержать массив зависимостей." << std::endl;
        return false;
    }

    for (const auto& dep_json : config["dependencies"]) {
        if (!dep_json.contains("from") || !dep_json.contains("to")) {
            std::cerr << "Каждая зависимость должна иметь 'from' и 'to'." << std::endl;
            return false;
        }
        std::string from = dep_json["from"].get<std::string>();
        std::string to = dep_json["to"].get<std::string>();

        if (jobs_map.find(from) == jobs_map.end() || jobs_map.find(to) == jobs_map.end()) {
            std::cerr << "Зависимость ссылается на неопределённые задачи: " << from << " -> " << to << std::endl;
            return false;
        }

        jobs_map[from].dependents.push_back(to);
        jobs_map[to].in_degree++;
    }

    return true;
}

// Функция для проверки ацикличности 
bool is_acyclic() {
    std::queue<std::string> q;
    std::unordered_map<std::string, int> in_deg;
    for (const auto& [id, job] : jobs_map) {
        in_deg[id] = job.in_degree;
        if (job.in_degree == 0) {
            q.push(id);
        }
    }

    int visited = 0;
    while (!q.empty()) {
        std::string current = q.front();
        q.pop();
        visited++;

        for (const auto& dep : jobs_map[current].dependents) {
            in_deg[dep]--;
            if (in_deg[dep] == 0) {
                q.push(dep);
            }
        }
    }

    return visited == jobs_map.size();
}

// Функция для проверки связности графа
bool is_connected() {
    if (jobs_map.empty()) return true;

    std::unordered_set<std::string> visited;
    std::queue<std::string> q;

    // Найти узел с in_degree == 0 для начала поиска в ширину
    std::string start;
    for (const auto& [id, job] : jobs_map) {
        if (job.in_degree == 0) {
            start = id;
            break;
        }
    }

    if (start.empty()) {
        // Если стартового узла нет, выбрать любой
        start = jobs_map.begin()->first;
    }

    q.push(start);
    visited.insert(start);

    while (!q.empty()) {
        std::string current = q.front();
        q.pop();

        for (const auto& dep : jobs_map[current].dependents) {
            if (visited.find(dep) == visited.end()) {
                visited.insert(dep);
                q.push(dep);
            }
        }

        // Чтобы обеспечить двунаправленную связность, проверить обратные зависимости
        for (const auto& [id, job] : jobs_map) {
            if (std::find(job.dependents.begin(), job.dependents.end(), current) != job.dependents.end()) {
                if (visited.find(id) == visited.end()) {
                    visited.insert(id);
                    q.push(id);
                }
            }
        }
    }

    return visited.size() == jobs_map.size();
}

// Функция для валидации DAG
bool validate_dag() {
    // Проверка на ацикличность
    if (!is_acyclic()) {
        std::cerr << "Граф содержит циклы." << std::endl;
        return false;
    }

    // Проверка на единую компоненту связности
    if (!is_connected()) {
        std::cerr << "Граф не является одной компонентой связности." << std::endl;
        return false;
    }

    // Проверка наличия хотя бы одной стартовой и завершающей задачи
    int start_jobs = 0, end_jobs = 0;
    for (const auto& [id, job] : jobs_map) {
        if (job.in_degree == 0) start_jobs++;
        if (job.dependents.empty()) end_jobs++;
    }

    if (start_jobs == 0) {
        std::cerr << "Не найдены стартовые задачи (задачи без зависимостей)." << std::endl;
        return false;
    }

    if (end_jobs == 0) {
        std::cerr << "Не найдены завершающие задачи (задачи без зависимых задач)." << std::endl;
        return false;
    }

    return true;
}

// Функция для выполнения одной задачи
void execute_job(const std::string& job_id) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (error_flag.load()) return;
    }

    Job job = jobs_map[job_id];
    std::cout << "Запуск задачи " << job.id << ": " << job.command << std::endl;

    pid_t pid = fork();
    if (pid == 0) {
        // Дочерний процесс
        // Выполнение команды с помощью /bin/sh -c
        execl("/bin/sh", "sh", "-c", job.command.c_str(), (char*) nullptr);
        // Если exec завершился с ошибкой
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Родительский процесс
        {
            std::lock_guard<std::mutex> lock(mtx);
            running_pids.push_back(pid);
        }

        int status;
        if (waitpid(pid, &status, 0) == -1) {
            std::cerr << "Не удалось дождаться завершения задачи " << job.id << std::endl;
            error_flag.store(true);
        } else {
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                if (exit_code != 0) {
                    std::cerr << "Задача " << job.id << " завершилась с ошибкой (код выхода " << exit_code << ")" << std::endl;
                    error_flag.store(true);
                } else {
                    std::cout << "Задача " << job.id << " успешно завершена." << std::endl;
                }
            } else {
                std::cerr << "Задача " << job.id << " завершилась ненормально." << std::endl;
                error_flag.store(true);
            }
        }

        {
            std::lock_guard<std::mutex> lock(mtx);
            // Удаление pid из списка запущенных процессов
            running_pids.erase(std::remove(running_pids.begin(), running_pids.end(), pid), running_pids.end());
        }
    } else {
        // Ошибка при fork
        std::cerr << "Не удалось создать процесс для задачи " << job.id << std::endl;
        error_flag.store(true);
    }

    // Уведомить главный поток в случае ошибки
    if (error_flag.load()) {
        cv.notify_one();
    }
}

// Функция для завершения всех запущенных задач
void terminate_running_jobs() {
    std::lock_guard<std::mutex> lock(mtx);
    for (pid_t pid : running_pids) {
        kill(pid, SIGTERM);
    }
    running_pids.clear();
}

// Основная функция для выполнения DAG
void execute_dag() {
    // Инициализация карты степеней входа
    std::unordered_map<std::string, int> in_degree;
    for (const auto& [id, job] : jobs_map) {
        in_degree[id] = job.in_degree;
    }

    // Очередь для готовых задач
    std::queue<std::string> q;
    for (const auto& [id, deg] : in_degree) {
        if (deg == 0) {
            q.push(id);
        }
    }

    // Отслеживание потоков
    std::vector<std::thread> threads;

    while (!q.empty()) {
        std::string current = q.front();
        q.pop();

        // Запуск потока для выполнения задачи
        threads.emplace_back([&, current]() {
            execute_job(current);
        });

        // После выполнения обновляем зависимости
        {
            // Ожидание завершения задачи
            // Здесь упрощено путем немедленного присоединения, но можно улучшить для параллелизма
            // Для демонстрации выполняем последовательно
            threads.back().join();
            if (error_flag.load()) {
                terminate_running_jobs();
                return;
            }

            // Обновление степеней входа зависимых задач
            for (const auto& dep : jobs_map[current].dependents) {
                in_degree[dep]--;
                if (in_degree[dep] == 0) {
                    q.push(dep);
                }
            }
        }
    }

    // Ожидание завершения всех потоков
    for (auto& th : threads) {
        if (th.joinable()) th.join();
    }

    if (error_flag.load()) {
        std::cerr << "Выполнение DAG прервано из-за ошибки задачи." << std::endl;
    } else {
        std::cout << "DAG выполнен успешно." << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc !=2 ) {
        std::cerr << "Использование: " << argv[0] << " <config.json>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string config_file = argv[1];

    if (!parse_json(config_file)) {
        return EXIT_FAILURE;
    }

    if (!validate_dag()) {
        return EXIT_FAILURE;
    }

    execute_dag();

    return EXIT_SUCCESS;
}

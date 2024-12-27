#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <cstring>
#include <signal.h>
#include <fcntl.h>
#include <cstdlib>

// Имя для общей памяти
#define SHM_NAME "/my_shared_memory"

volatile sig_atomic_t error_signal_received = 0;
volatile sig_atomic_t valid_signal_received = 0;

// Обработчик сигнала для ошибки
void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        error_signal_received = 1;
    }
}

// Обработчик сигнала для валидной строки
void valid_signal_handler(int sig) {
    if (sig == SIGUSR2) {
        valid_signal_received = 1;
    }
}

// Функция для создания и запуска дочернего процесса
int create_process(const std::string& filename) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) { // Дочерний процесс
        // Запускаем дочерний процесс, передавая имя выходного файла и имя общей памяти
        execl("./child", "child", filename.c_str(), SHM_NAME, (char*)nullptr);
        perror("Ошибка при запуске child");
        exit(EXIT_FAILURE);
    }
    return pid;
}

int main() {
    std::string filename;
    std::cout << "Введите имя выходного файла: ";
    std::getline(std::cin, filename);

    // Размер отображаемой памяти
    const size_t size = 1024;

    // Создаём именованную общую память
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return EXIT_FAILURE;
    }

    // Устанавливаем размер общей памяти
    if (ftruncate(shm_fd, size) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    // Отображаем память в адресное пространство
    void* shared_memory = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    // Закрываем дескриптор, так как память уже отображена
    close(shm_fd);

    // Устанавливаем обработчики сигналов
    struct sigaction sa_error;
    sa_error.sa_handler = signal_handler;
    sa_error.sa_flags = 0;
    sigemptyset(&sa_error.sa_mask);
    if (sigaction(SIGUSR1, &sa_error, nullptr) == -1) {
        perror("sigaction");
        munmap(shared_memory, size);
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    struct sigaction sa_valid;
    sa_valid.sa_handler = valid_signal_handler;
    sa_valid.sa_flags = 0;
    sigemptyset(&sa_valid.sa_mask);
    if (sigaction(SIGUSR2, &sa_valid, nullptr) == -1) {
        perror("sigaction");
        munmap(shared_memory, size);
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    // Создаём и запускаем дочерний процесс
    pid_t pid = create_process(filename);

    std::string input;
    while (true) {
        std::cout << "Введите строку: ";
        std::getline(std::cin, input);

        if (input.empty()) {
            continue;
        }

        // Копируем строку в отображаемую память
        strncpy(static_cast<char*>(shared_memory), input.c_str(), size - 1);
        static_cast<char*>(shared_memory)[size - 1] = '\0'; 

        // Небольшая задержка, чтобы дать дочернему процессу обработать строку
        usleep(100000); // 100 миллисекунд

        // Проверяем флаги сигналов
        if (error_signal_received) {
            std::cerr << "Ошибка: строка не соответствует правилам!" << std::endl;
            error_signal_received = 0;  // Сбрасываем флаг
        }

        if (valid_signal_received) {
            std::cout << "Строка валидна и записана в файл." << std::endl;
            valid_signal_received = 0;  // Сбрасываем флаг
        }
    }

    munmap(shared_memory, size);  // Освобождаем общую память
    shm_unlink(SHM_NAME);         // Удаляем именованную память
    wait(NULL);                    // Ожидаем завершения дочернего процесса

    return 0;
}

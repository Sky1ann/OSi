#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstring>
#include <signal.h>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char* argv[]) {

    if (argc < 3) {
        std::cerr << "Не переданы необходимые аргументы!" << std::endl;
        std::cerr << "Использование: child <output_file> <shared_memory_name>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string filename = argv[1];
    std::string shm_name = argv[2];

    // Открываем файл для записи
    int file = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);  // O_APPEND добавляет строки в конец
    if (file < 0) {
        perror("Ошибка открытия файла");
        return EXIT_FAILURE;
    }

    // Размер области памяти
    const size_t size = 1024;

    // Открываем именованную общую память
    int shm_fd = shm_open(shm_name.c_str(), O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        close(file);
        return EXIT_FAILURE;
    }

    // Отображаем память в адресное пространство
    void* shared_memory = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        close(file);
        close(shm_fd);
        return EXIT_FAILURE;
    }

    // Закрываем дескриптор, так как память уже отображена
    close(shm_fd);

    while (true) {
        // Читаем строку из общей памяти
        std::string line(static_cast<char*>(shared_memory));

        if (line.empty()) {
            usleep(100000); // 100 миллисекунд
            continue;
        }

        // Проверяем строку на валидность
        if (line.back() == '.' || line.back() == ';') {
            // Если строка валидна, записываем её в файл
            std::string output = line + "\n";
            if (write(file, output.c_str(), output.size()) < 0) {
                perror("Ошибка записи в файл");
            }

            // Отправляем сигнал родителю, что строка валидна
            if (kill(getppid(), SIGUSR2) == -1) {
                perror("Ошибка отправки сигнала SIGUSR2");
            }
        } else {
            // Если строка не валидна, отправляем сигнал родителю
            if (kill(getppid(), SIGUSR1) == -1) {
                perror("Ошибка отправки сигнала SIGUSR1");
            }
        }

        // Очищаем память после обработки строки
        memset(shared_memory, 0, size);

        usleep(100000); // 100 миллисекунд
    }

    munmap(shared_memory, size);  // Освобождаем память
    close(file);
    return 0;
}

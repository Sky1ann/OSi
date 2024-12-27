#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h> //пока что хз надо ли
#include <fstream>
#include <cstring>


int create_process() {
    pid_t pid = fork();
    if (-1 == pid) {
        perror("fork");
        exit(-1);
    }
    return pid;
}


int main() {
    int pipe1[2], pipe2[2];
    std::string filename;
    
    std::cout << "Введите имя файла для записи: ";
    std::getline(std::cin, filename); 

    int file = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644); // O_WRONLY - открыть файл только для записи.
    if (file < 0) {                                                        // O_CREAT - создать файл, если его не существует.
        perror("Ошибка открытия файла");                                   // O_TRUNC - очистить файл при открытии.
        return -1;                                                         // 0644 - права доступа, без них не сможем создать
    }



    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        return -1;
    }

    pid_t pid = create_process();

    if (pid == 0) { // дочерний
        close(pipe1[1]);
        close(pipe2[0]);

        if (dup2(pipe1[0], STDIN_FILENO) < 0 ||
            dup2(pipe2[1], STDERR_FILENO) < 0 ||
            dup2(file, STDOUT_FILENO) < 0) {
                perror("Ошибка при вызове dup2");
                close(file); 
                return -1;
            }

        close(pipe1[0]);
        close(pipe2[1]);

        execl("./child", "child", filename.c_str(), NULL);
        perror("Ошибка при запуске child");
        exit(-1);

    } else {  // родительский
        close(pipe1[0]);
        close(pipe2[1]);

        std::string input;
        while (std::getline(std::cin, input)) {

            if (write(pipe1[1], input.c_str(), input.length()) < 0 ||
                write(pipe1[1], "\n", 1) < 0) {
                    perror("Ошибка при вызове write");
                    close(file);
                    return -1;
                };

            // Проверяем на ошибку
            char buffer[1024];
            ssize_t n = read(pipe2[0], buffer, sizeof(buffer));
            if (n < 0) {
                perror("Ошибка при вызове read");
                close(file);
                return -1;
            }
            buffer[n] = '\0';
            std::string line(buffer);
            if (line != ";")  {
                std::cerr << line << std::endl;
            }
            
        }
        close(pipe1[1]);
        close(pipe2[0]);
        wait(NULL);

    } 

    close(file);

    return 0;
}
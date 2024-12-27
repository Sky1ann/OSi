#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <fstream>
#include <cstring>


int main(int argc, char* argv[]) {

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.back() == '.' || line.back() == ';') {
            std::cerr << ";";
            std::cout << line << std::endl;
        } else {
            std::cerr << "Ошибка: строка не соответствует правилу: " << line << std::endl; 
        }
    }

    return 0;
}
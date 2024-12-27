#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

// Объявление функций из библиотеки
extern "C" {
    char* translation(long x);
    void sort(int* array, int size);
}

void handle_command_1() {
    long num;
    std::cout << "Введите число для перевода: ";
    std::cin >> num;
    char* result = translation(num);
    std::cout << "Перевод: " << result << std::endl;
    delete[] result;
}

void handle_command_2() {
    int size;
    std::cout << "Введите размер массива: ";
    std::cin >> size;
    if(size <= 0) {
        std::cout << "Некорректный размер массива." << std::endl;
        return;
    }
    std::vector<int> array(size);
    std::cout << "Введите элементы массива: ";
    for(int &elem : array) std::cin >> elem;
    sort(array.data(), size);
    std::cout << "Отсортированный массив: ";
    for(const int &elem : array) std::cout << elem << " ";
    std::cout << std::endl;
}

int main() {
    std::string input;
    while(true) {
        std::cout << "Введите команду: ";
        std::getline(std::cin, input);
        if(input.empty()) continue;

        if(input[0] == '1') {
            handle_command_1();
        }
        else if(input[0] == '2') {
            handle_command_2();
        }
        else {
            std::cout << "Неизвестная команда." << std::endl;
        }
    }
    return 0;
}

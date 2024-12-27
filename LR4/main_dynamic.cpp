#include <iostream>
#include <string>
#include <vector>
#include <dlfcn.h>
#include <cstdlib>

// Определение типов функций
typedef char* (*translation_func)(long);
typedef void (*sort_func)(int*, int);

int main() {
    // Пути к библиотекам
    const char* lib1_path = "./libbinary_bubble.so";
    const char* lib2_path = "./libternary_quick.so";

    // Загрузка библиотек
    void* handle1 = dlopen(lib1_path, RTLD_LAZY);
    if(!handle1) {
        std::cerr << "Не удалось загрузить библиотеку " << lib1_path << ": " << dlerror() << std::endl;
        return 1;
    }

    void* handle2 = dlopen(lib2_path, RTLD_LAZY);
    if(!handle2) {
        std::cerr << "Не удалось загрузить библиотеку " << lib2_path << ": " << dlerror() << std::endl;
        dlclose(handle1);
        return 1;
    }

    // Начальная библиотека
    void* current_handle = handle1;
    translation_func translation = (translation_func)dlsym(current_handle, "translation");
    sort_func sort = (sort_func)dlsym(current_handle, "sort");
    const char* dlsym_error = dlerror();
    if(dlsym_error) {
        std::cerr << "Ошибка поиска символов: " << dlsym_error << std::endl;
        dlclose(handle1);
        dlclose(handle2);
        return 1;
    }

    std::string input;
    while(true) {
        std::cout << "Введите команду: ";
        std::getline(std::cin, input);
        if(input.empty()) continue;

        if(input[0] == '0') {
            // Переключение библиотеки
            if(current_handle == handle1) {
                current_handle = handle2;
                std::cout << "Переключились на библиотеку ternary_quick." << std::endl;
            }
            else {
                current_handle = handle1;
                std::cout << "Переключились на библиотеку binary_bubble." << std::endl;
            }
            // Обновление функций
            translation = (translation_func)dlsym(current_handle, "translation");
            sort = (sort_func)dlsym(current_handle, "sort");
            dlsym_error = dlerror();
            if(dlsym_error) {
                std::cerr << "Ошибка поиска символов после переключения: " << dlsym_error << std::endl;
                dlclose(handle1);
                dlclose(handle2);
                return 1;
            }
        }
        else if(input[0] == '1') {
            long num;
            std::cout << "Введите число для перевода: ";
            std::cin >> num;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Очистка буфера
            char* result = translation(num);
            std::cout << "Перевод: " << result << std::endl;
            delete[] result;
        }
        else if(input[0] == '2') {
            int size;
            std::cout << "Введите размер массива: ";
            std::cin >> size;
            if(size <= 0) {
                std::cout << "Некорректный размер массива." << std::endl;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Очистка буфера
                continue;
            }
            std::vector<int> array(size);
            std::cout << "Введите элементы массива: ";
            for(int &elem : array) std::cin >> elem;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Очистка буфера
            sort(array.data(), size);
            std::cout << "Отсортированный массив: ";
            for(const int &elem : array) std::cout << elem << " ";
            std::cout << std::endl;
        }
        else {
            std::cout << "Неизвестная команда." << std::endl;
        }
    }

    // Закрытие библиотек (недостижимо в данном коде, но рекомендуется)
    dlclose(handle1);
    dlclose(handle2);
    return 0;
}

#include <cstdlib>
#include <string>
#include <algorithm>

// Вспомогательная функция быстрой сортировки для сортировки Хоара
void quicksort(int* array, int low, int high) {
    if (low < high) {
        int pivot = array[high];
        int i = low -1;
        for(int j = low; j < high; ++j) {
            if(array[j] < pivot) {
                ++i;
                std::swap(array[i], array[j]);
            }
        }
        std::swap(array[i+1], array[high]);
        int pi = i+1;
        quicksort(array, low, pi-1);
        quicksort(array, pi+1, high);
    }
}

extern "C" {

// перевод в троичную систему
char* translation(long x) {
    if (x == 0) {
        char* result = new char[2];
        result[0] = '0';
        result[1] = '\0';
        return result;
    }

    std::string ternary;
    long num = x;
    while (num > 0) {
        ternary += ('0' + (num % 3));
        num /= 3;
    }
    std::reverse(ternary.begin(), ternary.end());

    char* result = new char[ternary.size() + 1];
    std::copy(ternary.begin(), ternary.end(), result);
    result[ternary.size()] = '\0';
    return result;
}

// Функция сортировки Хоара
void sort(int* array, int size) {
    quicksort(array, 0, size-1);
}

} // extern "C"
